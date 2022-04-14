#pragma once

#include <eosio/vm/config.hpp>
#include <eosio/vm/base_visitor.hpp>
#include <eosio/vm/exceptions.hpp>
#include <eosio/vm/opcodes.hpp>
#include <eosio/vm/softfloat.hpp>
#include <eosio/vm/stack_elem.hpp>
#include <eosio/vm/utils.hpp>
#include <eosio/vm/wasm_stack.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

namespace eosio { namespace vm {

   template <typename ExecutionContext>
   struct interpret_visitor : base_visitor {
      using base_visitor::operator();
      interpret_visitor(ExecutionContext& ec) : context(ec) {}
      ExecutionContext& context;

      ExecutionContext& get_context() { return context; }

      static inline constexpr void* align_address(void* addr, size_t align_amt) {
         if constexpr (should_align_memory_ops) {
            addr = (void*)(((uintptr_t)addr + (1 << align_amt) - 1) & ~((1 << align_amt) - 1));
            return addr;
         } else {
            return addr;
         }
      }
      template<typename T>
      static inline T read_unaligned(const void* addr) {
         T result;
         std::memcpy(&result, addr, sizeof(T));
         return result;
      }
      template<typename T>
      static void write_unaligned(void* addr, T value) {
         std::memcpy(addr, &value, sizeof(T));
      }

      [[gnu::always_inline]] inline void operator()(const unreachable_t& op) {
         context.inc_pc();
         throw wasm_interpreter_exception{ "unreachable" };
      }

      [[gnu::always_inline]] inline void operator()(const nop_t& op) { context.inc_pc(); }

      [[gnu::always_inline]] inline void operator()(const end_t& op) { context.inc_pc(); }
      [[gnu::always_inline]] inline void operator()(const return_t& op) { context.apply_pop_call(op.data, op.pc); }
      [[gnu::always_inline]] inline void operator()(const block_t& op) { context.inc_pc(); }
      [[gnu::always_inline]] inline void operator()(const loop_t& op) { context.inc_pc(); }
      [[gnu::always_inline]] inline void operator()(const if_t& op) {
         context.inc_pc();
         const auto& oper = context.pop_operand();
         if (!oper.to_ui32()) {
            context.set_relative_pc(op.pc);
         }
      }
      [[gnu::always_inline]] inline void operator()(const else_t& op) { context.set_relative_pc(op.pc); }
      [[gnu::always_inline]] inline void operator()(const br_t& op) { context.jump(op.data, op.pc); }
      [[gnu::always_inline]] inline void operator()(const br_if_t& op) {
         const auto& val = context.pop_operand();
         if (context.is_true(val)) {
            context.jump(op.data, op.pc);
         } else {
            context.inc_pc();
         }
      }

      [[gnu::always_inline]] inline void operator()(const br_table_data_t& op) {
         context.inc_pc(op.index);
      }
      [[gnu::always_inline]] inline void operator()(const br_table_t& op) {
         const auto& in = context.pop_operand().to_ui32();
         const auto& entry = op.table[std::min(in, op.size)];
         context.jump(entry.stack_pop, entry.pc);
      }
      [[gnu::always_inline]] inline void operator()(const call_t& op) {
         context.call(op.index);
      }
      [[gnu::always_inline]] inline void operator()(const call_indirect_t& op) {
         const auto& index = context.pop_operand().to_ui32();
         uint32_t fn = context.table_elem(index);
         const auto& expected_type = context.get_module().types.at(op.index);
         const auto& actual_type = context.get_module().get_function_type(fn);
         EOS_VM_ASSERT(actual_type == expected_type, wasm_interpreter_exception, "bad call_indirect type");
         context.call(fn);
      }
      [[gnu::always_inline]] inline void operator()(const drop_t& op) {
         context.pop_operand();
         context.inc_pc();
      }
      [[gnu::always_inline]] inline void operator()(const select_t& op) {
         const auto& c  = context.pop_operand();
         const auto& v2 = context.pop_operand();
         if (c.to_ui32() == 0) {
            context.peek_operand() = v2;
         }
         context.inc_pc();
      }
      [[gnu::always_inline]] inline void operator()(const get_local_t& op) {
         context.inc_pc();
         context.push_operand(context.get_operand(op.index));
      }
      [[gnu::always_inline]] inline void operator()(const set_local_t& op) {
         context.inc_pc();
         context.set_operand(op.index, context.pop_operand());
      }
      [[gnu::always_inline]] inline void operator()(const tee_local_t& op) {
         context.inc_pc();
         const auto& oper = context.pop_operand();
         context.set_operand(op.index, oper);
         context.push_operand(oper);
      }
      [[gnu::always_inline]] inline void operator()(const get_global_t& op) {
         context.inc_pc();
         const auto& gl = context.get_global(op.index);
         context.push_operand(gl);
      }
      [[gnu::always_inline]] inline void operator()(const set_global_t& op) {
         context.inc_pc();
         const auto& oper = context.pop_operand();
         context.set_global(op.index, oper);
      }
      template<typename Op>
      inline void * pop_memop_addr(const Op& op) {
         const auto& ptr  = context.pop_operand();
         return align_address((context.linear_memory() + op.offset + ptr.to_ui32()), op.flags_align);
      }
      [[gnu::always_inline]] inline void operator()(const i32_load_t& op) {
         context.inc_pc();
         void* _ptr = pop_memop_addr(op);
         context.push_operand(i32_const_t{ read_unaligned<uint32_t>(_ptr) });
      }
      [[gnu::always_inline]] inline void operator()(const i32_load8_s_t& op) {
         context.inc_pc();
         void* _ptr = pop_memop_addr(op);
         context.push_operand(i32_const_t{ static_cast<int32_t>(read_unaligned<int8_t>(_ptr) ) });
      }
      [[gnu::always_inline]] inline void operator()(const i32_load16_s_t& op) {
         context.inc_pc();
         void* _ptr = pop_memop_addr(op);
         context.push_operand(i32_const_t{ static_cast<int32_t>( read_unaligned<int16_t>(_ptr) ) });
      }
      [[gnu::always_inline]] inline void operator()(const i32_load8_u_t& op) {
         context.inc_pc();
         void* _ptr = pop_memop_addr(op);
         context.push_operand(i32_const_t{ static_cast<uint32_t>( read_unaligned<uint8_t>(_ptr) ) });
      }
      [[gnu::always_inline]] inline void operator()(const i32_load16_u_t& op) {
         context.inc_pc();
         void* _ptr = pop_memop_addr(op);
         context.push_operand(i32_const_t{ static_cast<uint32_t>( read_unaligned<uint16_t>(_ptr) ) });
      }
      [[gnu::always_inline]] inline void operator()(const i64_load_t& op) {
         context.inc_pc();
         void* _ptr = pop_memop_addr(op);
         context.push_operand(i64_const_t{ static_cast<uint64_t>( read_unaligned<uint64_t>(_ptr) ) });
      }
      [[gnu::always_inline]] inline void operator()(const i64_load8_s_t& op) {
         context.inc_pc();
         void* _ptr = pop_memop_addr(op);
         context.push_operand(i64_const_t{ static_cast<int64_t>( read_unaligned<int8_t>(_ptr) ) });
      }
      [[gnu::always_inline]] inline void operator()(const i64_load16_s_t& op) {
         context.inc_pc();
         void* _ptr = pop_memop_addr(op);
         context.push_operand(i64_const_t{ static_cast<int64_t>( read_unaligned<int16_t>(_ptr) ) });
      }
      [[gnu::always_inline]] inline void operator()(const i64_load32_s_t& op) {
         context.inc_pc();
         void* _ptr = pop_memop_addr(op);
         context.push_operand(i64_const_t{ static_cast<int64_t>( read_unaligned<int32_t>(_ptr) ) });
      }
      [[gnu::always_inline]] inline void operator()(const i64_load8_u_t& op) {
         context.inc_pc();
         void* _ptr = pop_memop_addr(op);
         context.push_operand(i64_const_t{ static_cast<uint64_t>( read_unaligned<uint8_t>(_ptr) ) });
      }
      [[gnu::always_inline]] inline void operator()(const i64_load16_u_t& op) {
         context.inc_pc();
         void* _ptr = pop_memop_addr(op);
         context.push_operand(i64_const_t{ static_cast<uint64_t>( read_unaligned<uint16_t>(_ptr) ) });
      }
      [[gnu::always_inline]] inline void operator()(const i64_load32_u_t& op) {
         context.inc_pc();
         void* _ptr = pop_memop_addr(op);
         context.push_operand(i64_const_t{ static_cast<uint64_t>( read_unaligned<uint32_t>(_ptr) ) });
      }
      [[gnu::always_inline]] inline void operator()(const f32_load_t& op) {
         context.inc_pc();
         void* _ptr = pop_memop_addr(op);
         context.push_operand(f32_const_t{ read_unaligned<uint32_t>(_ptr) });
      }
      [[gnu::always_inline]] inline void operator()(const f64_load_t& op) {
         context.inc_pc();
         void* _ptr = pop_memop_addr(op);
         context.push_operand(f64_const_t{ read_unaligned<uint64_t>(_ptr) });
      }
      [[gnu::always_inline]] inline void operator()(const i32_store_t& op) {
         context.inc_pc();
         const auto& val     = context.pop_operand();
         void* store_loc     = pop_memop_addr(op);
         write_unaligned(store_loc, val.to_ui32());
      }
      [[gnu::always_inline]] inline void operator()(const i32_store8_t& op) {
         context.inc_pc();
         const auto& val = context.pop_operand();
         void* store_loc = pop_memop_addr(op);
         write_unaligned(store_loc, static_cast<uint8_t>(val.to_ui32()));
      }
      [[gnu::always_inline]] inline void operator()(const i32_store16_t& op) {
         context.inc_pc();
         const auto& val     = context.pop_operand();
         void* store_loc     = pop_memop_addr(op);
         write_unaligned(store_loc, static_cast<uint16_t>(val.to_ui32()));
      }
      [[gnu::always_inline]] inline void operator()(const i64_store_t& op) {
         context.inc_pc();
         const auto& val     = context.pop_operand();
         void* store_loc     = pop_memop_addr(op);
         write_unaligned(store_loc, static_cast<uint64_t>(val.to_ui64()));
      }
      [[gnu::always_inline]] inline void operator()(const i64_store8_t& op) {
         context.inc_pc();
         const auto& val = context.pop_operand();
         void* store_loc = pop_memop_addr(op);
         write_unaligned(store_loc, static_cast<uint8_t>(val.to_ui64()));
      }
      [[gnu::always_inline]] inline void operator()(const i64_store16_t& op) {
         context.inc_pc();
         const auto& val     = context.pop_operand();
         void* store_loc     = pop_memop_addr(op);
         write_unaligned(store_loc, static_cast<uint16_t>(val.to_ui64()));
      }
      [[gnu::always_inline]] inline void operator()(const i64_store32_t& op) {
         context.inc_pc();
         const auto& val     = context.pop_operand();
         void* store_loc     = pop_memop_addr(op);
         write_unaligned(store_loc, static_cast<uint32_t>(val.to_ui64()));
      }
      [[gnu::always_inline]] inline void operator()(const f32_store_t& op) {
         context.inc_pc();
         const auto& val     = context.pop_operand();
         void* store_loc     = pop_memop_addr(op);
         write_unaligned(store_loc, static_cast<uint32_t>(val.to_fui32()));
      }
      [[gnu::always_inline]] inline void operator()(const f64_store_t& op) {
         context.inc_pc();
         const auto& val     = context.pop_operand();
         void* store_loc     = pop_memop_addr(op);
         write_unaligned(store_loc, static_cast<uint64_t>(val.to_fui64()));
      }
      [[gnu::always_inline]] inline void operator()(const current_memory_t& op) {
         context.inc_pc();
         context.push_operand(i32_const_t{ context.current_linear_memory() });
      }
      [[gnu::always_inline]] inline void operator()(const grow_memory_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_ui32();
         oper       = context.grow_linear_memory(oper);
      }
      [[gnu::always_inline]] inline void operator()(const i32_const_t& op) {
         context.inc_pc();
         context.push_operand(op);
      }
      [[gnu::always_inline]] inline void operator()(const i64_const_t& op) {
         context.inc_pc();
         context.push_operand(op);
      }
      [[gnu::always_inline]] inline void operator()(const f32_const_t& op) {
         context.inc_pc();
         context.push_operand(op);
      }
      [[gnu::always_inline]] inline void operator()(const f64_const_t& op) {
         context.inc_pc();
         context.push_operand(op);
      }
      [[gnu::always_inline]] inline void operator()(const i32_eqz_t& op) {
         context.inc_pc();
         auto& t = context.peek_operand().to_ui32();
         t       = t == 0;
      }
      [[gnu::always_inline]] inline void operator()(const i32_eq_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui32();
         auto&       lhs = context.peek_operand().to_ui32();
         lhs             = lhs == rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_ne_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui32();
         auto&       lhs = context.peek_operand().to_ui32();
         lhs             = lhs != rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_lt_s_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_i32();
         auto&       lhs = context.peek_operand().to_i32();
         lhs             = lhs < rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_lt_u_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui32();
         auto&       lhs = context.peek_operand().to_ui32();
         lhs             = lhs < rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_le_s_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_i32();
         auto&       lhs = context.peek_operand().to_i32();
         lhs             = lhs <= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_le_u_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui32();
         auto&       lhs = context.peek_operand().to_ui32();
         lhs             = lhs <= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_gt_s_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_i32();
         auto&       lhs = context.peek_operand().to_i32();
         lhs             = lhs > rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_gt_u_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui32();
         auto&       lhs = context.peek_operand().to_ui32();
         lhs             = lhs > rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_ge_s_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_i32();
         auto&       lhs = context.peek_operand().to_i32();
         lhs             = lhs >= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_ge_u_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui32();
         auto&       lhs = context.peek_operand().to_ui32();
         lhs             = lhs >= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i64_eqz_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         oper       = i32_const_t{ oper.to_ui64() == 0 };
      }
      [[gnu::always_inline]] inline void operator()(const i64_eq_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui64();
         auto&       lhs = context.peek_operand();
         lhs             = i32_const_t{ lhs.to_ui64() == rhs };
      }
      [[gnu::always_inline]] inline void operator()(const i64_ne_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui64();
         auto&       lhs = context.peek_operand();
         lhs             = i32_const_t{ lhs.to_ui64() != rhs };
      }
      [[gnu::always_inline]] inline void operator()(const i64_lt_s_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_i64();
         auto&       lhs = context.peek_operand();
         lhs             = i32_const_t{ lhs.to_i64() < rhs };
      }
      [[gnu::always_inline]] inline void operator()(const i64_lt_u_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui64();
         auto&       lhs = context.peek_operand();
         lhs             = i32_const_t{ lhs.to_ui64() < rhs };
      }
      [[gnu::always_inline]] inline void operator()(const i64_le_s_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_i64();
         auto&       lhs = context.peek_operand();
         lhs             = i32_const_t{ lhs.to_i64() <= rhs };
      }
      [[gnu::always_inline]] inline void operator()(const i64_le_u_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui64();
         auto&       lhs = context.peek_operand();
         lhs             = i32_const_t{ lhs.to_ui64() <= rhs };
      }
      [[gnu::always_inline]] inline void operator()(const i64_gt_s_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_i64();
         auto&       lhs = context.peek_operand();
         lhs             = i32_const_t{ lhs.to_i64() > rhs };
      }
      [[gnu::always_inline]] inline void operator()(const i64_gt_u_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui64();
         auto&       lhs = context.peek_operand();
         lhs             = i32_const_t{ lhs.to_ui64() > rhs };
      }
      [[gnu::always_inline]] inline void operator()(const i64_ge_s_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_i64();
         auto&       lhs = context.peek_operand();
         lhs             = i32_const_t{ lhs.to_i64() >= rhs };
      }
      [[gnu::always_inline]] inline void operator()(const i64_ge_u_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui64();
         auto&       lhs = context.peek_operand();
         lhs             = i32_const_t{ lhs.to_ui64() >= rhs };
      }
      [[gnu::always_inline]] inline void operator()(const f32_eq_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_f32();
         auto&       lhs = context.peek_operand();
         if constexpr (use_softfloat)
            lhs = i32_const_t{ (uint32_t)_eosio_f32_eq(lhs.to_f32(), rhs) };
         else
            lhs = i32_const_t{ (uint32_t)(lhs.to_f32() == rhs) };
      }
      [[gnu::always_inline]] inline void operator()(const f32_ne_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_f32();
         auto&       lhs = context.peek_operand();
         if constexpr (use_softfloat)
            lhs = i32_const_t{ (uint32_t)_eosio_f32_ne(lhs.to_f32(), rhs) };
         else
            lhs = i32_const_t{ (uint32_t)(lhs.to_f32() != rhs) };
      }
      [[gnu::always_inline]] inline void operator()(const f32_lt_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_f32();
         auto&       lhs = context.peek_operand();
         if constexpr (use_softfloat)
            lhs = i32_const_t{ (uint32_t)_eosio_f32_lt(lhs.to_f32(), rhs) };
         else
            lhs = i32_const_t{ (uint32_t)(lhs.to_f32() < rhs) };
      }
      [[gnu::always_inline]] inline void operator()(const f32_gt_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_f32();
         auto&       lhs = context.peek_operand();
         if constexpr (use_softfloat)
            lhs = i32_const_t{ (uint32_t)_eosio_f32_gt(lhs.to_f32(), rhs) };
         else
            lhs = i32_const_t{ (uint32_t)(lhs.to_f32() > rhs) };
      }
      [[gnu::always_inline]] inline void operator()(const f32_le_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_f32();
         auto&       lhs = context.peek_operand();
         if constexpr (use_softfloat)
            lhs = i32_const_t{ (uint32_t)_eosio_f32_le(lhs.to_f32(), rhs) };
         else
            lhs = i32_const_t{ (uint32_t)(lhs.to_f32() <= rhs) };
      }
      [[gnu::always_inline]] inline void operator()(const f32_ge_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_f32();
         auto&       lhs = context.peek_operand();
         if constexpr (use_softfloat)
            lhs = i32_const_t{ (uint32_t)_eosio_f32_ge(lhs.to_f32(), rhs) };
         else
            lhs = i32_const_t{ (uint32_t)(lhs.to_f32() >= rhs) };
      }
      [[gnu::always_inline]] inline void operator()(const f64_eq_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_f64();
         auto&       lhs = context.peek_operand();
         if constexpr (use_softfloat)
            lhs = i32_const_t{ (uint32_t)_eosio_f64_eq(lhs.to_f64(), rhs) };
         else
            lhs = i32_const_t{ (uint32_t)(lhs.to_f64() == rhs) };
      }
      [[gnu::always_inline]] inline void operator()(const f64_ne_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_f64();
         auto&       lhs = context.peek_operand();
         if constexpr (use_softfloat)
            lhs = i32_const_t{ (uint32_t)_eosio_f64_ne(lhs.to_f64(), rhs) };
         else
            lhs = i32_const_t{ (uint32_t)(lhs.to_f64() != rhs) };
      }
      [[gnu::always_inline]] inline void operator()(const f64_lt_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_f64();
         auto&       lhs = context.peek_operand();
         if constexpr (use_softfloat)
            lhs = i32_const_t{ (uint32_t)_eosio_f64_lt(lhs.to_f64(), rhs) };
         else
            lhs = i32_const_t{ (uint32_t)(lhs.to_f64() < rhs) };
      }
      [[gnu::always_inline]] inline void operator()(const f64_gt_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_f64();
         auto&       lhs = context.peek_operand();
         if constexpr (use_softfloat)
            lhs = i32_const_t{ (uint32_t)_eosio_f64_gt(lhs.to_f64(), rhs) };
         else
            lhs = i32_const_t{ (uint32_t)(lhs.to_f64() > rhs) };
      }
      [[gnu::always_inline]] inline void operator()(const f64_le_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_f64();
         auto&       lhs = context.peek_operand();
         if constexpr (use_softfloat)
            lhs = i32_const_t{ (uint32_t)_eosio_f64_le(lhs.to_f64(), rhs) };
         else
            lhs = i32_const_t{ (uint32_t)(lhs.to_f64() <= rhs) };
      }
      [[gnu::always_inline]] inline void operator()(const f64_ge_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_f64();
         auto&       lhs = context.peek_operand();
         if constexpr (use_softfloat)
            lhs = i32_const_t{ (uint32_t)_eosio_f64_ge(lhs.to_f64(), rhs) };
         else
            lhs = i32_const_t{ (uint32_t)(lhs.to_f64() >= rhs) };
      }
      [[gnu::always_inline]] inline void operator()(const i32_clz_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_ui32();
         // __builtin_clz(0) is undefined
         oper = oper == 0 ? 32 : __builtin_clz(oper);
      }
      [[gnu::always_inline]] inline void operator()(const i32_ctz_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_ui32();

         // __builtin_ctz(0) is undefined
         oper = oper == 0 ? 32 : __builtin_ctz(oper);
      }
      [[gnu::always_inline]] inline void operator()(const i32_popcnt_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_ui32();
         oper       = __builtin_popcount(oper);
      }
      [[gnu::always_inline]] inline void operator()(const i32_add_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui32();
         auto&       lhs = context.peek_operand().to_ui32();
         lhs += rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_sub_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui32();
         auto&       lhs = context.peek_operand().to_ui32();
         lhs -= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_mul_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui32();
         auto&       lhs = context.peek_operand().to_ui32();
         lhs *= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_div_s_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_i32();
         auto&       lhs = context.peek_operand().to_i32();
         EOS_VM_ASSERT(rhs != 0, wasm_interpreter_exception, "i32.div_s divide by zero");
         EOS_VM_ASSERT(!(lhs == std::numeric_limits<int32_t>::min() && rhs == -1), wasm_interpreter_exception,
                       "i32.div_s traps when I32_MAX/-1");
         lhs /= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_div_u_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui32();
         auto&       lhs = context.peek_operand().to_ui32();
         EOS_VM_ASSERT(rhs != 0, wasm_interpreter_exception, "i32.div_u divide by zero");
         lhs /= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_rem_s_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_i32();
         auto&       lhs = context.peek_operand().to_i32();
         EOS_VM_ASSERT(rhs != 0, wasm_interpreter_exception, "i32.rem_s divide by zero");
         if (UNLIKELY(lhs == std::numeric_limits<int32_t>::min() && rhs == -1))
            lhs = 0;
         else
            lhs %= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_rem_u_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui32();
         auto&       lhs = context.peek_operand().to_ui32();
         EOS_VM_ASSERT(rhs != 0, wasm_interpreter_exception, "i32.rem_u divide by zero");
         lhs %= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_and_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui32();
         auto&       lhs = context.peek_operand().to_ui32();
         lhs &= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_or_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui32();
         auto&       lhs = context.peek_operand().to_ui32();
         lhs |= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_xor_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui32();
         auto&       lhs = context.peek_operand().to_ui32();
         lhs ^= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i32_shl_t& op) {
         context.inc_pc();
         static constexpr uint32_t mask = (8 * sizeof(uint32_t) - 1);
         const auto& rhs = context.pop_operand().to_ui32();
         auto&       lhs = context.peek_operand().to_ui32();
         lhs <<= (rhs & mask);
      }
      [[gnu::always_inline]] inline void operator()(const i32_shr_s_t& op) {
         context.inc_pc();
         static constexpr uint32_t mask = (8 * sizeof(uint32_t) - 1);
         const auto& rhs = context.pop_operand().to_ui32();
         auto&       lhs = context.peek_operand().to_i32();
         lhs >>= (rhs & mask);
      }
      [[gnu::always_inline]] inline void operator()(const i32_shr_u_t& op) {
         context.inc_pc();
         static constexpr uint32_t mask = (8 * sizeof(uint32_t) - 1);
         const auto& rhs = context.pop_operand().to_ui32();
         auto&       lhs = context.peek_operand().to_ui32();
         lhs >>= (rhs & mask);
      }
      [[gnu::always_inline]] inline void operator()(const i32_rotl_t& op) {

         context.inc_pc();
         static constexpr uint32_t mask = (8 * sizeof(uint32_t) - 1);
         const auto&               rhs  = context.pop_operand().to_ui32();
         auto&                     lhs  = context.peek_operand().to_ui32();
         uint32_t                  c    = rhs;
         c &= mask;
         lhs = (lhs << c) | (lhs >> ((-c) & mask));
      }
      [[gnu::always_inline]] inline void operator()(const i32_rotr_t& op) {
         context.inc_pc();
         static constexpr uint32_t mask = (8 * sizeof(uint32_t) - 1);
         const auto&               rhs  = context.pop_operand().to_ui32();
         auto&                     lhs  = context.peek_operand().to_ui32();
         uint32_t                  c    = rhs;
         c &= mask;
         lhs = (lhs >> c) | (lhs << ((-c) & mask));
      }
      [[gnu::always_inline]] inline void operator()(const i64_clz_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_ui64();
         // __builtin_clzll(0) is undefined
         oper = oper == 0 ? 64 : __builtin_clzll(oper);
      }
      [[gnu::always_inline]] inline void operator()(const i64_ctz_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_ui64();
         // __builtin_clzll(0) is undefined
         oper = oper == 0 ? 64 : __builtin_ctzll(oper);
      }
      [[gnu::always_inline]] inline void operator()(const i64_popcnt_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_ui64();
         oper       = __builtin_popcountll(oper);
      }
      [[gnu::always_inline]] inline void operator()(const i64_add_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui64();
         auto&       lhs = context.peek_operand().to_ui64();
         lhs += rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i64_sub_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui64();
         auto&       lhs = context.peek_operand().to_ui64();
         lhs -= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i64_mul_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui64();
         auto&       lhs = context.peek_operand().to_ui64();
         lhs *= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i64_div_s_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_i64();
         auto&       lhs = context.peek_operand().to_i64();
         EOS_VM_ASSERT(rhs != 0, wasm_interpreter_exception, "i64.div_s divide by zero");
         EOS_VM_ASSERT(!(lhs == std::numeric_limits<int64_t>::min() && rhs == -1), wasm_interpreter_exception,
                       "i64.div_s traps when I64_MAX/-1");
         lhs /= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i64_div_u_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui64();
         auto&       lhs = context.peek_operand().to_ui64();
         EOS_VM_ASSERT(rhs != 0, wasm_interpreter_exception, "i64.div_u divide by zero");
         lhs /= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i64_rem_s_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_i64();
         auto&       lhs = context.peek_operand().to_i64();
         EOS_VM_ASSERT(rhs != 0, wasm_interpreter_exception, "i64.rem_s divide by zero");
         if (UNLIKELY(lhs == std::numeric_limits<int64_t>::min() && rhs == -1))
            lhs = 0;
         else
            lhs %= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i64_rem_u_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui64();
         auto&       lhs = context.peek_operand().to_ui64();
         EOS_VM_ASSERT(rhs != 0, wasm_interpreter_exception, "i64.rem_s divide by zero");
         lhs %= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i64_and_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui64();
         auto&       lhs = context.peek_operand().to_ui64();
         lhs &= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i64_or_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui64();
         auto&       lhs = context.peek_operand().to_ui64();
         lhs |= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i64_xor_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand().to_ui64();
         auto&       lhs = context.peek_operand().to_ui64();
         lhs ^= rhs;
      }
      [[gnu::always_inline]] inline void operator()(const i64_shl_t& op) {
         context.inc_pc();
         static constexpr uint64_t mask = (8 * sizeof(uint64_t) - 1);
         const auto& rhs = context.pop_operand().to_ui64();
         auto&       lhs = context.peek_operand().to_ui64();
         lhs <<= (rhs & mask);
      }
      [[gnu::always_inline]] inline void operator()(const i64_shr_s_t& op) {
         context.inc_pc();
         static constexpr uint64_t mask = (8 * sizeof(uint64_t) - 1);
         const auto& rhs = context.pop_operand().to_ui64();
         auto&       lhs = context.peek_operand().to_i64();
         lhs >>= (rhs & mask);
      }
      [[gnu::always_inline]] inline void operator()(const i64_shr_u_t& op) {
         context.inc_pc();
         static constexpr uint64_t mask = (8 * sizeof(uint64_t) - 1);
         const auto& rhs = context.pop_operand().to_ui64();
         auto&       lhs = context.peek_operand().to_ui64();
         lhs >>= (rhs & mask);
      }
      [[gnu::always_inline]] inline void operator()(const i64_rotl_t& op) {
         context.inc_pc();
         static constexpr uint64_t mask = (8 * sizeof(uint64_t) - 1);
         const auto&               rhs  = context.pop_operand().to_ui64();
         auto&                     lhs  = context.peek_operand().to_ui64();
         uint32_t                  c    = rhs;
         c &= mask;
         lhs = (lhs << c) | (lhs >> (-c & mask));
      }
      [[gnu::always_inline]] inline void operator()(const i64_rotr_t& op) {
         context.inc_pc();
         static constexpr uint64_t mask = (8 * sizeof(uint64_t) - 1);
         const auto&               rhs  = context.pop_operand().to_ui64();
         auto&                     lhs  = context.peek_operand().to_ui64();
         uint32_t                  c    = rhs;
         c &= mask;
         lhs = (lhs >> c) | (lhs << (-c & mask));
      }
      [[gnu::always_inline]] inline void operator()(const f32_abs_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_f32();
         if constexpr (use_softfloat)
            oper = _eosio_f32_abs(oper);
         else
            oper = __builtin_fabsf(oper);
      }
      [[gnu::always_inline]] inline void operator()(const f32_neg_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_f32();
         if constexpr (use_softfloat)
            oper = _eosio_f32_neg(oper);
         else
            oper = -oper;
      }
      [[gnu::always_inline]] inline void operator()(const f32_ceil_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_f32();
         if constexpr (use_softfloat)
            oper = _eosio_f32_ceil<false>(oper);
         else
            oper = __builtin_ceilf(oper);
      }
      [[gnu::always_inline]] inline void operator()(const f32_floor_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_f32();
         if constexpr (use_softfloat)
            oper = _eosio_f32_floor<false>(oper);
         else
            oper = __builtin_floorf(oper);
      }
      [[gnu::always_inline]] inline void operator()(const f32_trunc_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_f32();
         if constexpr (use_softfloat)
            oper = _eosio_f32_trunc<false>(oper);
         else
            oper = __builtin_trunc(oper);
      }
      [[gnu::always_inline]] inline void operator()(const f32_nearest_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_f32();
         if constexpr (use_softfloat)
            oper = _eosio_f32_nearest<false>(oper);
         else
            oper = __builtin_nearbyintf(oper);
      }
      [[gnu::always_inline]] inline void operator()(const f32_sqrt_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_f32();
         if constexpr (use_softfloat)
            oper = _eosio_f32_sqrt(oper);
         else
            oper = __builtin_sqrtf(oper);
      }
      [[gnu::always_inline]] inline void operator()(const f32_add_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = context.peek_operand().to_f32();
         if constexpr (use_softfloat)
            lhs = _eosio_f32_add(lhs, rhs.to_f32());
         else
            lhs += rhs.to_f32();
      }
      [[gnu::always_inline]] inline void operator()(const f32_sub_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = context.peek_operand().to_f32();
         if constexpr (use_softfloat)
            lhs = _eosio_f32_sub(lhs, rhs.to_f32());
         else
            lhs -= rhs.to_f32();
      }
      [[gnu::always_inline]] inline void operator()(const f32_mul_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = context.peek_operand().to_f32();
         if constexpr (use_softfloat) {
            lhs = _eosio_f32_mul(lhs, rhs.to_f32());
         } else
            lhs *= rhs.to_f32();
      }
      [[gnu::always_inline]] inline void operator()(const f32_div_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = context.peek_operand().to_f32();
         if constexpr (use_softfloat)
            lhs = _eosio_f32_div(lhs, rhs.to_f32());
         else
            lhs /= rhs.to_f32();
      }
      [[gnu::always_inline]] inline void operator()(const f32_min_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = context.peek_operand().to_f32();
         if constexpr (use_softfloat)
            lhs = _eosio_f32_min<false>(lhs, rhs.to_f32());
         else
            lhs = __builtin_fminf(lhs, rhs.to_f32());
      }
      [[gnu::always_inline]] inline void operator()(const f32_max_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = context.peek_operand().to_f32();
         if constexpr (use_softfloat)
            lhs = _eosio_f32_max<false>(lhs, rhs.to_f32());
         else
            lhs = __builtin_fmaxf(lhs, rhs.to_f32());
      }
      [[gnu::always_inline]] inline void operator()(const f32_copysign_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = context.peek_operand().to_f32();
         if constexpr (use_softfloat)
            lhs = _eosio_f32_copysign(lhs, rhs.to_f32());
         else
            lhs = __builtin_copysignf(lhs, rhs.to_f32());
      }
      [[gnu::always_inline]] inline void operator()(const f64_abs_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_f64();
         if constexpr (use_softfloat)
            oper = _eosio_f64_abs(oper);
         else
            oper = __builtin_fabs(oper);
      }
      [[gnu::always_inline]] inline void operator()(const f64_neg_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_f64();
         if constexpr (use_softfloat)
            oper = _eosio_f64_neg(oper);
         else
            oper = -oper;
      }
      [[gnu::always_inline]] inline void operator()(const f64_ceil_t& op) {

         context.inc_pc();
         auto& oper = context.peek_operand().to_f64();
         if constexpr (use_softfloat)
            oper = _eosio_f64_ceil<false>(oper);
         else
            oper = __builtin_ceil(oper);
      }
      [[gnu::always_inline]] inline void operator()(const f64_floor_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_f64();
         if constexpr (use_softfloat)
            oper = _eosio_f64_floor<false>(oper);
         else
            oper = __builtin_floor(oper);
      }
      [[gnu::always_inline]] inline void operator()(const f64_trunc_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_f64();
         if constexpr (use_softfloat)
            oper = _eosio_f64_trunc<false>(oper);
         else
            oper = __builtin_trunc(oper);
      }
      [[gnu::always_inline]] inline void operator()(const f64_nearest_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_f64();
         if constexpr (use_softfloat)
            oper = _eosio_f64_nearest<false>(oper);
         else
            oper = __builtin_nearbyint(oper);
      }
      [[gnu::always_inline]] inline void operator()(const f64_sqrt_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand().to_f64();
         if constexpr (use_softfloat)
            oper = _eosio_f64_sqrt(oper);
         else
            oper = __builtin_sqrt(oper);
      }
      [[gnu::always_inline]] inline void operator()(const f64_add_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = context.peek_operand().to_f64();
         if constexpr (use_softfloat)
            lhs = _eosio_f64_add(lhs, rhs.to_f64());
         else
            lhs += rhs.to_f64();
      }
      [[gnu::always_inline]] inline void operator()(const f64_sub_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = context.peek_operand().to_f64();
         if constexpr (use_softfloat)
            lhs = _eosio_f64_sub(lhs, rhs.to_f64());
         else
            lhs -= rhs.to_f64();
      }
      [[gnu::always_inline]] inline void operator()(const f64_mul_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = context.peek_operand().to_f64();
         if constexpr (use_softfloat)
            lhs = _eosio_f64_mul(lhs, rhs.to_f64());
         else
            lhs *= rhs.to_f64();
      }
      [[gnu::always_inline]] inline void operator()(const f64_div_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = context.peek_operand().to_f64();
         if constexpr (use_softfloat)
            lhs = _eosio_f64_div(lhs, rhs.to_f64());
         else
            lhs /= rhs.to_f64();
      }
      [[gnu::always_inline]] inline void operator()(const f64_min_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = context.peek_operand().to_f64();
         if constexpr (use_softfloat)
            lhs = _eosio_f64_min<false>(lhs, rhs.to_f64());
         else
            lhs = __builtin_fmin(lhs, rhs.to_f64());
      }
      [[gnu::always_inline]] inline void operator()(const f64_max_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = context.peek_operand().to_f64();
         if constexpr (use_softfloat)
            lhs = _eosio_f64_max<false>(lhs, rhs.to_f64());
         else
            lhs = __builtin_fmax(lhs, rhs.to_f64());
      }
      [[gnu::always_inline]] inline void operator()(const f64_copysign_t& op) {
         context.inc_pc();
         const auto& rhs = context.pop_operand();
         auto&       lhs = context.peek_operand().to_f64();
         if constexpr (use_softfloat)
            lhs = _eosio_f64_copysign(lhs, rhs.to_f64());
         else
            lhs = __builtin_copysign(lhs, rhs.to_f64());
      }
      [[gnu::always_inline]] inline void operator()(const i32_wrap_i64_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         oper       = i32_const_t{ static_cast<int32_t>(oper.to_i64()) };
      }
      [[gnu::always_inline]] inline void operator()(const i32_trunc_s_f32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = i32_const_t{ _eosio_f32_trunc_i32s(oper.to_f32()) };
         } else {
            float af = oper.to_f32();
            EOS_VM_ASSERT(!((af >= 2147483648.0f) || (af < -2147483648.0f)), wasm_interpreter_exception, "Error, f32.trunc_s/i32 overflow" );
            EOS_VM_ASSERT(!__builtin_isnan(af), wasm_interpreter_exception, "Error, f32.trunc_s/i32 unrepresentable");
            oper = i32_const_t{ static_cast<int32_t>(af) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const i32_trunc_u_f32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = i32_const_t{ _eosio_f32_trunc_i32u(oper.to_f32()) };
         } else {
            float af = oper.to_f32();
            EOS_VM_ASSERT(!((af >= 4294967296.0f) || (af <= -1.0f)),wasm_interpreter_exception, "Error, f32.trunc_u/i32 overflow");
            EOS_VM_ASSERT(!__builtin_isnan(af), wasm_interpreter_exception, "Error, f32.trunc_u/i32 unrepresentable");
            oper = i32_const_t{ static_cast<uint32_t>(af) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const i32_trunc_s_f64_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = i32_const_t{ _eosio_f64_trunc_i32s(oper.to_f64()) };
         } else {
            double af = oper.to_f64();
            EOS_VM_ASSERT(!((af >= 2147483648.0) || (af < -2147483648.0)), wasm_interpreter_exception, "Error, f64.trunc_s/i32 overflow");
            EOS_VM_ASSERT(!__builtin_isnan(af), wasm_interpreter_exception, "Error, f64.trunc_s/i32 unrepresentable");
            oper = i32_const_t{ static_cast<int32_t>(af) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const i32_trunc_u_f64_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = i32_const_t{ _eosio_f64_trunc_i32u(oper.to_f64()) };
         } else {
            double af = oper.to_f64();
            EOS_VM_ASSERT(!((af >= 4294967296.0) || (af <= -1.0)), wasm_interpreter_exception, "Error, f64.trunc_u/i32 overflow");
            EOS_VM_ASSERT(!__builtin_isnan(af), wasm_interpreter_exception, "Error, f64.trunc_u/i32 unrepresentable");
            oper = i32_const_t{ static_cast<uint32_t>(af) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const i64_extend_s_i32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         oper       = i64_const_t{ static_cast<int64_t>(oper.to_i32()) };
      }
      [[gnu::always_inline]] inline void operator()(const i64_extend_u_i32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         oper       = i64_const_t{ static_cast<uint64_t>(oper.to_ui32()) };
      }
      [[gnu::always_inline]] inline void operator()(const i64_trunc_s_f32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = i64_const_t{ _eosio_f32_trunc_i64s(oper.to_f32()) };
         } else {
            float af = oper.to_f32();
            EOS_VM_ASSERT(!((af >= 9223372036854775808.0f) || (af < -9223372036854775808.0f)), wasm_interpreter_exception, "Error, f32.trunc_s/i64 overflow");
            EOS_VM_ASSERT(!__builtin_isnan(af), wasm_interpreter_exception, "Error, f32.trunc_s/i64 unrepresentable");
            oper = i64_const_t{ static_cast<int64_t>(af) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const i64_trunc_u_f32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = i64_const_t{ _eosio_f32_trunc_i64u(oper.to_f32()) };
         } else {
            float af = oper.to_f32();
            EOS_VM_ASSERT(!((af >= 18446744073709551616.0f) || (af <= -1.0f)), wasm_interpreter_exception, "Error, f32.trunc_u/i64 overflow");
            EOS_VM_ASSERT(!__builtin_isnan(af), wasm_interpreter_exception, "Error, f32.trunc_u/i64 unrepresentable");
            oper = i64_const_t{ static_cast<uint64_t>(af) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const i64_trunc_s_f64_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = i64_const_t{ _eosio_f64_trunc_i64s(oper.to_f64()) };
         } else {
            double af = oper.to_f64();
            EOS_VM_ASSERT(!((af >= 9223372036854775808.0) || (af < -9223372036854775808.0)), wasm_interpreter_exception, "Error, f64.trunc_s/i64 overflow");
            EOS_VM_ASSERT(!__builtin_isnan(af), wasm_interpreter_exception, "Error, f64.trunc_s/i64 unrepresentable");
            oper = i64_const_t{ static_cast<int64_t>(af) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const i64_trunc_u_f64_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = i64_const_t{ _eosio_f64_trunc_i64u(oper.to_f64()) };
         } else {
            double af = oper.to_f64();
            EOS_VM_ASSERT(!((af >= 18446744073709551616.0) || (af <= -1.0)), wasm_interpreter_exception, "Error, f64.trunc_u/i64 overflow");
            EOS_VM_ASSERT(!__builtin_isnan(af), wasm_interpreter_exception, "Error, f64.trunc_u/i64 unrepresentable");
            oper = i64_const_t{ static_cast<uint64_t>(af) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const f32_convert_s_i32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = f32_const_t{ _eosio_i32_to_f32(oper.to_i32()) };
         } else {
            oper = f32_const_t{ static_cast<float>(oper.to_i32()) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const f32_convert_u_i32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = f32_const_t{ _eosio_ui32_to_f32(oper.to_ui32()) };
         } else {
            oper = f32_const_t{ static_cast<float>(oper.to_ui32()) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const f32_convert_s_i64_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = f32_const_t{ _eosio_i64_to_f32(oper.to_i64()) };
         } else {
            oper = f32_const_t{ static_cast<float>(oper.to_i64()) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const f32_convert_u_i64_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = f32_const_t{ _eosio_ui64_to_f32(oper.to_ui64()) };
         } else {
            oper = f32_const_t{ static_cast<float>(oper.to_ui64()) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const f32_demote_f64_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = f32_const_t{ _eosio_f64_demote(oper.to_f64()) };
         } else {
            oper = f32_const_t{ static_cast<float>(oper.to_f64()) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const f64_convert_s_i32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = f64_const_t{ _eosio_i32_to_f64(oper.to_i32()) };
         } else {
            oper = f64_const_t{ static_cast<double>(oper.to_i32()) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const f64_convert_u_i32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = f64_const_t{ _eosio_ui32_to_f64(oper.to_ui32()) };
         } else {
            oper = f64_const_t{ static_cast<double>(oper.to_ui32()) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const f64_convert_s_i64_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = f64_const_t{ _eosio_i64_to_f64(oper.to_i64()) };
         } else {
            oper = f64_const_t{ static_cast<double>(oper.to_i64()) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const f64_convert_u_i64_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = f64_const_t{ _eosio_ui64_to_f64(oper.to_ui64()) };
         } else {
            oper = f64_const_t{ static_cast<double>(oper.to_ui64()) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const f64_promote_f32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         if constexpr (use_softfloat) {
            oper = f64_const_t{ _eosio_f32_promote(oper.to_f32()) };
         } else {
            oper = f64_const_t{ static_cast<double>(oper.to_f32()) };
         }
      }
      [[gnu::always_inline]] inline void operator()(const i32_reinterpret_f32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         oper       = i32_const_t{ oper.to_fui32() };
      }
      [[gnu::always_inline]] inline void operator()(const i64_reinterpret_f64_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         oper       = i64_const_t{ oper.to_fui64() };
      }
      [[gnu::always_inline]] inline void operator()(const f32_reinterpret_i32_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         oper       = f32_const_t{ oper.to_ui32() };
      }
      [[gnu::always_inline]] inline void operator()(const f64_reinterpret_i64_t& op) {
         context.inc_pc();
         auto& oper = context.peek_operand();
         oper       = f64_const_t{ oper.to_ui64() };
      }
      [[gnu::always_inline]] inline void operator()(const v128_load_t& op) {
         context.inc_pc();
         void* _ptr = pop_memop_addr(op);
         context.push_operand(v128_const_t{ read_unaligned<v128_t>(_ptr) });
      }
      template<typename T, typename U, typename Op>
      [[gnu::always_inline]] inline void v128_load_extend(const Op& op) {
         context.inc_pc();
         void* _ptr = pop_memop_addr(op);
         T a;
         static_assert(sizeof(T) == 16);
         for(int i = 0; i < sizeof(a)/sizeof(a[0]); ++i) {
            a[i] = read_unaligned<U>(static_cast<const char*>(_ptr) + i*sizeof(U));
         }
         v128_t res;
         std::memcpy(&res, &a, sizeof(res));
         context.push_operand(v128_const_t{ res });
      }
      [[gnu::always_inline]] inline void operator()(const v128_load8x8_s_t& op) {
         v128_load_extend<std::int16_t[8], std::int8_t>(op);
      }
      [[gnu::always_inline]] inline void operator()(const v128_load8x8_u_t& op) {
         v128_load_extend<std::uint16_t[8], std::uint8_t>(op);
      }
      [[gnu::always_inline]] inline void operator()(const v128_load16x4_s_t& op) {
         v128_load_extend<std::int32_t[4], std::int16_t>(op);
      }
      [[gnu::always_inline]] inline void operator()(const v128_load16x4_u_t& op) {
         v128_load_extend<std::uint32_t[4], std::uint16_t>(op);
      }
      [[gnu::always_inline]] inline void operator()(const v128_load32x2_s_t& op) {
         v128_load_extend<std::int64_t[2], std::int32_t>(op);
      }
      [[gnu::always_inline]] inline void operator()(const v128_load32x2_u_t& op) {
         v128_load_extend<std::uint64_t[2], std::uint32_t>(op);
      }
      template<typename T, typename Op>
      [[gnu::always_inline]] inline void v128_load_splat(const Op& op) {
         context.inc_pc();
         void* _ptr = pop_memop_addr(op);
         T a;
         static_assert(sizeof(T) == 16);
         auto val = read_unaligned<std::decay_t<decltype(a[0])>>(_ptr);
         for(int i = 0; i < sizeof(a)/sizeof(a[0]); ++i) {
            a[i] = val;
         }
         v128_t res;
         std::memcpy(&res, &a, sizeof(res));
         context.push_operand(v128_const_t{ res });
      }
      [[gnu::always_inline]] inline void operator()(const v128_load8_splat_t& op) {
         v128_load_splat<std::uint8_t[16]>(op);
      }
      [[gnu::always_inline]] inline void operator()(const v128_load16_splat_t& op) {
         v128_load_splat<std::uint16_t[8]>(op);
      }
      [[gnu::always_inline]] inline void operator()(const v128_load32_splat_t& op) {
         v128_load_splat<std::uint32_t[4]>(op);
      }
      [[gnu::always_inline]] inline void operator()(const v128_load64_splat_t& op) {
         v128_load_splat<std::uint64_t[2]>(op);
      }
      template<typename T, typename Op>
      [[gnu::always_inline]] inline void v128_load_zero(const Op& op) {
         context.inc_pc();
         void* _ptr = pop_memop_addr(op);
         T a = {};
         static_assert(sizeof(T) == 16);
         a[0] = read_unaligned<std::decay_t<decltype(a[0])>>(_ptr);
         v128_t res;
         std::memcpy(&res, &a, sizeof(res));
         context.push_operand(v128_const_t{ res });
      }
      [[gnu::always_inline]] inline void operator()(const v128_load32_zero_t& op) {
         v128_load_zero<std::uint32_t[4]>(op);
      }
      [[gnu::always_inline]] inline void operator()(const v128_load64_zero_t& op) {
         v128_load_zero<std::uint64_t[2]>(op);
      }
      [[gnu::always_inline]] inline void operator()(const v128_store_t& op) {
         context.inc_pc();
         const auto& val     = context.pop_operand();
         void* store_loc     = pop_memop_addr(op);
         write_unaligned(store_loc, val.to_v128());
      }
      template<typename T, typename Op>
      [[gnu::always_inline]] inline void v128_load_lane(const Op& op) {
         context.inc_pc();
         v128_t val = context.pop_operand().to_v128();
         void* _ptr = pop_memop_addr(op);
         T a;
         static_assert(sizeof(T) == sizeof(val));
         memcpy(&a, &val, sizeof(val));
         a[op.laneidx] = read_unaligned<std::decay_t<decltype(a[0])>>(_ptr);
         v128_t res;
         std::memcpy(&res, &a, sizeof(res));
         context.push_operand(v128_const_t{ res });
      }
      [[gnu::always_inline]] inline void operator()(const v128_load8_lane_t& op) {
         v128_load_lane<std::uint8_t[16]>(op);
      }
      [[gnu::always_inline]] inline void operator()(const v128_load16_lane_t& op) {
         v128_load_lane<std::uint16_t[8]>(op);
      }
      [[gnu::always_inline]] inline void operator()(const v128_load32_lane_t& op) {
         v128_load_lane<std::uint32_t[4]>(op);
      }
      [[gnu::always_inline]] inline void operator()(const v128_load64_lane_t& op) {
         v128_load_lane<std::uint64_t[2]>(op);
      }
      template<typename T, typename Op>
      [[gnu::always_inline]] inline void v128_store_lane(const Op& op) {
         context.inc_pc();
         v128_t val = context.pop_operand().to_v128();
         void* _ptr = pop_memop_addr(op);
         T a;
         static_assert(sizeof(T) == sizeof(val));
         std::memcpy(&a, &val, sizeof(val));
         write_unaligned(_ptr, a[op.laneidx]);
      }
      [[gnu::always_inline]] inline void operator()(const v128_store8_lane_t& op) {
         v128_store_lane<std::uint8_t[16]>(op);
      }
      [[gnu::always_inline]] inline void operator()(const v128_store16_lane_t& op) {
         v128_store_lane<std::uint16_t[8]>(op);
      }
      [[gnu::always_inline]] inline void operator()(const v128_store32_lane_t& op) {
         v128_store_lane<std::uint32_t[4]>(op);
      }
      [[gnu::always_inline]] inline void operator()(const v128_store64_lane_t& op) {
         v128_store_lane<std::uint64_t[2]>(op);
      }
      [[gnu::always_inline]] inline void operator()(const v128_const_t& op) {
         context.inc_pc();
         context.push_operand(op);
      }
      [[gnu::always_inline]] inline void operator()(const i8x16_shuffle_t& op) {
         context.inc_pc();
         v128_t c2 = context.pop_operand().to_v128();
         v128_t c1 = context.pop_operand().to_v128();
         uint8_t a[32];
         std::memcpy(a + 16, &c2, sizeof(c2));
         std::memcpy(a, &c1, sizeof(c1));
         uint8_t r[16];
         for(int i = 0; i < 16; ++i) {
            r[i] = a[op.lanes[i]];
         }
         v128_t res;
         std::memcpy(&res, &r, sizeof(res));
         context.push_operand(v128_const_t{ res });
      }
      template<typename A, typename T, typename R, typename Op>
      [[gnu::always_inline]] inline void v128_extract_lane(const Op& op) {
         context.inc_pc();
         v128_t c = context.pop_operand().to_v128();
         A a;
         static_assert(sizeof(a) == sizeof(c));
         std::memcpy(&a, &c, sizeof(c));
         context.push_operand(R{ static_cast<T>(a[op.laneidx]) });
      }
      template<typename A, typename T, typename Op>
      [[gnu::always_inline]] inline void v128_replace_lane(const Op& op, T c1) {
         context.inc_pc();
         v128_t c2 = context.pop_operand().to_v128();
         A a;
         static_assert(sizeof(a) == sizeof(c2));
         std::memcpy(&a, &c2, sizeof(c2));
         a[op.laneidx] = c1;
         v128_t res;
         std::memcpy(&res, &a, sizeof(res));
         context.push_operand(v128_const_t{ res });
      }
      [[gnu::always_inline]] inline void operator()(const i8x16_extract_lane_s_t& op) {
         v128_extract_lane<std::int8_t[16], std::int32_t, i32_const_t>(op);
      }
      [[gnu::always_inline]] inline void operator()(const i8x16_extract_lane_u_t& op) {
         v128_extract_lane<std::uint8_t[16], std::uint32_t, i32_const_t>(op);
      }
      [[gnu::always_inline]] inline void operator()(const i8x16_replace_lane_t& op) {
         v128_replace_lane<std::uint8_t[16]>(op, context.pop_operand().to_ui32());
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_extract_lane_s_t& op) {
         v128_extract_lane<std::int16_t[8], std::int32_t, i32_const_t>(op);
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_extract_lane_u_t& op) {
         v128_extract_lane<std::uint16_t[8], std::uint32_t, i32_const_t>(op);
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_replace_lane_t& op) {
         v128_replace_lane<std::uint16_t[8]>(op, context.pop_operand().to_ui32());
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_extract_lane_t& op) {
         v128_extract_lane<std::uint32_t[4], std::uint32_t, i32_const_t>(op);
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_replace_lane_t& op) {
         v128_replace_lane<std::uint32_t[4]>(op, context.pop_operand().to_ui32());
      }
      [[gnu::always_inline]] inline void operator()(const i64x2_extract_lane_t& op) {
         v128_extract_lane<std::uint64_t[2], std::uint64_t, i64_const_t>(op);
      }
      [[gnu::always_inline]] inline void operator()(const i64x2_replace_lane_t& op) {
         v128_replace_lane<std::uint64_t[2]>(op, context.pop_operand().to_ui64());
      }
      [[gnu::always_inline]] inline void operator()(const f32x4_extract_lane_t& op) {
         v128_extract_lane<float[4], float, f32_const_t>(op);
      }
      [[gnu::always_inline]] inline void operator()(const f32x4_replace_lane_t& op) {
         v128_replace_lane<float[4]>(op, context.pop_operand().to_f32());
      }
      [[gnu::always_inline]] inline void operator()(const f64x2_extract_lane_t& op) {
         v128_extract_lane<double[2], double, f64_const_t>(op);
      }
      [[gnu::always_inline]] inline void operator()(const f64x2_replace_lane_t& op) {
         v128_replace_lane<double[2]>(op, context.pop_operand().to_f64());
      }
      [[gnu::always_inline]] inline void operator()(const i8x16_swizzle_t& op) {
         context.inc_pc();
         v128_t c2 = context.pop_operand().to_v128();
         std::uint8_t i2[16];
         static_assert(sizeof(i2) == sizeof(c2));
         std::memcpy(&i2, &c2, sizeof(c2));
         v128_t c1 = context.pop_operand().to_v128();
         std::uint8_t j[16];
         static_assert(sizeof(j) == sizeof(c1));
         std::memcpy(&j, &c1, sizeof(c1));
         std::uint8_t r[16];
         for(int i = 0; i < 16; ++i) {
            r[i] = i2[i] < 16 ? j[i2[i]] : 0;
         }
         v128_t res;
         std::memcpy(&res, &r, sizeof(res));
         context.push_operand(v128_const_t{ res });
      }
      template<typename A, typename T>
      [[gnu::always_inline]] inline void v128_splat(const T& t) {
         context.inc_pc();
         A r;
         for(int i = 0; i < sizeof(r)/sizeof(r[0]); ++i) {
            r[i] = t;
         }
         v128_t res;
         static_assert(sizeof(r) == sizeof(res));
         std::memcpy(&res, &r, sizeof(res));
         context.push_operand(v128_const_t{ res });
      }
      [[gnu::always_inline]] inline void operator()(const i8x16_splat_t& op) {
         v128_splat<std::uint8_t[16]>(context.pop_operand().to_ui32());
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_splat_t& op) {
         v128_splat<std::uint16_t[8]>(context.pop_operand().to_ui32());
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_splat_t& op) {
         v128_splat<std::uint32_t[4]>(context.pop_operand().to_ui32());
      }
      [[gnu::always_inline]] inline void operator()(const i64x2_splat_t& op) {
         v128_splat<std::uint64_t[2]>(context.pop_operand().to_ui64());
      }
      [[gnu::always_inline]] inline void operator()(const f32x4_splat_t& op) {
         v128_splat<float[4]>(context.pop_operand().to_f32());
      }
      [[gnu::always_inline]] inline void operator()(const f64x2_splat_t& op) {
         v128_splat<double[2]>(context.pop_operand().to_f64());
      }
      template<typename F>
      [[gnu::always_inline]] inline void v128_unop(F&& f) {
         context.inc_pc();
         v128_t c1 = context.pop_operand().to_v128();
         context.push_operand(v128_const_t{ f(c1) });
      }
      template<typename F>
      [[gnu::always_inline]] inline void v128_binop(F&& f) {
         context.inc_pc();
         v128_t c2 = context.pop_operand().to_v128();
         v128_t c1 = context.pop_operand().to_v128();
         context.push_operand(v128_const_t{ f(c1, c2) });
      }
      template<typename A, typename R, typename F>
      [[gnu::always_inline]] inline void v128_unop_array(F&& f) {
         context.inc_pc();
         static_assert(sizeof(A) == sizeof(v128_t));
         static_assert(sizeof(R) == sizeof(v128_t));
         v128_t c1 = context.pop_operand().to_v128();
         A i1;
         std::memcpy(&i1, &c1, sizeof(c1));
         R r;
         f(i1, r);
         v128_t res;
         std::memcpy(&res, &r, sizeof(res));
         context.push_operand(v128_const_t{ res });
      }
      template<typename A, typename R, typename F>
      [[gnu::always_inline]] inline void v128_binop_array(F&& f) {
         context.inc_pc();
         static_assert(sizeof(A) == sizeof(v128_t));
         static_assert(sizeof(R) == sizeof(v128_t));
         v128_t c2 = context.pop_operand().to_v128();
         A i2;
         std::memcpy(&i2, &c2, sizeof(c2));
         v128_t c1 = context.pop_operand().to_v128();
         A i1;
         std::memcpy(&i1, &c1, sizeof(c1));
         R r;
         f(i1, i2, r);
         v128_t res;
         std::memcpy(&res, &r, sizeof(res));
         context.push_operand(v128_const_t{ res });
      }
      template<typename A, typename F>
      [[gnu::always_inline]] inline void v128_unop(F&& f) {
         return v128_unop_array<A, A>([f](A i1, A r){
            for(int i = 0; i < sizeof(A)/sizeof(i1[0]); ++i) {
               r[i] = f(i1[i]);
            }
         });
      }
      template<typename A, typename F>
      [[gnu::always_inline]] inline void v128_binop(F&& f) {
         return v128_binop_array<A, A>([f](A i1, A i2, A r){
            for(int i = 0; i < sizeof(A)/sizeof(i1[0]); ++i) {
               r[i] = f(i1[i], i2[i]);
            }
         });
      }
      template<typename A, typename F>
      [[gnu::always_inline]] inline void v128_ternop(F&& f) {
         context.inc_pc();
         v128_t c3 = context.pop_operand().to_v128();
         A i3;
         static_assert(sizeof(i3) == sizeof(c3));
         std::memcpy(&i3, &c3, sizeof(c3));
         v128_t c2 = context.pop_operand().to_v128();
         A i2;
         static_assert(sizeof(i2) == sizeof(c2));
         std::memcpy(&i2, &c2, sizeof(c2));
         v128_t c1 = context.pop_operand().to_v128();
         A i1;
         static_assert(sizeof(i1) == sizeof(c1));
         std::memcpy(&i1, &c1, sizeof(c1));
         A r;
         for(int i = 0; i < sizeof(i1)/sizeof(i1[0]); ++i) {
            r[i] = f(i1[i], i2[i], i3[i]);
         }
         v128_t res;
         std::memcpy(&res, &r, sizeof(res));
         context.push_operand(v128_const_t{ res });
      }
      template<typename A, typename F>
      [[gnu::always_inline]] inline void v128_unop_i32(F&& f) {
         context.inc_pc();
         v128_t c1 = context.pop_operand().to_v128();
         A i1;
         static_assert(sizeof(i1) == sizeof(c1));
         std::memcpy(&i1, &c1, sizeof(c1));
         context.push_operand(i32_const_t{ f(i1) });
      }
      template<typename A, typename R, typename F>
      [[gnu::always_inline]] inline void v128_narrow(F&& f) {
         v128_binop_array<A, R>([f](A a, A b, R r){
            static_assert(2*sizeof(A)/sizeof(a[0]) == sizeof(R)/sizeof(r[0]));
            for(int i = 0; i < sizeof(A)/sizeof(a[0]); ++i) {
               r[i] = f(a[i]);
            }
            for(int i = 0; i < sizeof(A)/sizeof(b[0]); ++i) {
               r[i+sizeof(R)/sizeof(r[0])/2] = f(b[i]);
            }
         });
      }
      template<typename A, typename R>
      [[gnu::always_inline]] inline void v128_extend(bool high) {
         v128_unop_array<A, R>([high](A a, R r){
            static_assert(sizeof(A)/sizeof(a[0]) == 2*sizeof(R)/sizeof(r[0]));
            for(int i = 0; i < sizeof(R)/sizeof(r[0]); ++i) {
               r[i] = a[i + high * sizeof(R)/sizeof(r[0])];
            }
         });
      }
      template<typename A, typename R>
      [[gnu::always_inline]] inline void v128_extadd_pairwise() {
         v128_unop_array<A, R>([](A a, R r){
            static_assert(sizeof(A)/sizeof(a[0]) == 2*sizeof(R)/sizeof(r[0]));
            for(int i = 0; i < sizeof(R)/sizeof(r[0]); ++i) {
               r[i] = static_cast<std::remove_extent_t<R>>(a[2*i]) + a[2*i+1];
            }
         });
      }
      template<typename A, typename R>
      [[gnu::always_inline]] inline void v128_extmul(bool high) {
         v128_binop_array<A, R>([high](A a, A b, R r){
            static_assert(sizeof(A)/sizeof(a[0]) == 2*sizeof(R)/sizeof(r[0]));
            if(high) {
               a += sizeof(R)/sizeof(r[0]);
            }
            for(int i = 0; i < sizeof(R)/sizeof(r[0]); ++i) {
               r[i] = static_cast<std::remove_extent_t<R>>(a[i]) * b[i];
            }
         });
      }

      [[gnu::always_inline]] inline void operator()(const i8x16_eq_t& op) {
         v128_binop<std::uint8_t[16]>([](std::uint8_t a, std::uint8_t b){ return a == b? -1 : 0; });
      }
      [[gnu::always_inline]] inline void operator()(const i8x16_ne_t& op) {
         v128_binop<std::uint8_t[16]>([](std::uint8_t a, std::uint8_t b){ return a != b? -1 : 0; });
      }
      [[gnu::always_inline]] inline void operator()(const i8x16_lt_s_t& op) {
         v128_binop<std::int8_t[16]>([](std::int8_t a, std::int8_t b){ return a < b? -1 : 0; });
      }
      [[gnu::always_inline]] inline void operator()(const i8x16_lt_u_t& op) {
         v128_binop<std::uint8_t[16]>([](std::uint8_t a, std::uint8_t b){ return a < b? -1 : 0; });
      }
      [[gnu::always_inline]] inline void operator()(const i8x16_gt_s_t& op) {
         v128_binop<std::int8_t[16]>([](std::int8_t a, std::int8_t b){ return a > b? -1 : 0; });
      }
      [[gnu::always_inline]] inline void operator()(const i8x16_gt_u_t& op) {
         v128_binop<std::uint8_t[16]>([](std::uint8_t a, std::uint8_t b){ return a > b? -1 : 0; });
      }
      [[gnu::always_inline]] inline void operator()(const i8x16_le_s_t& op) {
         v128_binop<std::int8_t[16]>([](std::int8_t a, std::int8_t b){ return a <= b? -1 : 0; });
      }
      [[gnu::always_inline]] inline void operator()(const i8x16_le_u_t& op) {
         v128_binop<std::uint8_t[16]>([](std::uint8_t a, std::uint8_t b){ return a <= b? -1 : 0; });
      }
      [[gnu::always_inline]] inline void operator()(const i8x16_ge_s_t& op) {
         v128_binop<std::int8_t[16]>([](std::int8_t a, std::int8_t b){ return a >= b? -1 : 0; });
      }
      [[gnu::always_inline]] inline void operator()(const i8x16_ge_u_t& op) {
         v128_binop<std::uint8_t[16]>([](std::uint8_t a, std::uint8_t b){ return a >= b? -1 : 0; });
      }

      [[gnu::always_inline]] inline void operator()(const i16x8_eq_t& op) {
         v128_binop<std::uint16_t[8]>([](std::uint16_t a, std::uint16_t b){ return a == b? -1 : 0; });
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_ne_t& op) {
         v128_binop<std::uint16_t[8]>([](std::uint16_t a, std::uint16_t b){ return a != b? -1 : 0; });
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_lt_s_t& op) {
         v128_binop<std::int16_t[8]>([](std::int16_t a, std::int16_t b){ return a < b? -1 : 0; });
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_lt_u_t& op) {
         v128_binop<std::uint16_t[8]>([](std::uint16_t a, std::uint16_t b){ return a < b? -1 : 0; });
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_gt_s_t& op) {
         v128_binop<std::int16_t[8]>([](std::int16_t a, std::int16_t b){ return a > b? -1 : 0; });
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_gt_u_t& op) {
         v128_binop<std::uint16_t[8]>([](std::uint16_t a, std::uint16_t b){ return a > b? -1 : 0; });
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_le_s_t& op) {
         v128_binop<std::int16_t[8]>([](std::int16_t a, std::int16_t b){ return a <= b? -1 : 0; });
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_le_u_t& op) {
         v128_binop<std::uint16_t[8]>([](std::uint16_t a, std::uint16_t b){ return a <= b? -1 : 0; });
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_ge_s_t& op) {
         v128_binop<std::int16_t[8]>([](std::int16_t a, std::int16_t b){ return a >= b? -1 : 0; });
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_ge_u_t& op) {
         v128_binop<std::uint16_t[8]>([](std::uint16_t a, std::uint16_t b){ return a >= b? -1 : 0; });
      }

      [[gnu::always_inline]] inline void operator()(const i32x4_eq_t& op) {
         v128_binop<std::uint32_t[4]>([](std::uint32_t a, std::uint32_t b){ return a == b? -1 : 0; });
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_ne_t& op) {
         v128_binop<std::uint32_t[4]>([](std::uint32_t a, std::uint32_t b){ return a != b? -1 : 0; });
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_lt_s_t& op) {
         v128_binop<std::int32_t[4]>([](std::int32_t a, std::int32_t b){ return a < b? -1 : 0; });
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_lt_u_t& op) {
         v128_binop<std::uint32_t[4]>([](std::uint32_t a, std::uint32_t b){ return a < b? -1 : 0; });
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_gt_s_t& op) {
         v128_binop<std::int32_t[4]>([](std::int32_t a, std::int32_t b){ return a > b? -1 : 0; });
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_gt_u_t& op) {
         v128_binop<std::uint32_t[4]>([](std::uint32_t a, std::uint32_t b){ return a > b? -1 : 0; });
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_le_s_t& op) {
         v128_binop<std::int32_t[4]>([](std::int32_t a, std::int32_t b){ return a <= b? -1 : 0; });
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_le_u_t& op) {
         v128_binop<std::uint32_t[4]>([](std::uint32_t a, std::uint32_t b){ return a <= b? -1 : 0; });
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_ge_s_t& op) {
         v128_binop<std::int32_t[4]>([](std::int32_t a, std::int32_t b){ return a >= b? -1 : 0; });
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_ge_u_t& op) {
         v128_binop<std::uint32_t[4]>([](std::uint32_t a, std::uint32_t b){ return a >= b? -1 : 0; });
      }

      [[gnu::always_inline]] inline void operator()(const i64x2_eq_t& op) {
         v128_binop<std::uint64_t[2]>([](std::uint64_t a, std::uint64_t b){ return a == b? -1 : 0; });
      }
      [[gnu::always_inline]] inline void operator()(const i64x2_ne_t& op) {
         v128_binop<std::uint64_t[2]>([](std::uint64_t a, std::uint64_t b){ return a != b? -1 : 0; });
      }
      [[gnu::always_inline]] inline void operator()(const i64x2_lt_s_t& op) {
         v128_binop<std::int64_t[2]>([](std::int64_t a, std::int64_t b){ return a < b? -1 : 0; });
      }
      [[gnu::always_inline]] inline void operator()(const i64x2_gt_s_t& op) {
         v128_binop<std::int64_t[2]>([](std::int64_t a, std::int64_t b){ return a > b? -1 : 0; });
      }
      [[gnu::always_inline]] inline void operator()(const i64x2_le_s_t& op) {
         v128_binop<std::int64_t[2]>([](std::int64_t a, std::int64_t b){ return a <= b? -1 : 0; });
      }
      [[gnu::always_inline]] inline void operator()(const i64x2_ge_s_t& op) {
         v128_binop<std::int64_t[2]>([](std::int64_t a, std::int64_t b){ return a >= b? -1 : 0; });
      }

      [[gnu::always_inline]] inline void operator()(const f32x4_eq_t& op) {
         v128_binop(&_eosio_f32x4_eq);
      }
      [[gnu::always_inline]] inline void operator()(const f32x4_ne_t& op) {
         v128_binop(&_eosio_f32x4_ne);
      }
      [[gnu::always_inline]] inline void operator()(const f32x4_lt_t& op) {
         v128_binop(&_eosio_f32x4_lt);
      }
      [[gnu::always_inline]] inline void operator()(const f32x4_gt_t& op) {
         v128_binop(&_eosio_f32x4_gt);
      }
      [[gnu::always_inline]] inline void operator()(const f32x4_le_t& op) {
         v128_binop(&_eosio_f32x4_le);
      }
      [[gnu::always_inline]] inline void operator()(const f32x4_ge_t& op) {
         v128_binop(&_eosio_f32x4_ge);
      }

      [[gnu::always_inline]] inline void operator()(const f64x2_eq_t& op) {
         v128_binop(&_eosio_f64x2_eq);
      }
      [[gnu::always_inline]] inline void operator()(const f64x2_ne_t& op) {
         v128_binop(&_eosio_f64x2_ne);
      }
      [[gnu::always_inline]] inline void operator()(const f64x2_lt_t& op) {
         v128_binop(&_eosio_f64x2_lt);
      }
      [[gnu::always_inline]] inline void operator()(const f64x2_gt_t&) {
         v128_binop(&_eosio_f64x2_gt);
      }
      [[gnu::always_inline]] inline void operator()(const f64x2_le_t&) {
         v128_binop(&_eosio_f64x2_le);
      }
      [[gnu::always_inline]] inline void operator()(const f64x2_ge_t&) {
         v128_binop(&_eosio_f64x2_ge);
      }

      [[gnu::always_inline]] inline void operator()(const v128_not_t&) {
         v128_unop<std::uint64_t[2]>([](std::uint64_t a){ return ~a; });
      }
      [[gnu::always_inline]] inline void operator()(const v128_and_t&) {
         v128_binop<std::uint64_t[2]>([](std::uint64_t a, std::uint64_t b){ return a & b; });
      }
      [[gnu::always_inline]] inline void operator()(const v128_andnot_t&) {
         v128_binop<std::uint64_t[2]>([](std::uint64_t a, std::uint64_t b){ return a & ~b; });
      }
      [[gnu::always_inline]] inline void operator()(const v128_or_t&) {
         v128_binop<std::uint64_t[2]>([](std::uint64_t a, std::uint64_t b){ return a | b; });
      }
      [[gnu::always_inline]] inline void operator()(const v128_xor_t&) {
         v128_binop<std::uint64_t[2]>([](std::uint64_t a, std::uint64_t b){ return a ^ b; });
      }
      [[gnu::always_inline]] inline void operator()(const v128_bitselect_t&) {
         v128_ternop<std::uint64_t[2]>([](std::uint64_t a, std::uint64_t b, std::uint64_t c){
            return (a & c) | (b & ~c);
         });
      }
      [[gnu::always_inline]] inline void operator()(const v128_any_true_t&) {
         v128_unop_i32<std::uint64_t[2]>([](std::uint64_t a[2]){
            uint32_t result = 0;
            for(int i = 0; i < 2; ++i) {
               if(a[i] != 0) { result = 1; }
            }
            return result;
         });
      }

      [[gnu::always_inline]] inline void operator()(const i8x16_abs_t&) {
         v128_unop<std::uint8_t[16]>([](std::int8_t a){ return a < 0? -a : a; });
      }
      [[gnu::always_inline]] inline void operator()(const i8x16_neg_t&) {
         v128_unop<std::uint8_t[16]>([](std::uint8_t a){ return -a; });
      }
      [[gnu::always_inline]] inline void operator()(const i8x16_popcnt_t&) {
         v128_unop<std::uint8_t[16]>([](std::uint8_t a){ return __builtin_popcount(a); });
      }
      [[gnu::always_inline]] inline void operator()(const i8x16_all_true_t&) {
         v128_unop_i32<std::uint8_t[16]>([](std::uint8_t a[16]){
            uint32_t result = 1;
            for(int i = 0; i < 16; ++i) {
               if(a[i] == 0) { result = 0; }
            }
            return result;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i8x16_bitmask_t&) {
         v128_unop_i32<std::int8_t[16]>([](std::int8_t a[16]){
            uint32_t result = 0;
            for(int i = 0; i < 16; ++i) {
               result |= (a[i] < 0) << i;
            }
            return result;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i8x16_narrow_i16x8_s_t&) {
         v128_narrow<std::int16_t[8], std::int8_t[16]>([](std::int16_t a) -> std::int8_t {
            if(a < -0x80) return -0x80;
            else if(a > 0x7f) return 0x7f;
            else return a;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i8x16_narrow_i16x8_u_t&) {
         v128_narrow<std::int16_t[8], std::uint8_t[16]>([](std::int16_t a) -> std::uint8_t {
            if(a < 0) return 0;
            else if(a > 0xff) return 0xff;
            else return a;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i8x16_shl_t&) {
         uint32_t b = context.pop_operand().to_ui32() & 0x7;
         v128_unop<std::uint8_t[16]>([b](std::uint8_t a) {
            return a << b;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i8x16_shr_s_t&) {
         uint32_t b = context.pop_operand().to_ui32() & 0x7;
         v128_unop<std::uint8_t[16]>([b](std::int8_t a) {
            return a >> b;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i8x16_shr_u_t&) {
         uint32_t b = context.pop_operand().to_ui32() & 0x7;
         v128_unop<std::uint8_t[16]>([b](std::uint8_t a) {
            return a >> b;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i8x16_add_t&) {
         v128_binop<std::uint8_t[16]>([](std::uint8_t a, std::uint8_t b) {
            return a + b;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i8x16_add_sat_s_t&) {
         v128_binop<std::int8_t[16]>([](std::int8_t a, std::int8_t b) {
            auto r = a + b;
            if(r < -0x80) return -0x80;
            else if(r > 0x7f) return 0x7f;
            else return r;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i8x16_add_sat_u_t&) {
         v128_binop<std::uint8_t[16]>([](std::uint8_t a, std::uint8_t b) {
            auto r = a + b;
            if(r > 0xff) return 0xff;
            else return r;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i8x16_sub_t&) {
         v128_binop<std::uint8_t[16]>([](std::uint8_t a, std::uint8_t b) {
            return a - b;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i8x16_sub_sat_s_t&) {
         v128_binop<std::int8_t[16]>([](std::int8_t a, std::int8_t b) {
            auto r = a - b;
            if(r < -0x80) return -0x80;
            else if(r > 0x7f) return 0x7f;
            else return r;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i8x16_sub_sat_u_t&) {
         v128_binop<std::uint8_t[16]>([](std::uint8_t a, std::uint8_t b) {
            auto r = a - b;
            if(r < 0) return 0;
            else if(r > 0xff) return 0xff;
            else return r;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i8x16_min_s_t&) {
         v128_binop<std::int8_t[16]>([](std::int8_t a, std::int8_t b) {
            return a < b? a : b;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i8x16_min_u_t&) {
         v128_binop<std::uint8_t[16]>([](std::uint8_t a, std::uint8_t b) {
            return a < b? a : b;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i8x16_max_s_t&) {
         v128_binop<std::int8_t[16]>([](std::int8_t a, std::int8_t b) {
            return a < b? b : a;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i8x16_max_u_t&) {
         v128_binop<std::uint8_t[16]>([](std::uint8_t a, std::uint8_t b) {
            return a < b? b : a;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i8x16_avgr_u_t&) {
         v128_binop<std::uint8_t[16]>([](std::uint8_t a, std::uint8_t b) {
            return (a + b + 1)/2;
         });
      }

      [[gnu::always_inline]] inline void operator()(const i16x8_extadd_pairwise_i8x16_s_t&) {
         v128_extadd_pairwise<std::int8_t[16], std::int16_t[8]>();
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_extadd_pairwise_i8x16_u_t&) {
         v128_extadd_pairwise<std::uint8_t[16], std::uint16_t[8]>();
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_abs_t&) {
         v128_unop<std::uint16_t[8]>([](std::int16_t a) {
            return a < 0 ? -a : a;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_neg_t&) {
         v128_unop<std::uint16_t[8]>([](std::uint16_t a) {
            return -a;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_q15mulr_sat_s_t&) {
         v128_binop<std::uint16_t[8]>([](std::int16_t a, std::int16_t b) {
            uint16_t tmp = (static_cast<std::int32_t>(a) * b + 0x4000) >> 15;
            if(tmp == 0x8000) tmp = 0x7fff;
            return tmp;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_all_true_t&) {
         v128_unop_i32<std::uint16_t[8]>([](std::uint16_t a[8]){
            uint32_t result = 1;
            for(int i = 0; i < 8; ++i) {
               if(a[i] == 0) { result = 0; }
            }
            return result;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_bitmask_t&) {
         v128_unop_i32<std::int16_t[8]>([](std::int16_t a[8]){
            uint32_t result = 0;
            for(int i = 0; i < 8; ++i) {
               result |= (a[i] < 0) << i;
            }
            return result;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_narrow_i32x4_s_t&) {
         v128_narrow<std::int32_t[4], std::int16_t[8]>([](std::int32_t a) -> std::int16_t {
            if(a < -0x8000) return -0x8000;
            else if(a > 0x7fff) return 0x7fff;
            else return a;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_narrow_i32x4_u_t&) {
         v128_narrow<std::uint32_t[4], std::uint16_t[8]>([](std::int32_t a) -> std::uint16_t {
            if(a < 0) return 0;
            else if(a > 0xffff) return 0xffff;
            else return a;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_extend_low_i8x16_s_t&) {
         v128_extend<std::int8_t[16], std::int16_t[8]>(false);
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_extend_high_i8x16_s_t&) {
         v128_extend<std::int8_t[16], std::int16_t[8]>(true);
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_extend_low_i8x16_u_t&) {
         v128_extend<std::uint8_t[16], std::uint16_t[8]>(false);
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_extend_high_i8x16_u_t&) {
         v128_extend<std::uint8_t[16], std::uint16_t[8]>(true);
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_shl_t&) {
         uint32_t b = context.pop_operand().to_ui32() & 0xf;
         v128_unop<std::uint16_t[8]>([b](std::uint16_t a) {
            return a << b;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_shr_s_t&) {
         uint32_t b = context.pop_operand().to_ui32() & 0xf;
         v128_unop<std::uint16_t[8]>([b](std::int16_t a) {
            return a >> b;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_shr_u_t&) {
         uint32_t b = context.pop_operand().to_ui32() & 0xf;
         v128_unop<std::uint16_t[8]>([b](std::uint16_t a) {
            return a >> b;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_add_t&) {
         v128_binop<std::uint16_t[8]>([](std::uint16_t a, std::uint16_t b) {
            return a + b;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_add_sat_s_t&) {
         v128_binop<std::int16_t[8]>([](std::int16_t a, std::int16_t b) {
            auto r = a + b;
            if(r < -0x8000) return -0x8000;
            else if(r > 0x7fff) return 0x7fff;
            else return r;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_add_sat_u_t&) {
         v128_binop<std::uint16_t[8]>([](std::uint16_t a, std::uint16_t b) {
            auto r = a + b;
            if(r > 0xffff) return 0xffff;
            else return r;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_sub_t&) {
         v128_binop<std::uint16_t[8]>([](std::uint16_t a, std::uint16_t b) {
            return a - b;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_sub_sat_s_t&) {
         v128_binop<std::int16_t[8]>([](std::int16_t a, std::int16_t b) {
            auto r = static_cast<int32_t>(a) - static_cast<int32_t>(b);
            if(r < -0x8000) return -0x8000;
            else if(r > 0x7fff) return 0x7fff;
            else return r;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_sub_sat_u_t&) {
         v128_binop<std::uint16_t[8]>([](std::uint16_t a, std::uint16_t b) {
            auto r = static_cast<int32_t>(a) - static_cast<int32_t>(b);
            if(r < 0) return 0;
            if(r > 0xffff) return 0xffff;
            else return r;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_mul_t&) {
         v128_binop<std::uint16_t[8]>([](std::uint16_t a, std::uint16_t b) {
            return a * b;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_min_s_t&) {
         v128_binop<std::int16_t[8]>([](std::int16_t a, std::int16_t b) {
            return a < b? a : b;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_min_u_t&) {
         v128_binop<std::uint16_t[8]>([](std::uint16_t a, std::uint16_t b) {
            return a < b? a : b;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_max_s_t&) {
         v128_binop<std::int16_t[8]>([](std::int16_t a, std::int16_t b) {
            return a < b? b : a;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_max_u_t&) {
         v128_binop<std::uint16_t[8]>([](std::uint16_t a, std::uint16_t b) {
            return a < b? b : a;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_avgr_u_t&) {
         v128_binop<std::uint16_t[8]>([](std::uint16_t a, std::uint16_t b) {
            return (a + b + 1)/2;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_extmul_low_i8x16_s_t&) {
         v128_extmul<std::int8_t[16], std::int16_t[8]>(false);
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_extmul_high_i8x16_s_t&) {
         v128_extmul<std::int8_t[16], std::int16_t[8]>(true);
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_extmul_low_i8x16_u_t&) {
         v128_extmul<std::uint8_t[16], std::uint16_t[8]>(false);
      }
      [[gnu::always_inline]] inline void operator()(const i16x8_extmul_high_i8x16_u_t&) {
         v128_extmul<std::uint8_t[16], std::uint16_t[8]>(true);
      }

      [[gnu::always_inline]] inline void operator()(const i32x4_extadd_pairwise_i16x8_s_t&) {
         v128_extadd_pairwise<std::int16_t[8], std::int32_t[4]>();
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_extadd_pairwise_i16x8_u_t&) {
         v128_extadd_pairwise<std::uint16_t[8], std::uint32_t[4]>();
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_abs_t&) {
         v128_unop<std::uint32_t[4]>([](std::int32_t a) {
            return a < 0 ? -a : a;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_neg_t&) {
         v128_unop<std::uint32_t[4]>([](std::uint32_t a) {
            return -a;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_all_true_t&) {
         v128_unop_i32<std::uint32_t[4]>([](std::uint32_t a[4]){
            uint32_t result = 1;
            for(int i = 0; i < 4; ++i) {
               if(a[i] == 0) { result = 0; }
            }
            return result;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_bitmask_t&) {
         v128_unop_i32<std::int32_t[4]>([](std::int32_t a[4]){
            uint32_t result = 0;
            for(int i = 0; i < 4; ++i) {
               result |= (a[i] < 0) << i;
            }
            return result;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_extend_low_i16x8_s_t&) {
         v128_extend<std::int16_t[8], std::int32_t[4]>(false);
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_extend_high_i16x8_s_t&) {
         v128_extend<std::int16_t[8], std::int32_t[4]>(true);
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_extend_low_i16x8_u_t&) {
         v128_extend<std::uint16_t[8], std::uint32_t[4]>(false);
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_extend_high_i16x8_u_t&) {
         v128_extend<std::uint16_t[8], std::uint32_t[4]>(true);
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_shl_t&) {
         uint32_t b = context.pop_operand().to_ui32() & 0x1f;
         v128_unop<std::uint32_t[4]>([b](std::uint32_t a) {
            return a << b;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_shr_s_t&) {
         uint32_t b = context.pop_operand().to_ui32() & 0x1f;
         v128_unop<std::uint32_t[4]>([b](std::int32_t a) {
            return a >> b;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_shr_u_t&) {
         uint32_t b = context.pop_operand().to_ui32() & 0x1f;
         v128_unop<std::uint32_t[4]>([b](std::uint32_t a) {
            return a >> b;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_add_t&) {
         v128_binop<std::uint32_t[4]>([](std::uint32_t a, std::uint32_t b) {
            return a + b;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_sub_t&) {
         v128_binop<std::uint32_t[4]>([](std::uint32_t a, std::uint32_t b) {
            return a - b;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_mul_t&) {
         v128_binop<std::uint32_t[4]>([](std::uint32_t a, std::uint32_t b) {
            return a * b;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_min_s_t&) {
         v128_binop<std::int32_t[4]>([](std::int32_t a, std::int32_t b) {
            return a < b? a : b;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_min_u_t&) {
         v128_binop<std::uint32_t[4]>([](std::uint32_t a, std::uint32_t b) {
            return a < b? a : b;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_max_s_t&) {
         v128_binop<std::int32_t[4]>([](std::int32_t a, std::int32_t b) {
            return a < b? b : a;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_max_u_t&) {
         v128_binop<std::uint32_t[4]>([](std::uint32_t a, std::uint32_t b) {
            return a < b? b : a;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_dot_i16x8_s_t&) {
         v128_binop_array<std::int16_t[8], std::int32_t[4]>([](std::int16_t a[8], std::int16_t b[8], std::int32_t r[4]) {
            for(int i = 0; i < 4; ++i) {
               auto p1 = static_cast<int32_t>(a[2*i]) * b[2*i];
               auto p2 = static_cast<int32_t>(a[2*i+1]) * b[2*i+1];
               r[i] = p1 + p2;
            }
         });
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_extmul_low_i16x8_s_t&) {
         v128_extmul<std::int16_t[8], std::int32_t[4]>(false);
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_extmul_high_i16x8_s_t&) {
         v128_extmul<std::int16_t[8], std::int32_t[4]>(true);
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_extmul_low_i16x8_u_t&) {
         v128_extmul<std::uint16_t[8], std::uint32_t[4]>(false);
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_extmul_high_i16x8_u_t&) {
         v128_extmul<std::uint16_t[8], std::uint32_t[4]>(true);
      }
      
      [[gnu::always_inline]] inline void operator()(const i64x2_abs_t&) {
         v128_unop<std::uint64_t[2]>([](std::int64_t a) {
            return a < 0 ? -a : a;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i64x2_neg_t&) {
         v128_unop<std::uint64_t[2]>([](std::uint64_t a) {
            return -a;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i64x2_all_true_t&) {
         v128_unop_i32<std::uint64_t[2]>([](std::uint64_t a[2]){
            uint32_t result = 1;
            for(int i = 0; i < 2; ++i) {
               if(a[i] == 0) { result = 0; }
            }
            return result;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i64x2_bitmask_t&) {
         v128_unop_i32<std::int64_t[2]>([](std::int64_t a[2]){
            uint32_t result = 0;
            for(int i = 0; i < 2; ++i) {
               result |= (a[i] < 0) << i;
            }
            return result;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i64x2_extend_low_i32x4_s_t&) {
         v128_extend<std::int32_t[4], std::int64_t[2]>(false);
      }
      [[gnu::always_inline]] inline void operator()(const i64x2_extend_high_i32x4_s_t&) {
         v128_extend<std::int32_t[4], std::int64_t[2]>(true);
      }
      [[gnu::always_inline]] inline void operator()(const i64x2_extend_low_i32x4_u_t&) {
         v128_extend<std::uint32_t[4], std::uint64_t[2]>(false);
      }
      [[gnu::always_inline]] inline void operator()(const i64x2_extend_high_i32x4_u_t&) {
         v128_extend<std::uint32_t[4], std::uint64_t[2]>(true);
      }
      [[gnu::always_inline]] inline void operator()(const i64x2_shl_t&) {
         uint32_t b = context.pop_operand().to_ui32() & 0x3f;
         v128_unop<std::uint64_t[2]>([b](std::uint64_t a) {
            return a << b;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i64x2_shr_s_t&) {
         uint32_t b = context.pop_operand().to_ui32() & 0x3f;
         v128_unop<std::uint64_t[2]>([b](std::int64_t a) {
            return a >> b;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i64x2_shr_u_t&) {
         uint32_t b = context.pop_operand().to_ui32() & 0x3f;
         v128_unop<std::uint64_t[2]>([b](std::uint64_t a) {
            return a >> b;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i64x2_add_t&) {
         v128_binop<std::uint64_t[2]>([](std::uint64_t a, std::uint64_t b) {
            return a + b;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i64x2_sub_t&) {
         v128_binop<std::uint64_t[2]>([](std::uint64_t a, std::uint64_t b) {
            return a - b;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i64x2_mul_t&) {
         v128_binop<std::uint64_t[2]>([](std::uint64_t a, std::uint64_t b) {
            return a * b;
         });
      }
      [[gnu::always_inline]] inline void operator()(const i64x2_extmul_low_i32x4_s_t&) {
         v128_extmul<std::int32_t[4], std::int64_t[2]>(false);
      }
      [[gnu::always_inline]] inline void operator()(const i64x2_extmul_high_i32x4_s_t&) {
         v128_extmul<std::int32_t[4], std::int64_t[2]>(true);
      }
      [[gnu::always_inline]] inline void operator()(const i64x2_extmul_low_i32x4_u_t&) {
         v128_extmul<std::uint32_t[4], std::uint64_t[2]>(false);
      }
      [[gnu::always_inline]] inline void operator()(const i64x2_extmul_high_i32x4_u_t&) {
         v128_extmul<std::uint32_t[4], std::uint64_t[2]>(true);
      }

      [[gnu::always_inline]] inline void operator()(const f32x4_ceil_t&) {
         v128_unop(&_eosio_f32x4_ceil);
      }
      [[gnu::always_inline]] inline void operator()(const f32x4_floor_t&) {
         v128_unop(&_eosio_f32x4_floor);
      }
      [[gnu::always_inline]] inline void operator()(const f32x4_trunc_t&) {
         v128_unop(&_eosio_f32x4_trunc);
      }
      [[gnu::always_inline]] inline void operator()(const f32x4_nearest_t&) {
         v128_unop(&_eosio_f32x4_nearest);
      }
      [[gnu::always_inline]] inline void operator()(const f32x4_abs_t&) {
         v128_unop(&_eosio_f32x4_abs);
      }
      [[gnu::always_inline]] inline void operator()(const f32x4_neg_t&) {
         v128_unop(&_eosio_f32x4_neg);
      }
      [[gnu::always_inline]] inline void operator()(const f32x4_sqrt_t&) {
         v128_unop(&_eosio_f32x4_sqrt);
      }
      [[gnu::always_inline]] inline void operator()(const f32x4_add_t&) {
         v128_binop(&_eosio_f32x4_add);
      }
      [[gnu::always_inline]] inline void operator()(const f32x4_sub_t&) {
         v128_binop(&_eosio_f32x4_sub);
      }
      [[gnu::always_inline]] inline void operator()(const f32x4_mul_t&) {
         v128_binop(&_eosio_f32x4_mul);
      }
      [[gnu::always_inline]] inline void operator()(const f32x4_div_t&) {
         v128_binop(&_eosio_f32x4_div);
      }
      [[gnu::always_inline]] inline void operator()(const f32x4_min_t&) {
         v128_binop(&_eosio_f32x4_min);
      }
      [[gnu::always_inline]] inline void operator()(const f32x4_max_t&) {
         v128_binop(&_eosio_f32x4_max);
      }
      [[gnu::always_inline]] inline void operator()(const f32x4_pmin_t&) {
         v128_binop(&_eosio_f32x4_pmin);
      }
      [[gnu::always_inline]] inline void operator()(const f32x4_pmax_t&) {
         v128_binop(&_eosio_f32x4_pmax);
      }

      [[gnu::always_inline]] inline void operator()(const f64x2_ceil_t&) {
         v128_unop(&_eosio_f64x2_ceil);
      }
      [[gnu::always_inline]] inline void operator()(const f64x2_floor_t&) {
         v128_unop(&_eosio_f64x2_floor);
      }
      [[gnu::always_inline]] inline void operator()(const f64x2_trunc_t&) {
         v128_unop(&_eosio_f64x2_trunc);
      }
      [[gnu::always_inline]] inline void operator()(const f64x2_nearest_t&) {
         v128_unop(&_eosio_f64x2_nearest);
      }
      [[gnu::always_inline]] inline void operator()(const f64x2_abs_t&) {
         v128_unop(&_eosio_f64x2_abs);
      }
      [[gnu::always_inline]] inline void operator()(const f64x2_neg_t&) {
         v128_unop(&_eosio_f64x2_neg);
      }
      [[gnu::always_inline]] inline void operator()(const f64x2_sqrt_t&) {
         v128_unop(&_eosio_f64x2_sqrt);
      }
      [[gnu::always_inline]] inline void operator()(const f64x2_add_t&) {
         v128_binop(&_eosio_f64x2_add);
      }
      [[gnu::always_inline]] inline void operator()(const f64x2_sub_t&) {
         v128_binop(&_eosio_f64x2_sub);
      }
      [[gnu::always_inline]] inline void operator()(const f64x2_mul_t&) {
         v128_binop(&_eosio_f64x2_mul);
      }
      [[gnu::always_inline]] inline void operator()(const f64x2_div_t&) {
         v128_binop(&_eosio_f64x2_div);
      }
      [[gnu::always_inline]] inline void operator()(const f64x2_min_t&) {
         v128_binop(&_eosio_f64x2_min);
      }
      [[gnu::always_inline]] inline void operator()(const f64x2_max_t&) {
         v128_binop(&_eosio_f64x2_max);
      }
      [[gnu::always_inline]] inline void operator()(const f64x2_pmin_t&) {
         v128_binop(&_eosio_f64x2_pmin);
      }
      [[gnu::always_inline]] inline void operator()(const f64x2_pmax_t&) {
         v128_binop(&_eosio_f64x2_pmax);
      }

      [[gnu::always_inline]] inline void operator()(const i32x4_trunc_sat_f32x4_s_t&) {
         v128_unop(&_eosio_i32x4_trunc_sat_f32x4_s);
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_trunc_sat_f32x4_u_t&) {
         v128_unop(&_eosio_i32x4_trunc_sat_f32x4_u);
      }
      [[gnu::always_inline]] inline void operator()(const f32x4_convert_i32x4_s_t&) {
         v128_unop(&_eosio_f32x4_convert_i32x4_s);
      }
      [[gnu::always_inline]] inline void operator()(const f32x4_convert_i32x4_u_t&) {
         v128_unop(&_eosio_f32x4_convert_i32x4_u);
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_trunc_sat_f64x2_s_zero_t&) {
         v128_unop(&_eosio_i32x4_trunc_sat_f64x2_s_zero);
      }
      [[gnu::always_inline]] inline void operator()(const i32x4_trunc_sat_f64x2_u_zero_t&) {
         v128_unop(&_eosio_i32x4_trunc_sat_f64x2_u_zero);
      }
      [[gnu::always_inline]] inline void operator()(const f64x2_convert_low_i32x4_s_t&) {
         v128_unop(&_eosio_f64x2_convert_low_i32x4_s);
      }
      [[gnu::always_inline]] inline void operator()(const f64x2_convert_low_i32x4_u_t&) {
         v128_unop(&_eosio_f64x2_convert_low_i32x4_u);
      }
      [[gnu::always_inline]] inline void operator()(const f32x4_demote_f64x2_zero_t&) {
         v128_unop(&_eosio_f32x4_demote_f64x2_zero);
      }
      [[gnu::always_inline]] inline void operator()(const f64x2_promote_low_f32x4_t&) {
         v128_unop(&_eosio_f64x2_promote_low_f32x4);
      }
   };

}} // namespace eosio::vm
