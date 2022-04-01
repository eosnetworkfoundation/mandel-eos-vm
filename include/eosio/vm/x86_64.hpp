#pragma once

#include <eosio/vm/allocator.hpp>
#include <eosio/vm/exceptions.hpp>
#include <eosio/vm/signals.hpp>
#include <eosio/vm/softfloat.hpp>
#include <eosio/vm/types.hpp>
#include <eosio/vm/utils.hpp>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <variant>
#include <vector>
#include <cpuid.h>


namespace eosio { namespace vm {



   // Random notes:
   // - branch instructions return the address that will need to be updated
   // - label instructions return the address of the target
   // - fix_branch will be called when the branch target is resolved
   // - It would make everything more efficient to make RAX always represent the top of
   //   the stack.
   //
   // - The base of memory is stored in rsi
   // - rdi hold the execution context
   // - rbx holds the remaining stack frames
   //
   // - FIXME: Factor the machine instructions into a separate assembler class.
   template<typename Context>
   class machine_code_writer {
    public:
      machine_code_writer(growable_allocator& alloc, std::size_t source_bytes, module& mod) :
         _mod(mod), _code_segment_base(alloc.start_code()) {
         _code_start = _mod.allocator.alloc<unsigned char>(128);
         _code_end = _code_start + 128;
         code = _code_start;
         // TODO: shrink to fit

         emit_sysv_abi_interface();
         const std::size_t code_size = 4 * 16; // 4 error handlers, each is 16 bytes.
         _code_start = _mod.allocator.alloc<unsigned char>(code_size);
         _code_end = _code_start + code_size;
         code = _code_start;

         // always emit these functions
         fpe_handler = emit_error_handler(&on_fp_error);
         call_indirect_handler = emit_error_handler(&on_call_indirect_error);
         type_error_handler = emit_error_handler(&on_type_error);
         stack_overflow_handler = emit_error_handler(&on_stack_overflow);

         assert(code == _code_end); // verify that the manual instruction count is correct

         // emit host functions
         const uint32_t num_imported = mod.get_imported_functions_size();
         const std::size_t host_functions_size = (40 + 10 * Context::async_backtrace()) * num_imported;
         _code_start = _mod.allocator.alloc<unsigned char>(host_functions_size);
         _code_end = _code_start + host_functions_size;
         // code already set
         for(uint32_t i = 0; i < num_imported; ++i) {
            start_function(code, i);
            emit_host_call(i);
         }
         assert(code == _code_end);

         jmp_table = code;
         if (_mod.tables.size() > 0) {
            // Each function table entry consumes exactly 17 bytes (counted
            // manually).  The size must be constant, so that call_indirect
            // can use random access
            _table_element_size = 17;
            const std::size_t table_size = _table_element_size*_mod.tables[0].table.size();
            _code_start = _mod.allocator.alloc<unsigned char>(table_size);
            _code_end = _code_start + table_size;
            // code already set
            for(uint32_t i = 0; i < _mod.tables[0].table.size(); ++i) {
               uint32_t fn_idx = _mod.tables[0].table[i];
               if (fn_idx < _mod.fast_functions.size()) {
                  // cmp _mod.fast_functions[fn_idx], %edx
                  emit_bytes(0x81, 0xfa);
                  emit_operand32(_mod.fast_functions[fn_idx]);
                  // je fn
                  emit_bytes(0x0f, 0x84);
                  register_call(emit_branch_target32(), fn_idx);
                  // jmp ERROR
                  emit_bytes(0xe9);
                  fix_branch(emit_branch_target32(), type_error_handler);
               } else {
                  // jmp ERROR
                  emit_bytes(0xe9);
                  // default for out-of-range functions
                  fix_branch(emit_branch_target32(), call_indirect_handler);
                  // padding
                  emit_bytes(0xcc, 0xcc, 0xcc, 0xcc);
                  emit_bytes(0xcc, 0xcc, 0xcc, 0xcc);
                  emit_bytes(0xcc, 0xcc, 0xcc, 0xcc);
               }
            }
            assert(code == _code_end);
         }
      }
      ~machine_code_writer() { _mod.allocator.end_code<true>(_code_segment_base); }

      void emit_sysv_abi_interface() {
         // jit_execution_context* context -> RDI
         // void* linear_memory -> RSI
         // native_value* data, -> RDX
         // native_value (*fun)(void*, void*) -> RCX
         // void* stack -> R8
         // uint64_t count -> R9
         // uint32_t vector_result -> (RBP + 16)
         emit_push(rbp);
         emit_movq(rsp, rbp);
         emit_sub(16, rsp);

         // switch stack
         emit(TESTQ, r8, r8);
         // cmovnz
         emit(IA32_REX_W(0x0f, 0x45), r8, rsp);

         // save and set mxcsr
         emit(STMXCSR, *(rbp - 4));
         emit_movd(0x1f80, *(rbp - 8));
         emit(LDMXCSR, *(rbp - 8));

         // copy args
         emit(TESTQ, r9, r9);
         void* loop_end = emit_branch8(JZ);
         void* loop = code;
         emit_movq(*rdx, rax);
         emit_add(8, rdx);
         emit_push(rax);
         emit(DECQ, r9);
         fix_branch8(emit_branch8(JNZ), loop);
         fix_branch8(loop_end, code);

         // load call depth counter
         emit_movq(rbx, *(rbp - 16));
         emit_movd(*rdi, ebx);

         if constexpr (Context::async_backtrace()) {
            emit_movq(rbp, *(rdi + 16));
         }
         emit_call(rcx);
         if constexpr (Context::async_backtrace()) {
            emit_xord(edx, edx);
            emit_movq(rdx, *(rdi + 16));
         }

         emit_movq(*(rbp - 16), rbx);

         emit(LDMXCSR, *(rbp - 4));

         emit_movd(*(rbp + 16), edx);
         emit(TESTD, edx, edx);
         void* is_vector = emit_branch8(JZ);
         emit_vpextrq(0, xmm0, rax);
         emit_vpextrq(1, xmm0, rdx);
         fix_branch8(is_vector, code);

         emit_movq(rbp, rsp);
         emit_pop(rbp);
         emit(RET);
      }

      static constexpr std::size_t max_prologue_size = 21;
      static constexpr std::size_t max_epilogue_size = 18;
      void emit_prologue(const func_type& /*ft*/, const guarded_vector<local_entry>& locals, uint32_t funcnum) {
         _ft = &_mod.types[_mod.functions[funcnum]];
         _params = function_parameters{_ft};
         // FIXME: This is not a tight upper bound
         const std::size_t instruction_size_ratio_upper_bound = use_softfloat?(Context::async_backtrace()?63:49):79;
         std::size_t code_size = max_prologue_size + _mod.code[funcnum].size * instruction_size_ratio_upper_bound + max_epilogue_size;
         _code_start = _mod.allocator.alloc<unsigned char>(code_size);
         _code_end = _code_start + code_size;
         code = _code_start;
         start_function(code, funcnum + _mod.get_imported_functions_size());
         // pushq RBP
         emit_bytes(0x55);
         // movq RSP, RBP
         emit_bytes(0x48, 0x89, 0xe5);
         // No more than 2^32-1 locals.  Already validated by the parser.
         uint32_t count = 0;
         for(uint32_t i = 0; i < locals.size(); ++i) {
            assert(uint64_t(count) + locals[i].count <= 0xFFFFFFFFu);
            count += locals[i].count;
         }
         _local_count = count;
         if (_local_count > 0) {
            // xor %rax, %rax
            emit_bytes(0x48, 0x31, 0xc0);
            if (_local_count > 14) { // only use a loop if it would save space
               // mov $count, %ecx
               emit_bytes(0xb9);
               emit_operand32(_local_count);
               // loop:
               void* loop = code;
               // pushq %rax
               emit_bytes(0x50);
               // dec %ecx
               emit_bytes(0xff, 0xc9);
               // jnz loop
               emit_bytes(0x0f, 0x85);
               fix_branch(emit_branch_target32(), loop);
            } else {
               for (uint32_t i = 0; i < _local_count; ++i) {
                  // pushq %rax
                  emit_bytes(0x50);
               }
            }
         }
         assert((char*)code <= (char*)_code_start + max_prologue_size);
      }
      void emit_epilogue(const func_type& ft, const guarded_vector<local_entry>& locals, uint32_t /*funcnum*/) {
#ifndef NDEBUG
         void * epilogue_start = code;
#endif
         if(ft.return_count != 0) {
            if(ft.return_type == types::v128) {
               emit_vmovups(*rsp, xmm0);
               emit_add(16, rsp);
            } else {
               // pop RAX
               emit_bytes(0x58);
            }
         }
         if (_local_count & 0xF0000000u) unimplemented();
         emit_multipop(_local_count);
         // popq RBP
         emit_bytes(0x5d);
         // retq
         emit_bytes(0xc3);
         assert((char*)code <= (char*)epilogue_start + max_epilogue_size);
      }

      void emit_unreachable() {
         auto icount = fixed_size_instr(16);
         emit_error_handler(&on_unreachable);
      }
      void emit_nop() {}
      void* emit_end() { return code; }
      void* emit_return(uint32_t depth_change) {
         // Return is defined as equivalent to branching to the outermost label
         return emit_br(depth_change);
      }
      void emit_block() {}
      void* emit_loop() { return code; }
      void* emit_if() {
         auto icount = fixed_size_instr(9);
         // pop RAX
         emit_bytes(0x58);
         // test EAX, EAX
         emit_bytes(0x85, 0xC0);
         // jz DEST
         emit_bytes(0x0F, 0x84);
         return emit_branch_target32();
      }
      void* emit_else(void* if_loc) {
         auto icount = fixed_size_instr(5);
         void* result = emit_br(0);
         fix_branch(if_loc, code);
         return result;
      }
      void* emit_br(uint32_t depth_change) {
         auto icount = variable_size_instr(5, 17);
         // add RSP, depth_change * 8
         emit_multipop(depth_change);
         // jmp DEST
         emit_bytes(0xe9);
         return emit_branch_target32();
      }
      void* emit_br_if(uint32_t depth_change) {
         auto icount = variable_size_instr(9, 26);
         // pop RAX
         emit_bytes(0x58);
         // test EAX, EAX
         emit_bytes(0x85, 0xC0);

         if(depth_change == 0u || depth_change == 0x80000001u) {
            // jnz DEST
            emit_bytes(0x0F, 0x85);
            return emit_branch_target32();
         } else {
            // jz SKIP
            emit_bytes(0x0f, 0x84);
            void* skip = emit_branch_target32();
            // add depth_change*8, %rsp
            emit_multipop(depth_change);
            // jmp DEST
            emit_bytes(0xe9);
            void* result = emit_branch_target32();
            // SKIP:
            fix_branch(skip, code);
            return result;
         }
      }

      // Generate a binary search.
      struct br_table_generator {
         void* emit_case(uint32_t depth_change) {
            while(true) {
               assert(!stack.empty() && "The parser is supposed to handle the number of elements in br_table.");
               auto [min, max, label] = stack.back();
               stack.pop_back();
               if (label) {
                  fix_branch(label, _this->code);
               }
               if (max - min > 1) {
                  // Emit a comparison to the midpoint of the current range
                  uint32_t mid = min + (max - min)/2;
                  // cmp i, %mid
                  _this->emit_bytes(0x3d);
                  _this->emit_operand32(mid);
                  // jae MID
                  _this->emit_bytes(0x0f, 0x83);
                  void* mid_label = _this->emit_branch_target32();
                  stack.push_back({mid,max,mid_label});
                  stack.push_back({min,mid,nullptr});
               } else {
                  assert(min == static_cast<uint32_t>(_i));
                  _i++;
                  if (depth_change == 0u || depth_change == 0x80000001u) {
                     if(label) {
                        return label;
                     } else {
                        // jmp TARGET
                        _this->emit_bytes(0xe9);
                        return _this->emit_branch_target32();
                     }
                  } else {
                     // jne NEXT
                    _this->emit_multipop(depth_change);
                    // jmp TARGET
                    _this->emit_bytes(0xe9);
                    return _this->emit_branch_target32();
                  }
               }
            }

         }
         void* emit_default(uint32_t depth_change) {
            void* result = emit_case(depth_change);
            assert(stack.empty() && "unexpected default.");
            return result;
         }
         machine_code_writer * _this;
         int _i = 0;
         struct stack_item {
            uint32_t min;
            uint32_t max;
            void* branch_target = nullptr;
         };
         // stores a stack of ranges to be handled.
         // the ranges are strictly contiguous and non-ovelapping, with
         // the lower values at the back.
         std::vector<stack_item> stack;
      };
      br_table_generator emit_br_table(uint32_t table_size) {
         // pop %rax
         emit_bytes(0x58);
         // Increase the size by one to account for the default.
         // The current algorithm handles this correctly, without
         // any special cases.
         return { this, 0, { {0, table_size+1, nullptr} } };
      }

      void register_call(void* ptr, uint32_t funcnum) {
         auto& vec = _function_relocations;
         if(funcnum >= vec.size()) vec.resize(funcnum + 1);
         if(void** addr = std::get_if<void*>(&vec[funcnum])) {
            fix_branch(ptr, *addr);
         } else {
            std::get<std::vector<void*>>(vec[funcnum]).push_back(ptr);
         }
      }
      void start_function(void* func_start, uint32_t funcnum) {
         auto& vec = _function_relocations;
         if(funcnum >= vec.size()) vec.resize(funcnum + 1);
         for(void* branch : std::get<std::vector<void*>>(vec[funcnum])) {
            fix_branch(branch, func_start);
         }
         vec[funcnum] = func_start;
      }

      void emit_call(const func_type& ft, uint32_t funcnum) {
         auto icount = variable_size_instr(15, 23);
         emit_check_call_depth();
         // callq TARGET
         emit_bytes(0xe8);
         void * branch = emit_branch_target32();
         emit_multipop(ft);
         register_call(branch, funcnum);
         if(ft.return_count != 0) {
            if(ft.return_type == v128) {
               emit_sub(16, rsp);
               emit_vmovups(xmm0, *rsp);
            } else {
               // pushq %rax
               emit_bytes(0x50);
            }
         }
         emit_check_call_depth_end();
      }

      void emit_call_indirect(const func_type& ft, uint32_t functypeidx) {
         auto icount = variable_size_instr(43, 51);
         emit_check_call_depth();
         auto& table = _mod.tables[0].table;
         functypeidx = _mod.type_aliases[functypeidx];
         // pop %rax
         emit_bytes(0x58);
         // cmp $size, %rax
         emit_bytes(0x48, 0x3d);
         emit_operand32(table.size());
         // jae ERROR
         emit_bytes(0x0f, 0x83);
         fix_branch(emit_branch_target32(), call_indirect_handler);
         // leaq table(%rip), %rdx
         emit_bytes(0x48, 0x8d, 0x15);
         fix_branch(emit_branch_target32(), jmp_table);
         // imul $17, %eax, %eax
         assert(_table_element_size <= 127); // must fit in 8-bit signed value for imul
         emit_bytes(0x6b, 0xc0, _table_element_size);
         // addq %rdx, %rax
         emit_bytes(0x48, 0x01, 0xd0);
         // mov $funtypeidx, %edx
         emit_bytes(0xba);
         emit_operand32(functypeidx);
         // callq *%rax
         emit_bytes(0xff, 0xd0);
         emit_multipop(ft);
         if(ft.return_count != 0){
            if(ft.return_type == v128) {
               emit_sub(16, rsp);
               emit_vmovups(xmm0, *rsp);
            } else {
               // pushq %rax
               emit_bytes(0x50);
            }
         }
         emit_check_call_depth_end();
      }

      void emit_drop(uint8_t type) {
         auto icount = variable_size_instr(1, 4);
         if(type == types::v128) {
            emit_add(16, rsp);
         } else {
            emit_pop(rax);
         }
      }

      void emit_select() {
         auto icount = fixed_size_instr(13);
         // popq RAX
         emit_bytes(0x58);
         // popq RCX
         emit_bytes(0x59);
         // test EAX, EAX
         emit_bytes(0x85, 0xc0);
         // cmovnzq RCX, (RSP)
         emit_bytes(0x48, 0x0f, 0x45, 0x0c, 0x24);
         // movq (RSP), RCX
         emit_bytes(0x48, 0x89, 0x0c, 0x24);
      }

      void emit_get_local(uint32_t local_idx) {
         auto icount = fixed_size_instr(8);
         // stack layout:
         //   param0    <----- %rbp + 8*(nparams + 1)
         //   param1
         //   param2
         //   ...
         //   paramN
         //   return address
         //   old %rbp    <------ %rbp
         //   local0    <------ %rbp - 8
         //   local1
         //   ...
         //   localN
         if (local_idx < _ft->param_types.size()) {
            auto addr = *(rbp + _params.get_frame_offset(local_idx));
            if (_ft->param_types[local_idx] != types::v128) {
               emit_movq(addr, rax);
               emit_push(rax);
            } else {
               emit_vmovups(addr, xmm0);
               emit_sub(16, rsp);
               emit_vmovups(xmm0, *rsp);
            }
         } else {
            // mov -8*(local_idx+1)(%RBP), RAX
            emit_bytes(0x48, 0x8b, 0x85);
            emit_operand32(-8 * (local_idx - _ft->param_types.size() + 1));
            // push RAX
            emit_bytes(0x50);
         }
      }

      void emit_set_local(uint32_t local_idx) {
         auto icount = fixed_size_instr(8);
         if (local_idx < _ft->param_types.size()) {
            auto addr = *(rbp + _params.get_frame_offset(local_idx));
            if (_ft->param_types[local_idx] != types::v128) {
               emit_pop(rax);
               emit_movq(rax, addr);
            } else {
               emit_vmovups(*rsp, xmm0);
               emit_add(16, rsp);
               emit_vmovups(xmm0, addr);
            }
         } else {
            // pop RAX
            emit_bytes(0x58);
            // mov RAX, -8*local_idx(EBP)
            emit_bytes(0x48, 0x89, 0x85);
            emit_operand32(-8 * (local_idx - _ft->param_types.size() + 1));
         }
      }

      void emit_tee_local(uint32_t local_idx) {
         auto icount = fixed_size_instr(9); // FIXME
         if (local_idx < _ft->param_types.size()) {
            auto addr = *(rbp + _params.get_frame_offset(local_idx));
            if (_ft->param_types[local_idx] != types::v128) {
               emit_movq(*rsp, rax);
               emit_movq(rax, *(rbp + _params.get_frame_offset(local_idx)));
            } else {
               emit_vmovups(*rsp, xmm0);
               emit_vmovups(xmm0, addr);
            }
         } else {
            // pop RAX
            emit_bytes(0x58);
            // push RAX
            emit_bytes(0x50);
            // mov RAX, -8*local_idx(EBP)
            emit_bytes(0x48, 0x89, 0x85);
            emit_operand32(-8 * (local_idx - _ft->param_types.size() + 1));
         }
      }

      void emit_get_global(uint32_t globalidx) {
         auto icount = variable_size_instr(13, 14);
         auto& gl = _mod.globals[globalidx];
         void *ptr = &gl.current.value;
         switch(gl.type.content_type) {
          case types::i32:
          case types::f32:
            // movabsq $ptr, %rax
            emit_bytes(0x48, 0xb8);
            emit_operand_ptr(ptr);
            // movl (%rax), eax
            emit_bytes(0x8b, 0x00);
            // push %rax
            emit_bytes(0x50);
            break;
          case types::i64:
          case types::f64:
            // movabsq $ptr, %rax
            emit_bytes(0x48, 0xb8);
            emit_operand_ptr(ptr);
            // movl (%rax), %rax
            emit_bytes(0x48, 0x8b, 0x00);
            // push %rax
            emit_bytes(0x50);
            break;
         }
      }
      void emit_set_global(uint32_t globalidx) {
         auto icount = fixed_size_instr(14);
         auto& gl = _mod.globals[globalidx];
         void *ptr = &gl.current.value;
         // popq %rcx
         emit_bytes(0x59);
         // movabsq $ptr, %rax
         emit_bytes(0x48, 0xb8);
         emit_operand_ptr(ptr);
         // movq %rcx, (%rax)
         emit_bytes(0x48, 0x89, 0x08);
      }

      void emit_i32_load(uint32_t /*alignment*/, uint32_t offset) {
         auto icount = variable_size_instr(7, 15);
         // movl (RAX), EAX
         emit_load_impl(offset, 0x8b, 0x00);
      }

      void emit_i64_load(uint32_t /*alignment*/, uint32_t offset) {
         auto icount = variable_size_instr(8, 16);
         // movq (RAX), RAX
         emit_load_impl(offset, 0x48, 0x8b, 0x00);
      }

      void emit_f32_load(uint32_t /*alignment*/, uint32_t offset) {
         auto icount = variable_size_instr(7, 15);
         // movl (RAX), EAX
         emit_load_impl(offset, 0x8b, 0x00);
      }

      void emit_f64_load(uint32_t /*alignment*/, uint32_t offset) {
         auto icount = variable_size_instr(8, 16);
         // movq (RAX), RAX
         emit_load_impl(offset, 0x48, 0x8b, 0x00);
      }

      void emit_i32_load8_s(uint32_t /*alignment*/, uint32_t offset) {
         auto icount = variable_size_instr(8, 16);
         // movsbl (RAX), EAX; 
         emit_load_impl(offset, 0x0F, 0xbe, 0x00);
      }

      void emit_i32_load16_s(uint32_t /*alignment*/, uint32_t offset) {
         auto icount = variable_size_instr(8, 16);
         // movswl (RAX), EAX; 
         emit_load_impl(offset, 0x0F, 0xbf, 0x00);
      }

      void emit_i32_load8_u(uint32_t /*alignment*/, uint32_t offset) {
         auto icount = variable_size_instr(8, 16);
         // movzbl (RAX), EAX; 
         emit_load_impl(offset, 0x0f, 0xb6, 0x00);
      }

      void emit_i32_load16_u(uint32_t /*alignment*/, uint32_t offset) {
         auto icount = variable_size_instr(8, 16);
         // movzwl (RAX), EAX; 
         emit_load_impl(offset, 0x0f, 0xb7, 0x00);
      }

      void emit_i64_load8_s(uint32_t /*alignment*/, uint32_t offset) {
         auto icount = variable_size_instr(9, 17);
         // movsbq (RAX), RAX; 
         emit_load_impl(offset, 0x48, 0x0F, 0xbe, 0x00);
      }

      void emit_i64_load16_s(uint32_t /*alignment*/, uint32_t offset) {
         auto icount = variable_size_instr(9, 17);
         // movswq (RAX), RAX; 
         emit_load_impl(offset, 0x48, 0x0F, 0xbf, 0x00);
      }

      void emit_i64_load32_s(uint32_t /*alignment*/, uint32_t offset) {
         auto icount = variable_size_instr(8, 16);
         // movslq (RAX), RAX
         emit_load_impl(offset, 0x48, 0x63, 0x00);
      }

      void emit_i64_load8_u(uint32_t /*alignment*/, uint32_t offset) {
         auto icount = variable_size_instr(8, 16);
         // movzbl (RAX), EAX; 
         emit_load_impl(offset, 0x0f, 0xb6, 0x00);
      }

      void emit_i64_load16_u(uint32_t /*alignment*/, uint32_t offset) {
         auto icount = variable_size_instr(8, 16);
         // movzwl (RAX), EAX; 
         emit_load_impl(offset, 0x0f, 0xb7, 0x00);
      }

     void emit_i64_load32_u(uint32_t /*alignment*/, uint32_t offset) {
         auto icount = variable_size_instr(7, 15);
         // movl (RAX), EAX
         emit_load_impl(offset, 0x8b, 0x00);
      }

      void emit_i32_store(uint32_t /*alignment*/, uint32_t offset) {
         auto icount = variable_size_instr(7, 15);
         // movl ECX, (RAX)
         emit_store_impl(offset, 0x89, 0x08);
      }

      void emit_i64_store(uint32_t /*alignment*/, uint32_t offset) {
         auto icount = variable_size_instr(8, 16);
         // movl ECX, (RAX)
         emit_store_impl(offset, 0x48, 0x89, 0x08);
      }

      void emit_f32_store(uint32_t /*alignment*/, uint32_t offset) {
         auto icount = variable_size_instr(7, 15);
         // movl ECX, (RAX)
         emit_store_impl(offset, 0x89, 0x08);
      }

      void emit_f64_store(uint32_t /*alignment*/, uint32_t offset) {
         auto icount = variable_size_instr(8, 16);
         // movl ECX, (RAX)
         emit_store_impl(offset, 0x48, 0x89, 0x08);
      }

      void emit_i32_store8(uint32_t /*alignment*/, uint32_t offset) {
         auto icount = variable_size_instr(7, 15);
         // movb CL, (RAX)
         emit_store_impl(offset, 0x88, 0x08);
      }

      void emit_i32_store16(uint32_t /*alignment*/, uint32_t offset) {
         auto icount = variable_size_instr(8, 16);
         // movb CX, (RAX)
         emit_store_impl(offset, 0x66, 0x89, 0x08);
      }

      void emit_i64_store8(uint32_t /*alignment*/, uint32_t offset) {
         auto icount = variable_size_instr(7, 15);
         // movb CL, (RAX)
         emit_store_impl(offset, 0x88, 0x08);
      }

      void emit_i64_store16(uint32_t /*alignment*/, uint32_t offset) {
         auto icount = variable_size_instr(8, 16);
         // movb CX, (RAX)
         emit_store_impl(offset, 0x66, 0x89, 0x08);
      }

      void emit_i64_store32(uint32_t /*alignment*/, uint32_t offset) {
         auto icount = variable_size_instr(7, 15);
         // movl ECX, (RAX)
         emit_store_impl(offset, 0x89, 0x08);
      }

      void emit_current_memory() {
         auto icount = variable_size_instr(17, 35);
         emit_setup_backtrace();
         // pushq %rdi
         emit_bytes(0x57);
         // pushq %rsi
         emit_bytes(0x56);
         // movabsq $current_memory, %rax
         emit_bytes(0x48, 0xb8);
         emit_operand_ptr(&current_memory);
         // call *%rax
         emit_bytes(0xff, 0xd0);
         // pop %rsi
         emit_bytes(0x5e);
         // pop %rdi
         emit_bytes(0x5f);
         emit_restore_backtrace();
         // push %rax
         emit_bytes(0x50);
      }
      void emit_grow_memory() {
         auto icount = variable_size_instr(21, 39);
         // popq %rax
         emit_bytes(0x58);
         emit_setup_backtrace();
         // pushq %rdi
         emit_bytes(0x57);
         // pushq %rsi
         emit_bytes(0x56);
         // movq %rax, %rsi
         emit_bytes(0x48, 0x89, 0xc6);
         // movabsq $grow_memory, %rax
         emit_bytes(0x48, 0xb8);
         emit_operand_ptr(&grow_memory);
         // call *%rax
         emit_bytes(0xff, 0xd0);
         // pop %rsi
         emit_bytes(0x5e);
         // pop %rdi
         emit_bytes(0x5f);
         emit_restore_backtrace();
         // push %rax
         emit_bytes(0x50);
      }

      void emit_i32_const(uint32_t value) {
         auto icount = fixed_size_instr(6);
         // mov $value, %eax
         emit_bytes(0xb8);
         emit_operand32(value);
         // push %rax
         emit_bytes(0x50);
      }

      void emit_i64_const(uint64_t value) {
         auto icount = fixed_size_instr(11);
         // movabsq $value, %rax
         emit_bytes(0x48, 0xb8);
         emit_operand64(value);
         // push %rax
         emit_bytes(0x50);
      }

      void emit_f32_const(float value) {
         auto icount = fixed_size_instr(6);
         // mov $value, %eax
         emit_bytes(0xb8);
         emit_operandf32(value);
         // push %rax
         emit_bytes(0x50);
      }
      void emit_f64_const(double value) {
         auto icount = fixed_size_instr(11);
         // movabsq $value, %rax
         emit_bytes(0x48, 0xb8);
         emit_operandf64(value);
         // push %rax
         emit_bytes(0x50);
      }

      void emit_i32_eqz() {
         auto icount = fixed_size_instr(10);
         // pop %rax
         emit_bytes(0x58);
         // xor %rcx, %rcx
         emit_bytes(0x48, 0x31, 0xc9);
         // test %eax, %eax
         emit_bytes(0x85, 0xc0);
         // setz %cl
         emit_bytes(0x0f, 0x94, 0xc1);
         // push %rcx
         emit_bytes(0x51);
      }

      // i32 relops
      void emit_i32_eq() {
         auto icount = fixed_size_instr(11);
         // sete %dl
         emit_i32_relop(0x94);
      }

      void emit_i32_ne() {
         auto icount = fixed_size_instr(11);
         // sete %dl
         emit_i32_relop(0x95);
      }

      void emit_i32_lt_s() {
         auto icount = fixed_size_instr(11);
         // setl %dl
         emit_i32_relop(0x9c);
      }

      void emit_i32_lt_u() {
         auto icount = fixed_size_instr(11);
         // setl %dl
         emit_i32_relop(0x92);
      }

      void emit_i32_gt_s() {
         auto icount = fixed_size_instr(11);
         // setg %dl
         emit_i32_relop(0x9f);
      }

      void emit_i32_gt_u() {
         auto icount = fixed_size_instr(11);
         // seta %dl
         emit_i32_relop(0x97);
      }

      void emit_i32_le_s() {
         auto icount = fixed_size_instr(11);
         // setle %dl
         emit_i32_relop(0x9e);
      }

      void emit_i32_le_u() {
         auto icount = fixed_size_instr(11);
         // setbe %dl
         emit_i32_relop(0x96);
      }

      void emit_i32_ge_s() {
         auto icount = fixed_size_instr(11);
         // setge %dl
         emit_i32_relop(0x9d);
      }

      void emit_i32_ge_u() {
         auto icount = fixed_size_instr(11);
         // setae %dl
         emit_i32_relop(0x93);
      }

      void emit_i64_eqz() {
         auto icount = fixed_size_instr(11);
         // pop %rax
         emit_bytes(0x58);
         // xor %rcx, %rcx
         emit_bytes(0x48, 0x31, 0xc9);
         // test %rax, %rax
         emit_bytes(0x48, 0x85, 0xc0);
         // setz %cl
         emit_bytes(0x0f, 0x94, 0xc1);
         // push %rcx
         emit_bytes(0x51);
      }
      // i64 relops
      void emit_i64_eq() {
         auto icount = fixed_size_instr(12);
         // sete %dl
         emit_i64_relop(0x94);
      }

      void emit_i64_ne() {
         auto icount = fixed_size_instr(12);
         // sete %dl
         emit_i64_relop(0x95);
      }

      void emit_i64_lt_s() {
         auto icount = fixed_size_instr(12);
         // setl %dl
         emit_i64_relop(0x9c);
      }

      void emit_i64_lt_u() {
         auto icount = fixed_size_instr(12);
         // setl %dl
         emit_i64_relop(0x92);
      }

      void emit_i64_gt_s() {
         auto icount = fixed_size_instr(12);
         // setg %dl
         emit_i64_relop(0x9f);
      }

      void emit_i64_gt_u() {
         auto icount = fixed_size_instr(12);
         // seta %dl
         emit_i64_relop(0x97);
      }

      void emit_i64_le_s() {
         auto icount = fixed_size_instr(12);
         // setle %dl
         emit_i64_relop(0x9e);
      }

      void emit_i64_le_u() {
         auto icount = fixed_size_instr(12);
         // setbe %dl
         emit_i64_relop(0x96);
      }

      void emit_i64_ge_s() {
         auto icount = fixed_size_instr(12);
         // setge %dl
         emit_i64_relop(0x9d);
      }

      void emit_i64_ge_u() {
         auto icount = fixed_size_instr(12);
         // setae %dl
         emit_i64_relop(0x93);
      }

#ifdef EOS_VM_SOFTFLOAT
      // Make sure that the result doesn't contain any garbage bits in rax
      static uint64_t adapt_result(bool val) {
         return val?1:0;
      }
      static uint64_t adapt_result(float32_t val) {
         uint64_t result = 0;
         std::memcpy(&result, &val, sizeof(float32_t));
         return result;
      }
      static float64_t adapt_result(float64_t val) {
         return val;
      }

      template<auto F>
      static auto adapt_f32_unop(float32_t arg) {
        return ::to_softfloat32(static_cast<decltype(F)>(F)(::from_softfloat32(arg)));
      }
      template<auto F>
      static auto adapt_f32_binop(float32_t lhs, float32_t rhs) {
         return ::to_softfloat32(static_cast<decltype(F)>(F)(::from_softfloat32(lhs), ::from_softfloat32(rhs)));
      }
      template<auto F>
      static auto adapt_f32_cmp(float32_t lhs, float32_t rhs) {
         return adapt_result(static_cast<decltype(F)>(F)(::from_softfloat32(lhs), ::from_softfloat32(rhs)));
      }

      template<auto F>
      static auto adapt_f64_unop(float64_t arg) {
         return ::to_softfloat64(static_cast<decltype(F)>(F)(::from_softfloat64(arg)));
      }
      template<auto F>
      static auto adapt_f64_binop(float64_t lhs, float64_t rhs) {
         return ::to_softfloat64(static_cast<decltype(F)>(F)(::from_softfloat64(lhs), ::from_softfloat64(rhs)));
      }
      template<auto F>
      static auto adapt_f64_cmp(float64_t lhs, float64_t rhs) {
         return adapt_result(static_cast<decltype(F)>(F)(::from_softfloat64(lhs), ::from_softfloat64(rhs)));
      }

      static float32_t to_softfloat(float arg) { return ::to_softfloat32(arg); }
      static float64_t to_softfloat(double arg) { return ::to_softfloat64(arg); }
      template<typename T>
      static T to_softfloat(T arg) { return arg; }
      static float from_softfloat(float32_t arg) { return ::from_softfloat32(arg); }
      static double from_softfloat(float64_t arg) { return ::from_softfloat64(arg); }
      template<typename T>
      static T from_softfloat(T arg) { return arg; }

      template<typename T>
      using softfloat_arg_t = decltype(to_softfloat(T{}));

      template<auto F, typename T>
      static auto adapt_float_convert(softfloat_arg_t<T> arg) {
         auto result = to_softfloat(F(from_softfloat(arg)));
         if constexpr (sizeof(result) == 4 && sizeof(T) == 8) {
            uint64_t buffer = 0;
            std::memcpy(&buffer, &result, sizeof(result));
            return buffer;
         } else {
            return result;
         }
      }

      template<auto F, typename R, typename T>
      static constexpr auto choose_unop(R(*)(T)) {
         if constexpr(sizeof(R) == 4 && sizeof(T) == 8) {
            return static_cast<uint64_t(*)(softfloat_arg_t<T>)>(&adapt_float_convert<F, T>);
         } else {
            return static_cast<softfloat_arg_t<R>(*)(softfloat_arg_t<T>)>(&adapt_float_convert<F, T>);
         }
      }

      // HACK: avoid linking to softfloat if we aren't using it
      // and also avoid passing arguments in floating point registers,
      // since softfloat uses integer registers.
      template<auto F>
      constexpr auto choose_fn() {
         if constexpr (use_softfloat) {
            if constexpr (std::is_same_v<decltype(F), float(*)(float)>) {
               return &adapt_f32_unop<F>;
            } else if constexpr(std::is_same_v<decltype(F), float(*)(float,float)>) {
               return &adapt_f32_binop<F>;
            } else if constexpr(std::is_same_v<decltype(F), bool(*)(float,float)>) {
               return &adapt_f32_cmp<F>;
            } else if constexpr (std::is_same_v<decltype(F), double(*)(double)>) {
               return &adapt_f64_unop<F>;
            } else if constexpr(std::is_same_v<decltype(F), double(*)(double,double)>) {
               return &adapt_f64_binop<F>;
            } else if constexpr(std::is_same_v<decltype(F), bool(*)(double,double)>) {
               return &adapt_f64_cmp<F>;
            } else {
               return choose_unop<F>(F);
            }
         } else {
            return nullptr;
         }
      }

      template<auto F, typename R, typename... A>
      static R softfloat_trap_fn(A... a) {
         R result;
         longjmp_on_exception([&]() {
            result = F(a...);
         });
         return result;
      }

      template<auto F, typename R, typename... A>
      static constexpr auto make_softfloat_trap_fn(R(*)(A...)) -> R(*)(A...) {
         return softfloat_trap_fn<F, R, A...>;
      }

      template<auto F>
      static constexpr decltype(auto) softfloat_trap() {
         return *make_softfloat_trap_fn<F>(F);
      }

   #define CHOOSE_FN(name) choose_fn<&name>()
#else
      using float32_t = float;
      using float64_t = double;
   #define CHOOSE_FN(name) nullptr
#endif

      // --------------- f32 relops ----------------------
      void emit_f32_eq() {
         auto icount = softfloat_instr(25,45,59);
         emit_f32_relop(0x00, CHOOSE_FN(_eosio_f32_eq), false, false);
      }

      void emit_f32_ne() {
         auto icount = softfloat_instr(24,47,61);
         emit_f32_relop(0x00, CHOOSE_FN(_eosio_f32_eq), false, true);
      }

      void emit_f32_lt() {
         auto icount = softfloat_instr(25,45,59);
         emit_f32_relop(0x01, CHOOSE_FN(_eosio_f32_lt), false, false);
      }

      void emit_f32_gt() {
         auto icount = softfloat_instr(25,45,59);
         emit_f32_relop(0x01, CHOOSE_FN(_eosio_f32_lt), true, false);
      }

      void emit_f32_le() {
         auto icount = softfloat_instr(25,45,59);
         emit_f32_relop(0x02, CHOOSE_FN(_eosio_f32_le), false, false);
      }

      void emit_f32_ge() {
         auto icount = softfloat_instr(25,45,59);
         emit_f32_relop(0x02, CHOOSE_FN(_eosio_f32_le), true, false);
      }

      // --------------- f64 relops ----------------------
      void emit_f64_eq() {
         auto icount = softfloat_instr(25,47,61);
         emit_f64_relop(0x00, CHOOSE_FN(_eosio_f64_eq), false, false);
      }

      void emit_f64_ne() {
         auto icount = softfloat_instr(24,49,63);
         emit_f64_relop(0x00, CHOOSE_FN(_eosio_f64_eq), false, true);
      }

      void emit_f64_lt() {
         auto icount = softfloat_instr(25,47,61);
         emit_f64_relop(0x01, CHOOSE_FN(_eosio_f64_lt), false, false);
      }

      void emit_f64_gt() {
         auto icount = softfloat_instr(25,47,61);
         emit_f64_relop(0x01, CHOOSE_FN(_eosio_f64_lt), true, false);
      }

      void emit_f64_le() {
         auto icount = softfloat_instr(25,47,61);
         emit_f64_relop(0x02, CHOOSE_FN(_eosio_f64_le), false, false);
      }

      void emit_f64_ge() {
         auto icount = softfloat_instr(25,47,61);
         emit_f64_relop(0x02, CHOOSE_FN(_eosio_f64_le), true, false);
      }

      // --------------- i32 unops ----------------------

      bool has_tzcnt_impl() {
         unsigned a, b, c, d;
         return __get_cpuid_count(7, 0, &a, &b, &c, &d) && (b & bit_BMI) &&
                __get_cpuid(0x80000001, &a, &b, &c, &d) && (c & bit_LZCNT);
      }

      bool has_tzcnt() {
         static bool result = has_tzcnt_impl();
         return result;
      }

      void emit_i32_clz() {
         auto icount = fixed_size_instr(has_tzcnt()?6:18);
         if(!has_tzcnt()) {
            // pop %rax
            emit_bytes(0x58);
            // mov $-1, %ecx
            emit_bytes(0xb9, 0xff, 0xff, 0xff, 0xff);
            // bsr %eax, %eax
            emit_bytes(0x0f, 0xbd, 0xc0);
            // cmovz %ecx, %eax
            emit_bytes(0x0f, 0x44, 0xc1);
            // sub $31, %eax
            emit_bytes(0x83, 0xe8, 0x1f);
            // neg %eax
            emit_bytes(0xf7, 0xd8);
            // push %rax
            emit_bytes(0x50);
         } else {
            // popq %rax
            emit_bytes(0x58);
            // lzcntl %eax, %eax
            emit_bytes(0xf3, 0x0f, 0xbd, 0xc0);
            // pushq %rax
            emit_bytes(0x50);
         }
      }

      void emit_i32_ctz() {
         auto icount = fixed_size_instr(has_tzcnt()?6:13);
         if(!has_tzcnt()) {
            // pop %rax
            emit_bytes(0x58);
            // mov $32, %ecx
            emit_bytes(0xb9, 0x20, 0x00, 0x00, 0x00);
            // bsf %eax, %eax
            emit_bytes(0x0f, 0xbc, 0xc0);
            // cmovz %ecx, %eax
            emit_bytes(0x0f, 0x44, 0xc1);
            // push %rax
            emit_bytes(0x50);
         } else {
            // popq %rax
            emit_bytes(0x58);
            // tzcntl %eax, %eax
            emit_bytes(0xf3, 0x0f, 0xbc, 0xc0);
            // pushq %rax
            emit_bytes(0x50);
         }
      }

      void emit_i32_popcnt() {
         auto icount = fixed_size_instr(6);
         // popq %rax
         emit_bytes(0x58);
         // popcntl %eax, %eax
         emit_bytes(0xf3, 0x0f, 0xb8, 0xc0);
         // pushq %rax
         emit_bytes(0x50);
      }

      // --------------- i32 binops ----------------------

      void emit_i32_add() {
         auto icount = fixed_size_instr(5);
         emit_i32_binop(0x01, 0xc8, 0x50);
      }
      void emit_i32_sub() {
         auto icount = fixed_size_instr(5);
         emit_i32_binop(0x29, 0xc8, 0x50);
      }
      void emit_i32_mul() {
         auto icount = fixed_size_instr(6);
         emit_i32_binop(0x0f, 0xaf, 0xc1, 0x50);
      }
      // cdq; idiv %ecx; pushq %rax
      void emit_i32_div_s() {
         auto icount = fixed_size_instr(6);
         emit_i32_binop(0x99, 0xf7, 0xf9, 0x50);
      }
      void emit_i32_div_u() {
         auto icount = fixed_size_instr(7);
         emit_i32_binop(0x31, 0xd2, 0xf7, 0xf1, 0x50);
      }
      void emit_i32_rem_s() {
         auto icount = fixed_size_instr(22);
         // pop %rcx
         emit_bytes(0x59);
         // pop %rax
         emit_bytes(0x58);
         // cmp $-1, %edx
         emit_bytes(0x83, 0xf9, 0xff);
         // je MINUS1
         emit_bytes(0x0f, 0x84);
         void* minus1 = emit_branch_target32();
         // cdq
         emit_bytes(0x99);
         // idiv %ecx
         emit_bytes(0xf7, 0xf9);
         // jmp END
         emit_bytes(0xe9);
         void* end = emit_branch_target32();
         // MINUS1:
         fix_branch(minus1, code);
         // xor %edx, %edx
         emit_bytes(0x31, 0xd2);
         // END:
         fix_branch(end, code);
         // push %rdx
         emit_bytes(0x52);
      }
      void emit_i32_rem_u() {
         auto icount = fixed_size_instr(7);
         emit_i32_binop(0x31, 0xd2, 0xf7, 0xf1, 0x52);
      }
      void emit_i32_and() {
         auto icount = fixed_size_instr(5);
         emit_i32_binop(0x21, 0xc8, 0x50);
      }
      void emit_i32_or() {
         auto icount = fixed_size_instr(5);
         emit_i32_binop(0x09, 0xc8, 0x50);
      }
      void emit_i32_xor() {
         auto icount = fixed_size_instr(5);
         emit_i32_binop(0x31, 0xc8, 0x50);
      }
      void emit_i32_shl() {
         auto icount = fixed_size_instr(5);
         emit_i32_binop(0xd3, 0xe0, 0x50);
      }
      void emit_i32_shr_s() {
         auto icount = fixed_size_instr(5);
         emit_i32_binop(0xd3, 0xf8, 0x50);
      }
      void emit_i32_shr_u() {
         auto icount = fixed_size_instr(5);
         emit_i32_binop(0xd3, 0xe8, 0x50);
      }
      void emit_i32_rotl() {
         auto icount = fixed_size_instr(5);
         emit_i32_binop(0xd3, 0xc0, 0x50);
      }
      void emit_i32_rotr() {
         auto icount = fixed_size_instr(5);
         emit_i32_binop(0xd3, 0xc8, 0x50);
      }

      // --------------- i64 unops ----------------------

      void emit_i64_clz() {
         auto icount = fixed_size_instr(has_tzcnt()?7:24);
         if(!has_tzcnt()) {
            // pop %rax
            emit_bytes(0x58);
            // mov $-1, %ecx
            emit_bytes(0x48, 0xc7, 0xc1, 0xff, 0xff, 0xff, 0xff);
            // bsr %eax, %eax
            emit_bytes(0x48, 0x0f, 0xbd, 0xc0);
            // cmovz %ecx, %eax
            emit_bytes(0x48, 0x0f, 0x44, 0xc1);
            // sub $63, %eax
            emit_bytes(0x48, 0x83, 0xe8, 0x3f);
            // neg %eax
            emit_bytes(0x48, 0xf7, 0xd8);
            // push %rax
            emit_bytes(0x50);
         } else {
            // popq %rax
            emit_bytes(0x58);
            // lzcntq %eax, %eax
            emit_bytes(0xf3, 0x48, 0x0f, 0xbd, 0xc0);
            // pushq %rax
            emit_bytes(0x50);
         }
      }

      void emit_i64_ctz() {
         auto icount = fixed_size_instr(has_tzcnt()?7:17);
         if(!has_tzcnt()) {
            // pop %rax
            emit_bytes(0x58);
            // mov $64, %ecx
            emit_bytes(0x48, 0xc7, 0xc1, 0x40, 0x00, 0x00, 0x00);
            // bsf %eax, %eax
            emit_bytes(0x48, 0x0f, 0xbc, 0xc0);
            // cmovz %ecx, %eax
            emit_bytes(0x48, 0x0f, 0x44, 0xc1);
            // push %rax
            emit_bytes(0x50);
         } else {
            // popq %rax
            emit_bytes(0x58);
            // tzcntq %eax, %eax
            emit_bytes(0xf3, 0x48, 0x0f, 0xbc, 0xc0);
            // pushq %rax
            emit_bytes(0x50);
         }
      }

      void emit_i64_popcnt() {
         auto icount = fixed_size_instr(7);
         // popq %rax
         emit_bytes(0x58);
         // popcntq %rax, %rax
         emit_bytes(0xf3, 0x48, 0x0f, 0xb8, 0xc0);
         // pushq %rax
         emit_bytes(0x50);
      }

      // --------------- i64 binops ----------------------

      void emit_i64_add() {
         auto icount = fixed_size_instr(6);
         emit_i64_binop(0x48, 0x01, 0xc8, 0x50);
      }
      void emit_i64_sub() {
         auto icount = fixed_size_instr(6);
         emit_i64_binop(0x48, 0x29, 0xc8, 0x50);
      }
      void emit_i64_mul() {
         auto icount = fixed_size_instr(7);
         emit_i64_binop(0x48, 0x0f, 0xaf, 0xc1, 0x50);
      }
      // cdq; idiv %rcx; pushq %rax
      void emit_i64_div_s() {
         auto icount = fixed_size_instr(8);
         emit_i64_binop(0x48, 0x99, 0x48, 0xf7, 0xf9, 0x50);
      }
      void emit_i64_div_u() {
         auto icount = fixed_size_instr(9);
         emit_i64_binop(0x48, 0x31, 0xd2, 0x48, 0xf7, 0xf1, 0x50);
      }
      void emit_i64_rem_s() {
         auto icount = fixed_size_instr(25);
         // pop %rcx
         emit_bytes(0x59);
         // pop %rax
         emit_bytes(0x58);
         // cmp $-1, %rcx
         emit_bytes(0x48, 0x83, 0xf9, 0xff);
         // je MINUS1
         emit_bytes(0x0f, 0x84);
         void* minus1 = emit_branch_target32();
         // cqo
         emit_bytes(0x48, 0x99);
         // idiv %rcx
         emit_bytes(0x48, 0xf7, 0xf9);
         // jmp END
         emit_bytes(0xe9);
         void* end = emit_branch_target32();
         // MINUS1:
         fix_branch(minus1, code);
         // xor %edx, %edx
         emit_bytes(0x31, 0xd2);
         // END:
         fix_branch(end, code);
         // push %rdx
         emit_bytes(0x52);
      }
      void emit_i64_rem_u() {
         auto icount = fixed_size_instr(9);
         emit_i64_binop(0x48, 0x31, 0xd2, 0x48, 0xf7, 0xf1, 0x52);
      }
      void emit_i64_and() {
         auto icount = fixed_size_instr(6);
         emit_i64_binop(0x48, 0x21, 0xc8, 0x50);
      }
      void emit_i64_or() {
         auto icount = fixed_size_instr(6);
         emit_i64_binop(0x48, 0x09, 0xc8, 0x50);
      }
      void emit_i64_xor() {
         auto icount = fixed_size_instr(6);
         emit_i64_binop(0x48, 0x31, 0xc8, 0x50);
      }
      void emit_i64_shl() {
         auto icount = fixed_size_instr(6);
         emit_i64_binop(0x48, 0xd3, 0xe0, 0x50);
      }
      void emit_i64_shr_s() {
         auto icount = fixed_size_instr(6);
         emit_i64_binop(0x48, 0xd3, 0xf8, 0x50);
      }
      void emit_i64_shr_u() {
         auto icount = fixed_size_instr(6);
         emit_i64_binop(0x48, 0xd3, 0xe8, 0x50);
      }
      void emit_i64_rotl() {
         auto icount = fixed_size_instr(6);
         emit_i64_binop(0x48, 0xd3, 0xc0, 0x50);
      }
      void emit_i64_rotr() {
         auto icount = fixed_size_instr(6);
         emit_i64_binop(0x48, 0xd3, 0xc8, 0x50);
      }

      // --------------- f32 unops ----------------------

      void emit_f32_abs() {
         auto icount = fixed_size_instr(7);
         // popq %rax; 
         emit_bytes(0x58);
         // andl 0x7fffffff, %eax
         emit_bytes(0x25);
         emit_operand32(0x7fffffff);
         // pushq %rax
         emit_bytes(0x50);
      }

      void emit_f32_neg() {
         auto icount = fixed_size_instr(7);
         // popq %rax
         emit_bytes(0x58);
         // xorl 0x80000000, %eax
         emit_bytes(0x35);
         emit_operand32(0x80000000);
         // pushq %rax
         emit_bytes(0x50);
      }

      void emit_f32_ceil() {
         auto icount = softfloat_instr(12, 36, 54);
         if constexpr (use_softfloat) {
            return emit_softfloat_unop(CHOOSE_FN(_eosio_f32_ceil));
         }
         // roundss 0b1010, (%rsp), %xmm0
         emit_bytes(0x66, 0x0f, 0x3a, 0x0a, 0x04, 0x24, 0x0a);
         // movss %xmm0, (%rsp)
         emit_bytes(0xf3, 0x0f, 0x11, 0x04, 0x24);
      }

      void emit_f32_floor() {
         auto icount = softfloat_instr(12, 36, 54);
         if constexpr (use_softfloat) {
            return emit_softfloat_unop(CHOOSE_FN(_eosio_f32_floor));
         }
         // roundss 0b1001, (%rsp), %xmm0
         emit_bytes(0x66, 0x0f, 0x3a, 0x0a, 0x04, 0x24, 0x09);
         // movss %xmm0, (%rsp)
         emit_bytes(0xf3, 0x0f, 0x11, 0x04, 0x24);
      }

      void emit_f32_trunc() {
         auto icount = softfloat_instr(12, 36, 54);
         if constexpr (use_softfloat) {
            return emit_softfloat_unop(CHOOSE_FN(_eosio_f32_trunc));
         }
         // roundss 0b1011, (%rsp), %xmm0
         emit_bytes(0x66, 0x0f, 0x3a, 0x0a, 0x04, 0x24, 0x0b);
         // movss %xmm0, (%rsp)
         emit_bytes(0xf3, 0x0f, 0x11, 0x04, 0x24);
      }

      void emit_f32_nearest() {
         auto icount = softfloat_instr(12, 36, 54);
         if constexpr (use_softfloat) {
            return emit_softfloat_unop(CHOOSE_FN(_eosio_f32_nearest));
         }
         // roundss 0b1000, (%rsp), %xmm0
         emit_bytes(0x66, 0x0f, 0x3a, 0x0a, 0x04, 0x24, 0x08);
         // movss %xmm0, (%rsp)
         emit_bytes(0xf3, 0x0f, 0x11, 0x04, 0x24);
      }

      void emit_f32_sqrt() {
         auto icount = softfloat_instr(10, 36, 54);
         if constexpr (use_softfloat) {
            return emit_softfloat_unop(CHOOSE_FN(_eosio_f32_sqrt));
         }
         // sqrtss (%rsp), %xmm0
         emit_bytes(0xf3, 0x0f, 0x51, 0x04, 0x24);
         // movss %xmm0, (%rsp)
         emit_bytes(0xf3, 0x0f, 0x11, 0x04, 0x24);
      }

      // --------------- f32 binops ----------------------

      void emit_f32_add() {
         auto icount = softfloat_instr(21, 44, 58);
         emit_f32_binop(0x58, CHOOSE_FN(_eosio_f32_add));
      }
      void emit_f32_sub() {
         auto icount = softfloat_instr(21, 44, 58);
         emit_f32_binop(0x5c, CHOOSE_FN(_eosio_f32_sub));
      }
      void emit_f32_mul() {
         auto icount = softfloat_instr(21, 44, 58);
         emit_f32_binop(0x59, CHOOSE_FN(_eosio_f32_mul));
      }
      void emit_f32_div() {
         auto icount = softfloat_instr(21, 44, 58);
         emit_f32_binop(0x5e, CHOOSE_FN(_eosio_f32_div));
      }
      void emit_f32_min() {
         auto icount = softfloat_instr(47, 44, 58);
        if constexpr(use_softfloat) {
           emit_f32_binop_softfloat(CHOOSE_FN(_eosio_f32_min));
           return;
        }
        // mov (%rsp), %eax
        emit_bytes(0x8b, 0x04, 0x24);
        // test %eax, %eax
        emit_bytes(0x85, 0xc0);
        // je ZERO
        emit_bytes(0x0f, 0x84);
        void* zero = emit_branch_target32();
        // movss 8(%rsp), %xmm0
        emit_bytes(0xf3, 0x0f, 0x10, 0x44, 0x24, 0x08);
        // minss (%rsp), %xmm0
        emit_bytes(0xf3, 0x0f, 0x5d, 0x04, 0x24);
        // jmp DONE
        emit_bytes(0xe9);
        void* done = emit_branch_target32();
        // ZERO:
        fix_branch(zero, code);
        // movss (%rsp), %xmm0
        emit_bytes(0xf3, 0x0f, 0x10, 0x04, 0x24);
        // minss 8(%rsp), %xmm0
        emit_bytes(0xf3, 0x0f, 0x5d, 0x44, 0x24, 0x08);
        // DONE:
        fix_branch(done, code);
        // add $8, %rsp
        emit_bytes(0x48, 0x83, 0xc4, 0x08);
        // movss %xmm0, (%rsp)
        emit_bytes(0xf3, 0x0f, 0x11, 0x04, 0x24);
      }
      void emit_f32_max() {
         auto icount = softfloat_instr(47, 44, 58);
        if(use_softfloat) {
           emit_f32_binop_softfloat(CHOOSE_FN(_eosio_f32_max));
           return;
        }
        // mov (%rsp), %eax
        emit_bytes(0x8b, 0x04, 0x24);
        // test %eax, %eax
        emit_bytes(0x85, 0xc0);
        // je ZERO
        emit_bytes(0x0f, 0x84);
        void* zero = emit_branch_target32();
        // movss (%rsp), %xmm0
        emit_bytes(0xf3, 0x0f, 0x10, 0x04, 0x24);
        // maxss 8(%rsp), %xmm0
        emit_bytes(0xf3, 0x0f, 0x5f, 0x44, 0x24, 0x08);
        // jmp DONE
        emit_bytes(0xe9);
        void* done = emit_branch_target32();
        // ZERO:
        fix_branch(zero, code);
        // movss 8(%rsp), %xmm0
        emit_bytes(0xf3, 0x0f, 0x10, 0x44, 0x24, 0x08);
        // maxss (%rsp), %xmm0
        emit_bytes(0xf3, 0x0f, 0x5f, 0x04, 0x24);
        // DONE:
        fix_branch(done, code);
        // add $8, %rsp
        emit_bytes(0x48, 0x83, 0xc4, 0x08);
        // movss %xmm0, (%rsp)
        emit_bytes(0xf3, 0x0f, 0x11, 0x04, 0x24);
      }

      void emit_f32_copysign() {
         auto icount = fixed_size_instr(16);
         // popq %rax; 
         emit_bytes(0x58);
         // andl 0x80000000, %eax
         emit_bytes(0x25);
         emit_operand32(0x80000000);
         // popq %rcx
         emit_bytes(0x59);
         // andl 0x7fffffff, %ecx
         emit_bytes(0x81, 0xe1);
         emit_operand32(0x7fffffff);
         // orl %ecx, %eax
         emit_bytes(0x09, 0xc8);
         // pushq %rax
         emit_bytes(0x50);
      }
      
      // --------------- f64 unops ----------------------

      void emit_f64_abs() {
         auto icount = fixed_size_instr(15);
         // popq %rcx; 
         emit_bytes(0x59);
         // movabsq $0x7fffffffffffffff, %rax
         emit_bytes(0x48, 0xb8);
         emit_operand64(0x7fffffffffffffffull);
         // andq %rcx, %rax
         emit_bytes(0x48, 0x21, 0xc8);
         // pushq %rax
         emit_bytes(0x50);
      }

      void emit_f64_neg() {
         auto icount = fixed_size_instr(15);
         // popq %rcx; 
         emit_bytes(0x59);
         // movabsq $0x8000000000000000, %rax
         emit_bytes(0x48, 0xb8);
         emit_operand64(0x8000000000000000ull);
         // xorq %rcx, %rax
         emit_bytes(0x48, 0x31, 0xc8);
         // pushq %rax
         emit_bytes(0x50);
      }

      void emit_f64_ceil() {
         auto icount = softfloat_instr(12, 38, 56);
         if constexpr (use_softfloat) {
            return emit_softfloat_unop(CHOOSE_FN(_eosio_f64_ceil));
         }
         // roundsd 0b1010, (%rsp), %xmm0
         emit_bytes(0x66, 0x0f, 0x3a, 0x0b, 0x04, 0x24, 0x0a);
         // movsd %xmm0, (%rsp)
         emit_bytes(0xf2, 0x0f, 0x11, 0x04, 0x24);
      }

      void emit_f64_floor() {
         auto icount = softfloat_instr(12, 38, 56);
         if constexpr (use_softfloat) {
            return emit_softfloat_unop(CHOOSE_FN(_eosio_f64_floor));
         }
         // roundsd 0b1001, (%rsp), %xmm0
         emit_bytes(0x66, 0x0f, 0x3a, 0x0b, 0x04, 0x24, 0x09);
         // movss %xmm0, (%rsp)
         emit_bytes(0xf2, 0x0f, 0x11, 0x04, 0x24);
      }

      void emit_f64_trunc() {
         auto icount = softfloat_instr(12, 38, 56);
         if constexpr (use_softfloat) {
            return emit_softfloat_unop(CHOOSE_FN(_eosio_f64_trunc));
         }
         // roundsd 0b1011, (%rsp), %xmm0
         emit_bytes(0x66, 0x0f, 0x3a, 0x0b, 0x04, 0x24, 0x0b);
         // movss %xmm0, (%rsp)
         emit_bytes(0xf2, 0x0f, 0x11, 0x04, 0x24);
      }

      void emit_f64_nearest() {
         auto icount = softfloat_instr(12, 38, 56);
         if constexpr (use_softfloat) {
            return emit_softfloat_unop(CHOOSE_FN(_eosio_f64_nearest));
         }
         // roundsd 0b1000, (%rsp), %xmm0
         emit_bytes(0x66, 0x0f, 0x3a, 0x0b, 0x04, 0x24, 0x08);
         // movss %xmm0, (%rsp)
         emit_bytes(0xf2, 0x0f, 0x11, 0x04, 0x24);
      }

      void emit_f64_sqrt() {
         auto icount = softfloat_instr(10, 38, 56);
         if constexpr (use_softfloat) {
            return emit_softfloat_unop(CHOOSE_FN(_eosio_f64_sqrt));
         }
         // sqrtss (%rsp), %xmm0
         emit_bytes(0xf2, 0x0f, 0x51, 0x04, 0x24);
         // movss %xmm0, (%rsp)
         emit_bytes(0xf2, 0x0f, 0x11, 0x04, 0x24);
      }

      // --------------- f64 binops ----------------------

      void emit_f64_add() {
         auto icount = softfloat_instr(21, 47, 61);
         emit_f64_binop(0x58, CHOOSE_FN(_eosio_f64_add));
      }
      void emit_f64_sub() {
         auto icount = softfloat_instr(21, 47, 61);
         emit_f64_binop(0x5c, CHOOSE_FN(_eosio_f64_sub));
      }
      void emit_f64_mul() {
         auto icount = softfloat_instr(21, 47, 61);
         emit_f64_binop(0x59, CHOOSE_FN(_eosio_f64_mul));
      }
      void emit_f64_div() {
         auto icount = softfloat_instr(21, 47, 61);
         emit_f64_binop(0x5e, CHOOSE_FN(_eosio_f64_div));
      }
      void emit_f64_min() {
         auto icount = softfloat_instr(49, 47, 61);
         if(use_softfloat) {
            emit_f64_binop_softfloat(CHOOSE_FN(_eosio_f64_min));
            return;
         }
         // mov (%rsp), %rax
         emit_bytes(0x48, 0x8b, 0x04, 0x24);
         // test %rax, %rax
         emit_bytes(0x48, 0x85, 0xc0);
         // je ZERO
         emit_bytes(0x0f, 0x84);
         void* zero = emit_branch_target32();
         // movsd 8(%rsp), %xmm0
         emit_bytes(0xf2, 0x0f, 0x10, 0x44, 0x24, 0x08);
         // minsd (%rsp), %xmm0
         emit_bytes(0xf2, 0x0f, 0x5d, 0x04, 0x24);
         // jmp DONE
         emit_bytes(0xe9);
         void* done = emit_branch_target32();
         // ZERO:
         fix_branch(zero, code);
         // movsd (%rsp), %xmm0
         emit_bytes(0xf2, 0x0f, 0x10, 0x04, 0x24);
         // minsd 8(%rsp), %xmm0
         emit_bytes(0xf2, 0x0f, 0x5d, 0x44, 0x24, 0x08);
         // DONE:
         fix_branch(done, code);
         // add $8, %rsp
         emit_bytes(0x48, 0x83, 0xc4, 0x08);
         // movsd %xmm0, (%rsp)
         emit_bytes(0xf2, 0x0f, 0x11, 0x04, 0x24);
      }
      void emit_f64_max() {
         auto icount = softfloat_instr(49, 47, 61);
         if(use_softfloat) {
            emit_f64_binop_softfloat(CHOOSE_FN(_eosio_f64_max));
            return;
         }
         // mov (%rsp), %rax
         emit_bytes(0x48, 0x8b, 0x04, 0x24);
         // test %rax, %rax
         emit_bytes(0x48, 0x85, 0xc0);
         // je ZERO
         emit_bytes(0x0f, 0x84);
         void* zero = emit_branch_target32();
         // maxsd (%rsp), %xmm0
         emit_bytes(0xf2, 0x0f, 0x10, 0x04, 0x24);
         // maxsd 8(%rsp), %xmm0
         emit_bytes(0xf2, 0x0f, 0x5f, 0x44, 0x24, 0x08);
         // jmp DONE
         emit_bytes(0xe9);
         void* done = emit_branch_target32();
         // ZERO:
         fix_branch(zero, code);
         // movsd 8(%rsp), %xmm0
         emit_bytes(0xf2, 0x0f, 0x10, 0x44, 0x24, 0x08);
         // maxsd (%rsp), %xmm0
         emit_bytes(0xf2, 0x0f, 0x5f, 0x04, 0x24);
         // DONE:
         fix_branch(done, code);
         // add $8, %rsp
         emit_bytes(0x48, 0x83, 0xc4, 0x08);
         // movsd %xmm0, (%rsp)
         emit_bytes(0xf2, 0x0f, 0x11, 0x04, 0x24);
      }

      void emit_f64_copysign() {
         auto icount = fixed_size_instr(25);
         // popq %rcx; 
         emit_bytes(0x59);
         // movabsq 0x8000000000000000, %rax
         emit_bytes(0x48, 0xb8);
         emit_operand64(0x8000000000000000ull);
         // andq %rax, %rcx
         emit_bytes(0x48, 0x21, 0xc1);
         // popq %rdx
         emit_bytes(0x5a);
         // notq %rax
         emit_bytes(0x48, 0xf7, 0xd0);
         // andq %rdx, %rax
         emit_bytes(0x48, 0x21, 0xd0);
         // orq %rcx, %rax
         emit_bytes(0x48, 0x09, 0xc8);
         // pushq %rax
         emit_bytes(0x50);
      }

      // --------------- conversions --------------------


      void emit_i32_wrap_i64() {
         auto icount = fixed_size_instr(6);
         // Zero out the high 4 bytes
         // xor %eax, %eax
         emit_bytes(0x31, 0xc0);
         // mov %eax, 4(%rsp)
         emit_bytes(0x89, 0x44, 0x24, 0x04);
      }

      void emit_i32_trunc_s_f32() {
         auto icount = softfloat_instr(33, 36, 54);
         if constexpr (use_softfloat) {
            return emit_softfloat_unop(CHOOSE_FN(softfloat_trap<&_eosio_f32_trunc_i32s>()));
         }
         // cvttss2si 8(%rsp), %eax
         emit_f2i(0xf3, 0x0f, 0x2c, 0x44, 0x24, 0x08);
         // mov %eax, (%rsp)
         emit_bytes(0x89, 0x04 ,0x24);
      }

      void emit_i32_trunc_u_f32() {
         auto icount = softfloat_instr(46, 36, 54);
         if constexpr (use_softfloat) {
            return emit_softfloat_unop(CHOOSE_FN(softfloat_trap<&_eosio_f32_trunc_i32u>()));
         }
         // cvttss2si 8(%rsp), %rax
         emit_f2i(0xf3, 0x48, 0x0f, 0x2c, 0x44, 0x24, 0x08);
         // mov %eax, (%rsp)
         emit_bytes(0x89, 0x04 ,0x24);
         // shr $32, %rax
         emit_bytes(0x48, 0xc1, 0xe8, 0x20);
         // test %eax, %eax
         emit_bytes(0x85, 0xc0);
         // jnz FP_ERROR_HANDLER
         emit_bytes(0x0f, 0x85);
         fix_branch(emit_branch_target32(), fpe_handler);
      }
      void emit_i32_trunc_s_f64() {
         auto icount = softfloat_instr(34, 38, 56);
         if constexpr (use_softfloat) {
            return emit_softfloat_unop(CHOOSE_FN(softfloat_trap<&_eosio_f64_trunc_i32s>()));
         }
         // cvttsd2si 8(%rsp), %eax
         emit_f2i(0xf2, 0x0f, 0x2c, 0x44, 0x24, 0x08);
         // movq %rax, (%rsp)
         emit_bytes(0x48, 0x89, 0x04 ,0x24);
      }

      void emit_i32_trunc_u_f64() {
         auto icount = softfloat_instr(47, 38, 56);
         if constexpr (use_softfloat) {
            return emit_softfloat_unop(CHOOSE_FN(softfloat_trap<&_eosio_f64_trunc_i32u>()));
         }
         // cvttsd2si 8(%rsp), %rax
         emit_f2i(0xf2, 0x48, 0x0f, 0x2c, 0x44, 0x24, 0x08);
         // movq %rax, (%rsp)
         emit_bytes(0x48, 0x89, 0x04 ,0x24);
         // shr $32, %rax
         emit_bytes(0x48, 0xc1, 0xe8, 0x20);
         // test %eax, %eax
         emit_bytes(0x85, 0xc0);
         // jnz FP_ERROR_HANDLER
         emit_bytes(0x0f, 0x85);
         fix_branch(emit_branch_target32(), fpe_handler);
      }

      void emit_i64_extend_s_i32() {
         auto icount = fixed_size_instr(8);
         // movslq (%rsp), %rax
         emit_bytes(0x48, 0x63, 0x04, 0x24);
         // mov %rax, (%rsp)
         emit_bytes(0x48, 0x89, 0x04, 0x24);
      }

      void emit_i64_extend_u_i32() { /* Nothing to do */ }
      
      void emit_i64_trunc_s_f32() {
         auto icount = softfloat_instr(35, 37, 55);
         if constexpr (use_softfloat) {
            return emit_softfloat_unop(CHOOSE_FN(softfloat_trap<&_eosio_f32_trunc_i64s>()));
         }
         // cvttss2si (%rsp), %rax
         emit_f2i(0xf3, 0x48, 0x0f, 0x2c, 0x44, 0x24, 0x08);
         // mov %rax, (%rsp)
         emit_bytes(0x48, 0x89, 0x04 ,0x24);
      }
      void emit_i64_trunc_u_f32() {
         auto icount = softfloat_instr(101, 37, 55);
         if constexpr (use_softfloat) {
            return emit_softfloat_unop(CHOOSE_FN(softfloat_trap<&_eosio_f32_trunc_i64u>()));
         }
         // mov $0x5f000000, %eax
         emit_bytes(0xb8);
         emit_operand32(0x5f000000);
         // movss (%rsp), %xmm0
         emit_bytes(0xf3, 0x0f, 0x10, 0x04, 0x24);
         // mov %eax, (%rsp)
         emit_bytes(0x89, 0x04, 0x24);
         // movss (%rsp), %xmm1
         emit_bytes(0xf3, 0x0f, 0x10, 0x0c, 0x24);
         // movaps %xmm0, %xmm2
         emit_bytes(0x0f, 0x28, 0xd0);
         // subss %xmm1, %xmm2
         emit_bytes(0xf3, 0x0f, 0x5c, 0xd1);
         // cvttss2siq %xmm2, %rax
         emit_f2i(0xf3, 0x48, 0x0f, 0x2c, 0xc2);
         // movabsq $0x8000000000000000, %rcx
         emit_bytes(0x48, 0xb9);
         emit_operand64(0x8000000000000000);
         // xorq %rax, %rcx
         emit_bytes(0x48, 0x31, 0xc1);
         // cvttss2siq %xmm0, %rax
         emit_bytes(0xf3, 0x48, 0x0f, 0x2c, 0xc0);
         // xor %rdx, %rdx
         emit_bytes(0x48, 0x31, 0xd2);
         // ucomiss %xmm0, %xmm1
         emit_bytes(0x0f, 0x2e, 0xc8);
         // cmovaq %rax, %rdx
         emit_bytes(0x48, 0x0f, 0x47, 0xd0);
         // cmovbeq %rcx, %rax
         emit_bytes(0x48, 0x0f, 0x46, 0xc1);
         // mov %rax, (%rsp)
         emit_bytes(0x48, 0x89, 0x04, 0x24);
         // bt $63, %rdx
         emit_bytes(0x48, 0x0f, 0xba, 0xe2, 0x3f);
         // jc FP_ERROR_HANDLER
         emit_bytes(0x0f, 0x82);
         fix_branch(emit_branch_target32(), fpe_handler);
      }
      void emit_i64_trunc_s_f64() {
         auto icount = softfloat_instr(35, 38, 56);
         if constexpr (use_softfloat) {
            return emit_softfloat_unop(CHOOSE_FN(softfloat_trap<&_eosio_f64_trunc_i64s>()));
         }
         // cvttsd2si (%rsp), %rax
         emit_f2i(0xf2, 0x48, 0x0f, 0x2c, 0x44, 0x24, 0x08);
         // mov %rax, (%rsp)
         emit_bytes(0x48, 0x89, 0x04 ,0x24);
      }
      void emit_i64_trunc_u_f64() {
         auto icount = softfloat_instr(109, 38, 56);
         if constexpr (use_softfloat) {
            return emit_softfloat_unop(CHOOSE_FN(softfloat_trap<&_eosio_f64_trunc_i64u>()));
         }
         // movabsq $0x43e0000000000000, %rax
         emit_bytes(0x48, 0xb8);
         emit_operand64(0x43e0000000000000);
         // movsd (%rsp), %xmm0
         emit_bytes(0xf2, 0x0f, 0x10, 0x04, 0x24);
         // movq %rax, (%rsp)
         emit_bytes(0x48, 0x89, 0x04, 0x24);
         // movsd (%rsp), %xmm1
         emit_bytes(0xf2, 0x0f, 0x10, 0x0c, 0x24);
         // movapd %xmm0, %xmm2
         emit_bytes(0x66, 0x0f, 0x28, 0xd0);
         // subsd %xmm1, %xmm2
         emit_bytes(0xf2, 0x0f, 0x5c, 0xd1);
         // cvttsd2siq %xmm2, %rax
         emit_f2i(0xf2, 0x48, 0x0f, 0x2c, 0xc2);
         // movabsq $0x8000000000000000, %rcx
         emit_bytes(0x48, 0xb9);
         emit_operand64(0x8000000000000000);
         // xorq %rax, %rcx
         emit_bytes(0x48, 0x31, 0xc1);
         // cvttsd2siq %xmm0, %rax
         emit_bytes(0xf2, 0x48, 0x0f, 0x2c, 0xc0);
         // xor %rdx, %rdx
         emit_bytes(0x48, 0x31, 0xd2);
         // ucomisd %xmm0, %xmm1
         emit_bytes(0x66, 0x0f, 0x2e, 0xc8);
         // cmovaq %rax, %rdx
         emit_bytes(0x48, 0x0f, 0x47, 0xd0);
         // cmovbeq %rcx, %rax
         emit_bytes(0x48, 0x0f, 0x46, 0xc1);
         // mov %rax, (%rsp)
         emit_bytes(0x48, 0x89, 0x04, 0x24);
         // bt $63, %rdx
         emit_bytes(0x48, 0x0f, 0xba, 0xe2, 0x3f);
         // jc FP_ERROR_HANDLER
         emit_bytes(0x0f, 0x82);
         fix_branch(emit_branch_target32(), fpe_handler);
      }

      void emit_f32_convert_s_i32() {
         auto icount = softfloat_instr(10, 36, 54);
         if constexpr (use_softfloat) {
            return emit_softfloat_unop(CHOOSE_FN(_eosio_i32_to_f32));
         }
         // cvtsi2ssl (%rsp), %xmm0
         emit_bytes(0xf3, 0x0f, 0x2a, 0x04, 0x24);
         // movss %xmm0, (%rsp)
         emit_bytes(0xf3, 0x0f, 0x11, 0x04, 0x24);
      }
      void emit_f32_convert_u_i32() {
         auto icount = softfloat_instr(11, 36, 54);
         if constexpr (use_softfloat) {
            return emit_softfloat_unop(CHOOSE_FN(_eosio_ui32_to_f32));
         }
         // zero-extend to 64-bits
         // cvtsi2sslq (%rsp), %xmm0
         emit_bytes(0xf3, 0x48, 0x0f, 0x2a, 0x04, 0x24);
         // movss %xmm0, (%rsp)
         emit_bytes(0xf3, 0x0f, 0x11, 0x04, 0x24);
      }
      void emit_f32_convert_s_i64() {
         auto icount = softfloat_instr(11, 38, 56);
         if constexpr (use_softfloat) {
            return emit_softfloat_unop(CHOOSE_FN(_eosio_i64_to_f32));
         }
         // cvtsi2sslq (%rsp), %xmm0
         emit_bytes(0xf3, 0x48, 0x0f, 0x2a, 0x04, 0x24);
         // movss %xmm0, (%rsp)
         emit_bytes(0xf3, 0x0f, 0x11, 0x04, 0x24);
      }
      void emit_f32_convert_u_i64() {
         auto icount = softfloat_instr(55, 38, 56);
         if constexpr (use_softfloat) {
            return emit_softfloat_unop(CHOOSE_FN(_eosio_ui64_to_f32));
         }
        // movq (%rsp), %rax
        emit_bytes(0x48, 0x8b, 0x04, 0x24);
        // testq %rax, %rax
        emit_bytes(0x48, 0x85, 0xc0);
        // js LARGE
        emit_bytes(0x0f, 0x88);
        void * large = emit_branch_target32();
        // cvtsi2ssq %rax, %xmm0
        emit_bytes(0xf3, 0x48, 0x0f, 0x2a, 0xc0);
        // jmp done
        emit_bytes(0xe9);
        void* done = emit_branch_target32();
        // LARGE:
        fix_branch(large, code);
        // movq %rax, %rcx
        emit_bytes(0x48, 0x89, 0xc1);
        // shrq %rax
        emit_bytes(0x48, 0xd1, 0xe8);
        // andl $1, %ecx
        emit_bytes(0x83, 0xe1, 0x01);
        // orq %rcx, %rax
        emit_bytes(0x48, 0x09, 0xc8);
        // cvtsi2ssq %rax, %xmm0
        emit_bytes(0xf3, 0x48, 0x0f, 0x2a, 0xc0);
        // addss %xmm0, %xmm0
        emit_bytes(0xf3, 0x0f, 0x58, 0xc0);
        // DONE:
        fix_branch(done, code);
        // xorl %eax, %eax
        emit_bytes(0x31, 0xc0);
        // movl %eax, 4(%rsp)
        emit_bytes(0x89, 0x44, 0x24, 0x04);
        // movss %xmm0, (%rsp)
        emit_bytes(0xf3, 0x0f, 0x11, 0x04, 0x24);
      }
      void emit_f32_demote_f64() {
         auto icount = softfloat_instr(16, 38, 56);
         if constexpr (use_softfloat) {
            return emit_softfloat_unop(CHOOSE_FN(_eosio_f64_demote));
         }
         // cvtsd2ss (%rsp), %xmm0
         emit_bytes(0xf2, 0x0f, 0x5a, 0x04, 0x24);
         // movss %xmm0, (%rsp)
         emit_bytes(0xf3, 0x0f, 0x11, 0x04, 0x24);
         // Zero out the high 4 bytes
         // xor %eax, %eax
         emit_bytes(0x31, 0xc0);
         // mov %eax, 4(%rsp)
         emit_bytes(0x89, 0x44, 0x24, 0x04);
      }
      void emit_f64_convert_s_i32() {
         auto icount = softfloat_instr(10, 37, 55);
         if constexpr (use_softfloat) {
            return emit_softfloat_unop(CHOOSE_FN(_eosio_i32_to_f64));
         }
         // cvtsi2sdl (%rsp), %xmm0
         emit_bytes(0xf2, 0x0f, 0x2a, 0x04, 0x24);
         // movsd %xmm0, (%rsp)
         emit_bytes(0xf2, 0x0f, 0x11, 0x04, 0x24);
      }
      void emit_f64_convert_u_i32() {
         auto icount = softfloat_instr(11, 37, 55);
         if constexpr (use_softfloat) {
            return emit_softfloat_unop(CHOOSE_FN(_eosio_ui32_to_f64));
         }
         //  cvtsi2sdq (%rsp), %xmm0
         emit_bytes(0xf2, 0x48, 0x0f, 0x2a, 0x04, 0x24);
         // movsd %xmm0, (%rsp)
         emit_bytes(0xf2, 0x0f, 0x11, 0x04, 0x24);
      }
      void emit_f64_convert_s_i64() {
         auto icount = softfloat_instr(11, 38, 56);
         if constexpr (use_softfloat) {
            return emit_softfloat_unop(CHOOSE_FN(_eosio_i64_to_f64));
         }
         //  cvtsi2sdq (%rsp), %xmm0
         emit_bytes(0xf2, 0x48, 0x0f, 0x2a, 0x04, 0x24);
         // movsd %xmm0, (%rsp)
         emit_bytes(0xf2, 0x0f, 0x11, 0x04, 0x24);
      }
      void emit_f64_convert_u_i64() {
         auto icount = softfloat_instr(49, 38, 56);
         if constexpr (use_softfloat) {
            return emit_softfloat_unop(CHOOSE_FN(_eosio_ui64_to_f64));
         }
        // movq (%rsp), %rax
        emit_bytes(0x48, 0x8b, 0x04, 0x24);
        // testq %rax, %rax
        emit_bytes(0x48, 0x85, 0xc0);
        // js LARGE
        emit_bytes(0x0f, 0x88);
        void * large = emit_branch_target32();
        // cvtsi2sdq %rax, %xmm0
        emit_bytes(0xf2, 0x48, 0x0f, 0x2a, 0xc0);
        // jmp done
        emit_bytes(0xe9);
        void* done = emit_branch_target32();
        // LARGE:
        fix_branch(large, code);
        // movq %rax, %rcx
        emit_bytes(0x48, 0x89, 0xc1);
        // shrq %rax
        emit_bytes(0x48, 0xd1, 0xe8);
        // andl $1, %ecx
        emit_bytes(0x83, 0xe1, 0x01);
        // orq %rcx, %rax
        emit_bytes(0x48, 0x09, 0xc8);
        // cvtsi2sdq %rax, %xmm0
        emit_bytes(0xf2, 0x48, 0x0f, 0x2a, 0xc0);
        // addsd %xmm0, %xmm0
        emit_bytes(0xf2, 0x0f, 0x58, 0xc0);
        // DONE:
        fix_branch(done, code);
        // movsd %xmm0, (%rsp)
        emit_bytes(0xf2, 0x0f, 0x11, 0x04, 0x24);
      }
      void emit_f64_promote_f32() {
         auto icount = softfloat_instr(10, 37, 55);
         if constexpr (use_softfloat) {
            return emit_softfloat_unop(CHOOSE_FN(_eosio_f32_promote));
         }
         // cvtss2sd (%rsp), %xmm0
         emit_bytes(0xf3, 0x0f, 0x5a, 0x04, 0x24);
         // movsd %xmm0, (%rsp)
         emit_bytes(0xf2, 0x0f, 0x11, 0x04, 0x24);
      }
      
      void emit_i32_reinterpret_f32() { /* Nothing to do */ }
      void emit_i64_reinterpret_f64() { /* Nothing to do */ }
      void emit_f32_reinterpret_i32() { /* Nothing to do */ }
      void emit_f64_reinterpret_i64() { /* Nothing to do */ }

#undef CHOOSE_FN

      void emit_v128_load(uint32_t /*alignment*/, uint32_t offset)
      {
         emit_pop_address(offset);
         // movups (%rax), %xmm0
         emit_bytes(0x0f, 0x10, 0x00);
         // sub $16, %rsp
         emit_bytes(0x48, 0x83, 0xec, 0x10);
         // movups %xmm0, (%rsp)
         emit_bytes(0x0f, 0x11, 0x04, 0x24);
      }

      void emit_v128_load8x8_s(uint32_t align, uint32_t offset)
      {
         emit_pop_address(offset);
         // vpmovsxbw (%rax), %xmm0
         emit_bytes(0xc4, 0xe2, 0x79, 0x20, 0x00);
         emit_sub(0x10, rsp);
         emit_movups(xmm0, *rsp);
      }

      void emit_v128_load8x8_u(uint32_t align, uint32_t offset)
      {
         emit_pop_address(offset);
         // vpmovzxbw (%rax), %xmm0
         emit_bytes(0xc4, 0xe2, 0x79, 0x30, 0x00);
         emit_sub(0x10, rsp);
         emit_movups(xmm0, *rsp);
      }

      void emit_v128_load16x4_s(uint32_t align, uint32_t offset)
      {
         emit_pop_address(offset);
         // vpmovsxwd (%rax), %xmm0
         emit_bytes(0xc4, 0xe2, 0x79, 0x23, 0x00);
         emit_sub(0x10, rsp);
         emit_movups(xmm0, *rsp);
      }

      void emit_v128_load16x4_u(uint32_t align, uint32_t offset)
      {
         emit_pop_address(offset);
         // vpmovzxwd (%rax), %xmm0
         emit_bytes(0xc4, 0xe2, 0x79, 0x33, 0x00);
         emit_sub(0x10, rsp);
         emit_movups(xmm0, *rsp);
      }

      void emit_v128_load32x2_s(uint32_t align, uint32_t offset)
      {
         emit_pop_address(offset);
         // vpmovsxdq (%rax), %xmm0
         emit_bytes(0xc4, 0xe2, 0x79, 0x25, 0x00);
         emit_sub(0x10, rsp);
         emit_movups(xmm0, *rsp);
      }

      void emit_v128_load32x2_u(uint32_t align, uint32_t offset)
      {
         emit_pop_address(offset);
         // vpmovzxdq (%rax), %xmm0
         emit_bytes(0xc4, 0xe2, 0x79, 0x35, 0x00);
         emit_sub(0x10, rsp);
         emit_movups(xmm0, *rsp);
      }

      void emit_v128_load8_splat(uint32_t align, uint32_t offset)
      {
         emit_pop_address(offset);
         // vpbroadcastb (%rax), %xmm0
         emit_bytes(0xc4, 0xe2, 0x79, 0x78, 0x00);
         emit_sub(0x10, rsp);
         emit_movups(xmm0, *rsp);
      }

      void emit_v128_load16_splat(uint32_t align, uint32_t offset)
      {
         emit_pop_address(offset);
         // vpbroadcastw (%rax), %xmm0
         emit_bytes(0xc4, 0xe2, 0x79, 0x79, 0x00);
         emit_sub(0x10, rsp);
         emit_movups(xmm0, *rsp);
      }

      void emit_v128_load32_splat(uint32_t align, uint32_t offset)
      {
         emit_pop_address(offset);
         // vpbroadcastd (%rax), %xmm0
         emit_bytes(0xc4, 0xe2, 0x79, 0x58, 0x00);
         emit_sub(0x10, rsp);
         emit_movups(xmm0, *rsp);
      }

      void emit_v128_load64_splat(uint32_t align, uint32_t offset)
      {
         emit_pop_address(offset);
         // vpbroadcastq (%rax), %xmm0
         emit_bytes(0xc4, 0xe2, 0x79, 0x59, 0x00);
         emit_sub(0x10, rsp);
         emit_movups(xmm0, *rsp);
      }

      void emit_v128_load32_zero(uint32_t align, uint32_t offset)
      {
         emit_pop_address(offset);
         // vmovd (%rax), %xmm0
         emit_bytes(0xc5, 0xf9, 0x6e, 0x00);
         emit_sub(0x10, rsp);
         emit_movups(xmm0, *rsp);
      }

      void emit_v128_load64_zero(uint32_t align, uint32_t offset)
      {
         emit_pop_address(offset);
         // vmovq (%rax), %xmm0
         emit_bytes(0xc5, 0xfa, 0x7e, 0x00);
         emit_sub(0x10, rsp);
         emit_movups(xmm0, *rsp);
      }

      void emit_v128_store(uint32_t /*alignment*/, uint32_t offset)
      {
         // movups (%rsp), %xmm0
         emit_bytes(0x0f, 0x10, 0x04, 0x24);
         // add $16, %rsp
         emit_bytes(0x48, 0x83, 0xc4, 0x10);
         emit_pop_address(offset);
         // movups %xmm0, (%rax)
         emit_bytes(0x0f, 0x11, 0x00);
      }

      void emit_v128_load8_lane(uint32_t /*alignment*/, uint32_t offset, uint8_t lane)
      {
         emit_movups(*rsp, xmm0);
         emit_add(16, rsp);
         emit_pop_address(offset);
         // vpinsrb $lane, (%rax), %xmm0, %xmm0
         emit_bytes(0xc4, 0xe3, 0x79, 0x20, 0x00, lane);
         emit_sub(16, rsp);
         emit_movups(xmm0, *rsp);
      }

      void emit_v128_load16_lane(uint32_t /*alignment*/, uint32_t offset, uint8_t lane)
      {
         emit_movups(*rsp, xmm0);
         emit_add(16, rsp);
         emit_pop_address(offset);
         // vpinsrw $lane, (%rax), %xmm0, %xmm0
         emit_bytes(0xc5, 0xf9, 0xc4, 0x00, lane);
         emit_sub(16, rsp);
         emit_movups(xmm0, *rsp);
      }

      void emit_v128_load32_lane(uint32_t /*alignment*/, uint32_t offset, uint8_t lane)
      {
         emit_movups(*rsp, xmm0);
         emit_add(16, rsp);
         emit_pop_address(offset);
         // vpinsrd $lane, (%rax), %xmm0, %xmm0
         emit_bytes(0xc4, 0xe3, 0x79, 0x22, 0x00, lane);
         emit_sub(16, rsp);
         emit_movups(xmm0, *rsp);
      }

      void emit_v128_load64_lane(uint32_t /*alignment*/, uint32_t offset, uint8_t lane)
      {
         emit_movups(*rsp, xmm0);
         emit_add(16, rsp);
         emit_pop_address(offset);
         // vpinsrq $lane, (%rax), %xmm0, %xmm0
         emit_bytes(0xc4, 0xe3, 0xf9, 0x22, 0x00, lane);
         emit_sub(16, rsp);
         emit_movups(xmm0, *rsp);
      }

      void emit_v128_store8_lane(uint32_t /*alignment*/, uint32_t offset, uint8_t lane)
      {
         emit_movups(*rsp, xmm0);
         emit_add(16, rsp);
         emit_pop_address(offset);
         // vpextrb $lane, %xmm0, (%rax)
         emit_bytes(0xc4, 0xe3, 0x79, 0x14, 0x00, lane);
      }

      void emit_v128_store16_lane(uint32_t /*alignment*/, uint32_t offset, uint8_t lane)
      {
         emit_movups(*rsp, xmm0);
         emit_add(16, rsp);
         emit_pop_address(offset);
         // vpextrw $lane, %xmm0, (%rax)
         emit_bytes(0xc4, 0xe3, 0x79, 0x15, 0x00, lane);
      }

      void emit_v128_store32_lane(uint32_t /*alignment*/, uint32_t offset, uint8_t lane)
      {
         emit_movups(*rsp, xmm0);
         emit_add(16, rsp);
         emit_pop_address(offset);
         // vpextrd $lane, %xmm0, (%rax)
         emit_bytes(0xc4, 0xe3, 0x79, 0x16, 0x00, lane);
      }

      void emit_v128_store64_lane(uint32_t /*alignment*/, uint32_t offset, uint8_t lane)
      {
         emit_movups(*rsp, xmm0);
         emit_add(16, rsp);
         emit_pop_address(offset);
         // vpextrq $lane, %xmm0, (%rax)
         emit_bytes(0xc4, 0xe3, 0xf9, 0x16, 0x00, lane);
      }

      void emit_v128_const(v128_t value)
      {
         uint64_t low,high;
         memcpy(&high, reinterpret_cast<const char*>(&value) + 8, 8);
         memcpy(&low, &value, 8);
         emit_i64_const(high);
         emit_i64_const(low);
      }

      void emit_i8x16_shuffle(const uint8_t lanes[16])
      {
         auto emit_shuffle_operand = [this](const uint8_t lanes[8]) {
            for(int i = 0; i < 8; ++i)
            {
               if(lanes[i] < 16)
               {
                  emit_byte(~lanes[i]);
               }
               else
               {
                  emit_byte(lanes[i]);
               }
            }
         };
         // general case:
         // movabsq $lanes[0-7], %rax
         emit_bytes(0x48, 0xb8);
         emit_shuffle_operand(lanes);
         // vmovq %rax, %xmm2
         emit_bytes(0xc4, 0xe1, 0xf9, 0x6e, 0xd0);
         // movabsq $lanes[8-15], %rax
         emit_bytes(0x48, 0xb8);
         emit_shuffle_operand(lanes + 8);
         // vpinsrq $1, %rax, %xmm2, %xmm2
         emit_bytes(0xc4, 0xe3, 0xe9, 0x22, 0xd0, 0x01);

         emit_movups(*rsp, xmm0);
         // vpshufb %xmm2, %xmm0, %xmm1
         emit_bytes(0xc4, 0xe2, 0x79, 0x00, 0xca);
         // vpcmpeqb %xmm0, %xmm0, %xmm0
         emit_bytes(0xc5, 0xf9, 0x74, 0xc0);
         // vpxor %xmm0, %xmm2, %xmm2
         emit_bytes(0xc5, 0xe9, 0xef, 0xd0);
         emit_add(16, rsp);
         emit_movups(*rsp, xmm0);
         // vpshufb %xmm2, %xmm0, %xmm0
         emit_bytes(0xc4, 0xe2, 0x79, 0x00, 0xc2);
         // vpor %xmm1, %xmm0, %xmm0
         emit_bytes(0xc5, 0xf9, 0xeb, 0xc1);
         emit_movups(xmm0, *rsp);
      }

      void emit_i8x16_extract_lane_s(uint8_t l)
      {
         emit_movups(*rsp, xmm0);
         emit_add(16, rsp);
         // vpextrb $l, %xmm0, %eax
         emit_bytes(0xc4, 0xe3, 0x79, 0x14, 0xc0, l);
         // movsx %al, %eax
         emit_bytes(0x0f, 0xbe, 0xc0);
         emit_push(rax);
      }

      void emit_i8x16_extract_lane_u(uint8_t l)
      {
         emit_movups(*rsp, xmm0);
         emit_add(16, rsp);
         // vpextrb $l, %xmm0, %eax
         emit_bytes(0xc4, 0xe3, 0x79, 0x14, 0xc0, l);
         emit_push(rax);
      }

      void emit_i8x16_replace_lane(uint8_t l)
      {
         emit_pop(rax);
         emit_movups(*rsp, xmm0);
         // vpinsrb $l, %eax, %xmm0, %xmm0
         emit_bytes(0xc4, 0xe3, 0x79, 0x20, 0xc0, l);
         emit_movups(xmm0, *rsp);
      }

      void emit_i16x8_extract_lane_s(uint8_t l)
      {
         emit_movups(*rsp, xmm0);
         emit_add(16, rsp);
         // vpextrw $l, %xmm0, %eax
         emit_bytes(0xc5, 0xf9, 0xc5, 0xc0, l);
         // movsx %ax, %eax
         emit_bytes(0x0f, 0xbf, 0xc0);
         emit_push(rax);
      }

      void emit_i16x8_extract_lane_u(uint8_t l)
      {
         emit_movups(*rsp, xmm0);
         emit_add(16, rsp);
         // vpextrw $l, %xmm0, %eax
         emit_bytes(0xc5, 0xf9, 0xc5, 0xc0, l);
         emit_push(rax);
      }

      void emit_i16x8_replace_lane(uint8_t l)
      {
         emit_pop(rax);
         emit_movups(*rsp, xmm0);
         // vpinsrw $l, %eax, %xmm0, %xmm0
         emit_bytes(0xc5, 0xf9, 0xc4, 0xc0, l);
         emit_movups(xmm0, *rsp);
      }

      void emit_i32x4_extract_lane(uint8_t l)
      {
         emit_movups(*rsp, xmm0);
         emit_add(16, rsp);
         // vpextrd $l, %xmm0, %eax
         emit_bytes(0xc4, 0xe3, 0x79, 0x16, 0xc0, l);
         emit_push(rax);
      }

      void emit_i32x4_replace_lane(uint8_t l)
      {
         emit_pop(rax);
         emit_movups(*rsp, xmm0);
         // vpinsrd $l, %eax, %xmm0, %xmm0
         emit_bytes(0xc4, 0xe3, 0x79, 0x22, 0xc0, l);
         emit_movups(xmm0, *rsp);
      }

      void emit_i64x2_extract_lane(uint8_t l)
      {
         emit_movups(*rsp, xmm0);
         emit_add(16, rsp);
         // vpextrq $l, %xmm0, %eax
         emit_bytes(0xc4, 0xe3, 0xf9, 0x16, 0xc0, l);
         emit_push(rax);
      }

      void emit_i64x2_replace_lane(uint8_t l)
      {
         emit_pop(rax);
         emit_movups(*rsp, xmm0);
         // vpinsrq $l, %eax, %xmm0, %xmm0
         emit_bytes(0xc4, 0xe3, 0xf9, 0x22, 0xc0, l);
         emit_movups(xmm0, *rsp);
      }

      void emit_f32x4_extract_lane(uint8_t l)
      {
         emit_movups(*rsp, xmm0);
         emit_add(16, rsp);
         // vpextrd $l, %xmm0, %eax
         emit_bytes(0xc4, 0xe3, 0x79, 0x16, 0xc0, l);
         emit_push(rax);
      }

      void emit_f32x4_replace_lane(uint8_t l)
      {
         emit_pop(rax);
         emit_movups(*rsp, xmm0);
         // vpinsrd $l, %eax, %xmm0, %xmm0
         emit_bytes(0xc4, 0xe3, 0x79, 0x22, 0xc0, l);
         emit_movups(xmm0, *rsp);
      }

      void emit_f64x2_extract_lane(uint8_t l)
      {
         emit_movups(*rsp, xmm0);
         emit_add(16, rsp);
         // vpextrq $l, %xmm0, %eax
         emit_bytes(0xc4, 0xe3, 0xf9, 0x16, 0xc0, l);
         emit_push(rax);
      }

      void emit_f64x2_replace_lane(uint8_t l)
      {
         emit_pop(rax);
         emit_movups(*rsp, xmm0);
         // vpinsrq $l, %rax, %xmm0, %xmm0
         emit_bytes(0xc4, 0xe3, 0xf9, 0x22, 0xc0, l);
         emit_movups(xmm0, *rsp);
      }

      void emit_i8x16_swizzle()
      {
         // test x>15 and saturate to 255
         emit_movups(*rsp, xmm0);
         emit_add(16, rsp);
         // mov $0x0F0F0F0F, %eax
         emit_bytes(0xb8);
         emit_operand32(0x0f0f0f0f);
         // vmovd %eax, %xmm1
         emit_bytes(0xc5, 0xf9, 0x6e, 0xc8);
         // vpshufd $0, %xmm1, %xmm1
         emit_bytes(0xc5, 0xf9, 0x70, 0xc9, 0x00);
         // vpcmpgtb %xmm1, %xmm0, %xmm1
         emit_bytes(0xc5, 0xf9, 0x64, 0xc9);
         // vpor %xmm0, %xmm1, %xmm1
         emit_bytes(0xc5, 0xf1, 0xeb, 0xc8);
         emit_movups(*rsp, xmm0);
         // vpshufb %xmm0, %xmm1, %xmm0
         emit_bytes(0xc4, 0xe2, 0x71, 0x00, 0xc0);
         emit_movups(xmm0, *rsp);
      }

      void emit_i8x16_splat()
      {
         // vpbroadcastb (%rsp), %xmm0
         emit_bytes(0xc4, 0xe2, 0x79, 0x78, 0x04, 0x24);
         emit_sub(0x08, rsp);
         emit_movups(xmm0, *rsp);
      }

      void emit_i16x8_splat()
      {
         // vpbroadcastw (%rsp), %xmm0
         emit_bytes(0xc4, 0xe2, 0x79, 0x79, 0x04, 0x24);
         emit_sub(0x08, rsp);
         emit_movups(xmm0, *rsp);
      }

      void emit_i32x4_splat()
      {
         // vpbroadcastd (%rsp), %xmm0
         emit_bytes(0xc4, 0xe2, 0x79, 0x58, 0x04, 0x24);
         emit_sub(0x08, rsp);
         emit_movups(xmm0, *rsp);
      }

      void emit_i64x2_splat()
      {
         // vpbroadcastq (%rsp), %xmm0
         emit_bytes(0xc4, 0xe2, 0x79, 0x59, 0x04, 0x24);
         emit_sub(0x08, rsp);
         emit_movups(xmm0, *rsp);
      }

      void emit_f32x4_splat()
      {
         // vpbroadcastd (%rsp), %xmm0
         emit_bytes(0xc4, 0xe2, 0x79, 0x58, 0x04, 0x24);
         emit_sub(0x08, rsp);
         emit_movups(xmm0, *rsp);
      }

      void emit_f64x2_splat()
      {
         // vpbroadcastq (%rsp), %xmm0
         emit_bytes(0xc4, 0xe2, 0x79, 0x59, 0x04, 0x24);
         emit_sub(0x08, rsp);
         emit_movups(xmm0, *rsp);
      }

      void emit_i8x16_eq()
      {
         emit_vmovups(*rsp, xmm0);
         emit_add(16, rsp);
         // vpcmpeqb (%rsp), %xmm0, %xmm0
         emit_VEX_128_66_0F_WIG(0x74, *rsp, xmm0, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_i8x16_ne()
      {
         emit_vmovups(*rsp, xmm0);
         emit_add(16, rsp);
         // vpcmpeqb (%rsp), %xmm0, %xmm0
         emit_VEX_128_66_0F_WIG(0x74, *rsp, xmm0, xmm0);
         emit_const_ones(xmm1);
         emit_vpxor(xmm1, xmm0, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_i8x16_lt_s() {
         emit_v128_irelop_cmp(VPCMPGTB, true, false);
      }

      void emit_i8x16_lt_u() {
         emit_v128_irelop_minmax(VPMINUB, false);
      }

      void emit_i8x16_gt_s() {
         emit_v128_irelop_cmp(VPCMPGTB, false, false);
      }

      void emit_i8x16_gt_u() {
         emit_v128_irelop_minmax(VPMAXUB, false);
      }

      void emit_i8x16_le_s() {
         emit_v128_irelop_cmp(VPCMPGTB, false, true);
      }

      void emit_i8x16_le_u() {
         emit_v128_irelop_minmax(VPMAXUB, true);
      }

      void emit_i8x16_ge_s() {
         emit_v128_irelop_cmp(VPCMPGTB, true, true);
      }

      void emit_i8x16_ge_u() {
         emit_v128_irelop_minmax(VPMINUB, true);
      }

      // i8x16 compare

      void emit_i16x8_eq() {
         emit_v128_irelop_cmp(VPCMPEQW, true, false);
      }

      void emit_i16x8_ne() {
         emit_v128_irelop_cmp(VPCMPEQW, true, true);
      }

      void emit_i16x8_lt_s() {
         emit_v128_irelop_cmp(VPCMPGTW, true, false);
      }

      void emit_i16x8_lt_u() {
         emit_v128_irelop_minmax(VPMINUW, false);
      }

      void emit_i16x8_gt_s() {
         emit_v128_irelop_cmp(VPCMPGTW, false, false);
      }

      void emit_i16x8_gt_u() {
         emit_v128_irelop_minmax(VPMAXUW, false);
      }

      void emit_i16x8_le_s() {
         emit_v128_irelop_cmp(VPCMPGTW, false, true);
      }

      void emit_i16x8_le_u() {
         emit_v128_irelop_minmax(VPMAXUW, true);
      }

      void emit_i16x8_ge_s() {
         emit_v128_irelop_cmp(VPCMPGTW, true, true);
      }

      void emit_i16x8_ge_u() {
         emit_v128_irelop_minmax(VPMINUW, true);
      }

      // i32x4 compare

      void emit_i32x4_eq() {
         emit_v128_irelop_cmp(VPCMPEQD, true, false);
      }

      void emit_i32x4_ne() {
         emit_v128_irelop_cmp(VPCMPEQD, true, true);
      }

      void emit_i32x4_lt_s() {
         emit_v128_irelop_cmp(VPCMPGTD, true, false);
      }

      void emit_i32x4_lt_u() {
         emit_v128_irelop_minmax(VPMINUD, false);
      }

      void emit_i32x4_gt_s() {
         emit_v128_irelop_cmp(VPCMPGTD, false, false);
      }

      void emit_i32x4_gt_u() {
         emit_v128_irelop_minmax(VPMAXUD, false);
      }

      void emit_i32x4_le_s() {
         emit_v128_irelop_cmp(VPCMPGTD, false, true);
      }

      void emit_i32x4_le_u() {
         emit_v128_irelop_minmax(VPMAXUD, true);
      }

      void emit_i32x4_ge_s() {
         emit_v128_irelop_cmp(VPCMPGTD, true, true);
      }

      void emit_i32x4_ge_u() {
         emit_v128_irelop_minmax(VPMINUD, true);
      }

      // i64x2 compare

      void emit_i64x2_eq() {
         emit_v128_irelop_cmp(VPCMPEQQ, true, false);
      }

      void emit_i64x2_ne() {
         emit_v128_irelop_cmp(VPCMPEQQ, true, true);
      }

      void emit_i64x2_lt_s() {
         emit_v128_irelop_cmp(VPCMPGTQ, true, false);
      }

      void emit_i64x2_gt_s() {
         emit_v128_irelop_cmp(VPCMPGTQ, false, false);
      }

      void emit_i64x2_le_s() {
         emit_v128_irelop_cmp(VPCMPGTQ, false, true);
      }

      void emit_i64x2_ge_s() {
         emit_v128_irelop_cmp(VPCMPGTQ, true, true);
      }

      // v128 logical ops

      void emit_v128_not() {
         emit_vmovups(*rsp, xmm0);
         emit_const_ones(xmm1);
         emit(VPXOR, xmm1, xmm0, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_v128_and() {
         emit_vmovups(*rsp, xmm0);
         emit_add(16, rsp);
         emit(VPAND, *rsp, xmm0, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_v128_andnot() {
         emit_vmovups(*rsp, xmm0);
         emit_add(16, rsp);
         emit_vmovups(*rsp, xmm1);
         emit(VPANDN, xmm1, xmm0, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_v128_or() {
         emit_vmovups(*rsp, xmm0);
         emit_add(16, rsp);
         emit(VPOR, *rsp, xmm0, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_v128_xor() {
         emit_vmovups(*rsp, xmm0);
         emit_add(16, rsp);
         emit(VPXOR, *rsp, xmm0, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_v128_bitselect() {
         emit_vmovups(*rsp, xmm2);
         emit_vmovups(*(rsp + 16), xmm1);
         emit_add(32, rsp);
         emit_vmovups(*rsp, xmm0);
         // With AVX512: VPTERNLOGD 0xAC
         emit(VPAND, xmm0, xmm2, xmm0);
         emit(VPANDN, xmm1, xmm2, xmm1);
         emit(VPOR, xmm0, xmm1, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_v128_any_true() {
         emit_vmovups(*rsp, xmm0);
         emit_add(16, rsp);
         // xor %eax, %eax
         emit_bytes(0x31, 0xc0);
         emit(VPTEST, xmm0, xmm0);
         // setnz %al
         emit_bytes(0x0f, 0x95, 0xc0);
         emit_push(rax);
      }

      void emit_i8x16_abs() {
         emit_v128_unop(VPABSB);
      }

      void emit_i8x16_neg() {
         emit_const_zero(xmm0);
         emit(VPSUBB, *rsp, xmm0, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_i8x16_popcnt() {
         static const uint8_t popcnt4[] = {0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4};

         // movabsq $popcnt4, %rax
         emit_bytes(0x48, 0xb8);
         emit_operand_ptr(&popcnt4);
         emit_vmovups(*rsp, xmm0);
         emit_vmovups(*rax, xmm3);
         // mov $0xF, $al
         emit_bytes(0xb0, 0x0f);
         // vmovd %eax, %xmm2
         emit_bytes(0xc5, 0xf9, 0x6e, 0xd0);
         emit(VPBROADCASTB, xmm2, xmm2);
         emit(VPSRLQ_c, imm8{4}, xmm0, xmm1);
         emit(VPAND, xmm2, xmm0, xmm0);
         emit(VPAND, xmm2, xmm1, xmm1);
         emit(VPSHUFB, xmm0, xmm3, xmm0);
         emit(VPSHUFB, xmm1, xmm3, xmm1);
         emit(VPADDB, xmm0, xmm1, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_i8x16_all_true() {
         emit_const_zero(xmm0);
         emit(VPCMPEQB, *rsp, xmm0, xmm0);
         emit_add(16, rsp);
         // xor %eax, %eax
         emit_bytes(0x31, 0xc0);
         emit(VPTEST, xmm0, xmm0);
         // setz %al
         emit_bytes(0x0f, 0x94, 0xc0);
         emit_push(rax);
      }

      void emit_i8x16_bitmask() {
         emit_vmovups(*rsp, xmm0);
         emit_add(16, rsp);
         emit(VPMOVMSKB, xmm0, rax);
         emit_push(rax);
      }

      void emit_i8x16_narrow_i16x8_s() {
         emit_vmovups(*rsp, xmm0);
         emit_add(16, rsp);
         emit_vmovups(*rsp, xmm1);
         emit(VPACKSSWB, xmm0, xmm1, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_i8x16_narrow_i16x8_u() {
         emit_vmovups(*rsp, xmm0);
         emit_add(16, rsp);
         emit_vmovups(*rsp, xmm1);
         emit(VPACKUSWB, xmm0, xmm1, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_i8x16_shl() {
         emit_pop(rax);
         emit_vmovups(*rsp, xmm0);
         // and $7, %eax
         emit_bytes(0x83, 0xe0, 0x07);
         emit_vmovd(eax, xmm2);
         emit_const_ones(xmm1);
         emit(VPSLLD, xmm2, xmm1, xmm1);
         emit(VPBROADCASTB, xmm1, xmm1);
         emit(VPSLLW, xmm2, xmm0, xmm0);
         emit(VPAND, xmm0, xmm1, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_i8x16_shr_s() {
         emit_pop(rax);
         emit_vmovups(*rsp, xmm0);
         // and $7, %eax
         emit_bytes(0x83, 0xe0, 0x07);
         emit_vmovd(eax, xmm2);
         emit_const_ones(xmm3);
         emit(VPSLLW_c, imm8{8}, xmm3, xmm3);
         emit(VPSLLW_c, imm8{8}, xmm0, xmm1);
         emit(VPSRAW_c, imm8{8}, xmm1, xmm1);
         emit(VPSLLW, xmm2, xmm3, xmm3);
         emit(VPANDN, xmm1, xmm3, xmm1);
         emit(VPAND, xmm3, xmm0, xmm0);
         emit(VPOR, xmm1, xmm0, xmm0);
         emit(VPSRAW, xmm2, xmm0, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_i8x16_shr_u() {
         emit_pop(rax);
         emit_vmovups(*rsp, xmm0);
         // and $7, %eax
         emit_bytes(0x83, 0xe0, 0x07);
         emit_vmovd(eax, xmm2);
         emit_const_ones(xmm1);
         emit(VPSLLW_c, imm8{8}, xmm1, xmm1);
         emit(VPSRLW, xmm2, xmm1, xmm1);
         emit(VPBROADCASTB, xmm1, xmm1);
         emit(VPSRLW, xmm2, xmm0, xmm0);
         emit(VPANDN, xmm0, xmm1, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_i8x16_add() {
         emit_v128_binop_r(VPADDB);
      }

      void emit_i8x16_add_sat_s() {
         emit_v128_binop_r(VPADDSB);
      }

      void emit_i8x16_add_sat_u() {
         emit_v128_binop_r(VPADDUSB);
      }

      void emit_i8x16_sub() {
         emit_v128_binop(VPSUBB);
      }

      void emit_i8x16_sub_sat_s() {
         emit_v128_binop(VPSUBSB);
      }

      void emit_i8x16_sub_sat_u() {
         emit_v128_binop(VPSUBUSB);
      }

      void emit_i8x16_min_s() {
         emit_v128_binop_r(VPMINSB);
      }

      void emit_i8x16_min_u() {
         emit_v128_binop_r(VPMINUB);
      }

      void emit_i8x16_max_s() {
         emit_v128_binop_r(VPMAXSB);
      }

      void emit_i8x16_max_u() {
         emit_v128_binop_r(VPMAXUB);
      }

      void emit_i8x16_avgr_u() {
         emit_v128_binop_r(VPAVGB);
      }

      // i16x8 ops

      void emit_i16x8_extadd_pairwise_i8x16_s() {
         emit_vmovups(*rsp, xmm0);
         emit(VPSRLDQ_c, imm8{8}, xmm0, xmm1);
         emit(VPMOVSXBW, xmm0, xmm0);
         emit(VPMOVSXBW, xmm1, xmm1);
         emit(VPHADDW, xmm1, xmm0, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_i16x8_extadd_pairwise_i8x16_u() {
         emit_vmovups(*rsp, xmm0);
         emit(VPSRLDQ_c, imm8{8}, xmm0, xmm1);
         emit(VPMOVZXBW, xmm0, xmm0);
         emit(VPMOVZXBW, xmm1, xmm1);
         emit(VPHADDW, xmm1, xmm0, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_i16x8_abs() {
         emit_v128_unop(VPABSW);
      }

      void emit_i16x8_neg() {
         emit_const_zero(xmm0);
         emit(VPSUBW, *rsp, xmm0, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_i16x8_q15mulr_sat_s() {
         emit_vmovups(*rsp, xmm0);
         emit_add(16, rsp);
         emit(VPMULHRSW, *rsp, xmm0, xmm0);
         emit_const_ones(xmm1);
         emit(VPSLLW_c, imm8{15}, xmm1, xmm1);
         emit(VPCMPEQB, xmm1, xmm0, xmm1);
         emit(VPXOR, xmm1, xmm0, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_i16x8_all_true() {
         emit_const_zero(xmm0);
         emit(VPCMPEQW, *rsp, xmm0, xmm0);
         emit_add(16, rsp);
         // xor %eax, %eax
         emit_bytes(0x31, 0xc0);
         emit(VPTEST, xmm0, xmm0);
         // setz %al
         emit_bytes(0x0f, 0x94, 0xc0);
         emit_push(rax);
      }

      void emit_i16x8_bitmask() {
         emit_const_zero(xmm0);
         emit(VPCMPGTW, *rsp, xmm0, xmm1);
         emit(VPACKSSWB, xmm0, xmm1, xmm0);
         emit_add(16, rsp);
         emit(VPMOVMSKB, xmm0, rax);
         emit_push(rax);
      }

      void emit_i16x8_narrow_i32x4_s() {
         emit_v128_binop(VPACKSSDW);
      }

      void emit_i16x8_narrow_i32x4_u() {
         emit_v128_binop(VPACKUSDW);
      }

      void emit_i16x8_extend_low_i8x16_s() {
         emit_v128_unop(VPMOVSXBW);
      }

      void emit_i16x8_extend_high_i8x16_s() {
         emit_vmovups(*rsp, xmm0);
         emit(VPSRLDQ_c, imm8{8}, xmm0, xmm0);
         emit(VPMOVSXBW, xmm0, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_i16x8_extend_low_i8x16_u() {
         emit_v128_unop(VPMOVZXBW);
      }

      void emit_i16x8_extend_high_i8x16_u() {
         emit_vmovups(*rsp, xmm0);
         emit(VPSRLDQ_c, imm8{8}, xmm0, xmm0);
         emit(VPMOVZXBW, xmm0, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_i16x8_shl() {
         emit_v128_shiftop(VPSLLW, 0x0f);
      }

      void emit_i16x8_shr_s() {
         emit_v128_shiftop(VPSRAW, 0x0f);
      }

      void emit_i16x8_shr_u() {
         emit_v128_shiftop(VPSRLW, 0x0f);
      }

      void emit_i16x8_add() {
         emit_v128_binop_r(VPADDW);
      }

      void emit_i16x8_add_sat_s() {
         emit_v128_binop_r(VPADDSW);
      }

      void emit_i16x8_add_sat_u() {
         emit_v128_binop_r(VPADDUSW);
      }

      void emit_i16x8_sub() {
         emit_v128_binop(VPSUBW);
      }

      void emit_i16x8_sub_sat_s() {
         emit_v128_binop(VPSUBSW);
      }

      void emit_i16x8_sub_sat_u() {
         emit_v128_binop(VPSUBUSW);
      }

      void emit_i16x8_mul() {
         emit_v128_binop_r(VPMULLW);
      }

      void emit_i16x8_min_s() {
         emit_v128_binop_r(VPMINSW);
      }

      void emit_i16x8_min_u() {
         emit_v128_binop_r(VPMINUW);
      }

      void emit_i16x8_max_s() {
         emit_v128_binop_r(VPMAXSW);
      }

      void emit_i16x8_max_u() {
         emit_v128_binop_r(VPMAXUW);
      }

      void emit_i16x8_avgr_u() {
         emit_v128_binop_r(VPAVGW);
      }

      void emit_i16x8_extmul_low_i8x16_s() {
         emit(VPMOVSXBW, *rsp, xmm1);
         emit_add(16, rsp);
         emit(VPMOVSXBW, *rsp, xmm0);
         emit(VPMULLW, xmm0, xmm1, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_i16x8_extmul_high_i8x16_s() {
         emit_movups(*rsp, xmm1);
         emit(VPSRLDQ_c, imm8{8}, xmm1, xmm1);
         emit(VPMOVSXBW, xmm1, xmm1);
         emit_add(16, rsp);
         emit_vmovups(*rsp, xmm0);
         emit(VPSRLDQ_c, imm8{8}, xmm0, xmm0);
         emit(VPMOVSXBW, xmm0, xmm0);
         emit(VPMULLW, xmm0, xmm1, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_i16x8_extmul_low_i8x16_u() {
         emit(VPMOVZXBW, *rsp, xmm1);
         emit_add(16, rsp);
         emit(VPMOVZXBW, *rsp, xmm0);
         emit(VPMULLW, xmm0, xmm1, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_i16x8_extmul_high_i8x16_u() {
         emit_movups(*rsp, xmm1);
         emit(VPSRLDQ_c, imm8{8}, xmm1, xmm1);
         emit(VPMOVZXBW, xmm1, xmm1);
         emit_add(16, rsp);
         emit_vmovups(*rsp, xmm0);
         emit(VPSRLDQ_c, imm8{8}, xmm0, xmm0);
         emit(VPMOVZXBW, xmm0, xmm0);
         emit(VPMULLW, xmm0, xmm1, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      // i32x4 ops

      void emit_i32x4_extadd_pairwise_i16x8_s() {
         emit_vmovups(*rsp, xmm0);
         emit(VPSRLDQ_c, imm8{8}, xmm0, xmm1);
         emit(VPMOVSXWD, xmm0, xmm0);
         emit(VPMOVSXWD, xmm1, xmm1);
         emit(VPHADDD, xmm1, xmm0, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_i32x4_extadd_pairwise_i16x8_u() {
         emit_vmovups(*rsp, xmm0);
         emit(VPSRLDQ_c, imm8{8}, xmm0, xmm1);
         emit(VPMOVZXWD, xmm0, xmm0);
         emit(VPMOVZXWD, xmm1, xmm1);
         emit(VPHADDD, xmm1, xmm0, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_i32x4_abs() {
         emit_v128_unop(VPABSW);
      }

      void emit_i32x4_neg() {
         emit_const_zero(xmm0);
         emit(VPSUBW, *rsp, xmm0, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_i32x4_all_true() {
         emit_const_zero(xmm0);
         emit(VPCMPEQD, *rsp, xmm0, xmm0);
         emit_add(16, rsp);
         // xor %eax, %eax
         emit_bytes(0x31, 0xc0);
         emit(VPTEST, xmm0, xmm0);
         // setz %al
         emit_bytes(0x0f, 0x94, 0xc0);
         emit_push(rax);
      }

      void emit_i32x4_bitmask() {
         emit_vmovups(*rsp, xmm0);
         emit_add(16, rsp);
         emit(VMOVMSKPS, xmm0, rax);
         emit_push(rax);
      }

      void emit_i32x4_extend_low_i16x8_s() {
         emit_v128_unop(VPMOVSXWD);
      }

      void emit_i32x4_extend_high_i16x8_s() {
         emit_vmovups(*rsp, xmm0);
         emit(VPSRLDQ_c, imm8{8}, xmm0, xmm0);
         emit(VPMOVZXWD, xmm0, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_i32x4_extend_low_i16x8_u() {
         emit_v128_unop(VPMOVSXWD);
      }

      void emit_i32x4_extend_high_i16x8_u() {
         emit_vmovups(*rsp, xmm0);
         emit(VPSRLDQ_c, imm8{8}, xmm0, xmm0);
         emit(VPMOVZXWD, xmm0, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_i32x4_shl() {
         emit_v128_shiftop(VPSLLD, 0x1f);
      }

      void emit_i32x4_shr_s() {
         emit_v128_shiftop(VPSRAD, 0x1f);
      }

      void emit_i32x4_shr_u() {
         emit_v128_shiftop(VPSRLD, 0x1f);
      }

      void emit_i32x4_add() {
         emit_v128_binop_r(VPADDD);
      }

      void emit_i32x4_sub() {
         emit_v128_binop(VPSUBD);
      }

      void emit_i32x4_mul() {
         emit_v128_binop_r(VPMULLD);
      }

      void emit_i32x4_min_s() {
         emit_v128_binop_r(VPMINSD);
      }

      void emit_i32x4_min_u() {
         emit_v128_binop_r(VPMINUD);
      }

      void emit_i32x4_max_s() {
         emit_v128_binop_r(VPMAXSD);
      }

      void emit_i32x4_max_u() {
         emit_v128_binop_r(VPMAXUD);
      }

      void emit_i32x4_dot_i16x8_s() {
         emit_v128_binop_r(VPMADDWD);
      }

      void emit_i32x4_extmul_low_i16x8_s() {
         emit_vmovups(*rsp, xmm0);
         emit_add(16, rsp);
         emit_vmovups(*rsp, xmm0);
         emit(VPMOVSXWD, xmm0, xmm0);
         emit(VPMOVSXWD, xmm1, xmm1);
         emit(VPMULLD, xmm0, xmm1, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_i32x4_extmul_high_i16x8_s() {
         emit_vmovups(*rsp, xmm0);
         emit_add(16, rsp);
         emit_vmovups(*rsp, xmm1);
         emit(VPMULLW, xmm0, xmm1, xmm2);
         emit(VPMULHW, xmm0, xmm1, xmm0);
         emit(VPUNPCKHWD, xmm0, xmm2, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_i32x4_extmul_low_i16x8_u() {
         emit_vmovups(*rsp, xmm0);
         emit_add(16, rsp);
         emit_vmovups(*rsp, xmm0);
         emit(VPMOVZXWD, xmm0, xmm0);
         emit(VPMOVZXWD, xmm1, xmm1);
         emit(VPMULLD, xmm0, xmm1, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_i32x4_extmul_high_i16x8_u() {
         emit_vmovups(*rsp, xmm0);
         emit_add(16, rsp);
         emit_vmovups(*rsp, xmm1);
         emit(VPMULLW, xmm0, xmm1, xmm2);
         emit(VPMULHUW, xmm0, xmm1, xmm0);
         emit(VPUNPCKHWD, xmm0, xmm2, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      // i64x2 ops

      void emit_i64x2_abs() {
         emit_vmovups(*rsp, xmm0);
         emit_const_zero(xmm1);
         emit(VPCMPGTQ, xmm0, xmm1, xmm1);
         emit(VPXOR, xmm0, xmm1, xmm0);
         emit(VPSUBQ, xmm1, xmm0, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_i64x2_neg() {
         emit_const_zero(xmm0);
         emit(VPSUBQ, *rsp, xmm0, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_i64x2_all_true() {
         emit_const_zero(xmm0);
         emit(VPCMPEQQ, *rsp, xmm0, xmm0);
         emit_add(16, rsp);
         // xor %eax, %eax
         emit_bytes(0x31, 0xc0);
         emit(VPTEST, xmm0, xmm0);
         // setz %al
         emit_bytes(0x0f, 0x94, 0xc0);
         emit_push(rax);
      }

      void emit_i64x2_bitmask() {
         emit_vmovups(*rsp, xmm0);
         emit_add(16, rsp);
         emit(VMOVMSKPD, xmm0, rax);
         emit_push(rax);
      }

      void emit_i64x2_extend_low_i32x4_s() {
         emit_v128_unop(VPMOVSXDQ);
      }

      void emit_i64x2_extend_high_i32x4_s() {
         emit_vmovups(*rsp, xmm0);
         emit(VPSRLDQ_c, imm8{8}, xmm0, xmm0);
         emit(VPMOVSXDQ, xmm0, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_i64x2_extend_low_i32x4_u() {
         emit_v128_unop(VPMOVZXDQ);
      }

      void emit_i64x2_extend_high_i32x4_u() {
         emit_vmovups(*rsp, xmm0);
         emit(VPSRLDQ_c, imm8{8}, xmm0, xmm0);
         emit(VPMOVZXDQ, xmm0, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_i64x2_shl() {
         emit_v128_shiftop(VPSLLQ, 0x3f);
      }

      void emit_i64x2_shr_s() {
         // (x >> n) | ((0 > x) << (64 - n))
         emit_pop(rax);
         // and $mask, %eax
         emit_bytes(0x83, 0xe0, 0x3f);
         // mov $64, %ecx
         emit_bytes(0xb9);
         emit_operand32(64);
         // sub %eax, %ecx
         emit_bytes(0x2b, 0xc8);
         emit_movups(*rsp, xmm0);
         emit_vmovd(eax, xmm1);
         emit_vmovd(ecx, xmm3);
         emit_const_zero(xmm2);
         emit(VPCMPGTQ, xmm0, xmm2, xmm2);
         emit(VPSLLQ, xmm3, xmm2, xmm2);
         emit(VPSRLQ, xmm1, xmm0, xmm0);
         emit(VPOR, xmm0, xmm2, xmm0);
         emit_movups(xmm0, *rsp);
      }

      void emit_i64x2_shr_u() {
         emit_v128_shiftop(VPSRLQ, 0x3f);
      }

      void emit_i64x2_add() {
         emit_v128_binop_r(VPADDQ);
      }

      void emit_i64x2_sub() {
         emit_v128_binop(VPSUBQ);
      }

      void emit_i64x2_mul() {
         emit_vmovups(*rsp, xmm0);
         emit_add(16, rsp);
         emit_vmovups(*rsp, xmm0);
         emit(VPMULUDQ, xmm0, xmm1, xmm2);
         emit(VPSHUFD, imm8{0xb1}, xmm0, xmm0);
         emit(VPMULLD, xmm0, xmm1, xmm0);
         emit(VPHADDD, xmm0, xmm0, xmm0);
         emit_const_zero(xmm1);
         emit(VPUNPCKLWD, xmm0, xmm1, xmm0);
         emit(VPADDQ, xmm0, xmm2, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_i64x2_extmul_low_i32x4_s() {
         emit(VPSHUFD, imm8{0x10}, *rsp, xmm0);
         emit_add(16, rsp);
         emit(VPSHUFD, imm8{0x10}, *rsp, xmm1);
         emit(VPMULDQ, xmm0, xmm1, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_i64x2_extmul_high_i32x4_s() {
         emit(VPSHUFD, imm8{0x32}, *rsp, xmm0);
         emit_add(16, rsp);
         emit(VPSHUFD, imm8{0x32}, *rsp, xmm1);
         emit(VPMULDQ, xmm0, xmm1, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_i64x2_extmul_low_i32x4_u() {
         emit(VPSHUFD, imm8{0x10}, *rsp, xmm0);
         emit_add(16, rsp);
         emit(VPSHUFD, imm8{0x10}, *rsp, xmm1);
         emit(VPMULUDQ, xmm0, xmm1, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_i64x2_extmul_high_i32x4_u() {
         emit(VPSHUFD, imm8{0x32}, *rsp, xmm0);
         emit_add(16, rsp);
         emit(VPSHUFD, imm8{0x32}, *rsp, xmm1);
         emit(VPMULUDQ, xmm0, xmm1, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      void emit_error() { unimplemented(); }

      // --------------- random  ------------------------
      static void fix_branch(void* branch, void* target) {
         auto branch_ = static_cast<uint8_t*>(branch);
         auto target_ = static_cast<uint8_t*>(target);
         auto relative = static_cast<uint32_t>(target_ - (branch_ + 4));
         if((target_ - (branch_ + 4)) > 0x7FFFFFFFll ||
            (target_ - (branch_ + 4)) < -0x80000000ll) unimplemented();
         memcpy(branch, &relative, 4);
      }

      static void fix_branch8(void* branch, void* target) {
         auto branch_ = static_cast<uint8_t*>(branch);
         auto target_ = static_cast<uint8_t*>(target);
         auto relative = static_cast<uint8_t>(target_ - (branch_ + 1));
         if((target_ - (branch_ + 1)) > 0x7Fll ||
            (target_ - (branch_ + 1)) < -0x80ll) unimplemented();
         memcpy(branch, &relative, 1);
      }

      // A 64-bit absolute address is used for function calls whose
      // address is too far away for a 32-bit relative call.
      static void fix_branch64(void* branch, void* target) {
         memcpy(branch, &target, 8);
      }

      using fn_type = native_value(*)(void* context, void* memory);
      void finalize(function_body& body) {
         _mod.allocator.reclaim(code, _code_end - code);
         body.jit_code_offset = _code_start - (unsigned char*)_code_segment_base;
      }

      // returns the current write address
      const void* get_addr() const {
         return code;
      }

      const void* get_base_addr() const { return _code_segment_base; }

    private:

      enum class imm8 : uint8_t {};
      enum general_register {
          rax, rcx, rdx, rbx, rsp, rbp, rsi, rdi,
          r8, r9, r10, r11, r12, r13, r14, r15,
          eax = rax, ecx, edx, ebx, esp, ebp, esi, edi
      };
      struct simple_memory_ref { general_register reg; };
      inline friend simple_memory_ref operator*(general_register reg) { return { reg }; }
      struct register_add_expr { general_register reg; int32_t offset; };
      inline friend register_add_expr operator+(general_register reg, int32_t offset) { return { reg, offset }; }
      inline friend register_add_expr operator+(int32_t offset, general_register reg) { return { reg, offset }; }
      inline friend register_add_expr operator-(general_register reg, int32_t offset) { return { reg, -offset }; }
      struct disp_memory_ref {
         constexpr disp_memory_ref(general_register reg, int32_t offset) : reg(reg), offset(offset) {}
         constexpr disp_memory_ref(simple_memory_ref other) : reg(other.reg), offset(0) {}
         general_register reg;
         int32_t offset;
      };
      inline friend disp_memory_ref operator*(register_add_expr expr) { return { expr.reg, expr.offset }; }
      enum xmm_register {
          xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7,
          xmm8, xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15
      };

      void emit_add(uint32_t immediate, general_register dest) {
         if(immediate <= 0x7Fu || immediate >= 0xFFFFFF80u) {
            emit(IA32_REX_W(0x83)/0, static_cast<imm8>(immediate), dest);
         } else {
            unimplemented();
         }
      }
      
      void emit_sub(uint32_t immediate, general_register dest) {
         if(immediate <= 0x7Fu || immediate >= 0xFFFFFF80u) {
            emit(IA32_REX_W(0x83)/5, static_cast<imm8>(immediate), dest);
         } else {
            unimplemented();
         }
      }

      void emit_call(general_register reg) {
         emit(IA32(0xff)/2, reg);
      }

      void emit_ldmxcsr(disp_memory_ref mem) {
         emit_REX_prefix(false, mem, 0);
         emit_bytes(0x0f, 0xae);
         emit_modrm_sib_disp(mem, 2);
      }

      void emit_stmxcsr(disp_memory_ref mem) {
         emit_REX_prefix(false, mem, 0);
         emit_bytes(0x0f, 0xae);
         emit_modrm_sib_disp(mem, 3);
      }

      void emit_movd(uint32_t src, disp_memory_ref dest) {
         emit(IA32(0xc7)/0, dest);
         emit_operand32(src);
      }

      void emit_movd(general_register src, general_register dest) {
         emit(IA32(0x8b), src, dest);
      }

      void emit_movd(disp_memory_ref mem, general_register reg) {
         emit(IA32(0x8b), mem, reg);
      }

      void emit_movq(general_register src, general_register dest) {
         emit(IA32_REX_W(0x8b), src, dest);
      }

      void emit_movq(disp_memory_ref mem, general_register reg) {
         emit(IA32_REX_W(0x8b), mem, reg);
      }

      void emit_movq(general_register reg, disp_memory_ref mem) {
         emit(IA32_REX_W(0x89), mem, reg);
      }

      void emit_pop(general_register reg) {
         emit_REX_prefix(false, false, false, reg & 8);
         emit_bytes(0x58 | (reg & 7));
      }

      void emit_push(general_register reg) {
         emit_REX_prefix(false, false, false, reg & 8);
         emit_bytes(0x50 | (reg & 7));
      }

      void emit_xord(general_register src, general_register dest) {
         emit(IA32(0x33), src, dest);
      }

      void emit_movups(simple_memory_ref mem, xmm_register reg) {
         if(mem.reg == rsp) {
            if(reg >= 8) {
               emit_bytes(0x44);
            }
            emit_bytes(0x0f, 0x10, 0x04 | ((reg & 7) << 3), 0x24);
         } else {
            unimplemented();
         }
      }
      
      void emit_movups(xmm_register reg, simple_memory_ref mem) {
         if(reg == xmm0 && mem.reg == rsp) {
            emit_bytes(0x0f, 0x11, 0x04, 0x24);
         } else {
            unimplemented();
         }
      }
      
      void emit_vmovups(disp_memory_ref mem, xmm_register reg) {
         emit(VEX_128_0F_WIG{0x10}, mem, reg);
      }
      
      void emit_vmovups(simple_memory_ref mem, xmm_register reg) {
         emit(VEX_128_0F_WIG{0x10}, mem, reg);
      }

      void emit_vmovups(xmm_register reg, disp_memory_ref mem) {
         emit(VEX_128_0F_WIG{0x11}, mem, reg);
      }

      void emit_vmovups(xmm_register reg, simple_memory_ref mem) {
         emit(VEX_128_0F_WIG{0x11}, mem, reg);
      }

      void emit_vmovd(general_register src, xmm_register dest) {
         emit(VEX_128_66_0F_W0{0x6e}, src, dest);
      }

      template<typename T>
      void emit_vpextrb(uint8_t offset, xmm_register src, T dest) {
         emit(VEX_128_66_0F3A_W0{0x14}, imm8{offset}, dest, src);
      }

      template<typename T>
      void emit_vpextrw(uint8_t offset, xmm_register src, T dest) {
         emit(VEX_128_66_0F3A_W0{0x15}, imm8{offset}, dest, src);
      }

      template<typename T>
      void emit_vpextrd(uint8_t offset, xmm_register src, T dest) {
         emit(VEX_128_66_0F3A_W0{0x16}, imm8{offset}, dest, src);
      }

      template<typename T>
      void emit_vpextrq(uint8_t offset, xmm_register src, T dest) {
         emit(VEX_128_66_0F3A_W1{0x16}, imm8{offset}, dest, src);
      }

      void emit_vpxor(xmm_register src1, xmm_register src2, xmm_register dest) {
         emit_VEX_128_66_0F_WIG(0xef, src1, src2, dest);
      }

      void emit_const_zero(xmm_register reg) {
         emit_vpxor(reg, reg, reg);
      }

      void emit_const_ones(xmm_register reg) {
         // vpcmpeqb %reg, %reg, %reg
         emit_VEX_128_66_0F_WIG(0x74, reg, reg, reg);
      }

      template<bool W, int N>
      struct IA32_t { uint8_t opcode[N]; };
      template<typename Base>
      struct IA32_opext { Base base; int opext; };
      template<bool W, int N>
      inline constexpr friend IA32_opext<IA32_t<W, N>> operator/(IA32_t<W, N> base, int opext) {
         return {base, opext};
      }

      template<typename... Op>
      static constexpr auto IA32(Op... op) {
         return IA32_t<false, sizeof...(Op)>{{ static_cast<uint8_t>(op)... }};
      }
      template<typename... Op>
      static constexpr auto IA32_REX_W(Op... op) {
         return IA32_t<true, sizeof...(Op)>{{ static_cast<uint8_t>(op)... }};
      }

      struct Jcc { uint8_t opcode; };

      // When adding jcc codes, verify that the rel8/rel32 versions are 7x and 0F 8x
      static constexpr auto DECQ = IA32_REX_W(0xFF)/1;
      static constexpr auto JZ = Jcc{0x74};
      static constexpr auto JNZ = Jcc{0x75};
      static constexpr auto LDMXCSR = IA32(0x0f, 0xae)/2;
      static constexpr auto STMXCSR = IA32(0x0f, 0xae)/3;
      static constexpr auto RET = IA32(0xc3);
      static constexpr auto TESTD = IA32(0x85);
      static constexpr auto TESTQ = IA32_REX_W(0x85);

      enum VEX_mmmm { mmmm_none, mmmm_0F, mmmm_0F38, mmmm_0F3A };
      enum VEX_pp { pp_none, pp_66, pp_F3, pp_F2 };

      template<int SZ, VEX_pp PP, VEX_mmmm MMMM, int W, int OpExt = -1>
      struct VEX { uint8_t opcode; };
      using VEX_128_66_0F_W0 = VEX<128, pp_66, mmmm_0F, 0>;
      using VEX_128_66_0F = VEX_128_66_0F_W0;
      using VEX_128_66_0F_WIG = VEX_128_66_0F_W0;
      using VEX_128_66_0F38_W0 = VEX<128, pp_66, mmmm_0F38, 0>;
      using VEX_128_66_0F38_WIG = VEX_128_66_0F38_W0;
      using VEX_128_66_0F38 = VEX_128_66_0F38_W0;
      using VEX_128_0F_W0 = VEX<128, pp_none, mmmm_0F, 0>;
      using VEX_128_0F_WIG = VEX_128_0F_W0;
      using VEX_128_66_0F3A_W0 = VEX<128, pp_66, mmmm_0F3A, 0>;
      using VEX_128_66_0F3A_W1 = VEX<128, pp_66, mmmm_0F3A, 1>;

      static constexpr auto VMOVMSKPS = VEX_128_0F_WIG{0x50};
      static constexpr auto VMOVMSKPD = VEX_128_66_0F_WIG{0x50};
      static constexpr auto VPABSB = VEX_128_66_0F38_WIG{0x1c};
      static constexpr auto VPABSW = VEX_128_66_0F38_WIG{0x1d};
      static constexpr auto VPABSD = VEX_128_66_0F38_WIG{0x1e};
      static constexpr auto VPACKSSWB = VEX_128_66_0F_WIG{0x63};
      static constexpr auto VPACKSSDW = VEX_128_66_0F_WIG{0x6b};
      static constexpr auto VPACKUSWB = VEX_128_66_0F_WIG{0x67};
      static constexpr auto VPACKUSDW = VEX_128_66_0F38{0x2B};
      static constexpr auto VPADDB = VEX_128_66_0F_WIG{0xfc};
      static constexpr auto VPADDW = VEX_128_66_0F_WIG{0xfd};
      static constexpr auto VPADDD = VEX_128_66_0F_WIG{0xfe};
      static constexpr auto VPADDQ = VEX_128_66_0F_WIG{0xd4};
      static constexpr auto VPADDSB = VEX_128_66_0F_WIG{0xec};
      static constexpr auto VPADDSW = VEX_128_66_0F_WIG{0xed};
      static constexpr auto VPADDUSB = VEX_128_66_0F_WIG{0xdc};
      static constexpr auto VPADDUSW = VEX_128_66_0F_WIG{0xdd};
      static constexpr auto VPAND = VEX_128_66_0F_WIG{0xdb};
      static constexpr auto VPANDN = VEX_128_66_0F_WIG{0xdf};
      static constexpr auto VPAVGB = VEX_128_66_0F_WIG{0xe0};
      static constexpr auto VPAVGW = VEX_128_66_0F_WIG{0xe3};
      static constexpr auto VPBROADCASTB = VEX_128_66_0F38_W0{0x78};
      static constexpr auto VPCMPEQB = VEX_128_66_0F_WIG{0x74};
      static constexpr auto VPCMPEQW = VEX_128_66_0F_WIG{0x75};
      static constexpr auto VPCMPEQD = VEX_128_66_0F_WIG{0x76};
      static constexpr auto VPCMPEQQ = VEX_128_66_0F38_WIG{0x29};
      static constexpr auto VPCMPGTB = VEX_128_66_0F_WIG{0x64};
      static constexpr auto VPCMPGTW = VEX_128_66_0F_WIG{0x65};
      static constexpr auto VPCMPGTD = VEX_128_66_0F_WIG{0x66};
      static constexpr auto VPCMPGTQ = VEX_128_66_0F38_WIG{0x37};
      static constexpr auto VPHADDW = VEX_128_66_0F38_WIG{0x01};
      static constexpr auto VPHADDD = VEX_128_66_0F38_WIG{0x02};
      static constexpr auto VPMADDWD = VEX_128_66_0F_WIG{0xf5};
      static constexpr auto VPMAXSB = VEX_128_66_0F38_WIG{0x3c};
      static constexpr auto VPMAXSW = VEX_128_66_0F_WIG{0xee};
      static constexpr auto VPMAXSD = VEX_128_66_0F38_WIG{0x3d};
      static constexpr auto VPMAXUB = VEX_128_66_0F{0xde};
      static constexpr auto VPMAXUW = VEX_128_66_0F38{0x3e};
      static constexpr auto VPMAXUD = VEX_128_66_0F38_WIG{0x3f};
      static constexpr auto VPMINSB = VEX_128_66_0F38{0x38};
      static constexpr auto VPMINSW = VEX_128_66_0F{0xea};
      static constexpr auto VPMINSD = VEX_128_66_0F38_WIG{0x39};
      static constexpr auto VPMINUB = VEX_128_66_0F{0xda};
      static constexpr auto VPMINUW = VEX_128_66_0F38{0x3a};
      static constexpr auto VPMINUD = VEX_128_66_0F38_WIG{0x3b};
      static constexpr auto VPMOVMSKB = VEX_128_66_0F_WIG{0xD7};
      static constexpr auto VPMOVSXBW = VEX_128_66_0F38_WIG{0x20};
      static constexpr auto VPMOVSXWD = VEX_128_66_0F38_WIG{0x23};
      static constexpr auto VPMOVSXDQ = VEX_128_66_0F38_WIG{0x25};
      static constexpr auto VPMOVZXBW = VEX_128_66_0F38_WIG{0x30};
      static constexpr auto VPMOVZXWD = VEX_128_66_0F38_WIG{0x33};
      static constexpr auto VPMOVZXDQ = VEX_128_66_0F38_WIG{0x35};
      static constexpr auto VPMULHW = VEX_128_66_0F_WIG{0xe5};
      static constexpr auto VPMULHUW = VEX_128_66_0F_WIG{0xe4};
      static constexpr auto VPMULHRSW = VEX_128_66_0F38_WIG{0x0b};
      static constexpr auto VPMULLW = VEX_128_66_0F_WIG{0xd5};
      static constexpr auto VPMULLD = VEX_128_66_0F38_WIG{0x40};
      static constexpr auto VPMULDQ = VEX_128_66_0F38_WIG{0x28};
      static constexpr auto VPMULUDQ = VEX_128_66_0F_WIG{0xf4};
      static constexpr auto VPOR = VEX_128_66_0F_WIG{0xeb};
      static constexpr auto VPSHUFB = VEX_128_66_0F38_WIG{0x00};
      static constexpr auto VPSHUFD = VEX_128_66_0F_WIG{0x70};
      static constexpr auto VPSLLW = VEX_128_66_0F_WIG{0xf1};
      static constexpr auto VPSLLW_c = VEX<128, pp_66, mmmm_0F, 0, 6>{0x71};
      static constexpr auto VPSLLD = VEX_128_66_0F_WIG{0xf2};
      static constexpr auto VPSLLQ = VEX_128_66_0F_WIG{0xf3};
      static constexpr auto VPSRAW = VEX_128_66_0F_WIG{0xe1};
      static constexpr auto VPSRAW_c = VEX<128, pp_66, mmmm_0F, 0, 4>{0x71};
      static constexpr auto VPSRAD = VEX_128_66_0F_WIG{0xe2};
      static constexpr auto VPSRLDQ_c = VEX<128, pp_66, mmmm_0F, 0, 3>{0x73};
      static constexpr auto VPSRLW = VEX_128_66_0F_WIG{0xd1};
      static constexpr auto VPSRLD = VEX_128_66_0F_WIG{0xd2};
      static constexpr auto VPSRLQ = VEX_128_66_0F_WIG{0xd3};
      static constexpr auto VPSRLQ_c = VEX<128, pp_66, mmmm_0F, 0, 2>{0x73};
      static constexpr auto VPSUBB = VEX_128_66_0F_WIG{0xf8};
      static constexpr auto VPSUBW = VEX_128_66_0F_WIG{0xf9};
      static constexpr auto VPSUBD = VEX_128_66_0F_WIG{0xfa};
      static constexpr auto VPSUBQ = VEX_128_66_0F_WIG{0xfb};
      static constexpr auto VPSUBSB = VEX_128_66_0F_WIG{0xe8};
      static constexpr auto VPSUBSW = VEX_128_66_0F_WIG{0xe9};
      static constexpr auto VPSUBUSB = VEX_128_66_0F_WIG{0xd8};
      static constexpr auto VPSUBUSW = VEX_128_66_0F_WIG{0xd9};
      static constexpr auto VPTEST = VEX_128_66_0F38_WIG{0x17};
      static constexpr auto VPUNPCKHBW = VEX_128_66_0F_WIG{0x68};
      static constexpr auto VPUNPCKHWD = VEX_128_66_0F_WIG{0x69};
      static constexpr auto VPUNPCKHDQ = VEX_128_66_0F_WIG{0x6a};
      static constexpr auto VPUNPCKLBW = VEX_128_66_0F_WIG{0x60};
      static constexpr auto VPUNPCKLWD = VEX_128_66_0F_WIG{0x61};
      static constexpr auto VPUNPCKLDQ = VEX_128_66_0F_WIG{0x62};
      static constexpr auto VPXOR = VEX_128_66_0F_WIG{0xef};

      void emit_REX_prefix(bool W, bool R, bool X, bool B) {
         if(W || R || X || B) {
            emit_bytes(0x40 | (W << 3) | (R << 2) | (X << 1) | (B << 0));
         }
      }

      void emit_REX_prefix(bool W, general_register r_m, int reg) {
         emit_REX_prefix(W, reg & 8, false, r_m & 8);
      }

      void emit_REX_prefix(bool W, disp_memory_ref mem, int reg) {
         emit_REX_prefix(W, reg & 8, false, mem.reg & 8);
      }

      void emit_VEX_prefix(bool R, bool X, bool B, VEX_mmmm mmmm, bool W, int vvvv, bool L, VEX_pp pp) {
         if(X || B || mmmm || W) {
            emit_bytes(0xc4, (!R << 7) | (!X << 6) | (!B << 5) | mmmm, (W << 7) | ((vvvv ^ 0xF) << 3) | (L << 2) | pp);
         } else {
            emit_bytes(0xc5, (!R << 7) | (vvvv << 3) | (L << 2) | pp);
         }
      }

      void emit_modrm_sib_disp(disp_memory_ref mem, int reg) {
         if(mem.offset == 0) {
            return emit_modrm_sib_disp(simple_memory_ref{mem.reg}, reg);
         } else if(mem.offset >= -0x80 && mem.offset <= 0x7f) {
            if(mem.reg == rsp) {
               emit_bytes(0x40 | ((reg & 7) << 3) | 0x04, 0x24, mem.offset);
            } else {
               emit_bytes(0x40 | ((reg & 7) << 3) | (mem.reg & 7), mem.offset);
            }
         } else {
            if(mem.reg == rsp) {
               emit_bytes(0x80 | ((reg & 7) << 3) | 0x04, 0x24);
            } else {
               emit_bytes(0x80 | ((reg & 7) << 3) | (mem.reg & 7));
            }
            emit_operand32(mem.offset);
         }
      }

      void emit_modrm_sib_disp(simple_memory_ref mem, int reg) {
         if(mem.reg == rsp) {
            emit_bytes(((reg & 7) << 3) | 0x04, 0x24);
         } else if(mem.reg == rbp) {
            emit_bytes(0x45 | ((reg & 7) << 3), 0x00);
         } else {
            emit_bytes(((reg & 7) << 3) | (mem.reg & 7));
         }
      }

      void emit_modrm_sib_disp(int r_m, int reg) {
         emit_bytes(0xc0 | ((reg & 7) << 3) | (r_m & 7));
      }

      template<bool W, int N>
      void emit(IA32_t<W, N> opcode) {
         emit_REX_prefix(W, false, false, false);
         for(int i = 0; i < N; ++i) {
            emit_bytes(opcode.opcode[i]);
         }
      }
      template<bool W, int N, typename RM>
      void emit(IA32_t<W, N> opcode, RM r_m, int reg) {
         emit_REX_prefix(W, r_m, reg);
         for(int i = 0; i < N; ++i) {
            emit_bytes(opcode.opcode[i]);
         }
         emit_modrm_sib_disp(r_m, reg);
      }
      template<bool W, int N, typename RM>
      void emit(IA32_t<W, N> opcode, imm8 imm, RM r_m, int reg) {
         emit(opcode, r_m, reg);
         emit_bytes(static_cast<uint8_t>(imm));
      }
      template<typename T, typename RM>
      void emit(IA32_opext<T> opcode, RM r_m) {
         emit(opcode.base, r_m, opcode.opext);
      }
      template<typename T, typename RM>
      void emit(IA32_opext<T> opcode, imm8 imm, RM r_m) {
         emit(opcode.base, imm, r_m, opcode.opext);
      }

      void* emit_branch8(Jcc opcode) {
         emit_bytes(opcode.opcode);
         return emit_branch_target8();
      }

      void* emit_branchcc32(Jcc opcode) {
         emit_bytes(0x0f, 0x10 + opcode.opcode);
         return emit_branch_target32();
      }

      template<int Sz, VEX_pp pp, VEX_mmmm mmmm, int W>
      void emit(VEX<Sz, pp, mmmm, W> opcode, disp_memory_ref src1, xmm_register src2) {
         emit_VEX_prefix(src2 & 8, false, src1.reg & 8, mmmm, W, 0, Sz == 256, pp);
         emit_bytes(opcode.opcode);
         emit_modrm_sib_disp(src1, src2);
      }

      template<int Sz, VEX_pp pp, VEX_mmmm mmmm, int W>
      void emit(VEX<Sz, pp, mmmm, W> opcode, simple_memory_ref src1, xmm_register src2) {
         emit_VEX_prefix(src2 & 8, false, src1.reg & 8, mmmm, W, 0, Sz == 256, pp);
         emit_bytes(opcode.opcode);
         emit_modrm_sib_disp(src1, src2);
      }

      template<int Sz, VEX_pp pp, VEX_mmmm mmmm, int W>
      void emit(VEX<Sz, pp, mmmm, W> opcode, general_register src1, xmm_register src2) {
         emit_VEX_prefix(src2 & 8, false, src1 & 8, mmmm, W, 0, Sz == 256, pp);
         emit_bytes(opcode.opcode);
         emit_modrm_sib_disp(src1, src2);
      }

      template<int Sz, VEX_pp pp, VEX_mmmm mmmm, int W>
      void emit(VEX<Sz, pp, mmmm, W> opcode, xmm_register src1, xmm_register src2) {
         emit_VEX_prefix(src2 & 8, false, src1 & 8, mmmm, W, 0, Sz == 256, pp);
         emit_bytes(opcode.opcode);
         emit_modrm_sib_disp(src1, src2);
      }

      template<int Sz, VEX_pp pp, VEX_mmmm mmmm, int W>
      void emit(VEX<Sz, pp, mmmm, W> opcode, xmm_register src1, general_register src2) {
         emit_VEX_prefix(src2 & 8, false, src1 & 8, mmmm, W, 0, Sz == 256, pp);
         emit_bytes(opcode.opcode);
         emit_modrm_sib_disp(src1, src2);
      }

      template<int Sz, VEX_pp pp, VEX_mmmm mmmm, int W>
      void emit(VEX<Sz, pp, mmmm, W> opcode, imm8 src1, simple_memory_ref src2, xmm_register dest) {
         emit_VEX_prefix(dest & 8, false, src2.reg & 8, mmmm, W, 0, Sz == 256, pp);
         emit_bytes(opcode.opcode);
         emit_modrm_sib_disp(src2, dest);
         emit_byte(static_cast<uint8_t>(src1));
      }

      template<int Sz, VEX_pp pp, VEX_mmmm mmmm, int W>
      void emit(VEX<Sz, pp, mmmm, W> opcode, imm8 src1, xmm_register src2, xmm_register dest) {
         emit_VEX_prefix(dest & 8, false, src2 & 8, mmmm, W, 0, Sz == 256, pp);
         emit_bytes(opcode.opcode);
         emit_modrm_sib_disp(src2, dest);
         emit_byte(static_cast<uint8_t>(src1));
      }

      template<int Sz, VEX_pp pp, VEX_mmmm mmmm, int W>
      void emit(VEX<Sz, pp, mmmm, W> opcode, imm8 src1, general_register src2, xmm_register dest) {
         emit_VEX_prefix(dest & 8, false, src2 & 8, mmmm, W, 0, Sz == 256, pp);
         emit_bytes(opcode.opcode);
         emit_modrm_sib_disp(src2, dest);
         emit_byte(static_cast<uint8_t>(src1));
      }

      template<int Sz, VEX_pp pp, VEX_mmmm mmmm, int W>
      void emit(VEX<Sz, pp, mmmm, W> opcode, imm8 src1, xmm_register src2, general_register dest) {
         emit_VEX_prefix(dest & 8, false, src2 & 8, mmmm, W, 0, Sz == 256, pp);
         emit_bytes(opcode.opcode);
         emit_modrm_sib_disp(src2, dest);
         emit_byte(static_cast<uint8_t>(src1));
      }

      template<int Sz, VEX_pp pp, VEX_mmmm mmmm, int W, int OpExt>
      void emit(VEX<Sz, pp, mmmm, W, OpExt> opcode, imm8 src1, xmm_register src2, xmm_register dest) {
         emit_VEX_prefix(false, false, src2 & 8, mmmm, W, dest, Sz == 256, pp);
         emit_bytes(opcode.opcode);
         emit_modrm_sib_disp(src2, OpExt);
         emit_byte(static_cast<uint8_t>(src1));
      }

      template<int Sz, VEX_pp pp, VEX_mmmm mmmm, int W>
      void emit(VEX<Sz, pp, mmmm, W> opcode, simple_memory_ref src1, xmm_register src2, xmm_register dest) {
         emit_VEX_prefix(dest & 8, false, src1.reg & 8, mmmm, W, src2, Sz == 256, pp);
         emit_bytes(opcode.opcode);
         emit_modrm_sib_disp(src1, dest);
      }

      template<int Sz, VEX_pp pp, VEX_mmmm mmmm, int W>
      void emit(VEX<Sz, pp, mmmm, W> opcode, xmm_register src1, xmm_register src2, xmm_register dest) {
         emit_VEX_prefix(dest & 8, false, src1 & 8, mmmm, W, src2, Sz == 256, pp);
         emit_bytes(opcode.opcode);
         emit_modrm_sib_disp(src1, dest);
      }

      void emit_VEX_128_66_0F_WIG(uint8_t opcode, simple_memory_ref src1, xmm_register src2, xmm_register dest) {
         emit(VEX_128_66_0F_WIG{opcode}, src1, src2, dest);
      }
      
      void emit_VEX_128_66_0F_WIG(uint8_t opcode, xmm_register src1, xmm_register src2, xmm_register dest) {
         emit(VEX_128_66_0F_WIG{opcode}, src1, src2, dest);
      }

      auto fixed_size_instr(std::size_t expected_bytes) {
         return scope_guard{[this, expected_code=code+expected_bytes](){
#ifdef EOS_VM_VALIDATE_JIT_SIZE
            assert(code == expected_code);
#endif
            ignore_unused_variable_warning(code, expected_code);
         }};
      }
      auto variable_size_instr(std::size_t min, std::size_t max) {
         return scope_guard{[this, min_code=code+min,max_code=code+max](){
#ifdef EOS_VM_VALIDATE_JIT_SIZE
            assert(min_code <= code && code <= max_code);
#endif
            ignore_unused_variable_warning(code, min_code, max_code);
         }};
      }
      auto softfloat_instr(std::size_t hard_expected, std::size_t soft_expected, std::size_t softbt_expected) {
         return fixed_size_instr(use_softfloat?(Context::async_backtrace()?softbt_expected:soft_expected):hard_expected);
      }

      struct function_parameters {
         function_parameters() = default;
         function_parameters(const func_type* ft) {
            uint32_t current_offset = 16;
            offsets.resize(ft->param_types.size());
            for(uint32_t i = 0; i < ft->param_types.size(); ++i) {
               if(current_offset > 0x7fffffffu) {
                  unimplemented(); // cannot represent the offset as a 32-bit immediate
               }
               offsets[offsets.size() - i - 1] = current_offset;
               if(ft->param_types[ft->param_types.size() - i - 1] == types::v128) {
                  current_offset += 16;
               } else {
                  current_offset += 8;
               }
            }
         }
         int32_t get_frame_offset(uint32_t localidx) {
            return offsets[localidx];
         }
         std::vector<uint32_t> offsets;
      };

      module& _mod;
      void * _code_segment_base;
      const func_type* _ft;
      function_parameters _params;
      unsigned char * _code_start;
      unsigned char * _code_end;
      unsigned char * code;
      std::vector<std::variant<std::vector<void*>, void*>> _function_relocations;
      void* fpe_handler;
      void* call_indirect_handler;
      void* type_error_handler;
      void* stack_overflow_handler;
      void* jmp_table;
      uint32_t _local_count;
      uint32_t _table_element_size;

      void emit_byte(uint8_t val) { *code++ = val; }
      void emit_bytes() {}
      template<class... T>
      void emit_bytes(uint8_t val0, T... vals) {
         emit_byte(val0);
         emit_bytes(vals...);
      }
      void emit_operand32(uint32_t val) { memcpy(code, &val, sizeof(val)); code += sizeof(val); }
      void emit_operand64(uint64_t val) { memcpy(code, &val, sizeof(val)); code += sizeof(val); }
      void emit_operandf32(float val) { memcpy(code, &val, sizeof(val)); code += sizeof(val); }
      void emit_operandf64(double val) { memcpy(code, &val, sizeof(val)); code += sizeof(val); }
      template<class T>
      void emit_operand_ptr(T* val) { memcpy(code, &val, sizeof(val)); code += sizeof(val); }

     void* emit_branch_target32() {
        void * result = code;
        emit_operand32(3735928555u - static_cast<uint32_t>(reinterpret_cast<uintptr_t>(code)));
        return result;
     }

      void* emit_branch_target8() {
         void* result = code;
         emit_bytes(0xcc);
         return result;
      }

      void emit_check_call_depth() {
         // decl %ebx
         emit_bytes(0xff, 0xcb);
         // jz stack_overflow
         emit_bytes(0x0f, 0x84);
         fix_branch(emit_branch_target32(), stack_overflow_handler);
      }
      void emit_check_call_depth_end() {
         // incl %ebx
         emit_bytes(0xff, 0xc3);
      }

      static void unimplemented() { EOS_VM_ASSERT(false, wasm_parse_exception, "Sorry, not implemented."); }

      // clobbers %rax if the high bit of count is set.
      void emit_multipop(uint32_t count) {
         if(count > 0 && count != 0x80000001) {
            if (count & 0x80000000) {
               // mov (%rsp), %rax
               emit_bytes(0x48, 0x8b, 0x04, 0x24);
            }
            if(count & 0x70000000) {
               // This code is probably unreachable.
               // int3
               emit_bytes(0xCC);
            }
            // add depth_change*8, %rsp
            emit_bytes(0x48, 0x81, 0xc4); // TODO: Prefer imm8 where appropriate
            emit_operand32(count * 8); // FIXME: handle overflow
            if (count & 0x80000000) {
               // push %rax
               emit_bytes(0x50);
            }
         }
      }

      void emit_multipop(const func_type& ft) {
         uint32_t total_size = 0;
         for(uint32_t i = 0; i < ft.param_types.size(); ++i) {
            if(ft.param_types[i] == v128) {
               total_size += 16;
            } else {
               total_size += 8;
            }
            if(total_size > 0x7fffffffu) {
               unimplemented();
            }
         }
         emit_add(total_size, rsp);
      }

      // pops an i32 wasm address off the stack
      // adds offset and converts the result to
      // a native address.  The result is in %rax.
      void emit_pop_address(uint32_t offset) {
         // pop %rax
         emit_bytes(0x58);
         if (offset & 0x80000000) {
            // mov $offset, %ecx
            emit_bytes(0xb9);
            emit_operand32(offset);
            // add %rcx, %rax
            emit_bytes(0x48, 0x01, 0xc8);
         } else if (offset != 0) {
            // add offset, %rax
            emit_bytes(0x48, 0x05);
            emit_operand32(offset);
         }
         // add %rsi, %rax
         emit_bytes(0x48, 0x01, 0xf0);
      }

      template<class... T>
      void emit_load_impl(uint32_t offset, T... loadop) {
         emit_pop_address(offset);
         // from the caller
         emit_bytes(static_cast<uint8_t>(loadop)...);
         // push RAX
         emit_bytes(0x50);
      }

      template<class... T>
      void emit_store_impl(uint32_t offset, T... storeop) {
         // pop RCX
         emit_bytes(0x59);
         emit_pop_address(offset);
         // from the caller
         emit_bytes(static_cast<uint8_t>(storeop)...);;
      }

      void emit_i32_relop(uint8_t opcode) {
         // popq %rax
         emit_bytes(0x58);
         // popq %rcx
         emit_bytes(0x59);
         // xorq %rdx, %rdx
         emit_bytes(0x48, 0x31, 0xd2);
         // cmpl %eax, %ecx
         emit_bytes(0x39, 0xc1);
         // SETcc %dl
         emit_bytes(0x0f, opcode, 0xc2);
         // pushq %rdx
         emit_bytes(0x52);
      }

      template<class... T>
      void emit_i64_relop(uint8_t opcode) {
         // popq %rax
         emit_bytes(0x58);
         // popq %rcx
         emit_bytes(0x59);
         // xorq %rdx, %rdx
         emit_bytes(0x48, 0x31, 0xd2);
         // cmpq %rax, %rcx
         emit_bytes(0x48, 0x39, 0xc1);
         // SETcc %dl
         emit_bytes(0x0f, opcode, 0xc2);
         // pushq %rdx
         emit_bytes(0x52);
      }

      template<typename Op>
      void emit_v128_irelop_subsat(Op opcode, bool switch_params, bool flip_result) {
         emit_vmovups(*rsp, xmm0);
         emit_add(16, rsp);
         if(switch_params) {
            emit_vmovups(*rsp, xmm1);
            // vpsubusb %xmm0, %xmm1, %xmm0
            emit(opcode, xmm0, xmm1, xmm0);
         } else {
            // vpsubusb (%rsp), %xmm0, %xmm0
            emit(opcode, *rsp, xmm0, xmm0);
         }
         emit_const_zero(xmm1);
         // vpcmpeqb %xmm1, %xmm0, %xmm0
         emit(VPCMPEQB, xmm1, xmm0, xmm0);
         if(!flip_result) {
            // vpcmpeqb %xmm1, %xmm0, %xmm0
            emit(VPCMPEQB, xmm1, xmm0, xmm0);
         }
         emit_vmovups(xmm0, *rsp);
      }

      // !(op(a,b) == b)
      // max -> gt
      // min -> lt
      template<typename Op>
      void emit_v128_irelop_minmax(Op opcode, bool flip_result) {
         emit_vmovups(*rsp, xmm0);
         emit_add(16, rsp);
         // vp[min/max]us[b/w/d/q] (%rsp), xmm0, xmm1
         emit(opcode, *rsp, xmm0, xmm1);
         emit(VPCMPEQB, xmm0, xmm1, xmm0);
         if(!flip_result) {
            emit_const_zero(xmm1);
            emit(VPCMPEQB, xmm1, xmm0, xmm0);
         }
         emit_vmovups(xmm0, *rsp);
      }

      // Note: for commutative functions switch_params=true is more efficient
      template<typename Op>
      void emit_v128_irelop_cmp(Op opcode, bool switch_params, bool flip_result) {
         emit_vmovups(*rsp, xmm0);
         emit_add(16, rsp);
         if(!switch_params) {
            emit_vmovups(*rsp, xmm1);
            // OP %xmm0, %xmm1, %xmm0
            emit(opcode, xmm0, xmm1, xmm0);
         } else {
            // OP *rsp, %xmm0, %xmm0
            emit(opcode, *rsp, xmm0, xmm0);
         }
         if(flip_result) {
            emit_const_ones(xmm1);
            emit_vpxor(xmm1, xmm0, xmm0);
         }
         emit_vmovups(xmm0, *rsp);
      }

      template<typename Op>
      void emit_v128_shiftop(Op opcode, uint8_t mask) {
         emit_pop(rax);
         // and $mask, %eax
         emit_bytes(0x83, 0xe0, mask);
         emit_movups(*rsp, xmm0);
         emit_vmovd(eax, xmm1);
         emit(opcode, xmm1, xmm0, xmm0);
         emit_movups(xmm0, *rsp);
      }

      template<typename Op>
      void emit_v128_unop(Op opcode) {
         emit(opcode, *rsp, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      template<typename Op>
      void emit_v128_binop(Op opcode) {
         emit_vmovups(*rsp, xmm0);
         emit_add(16, rsp);
         emit_vmovups(*rsp, xmm1);
         emit(opcode, xmm0, xmm1, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      template<typename Op>
      void emit_v128_binop_r(Op opcode) {
         emit_vmovups(*rsp, xmm0);
         emit_add(16, rsp);
         emit(opcode, *rsp, xmm0, xmm0);
         emit_vmovups(xmm0, *rsp);
      }

      template<typename T, typename U>
      void emit_softfloat_unop(T(*softfloatfun)(U)) {
         auto extra = emit_setup_backtrace();
         // pushq %rdi
         emit_bytes(0x57);
         // pushq %rsi
         emit_bytes(0x56);
         if constexpr(sizeof(U) == 4) {
            // movq 16(%rsp), %edi
            emit_bytes(0x8b, 0x7c, 0x24, 0x10 + extra);
         } else {
            // movq 16(%rsp), %rdi
            emit_bytes(0x48, 0x8b, 0x7c, 0x24, 0x10 + extra);
         }
         emit_align_stack();
         // movabsq $softfloatfun, %rax
         emit_bytes(0x48, 0xb8);
         emit_operand_ptr(softfloatfun);
         // callq *%rax
         emit_bytes(0xff, 0xd0);
         emit_restore_stack();
         // popq %rsi
         emit_bytes(0x5e);
         // popq %rdi
         emit_bytes(0x5f);
         emit_restore_backtrace();
         if constexpr(sizeof(T) == 4) {
            static_assert(sizeof(U) == 4, "Can only push 4-byte item if the upper 4 bytes are already 0");
            // movq %eax, (%rsp)
            emit_bytes(0x89, 0x04, 0x24);
         } else {
            // movq %rax, (%rsp)
            emit_bytes(0x48, 0x89, 0x04, 0x24);
         }
      }

      void emit_f32_binop_softfloat(float32_t (*softfloatfun)(float32_t, float32_t)) {
         auto extra = emit_setup_backtrace();
         // pushq %rdi
         emit_bytes(0x57);
         // pushq %rsi
         emit_bytes(0x56);
         // movq 16(%rsp), %esi
         emit_bytes(0x8b, 0x74, 0x24, 0x10 + extra);
         // movq 24(%rsp), %edi
         emit_bytes(0x8b, 0x7c, 0x24, 0x18 + extra);
         emit_align_stack();
         // movabsq $softfloatfun, %rax
         emit_bytes(0x48, 0xb8);
         emit_operand_ptr(softfloatfun);
         // callq *%rax
         emit_bytes(0xff, 0xd0);
         emit_restore_stack();
         // popq %rsi
         emit_bytes(0x5e);
         // popq %rdi
         emit_bytes(0x5f);
         emit_restore_backtrace_basic();
         // addq $8, %rsp
         emit_bytes(0x48, 0x83, 0xc4, 0x08 + extra);
         // movq %eax, (%rsp)
         emit_bytes(0x89, 0x04, 0x24);
      }

      void emit_f64_binop_softfloat(float64_t (*softfloatfun)(float64_t, float64_t)) {
         auto extra = emit_setup_backtrace();
         // pushq %rdi
         emit_bytes(0x57);
         // pushq %rsi
         emit_bytes(0x56);
         // movq 16(%rsp), %rsi
         emit_bytes(0x48, 0x8b, 0x74, 0x24, 0x10 + extra);
         // movq 24(%rsp), %rdi
         emit_bytes(0x48, 0x8b, 0x7c, 0x24, 0x18 + extra);
         emit_align_stack();
         // movabsq $softfloatfun, %rax
         emit_bytes(0x48, 0xb8);
         emit_operand_ptr(softfloatfun);
         // callq *%rax
         emit_bytes(0xff, 0xd0);
         emit_restore_stack();
         // popq %rsi
         emit_bytes(0x5e);
         // popq %rdi
         emit_bytes(0x5f);
         emit_restore_backtrace_basic();
         // addq $8, %rsp
         emit_bytes(0x48, 0x83, 0xc4, 0x08 + extra);
         // movq %rax, (%rsp)
         emit_bytes(0x48, 0x89, 0x04, 0x24);
      }

      void emit_f32_relop(uint8_t opcode, uint64_t (*softfloatfun)(float32_t, float32_t), bool switch_params, bool flip_result) {
         if constexpr (use_softfloat) {
            auto extra = emit_setup_backtrace();
            // pushq %rdi
            emit_bytes(0x57);
            // pushq %rsi
            emit_bytes(0x56);
            if(switch_params) {
               // movq 24(%rsp), %esi
               emit_bytes(0x8b, 0x74, 0x24, 0x18 + extra);
               // movq 16(%rsp), %edi
               emit_bytes(0x8b, 0x7c, 0x24, 0x10 + extra);
            } else {
               // movq 16(%rsp), %esi
               emit_bytes(0x8b, 0x74, 0x24, 0x10 + extra);
               // movq 24(%rsp), %edi
               emit_bytes(0x8b, 0x7c, 0x24, 0x18 + extra);
            }
            emit_align_stack();
            // movabsq $softfloatfun, %rax
            emit_bytes(0x48, 0xb8);
            emit_operand_ptr(softfloatfun);
            // callq *%rax
            emit_bytes(0xff, 0xd0);
            emit_restore_stack();
            // popq %rsi
            emit_bytes(0x5e);
            // popq %rdi
            emit_bytes(0x5f);
            emit_restore_backtrace_basic();
            if (flip_result) {
               // xor $0x1, %al
               emit_bytes(0x34, 0x01);
            }
            // addq $8, %rsp
            emit_bytes(0x48, 0x83, 0xc4, 0x08 + extra);
            // movq %rax, (%rsp)
            emit_bytes(0x48, 0x89, 0x04, 0x24);
         } else {
            // ucomiss+seta/setae is shorter but can't handle eq/ne
            if(switch_params) {
               // movss (%rsp), %xmm0
               emit_bytes(0xf3, 0x0f, 0x10, 0x04, 0x24);
               // cmpCCss 8(%rsp), %xmm0
               emit_bytes(0xf3, 0x0f, 0xc2, 0x44, 0x24, 0x08, opcode);
            } else {
               // movss 8(%rsp), %xmm0
               emit_bytes(0xf3, 0x0f, 0x10, 0x44, 0x24, 0x08);
               // cmpCCss (%rsp), %xmm0
               emit_bytes(0xf3, 0x0f, 0xc2, 0x04, 0x24, opcode);
            }               
            // movd %xmm0, %eax
            emit_bytes(0x66, 0x0f, 0x7e, 0xc0);
            if (!flip_result) {
               // andl $1, %eax
               emit_bytes(0x83, 0xe0, 0x01);
            } else {
               // incl %eax {0xffffffff, 0} -> {0, 1}
               emit_bytes(0xff, 0xc0);
            }
            // leaq 16(%rsp), %rsp
            emit_bytes(0x48, 0x8d, 0x64, 0x24, 0x10);
            // pushq %rax
            emit_bytes(0x50);
         }
      }

      void emit_f64_relop(uint8_t opcode, uint64_t (*softfloatfun)(float64_t, float64_t), bool switch_params, bool flip_result) {
         if constexpr (use_softfloat) {
            auto extra = emit_setup_backtrace();
            // pushq %rdi
            emit_bytes(0x57);
            // pushq %rsi
            emit_bytes(0x56);
            if(switch_params) {
               // movq 24(%rsp), %rsi
               emit_bytes(0x48, 0x8b, 0x74, 0x24, 0x18 + extra);
               // movq 16(%rsp), %rdi
               emit_bytes(0x48, 0x8b, 0x7c, 0x24, 0x10 + extra);
            } else {
               // movq 16(%rsp), %rsi
               emit_bytes(0x48, 0x8b, 0x74, 0x24, 0x10 + extra);
               // movq 24(%rsp), %rdi
               emit_bytes(0x48, 0x8b, 0x7c, 0x24, 0x18 + extra);
            }
            emit_align_stack();
            // movabsq $softfloatfun, %rax
            emit_bytes(0x48, 0xb8);
            emit_operand_ptr(softfloatfun);
            // callq *%rax
            emit_bytes(0xff, 0xd0);
            emit_restore_stack();
            // popq %rsi
            emit_bytes(0x5e);
            // popq %rdi
            emit_bytes(0x5f);
            emit_restore_backtrace_basic();
            if (flip_result) {
               // xor $0x1, %al
               emit_bytes(0x34, 0x01);
            }
            // addq $8, %rsp
            emit_bytes(0x48, 0x83, 0xc4, 0x08 + extra);
            // movq %rax, (%rsp)
            emit_bytes(0x48, 0x89, 0x04, 0x24);
         } else {
            // ucomisd+seta/setae is shorter but can't handle eq/ne
            if(switch_params) {
               // movsd (%rsp), %xmm0
               emit_bytes(0xf2, 0x0f, 0x10, 0x04, 0x24);
               // cmpCCsd 8(%rsp), %xmm0
               emit_bytes(0xf2, 0x0f, 0xc2, 0x44, 0x24, 0x08, opcode);
            } else {
               // movsd 8(%rsp), %xmm0
               emit_bytes(0xf2, 0x0f, 0x10, 0x44, 0x24, 0x08);
               // cmpCCsd (%rsp), %xmm0
               emit_bytes(0xf2, 0x0f, 0xc2, 0x04, 0x24, opcode);
            }               
            // movd %xmm0, %eax
            emit_bytes(0x66, 0x0f, 0x7e, 0xc0);
            if (!flip_result) {
               // andl $1, eax
               emit_bytes(0x83, 0xe0, 0x01);
            } else {
               // incl %eax {0xffffffff, 0} -> {0, 1}
               emit_bytes(0xff, 0xc0);
            }
            // leaq 16(%rsp), %rsp
            emit_bytes(0x48, 0x8d, 0x64, 0x24, 0x10);
            // pushq %rax
            emit_bytes(0x50);
         }
      }

      template<class... T>
      void emit_i32_binop(T... op) {
         // popq %rcx
         emit_bytes(0x59);
         // popq %rax
         emit_bytes(0x58);
         // OP %eax, %ecx
         emit_bytes(static_cast<uint8_t>(op)...);
         // pushq %rax
         // emit_bytes(0x50);
      }

      template<class... T>
      void emit_i64_binop(T... op) {
         // popq %rcx
         emit_bytes(0x59);
         // popq %rax
         emit_bytes(0x58);
         // OP %eax, %ecx
         emit_bytes(static_cast<uint8_t>(op)...);
      }

      void emit_f32_binop(uint8_t op, float32_t (*softfloatfun)(float32_t, float32_t)) {
         if constexpr (use_softfloat) {
            return emit_f32_binop_softfloat(softfloatfun);
         }
         // movss 8(%rsp), %xmm0
         emit_bytes(0xf3, 0x0f, 0x10, 0x44, 0x24, 0x08);
         // OPss (%rsp), %xmm0
         emit_bytes(0xf3, 0x0f, op, 0x04, 0x24);
         // leaq 8(%rsp), %rsp
         emit_bytes(0x48, 0x8d, 0x64, 0x24, 0x08);
         // movss %xmm0, (%rsp)
         emit_bytes(0xf3, 0x0f, 0x11, 0x04, 0x24);
      }

      void emit_f64_binop(uint8_t op, float64_t (*softfloatfun)(float64_t, float64_t)) {
         if constexpr (use_softfloat) {
            return emit_f64_binop_softfloat(softfloatfun);
         }
         // movsd 8(%rsp), %xmm0
         emit_bytes(0xf2, 0x0f, 0x10, 0x44, 0x24, 0x08);
         // OPsd (%rsp), %xmm0
         emit_bytes(0xf2, 0x0f, op, 0x04, 0x24);
         // leaq 8(%rsp), %rsp
         emit_bytes(0x48, 0x8d, 0x64, 0x24, 0x08);
         // movsd %xmm0, (%rsp)
         emit_bytes(0xf2, 0x0f, 0x11, 0x04, 0x24);
      }

      // Beware: This pushes and pops mxcsr around the user op.  Remember to adjust access to %rsp in the caller.
      // Note uses %rcx after the user instruction
      template<class... T>
      void emit_f2i(T... op) {
         // mov 0x0x1f80, %eax // round-to-even/all exceptions masked/no exceptions set
         emit_bytes(0xb8, 0x80, 0x1f, 0x00, 0x00);
         // push %rax
         emit_bytes(0x50);
         // ldmxcsr (%rsp)
         emit_bytes(0x0f, 0xae, 0x14, 0x24);
         // user op
         emit_bytes(op...);
         // stmxcsr (%rsp)
         emit_bytes(0x0f, 0xae, 0x1c, 0x24);
         // pop %rcx
         emit_bytes(0x59);
         // test %cl, 0x1 // invalid
         emit_bytes(0xf6, 0xc1, 0x01);
         // jnz FP_ERROR_HANDLER
         emit_bytes(0x0f, 0x85);
         fix_branch(emit_branch_target32(), fpe_handler);
      }

      void* emit_error_handler(void (*handler)()) {
         void* result = code;
         // andq $-16, %rsp;
         emit_bytes(0x48, 0x83, 0xe4, 0xf0);
         // movabsq &on_unreachable, %rax
         emit_bytes(0x48, 0xb8);
         emit_operand_ptr(handler);
         // callq *%rax
         emit_bytes(0xff, 0xd0);
         return result;
      }

      void emit_align_stack() {
         // mov %rsp, rcx; andq $-16, %rsp; push rcx; push %rcx
         emit_bytes(0x48, 0x89, 0xe1);
         emit_bytes(0x48, 0x83, 0xe4, 0xf0);
         emit_bytes(0x51);
         emit_bytes(0x51);
      }

      void emit_restore_stack() {
         // mov (%rsp), %rsp
         emit_bytes(0x48, 0x8b, 0x24, 0x24);
      }

      void emit_host_call(uint32_t funcnum) {
         uint32_t extra = 0;
         if constexpr (Context::async_backtrace()) {
            // pushq %rbp
            emit_bytes(0x55);
            // movq %rsp, (%rdi)
            emit_bytes(0x48, 0x89, 0x27);
            extra = 8;
         }
         // mov $funcnum, %edx
         emit_bytes(0xba);
         emit_operand32(funcnum);
         // pushq %rdi
         emit_bytes(0x57);
         // pushq %rsi
         emit_bytes(0x56);
         // lea 24(%rsp), %rsi
         emit_bytes(0x48, 0x8d, 0x74, 0x24, 0x18 + extra);
         emit_align_stack();
         // movabsq $call_host_function, %rax
         emit_bytes(0x48, 0xb8);
         emit_operand_ptr(&call_host_function);
         // callq *%rax
         emit_bytes(0xff, 0xd0);
         emit_restore_stack();
         // popq %rsi
         emit_bytes(0x5e);
         // popq %rdi
         emit_bytes(0x5f);
         if constexpr (Context::async_backtrace()) {
            emit_restore_backtrace_basic();
            // popq %rbp
            emit_bytes(0x5d);
         }
         // retq
         emit_bytes(0xc3);
      }

      // Needs to run before saving %rdi.  Returns the number of bytes pushed onto the stack.
      uint32_t emit_setup_backtrace() {
         if constexpr (Context::async_backtrace()) {
            // callq next
            emit_bytes(0xe8);
            emit_operand32(0);
            // next:
            // pushq %rbp
            emit_bytes(0x55);
            // movq %rsp, (%rdi)
            emit_bytes(0x48, 0x89, 0x27);
            return 16;
         } else {
            return 0;
         }
      }
      // Does not adjust the stack pointer.  Use this if the
      // stack pointer adjustment is combined with another instruction.
      void emit_restore_backtrace_basic() {
         if constexpr (Context::async_backtrace()) {
            // xorl %edx, %edx
            emit_bytes(0x31, 0xd2);
            // movq %rdx, (%rdi)
            emit_bytes(0x48, 0x89, 0x17);
         }
      }
      void emit_restore_backtrace() {
         if constexpr (Context::async_backtrace()) {
            emit_restore_backtrace_basic();
            // addq $16, %rsp
            emit_bytes(0x48, 0x83, 0xc4, 0x10);
         }
      }

      bool is_host_function(uint32_t funcnum) { return funcnum < _mod.get_imported_functions_size(); }

      static native_value call_host_function(Context* context /*rdi*/, native_value* stack /*rsi*/, uint32_t idx /*edx*/) {
         // It's currently unsafe to throw through a jit frame, because we don't set up
         // the exception tables for them.
         native_value result;
         vm::longjmp_on_exception([&]() {
            result = context->call_host_function(stack, idx);
         });
         return result;
      }

      static int32_t current_memory(Context* context /*rdi*/) {
         return context->current_linear_memory();
      }

      static int32_t grow_memory(Context* context /*rdi*/, int32_t pages) {
         return context->grow_linear_memory(pages);
      }

      static void on_unreachable() { vm::throw_<wasm_interpreter_exception>( "unreachable" ); }
      static void on_fp_error() { vm::throw_<wasm_interpreter_exception>( "floating point error" ); }
      static void on_call_indirect_error() { vm::throw_<wasm_interpreter_exception>( "call_indirect out of range" ); }
      static void on_type_error() { vm::throw_<wasm_interpreter_exception>( "call_indirect incorrect function type" ); }
      static void on_stack_overflow() { vm::throw_<wasm_interpreter_exception>( "stack overflow" ); }
   };
   
}}
