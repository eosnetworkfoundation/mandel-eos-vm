#pragma once
#include <eosio/vm/opcodes_def.hpp>
#include <eosio/vm/variant.hpp>

#include <map>

namespace eosio { namespace vm {
   enum opcodes {
      EOS_VM_CONTROL_FLOW_OPS(EOS_VM_CREATE_ENUM)
      EOS_VM_BR_TABLE_OP(EOS_VM_CREATE_ENUM)
      EOS_VM_RETURN_OP(EOS_VM_CREATE_ENUM)
      EOS_VM_CALL_OPS(EOS_VM_CREATE_ENUM)
      EOS_VM_CALL_IMM_OPS(EOS_VM_CREATE_ENUM)
      EOS_VM_PARAMETRIC_OPS(EOS_VM_CREATE_ENUM)
      EOS_VM_VARIABLE_ACCESS_OPS(EOS_VM_CREATE_ENUM)
      EOS_VM_MEMORY_OPS(EOS_VM_CREATE_ENUM)
      EOS_VM_I32_CONSTANT_OPS(EOS_VM_CREATE_ENUM)
      EOS_VM_I64_CONSTANT_OPS(EOS_VM_CREATE_ENUM)
      EOS_VM_F32_CONSTANT_OPS(EOS_VM_CREATE_ENUM)
      EOS_VM_F64_CONSTANT_OPS(EOS_VM_CREATE_ENUM)
      EOS_VM_COMPARISON_OPS(EOS_VM_CREATE_ENUM)
      EOS_VM_NUMERIC_OPS(EOS_VM_CREATE_ENUM)
      EOS_VM_CONVERSION_OPS(EOS_VM_CREATE_ENUM)
      EOS_VM_EXIT_OP(EOS_VM_CREATE_ENUM)
      EOS_VM_EXTENDED_OPS(EOS_VM_CREATE_ENUM)
      EOS_VM_EMPTY_OPS(EOS_VM_CREATE_ENUM)
      EOS_VM_ERROR_OPS(EOS_VM_CREATE_ENUM)
   };

   enum vec_opcodes {
      EOS_VM_VEC_MEMORY_OPS(EOS_VM_CREATE_ENUM)
      EOS_VM_VEC_LANE_MEMORY_OPS(EOS_VM_CREATE_ENUM)
      EOS_VM_VEC_CONSTANT_OPS(EOS_VM_CREATE_ENUM)
      EOS_VM_VEC_SHUFFLE_OPS(EOS_VM_CREATE_ENUM)
      EOS_VM_VEC_LANE_OPS(EOS_VM_CREATE_ENUM)
      EOS_VM_VEC_NUMERIC_OPS(EOS_VM_CREATE_ENUM)
   };

   struct opcode_utils {
      std::map<uint16_t, std::string> opcode_map{
         EOS_VM_CONTROL_FLOW_OPS(EOS_VM_CREATE_MAP)
         EOS_VM_BR_TABLE_OP(EOS_VM_CREATE_MAP)
         EOS_VM_RETURN_OP(EOS_VM_CREATE_MAP)
         EOS_VM_CALL_OPS(EOS_VM_CREATE_MAP)
         EOS_VM_CALL_IMM_OPS(EOS_VM_CREATE_MAP)
         EOS_VM_PARAMETRIC_OPS(EOS_VM_CREATE_MAP)
         EOS_VM_VARIABLE_ACCESS_OPS(EOS_VM_CREATE_MAP)
         EOS_VM_MEMORY_OPS(EOS_VM_CREATE_MAP)
         EOS_VM_I32_CONSTANT_OPS(EOS_VM_CREATE_MAP)
         EOS_VM_I64_CONSTANT_OPS(EOS_VM_CREATE_MAP)
         EOS_VM_F32_CONSTANT_OPS(EOS_VM_CREATE_MAP)
         EOS_VM_F64_CONSTANT_OPS(EOS_VM_CREATE_MAP)
         EOS_VM_COMPARISON_OPS(EOS_VM_CREATE_MAP)
         EOS_VM_NUMERIC_OPS(EOS_VM_CREATE_MAP)
         EOS_VM_CONVERSION_OPS(EOS_VM_CREATE_MAP)
         EOS_VM_EXIT_OP(EOS_VM_CREATE_MAP)
         EOS_VM_EXTENDED_OPS(EOS_VM_CREATE_MAP)
         EOS_VM_EMPTY_OPS(EOS_VM_CREATE_MAP)
         EOS_VM_ERROR_OPS(EOS_VM_CREATE_MAP)
      };
   };

   enum imm_types {
      none,
      block_imm,
      varuint32_imm,
      br_table_imm,
   };


   EOS_VM_CONTROL_FLOW_OPS(EOS_VM_CREATE_CONTROL_FLOW_TYPES)
   EOS_VM_BR_TABLE_OP(EOS_VM_CREATE_BR_TABLE_TYPE)
   EOS_VM_RETURN_OP(EOS_VM_CREATE_CONTROL_FLOW_TYPES)
   EOS_VM_CALL_OPS(EOS_VM_CREATE_CALL_TYPES)
   EOS_VM_CALL_IMM_OPS(EOS_VM_CREATE_CALL_IMM_TYPES)
   EOS_VM_PARAMETRIC_OPS(EOS_VM_CREATE_TYPES)
   EOS_VM_VARIABLE_ACCESS_OPS(EOS_VM_CREATE_VARIABLE_ACCESS_TYPES)
   EOS_VM_MEMORY_OPS(EOS_VM_CREATE_MEMORY_TYPES)
   EOS_VM_I32_CONSTANT_OPS(EOS_VM_CREATE_I32_CONSTANT_TYPE)
   EOS_VM_I64_CONSTANT_OPS(EOS_VM_CREATE_I64_CONSTANT_TYPE)
   EOS_VM_F32_CONSTANT_OPS(EOS_VM_CREATE_F32_CONSTANT_TYPE)
   EOS_VM_F64_CONSTANT_OPS(EOS_VM_CREATE_F64_CONSTANT_TYPE)
   EOS_VM_COMPARISON_OPS(EOS_VM_CREATE_TYPES)
   EOS_VM_NUMERIC_OPS(EOS_VM_CREATE_TYPES)
   EOS_VM_CONVERSION_OPS(EOS_VM_CREATE_TYPES)
   EOS_VM_EXIT_OP(EOS_VM_CREATE_EXIT_TYPE)
   EOS_VM_EMPTY_OPS(EOS_VM_CREATE_TYPES)
   EOS_VM_ERROR_OPS(EOS_VM_CREATE_TYPES)
   EOS_VM_VEC_MEMORY_OPS(EOS_VM_CREATE_VEC_MEMORY_TYPES)
   EOS_VM_VEC_LANE_MEMORY_OPS(EOS_VM_CREATE_VEC_LANE_MEMORY_TYPES)
   EOS_VM_VEC_CONSTANT_OPS(EOS_VM_CREATE_V128_CONSTANT_TYPE)
   EOS_VM_VEC_SHUFFLE_OPS(EOS_VM_CREATE_VEC_SHUFFLE_TYPE)
   EOS_VM_VEC_LANE_OPS(EOS_VM_CREATE_VEC_LANE_TYPES)
   EOS_VM_VEC_NUMERIC_OPS(EOS_VM_CREATE_VEC_TYPES)

   using opcode = variant<
      EOS_VM_CONTROL_FLOW_OPS(EOS_VM_IDENTITY)
      EOS_VM_BR_TABLE_OP(EOS_VM_IDENTITY)
      EOS_VM_RETURN_OP(EOS_VM_IDENTITY)
      EOS_VM_CALL_OPS(EOS_VM_IDENTITY)
      EOS_VM_CALL_IMM_OPS(EOS_VM_IDENTITY)
      EOS_VM_PARAMETRIC_OPS(EOS_VM_IDENTITY)
      EOS_VM_VARIABLE_ACCESS_OPS(EOS_VM_IDENTITY)
      EOS_VM_MEMORY_OPS(EOS_VM_IDENTITY)
      EOS_VM_I32_CONSTANT_OPS(EOS_VM_IDENTITY)
      EOS_VM_I64_CONSTANT_OPS(EOS_VM_IDENTITY)
      EOS_VM_F32_CONSTANT_OPS(EOS_VM_IDENTITY)
      EOS_VM_F64_CONSTANT_OPS(EOS_VM_IDENTITY)
      EOS_VM_COMPARISON_OPS(EOS_VM_IDENTITY)
      EOS_VM_NUMERIC_OPS(EOS_VM_IDENTITY)
      EOS_VM_CONVERSION_OPS(EOS_VM_IDENTITY)
      EOS_VM_EXIT_OP(EOS_VM_IDENTITY)
      EOS_VM_EMPTY_OPS(EOS_VM_IDENTITY)
      EOS_VM_ERROR_OPS(EOS_VM_IDENTITY_END)
      >;
}} // namespace eosio::vm
