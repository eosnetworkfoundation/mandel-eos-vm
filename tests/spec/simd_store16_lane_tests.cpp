// Generated by spec_test_generator.  DO NOT MODIFY THIS FILE.

#include <algorithm>
#include <vector>
#include <iostream>
#include <iterator>
#include <cmath>
#include <cstdlib>
#include <catch2/catch.hpp>
#include <utils.hpp>
#include <wasm_config.hpp>
#include <eosio/vm/backend.hpp>

using namespace eosio;
using namespace eosio::vm;
extern wasm_allocator wa;

BACKEND_TEST_CASE( "Testing wasm <simd_store16_lane_0_wasm>", "[simd_store16_lane_0_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_store16_lane.0.wasm");
   backend_t bkend( code, &wa );

   CHECK(bkend.call_with_return("env", "v128.store16_lane_0", UINT32_C(0), make_v128_i16(256u,0u,0u,0u,0u,0u,0u,0u))->to_ui64() == UINT64_C(256));
   CHECK(bkend.call_with_return("env", "v128.store16_lane_1", UINT32_C(1), make_v128_i16(0u,513u,0u,0u,0u,0u,0u,0u))->to_ui64() == UINT64_C(513));
   CHECK(bkend.call_with_return("env", "v128.store16_lane_2", UINT32_C(2), make_v128_i16(0u,0u,770u,0u,0u,0u,0u,0u))->to_ui64() == UINT64_C(770));
   CHECK(bkend.call_with_return("env", "v128.store16_lane_3", UINT32_C(3), make_v128_i16(0u,0u,0u,1027u,0u,0u,0u,0u))->to_ui64() == UINT64_C(1027));
   CHECK(bkend.call_with_return("env", "v128.store16_lane_4", UINT32_C(4), make_v128_i16(0u,0u,0u,0u,1284u,0u,0u,0u))->to_ui64() == UINT64_C(1284));
   CHECK(bkend.call_with_return("env", "v128.store16_lane_5", UINT32_C(5), make_v128_i16(0u,0u,0u,0u,0u,1541u,0u,0u))->to_ui64() == UINT64_C(1541));
   CHECK(bkend.call_with_return("env", "v128.store16_lane_6", UINT32_C(6), make_v128_i16(0u,0u,0u,0u,0u,0u,1798u,0u))->to_ui64() == UINT64_C(1798));
   CHECK(bkend.call_with_return("env", "v128.store16_lane_7", UINT32_C(7), make_v128_i16(0u,0u,0u,0u,0u,0u,0u,2055u))->to_ui64() == UINT64_C(2055));
   CHECK(bkend.call_with_return("env", "v128.store16_lane_0_offset_0", make_v128_i16(256u,0u,0u,0u,0u,0u,0u,0u))->to_ui64() == UINT64_C(256));
   CHECK(bkend.call_with_return("env", "v128.store16_lane_1_offset_1", make_v128_i16(0u,513u,0u,0u,0u,0u,0u,0u))->to_ui64() == UINT64_C(513));
   CHECK(bkend.call_with_return("env", "v128.store16_lane_2_offset_2", make_v128_i16(0u,0u,770u,0u,0u,0u,0u,0u))->to_ui64() == UINT64_C(770));
   CHECK(bkend.call_with_return("env", "v128.store16_lane_3_offset_3", make_v128_i16(0u,0u,0u,1027u,0u,0u,0u,0u))->to_ui64() == UINT64_C(1027));
   CHECK(bkend.call_with_return("env", "v128.store16_lane_4_offset_4", make_v128_i16(0u,0u,0u,0u,1284u,0u,0u,0u))->to_ui64() == UINT64_C(1284));
   CHECK(bkend.call_with_return("env", "v128.store16_lane_5_offset_5", make_v128_i16(0u,0u,0u,0u,0u,1541u,0u,0u))->to_ui64() == UINT64_C(1541));
   CHECK(bkend.call_with_return("env", "v128.store16_lane_6_offset_6", make_v128_i16(0u,0u,0u,0u,0u,0u,1798u,0u))->to_ui64() == UINT64_C(1798));
   CHECK(bkend.call_with_return("env", "v128.store16_lane_7_offset_7", make_v128_i16(0u,0u,0u,0u,0u,0u,0u,2055u))->to_ui64() == UINT64_C(2055));
   CHECK(bkend.call_with_return("env", "v128.store16_lane_0_align_1", UINT32_C(0), make_v128_i16(256u,0u,0u,0u,0u,0u,0u,0u))->to_ui64() == UINT64_C(256));
   CHECK(bkend.call_with_return("env", "v128.store16_lane_0_align_2", UINT32_C(0), make_v128_i16(256u,0u,0u,0u,0u,0u,0u,0u))->to_ui64() == UINT64_C(256));
   CHECK(bkend.call_with_return("env", "v128.store16_lane_1_align_1", UINT32_C(1), make_v128_i16(0u,513u,0u,0u,0u,0u,0u,0u))->to_ui64() == UINT64_C(513));
   CHECK(bkend.call_with_return("env", "v128.store16_lane_1_align_2", UINT32_C(1), make_v128_i16(0u,513u,0u,0u,0u,0u,0u,0u))->to_ui64() == UINT64_C(513));
   CHECK(bkend.call_with_return("env", "v128.store16_lane_2_align_1", UINT32_C(2), make_v128_i16(0u,0u,770u,0u,0u,0u,0u,0u))->to_ui64() == UINT64_C(770));
   CHECK(bkend.call_with_return("env", "v128.store16_lane_2_align_2", UINT32_C(2), make_v128_i16(0u,0u,770u,0u,0u,0u,0u,0u))->to_ui64() == UINT64_C(770));
   CHECK(bkend.call_with_return("env", "v128.store16_lane_3_align_1", UINT32_C(3), make_v128_i16(0u,0u,0u,1027u,0u,0u,0u,0u))->to_ui64() == UINT64_C(1027));
   CHECK(bkend.call_with_return("env", "v128.store16_lane_3_align_2", UINT32_C(3), make_v128_i16(0u,0u,0u,1027u,0u,0u,0u,0u))->to_ui64() == UINT64_C(1027));
   CHECK(bkend.call_with_return("env", "v128.store16_lane_4_align_1", UINT32_C(4), make_v128_i16(0u,0u,0u,0u,1284u,0u,0u,0u))->to_ui64() == UINT64_C(1284));
   CHECK(bkend.call_with_return("env", "v128.store16_lane_4_align_2", UINT32_C(4), make_v128_i16(0u,0u,0u,0u,1284u,0u,0u,0u))->to_ui64() == UINT64_C(1284));
   CHECK(bkend.call_with_return("env", "v128.store16_lane_5_align_1", UINT32_C(5), make_v128_i16(0u,0u,0u,0u,0u,1541u,0u,0u))->to_ui64() == UINT64_C(1541));
   CHECK(bkend.call_with_return("env", "v128.store16_lane_5_align_2", UINT32_C(5), make_v128_i16(0u,0u,0u,0u,0u,1541u,0u,0u))->to_ui64() == UINT64_C(1541));
   CHECK(bkend.call_with_return("env", "v128.store16_lane_6_align_1", UINT32_C(6), make_v128_i16(0u,0u,0u,0u,0u,0u,1798u,0u))->to_ui64() == UINT64_C(1798));
   CHECK(bkend.call_with_return("env", "v128.store16_lane_6_align_2", UINT32_C(6), make_v128_i16(0u,0u,0u,0u,0u,0u,1798u,0u))->to_ui64() == UINT64_C(1798));
   CHECK(bkend.call_with_return("env", "v128.store16_lane_7_align_1", UINT32_C(7), make_v128_i16(0u,0u,0u,0u,0u,0u,0u,2055u))->to_ui64() == UINT64_C(2055));
   CHECK(bkend.call_with_return("env", "v128.store16_lane_7_align_2", UINT32_C(7), make_v128_i16(0u,0u,0u,0u,0u,0u,0u,2055u))->to_ui64() == UINT64_C(2055));
}

BACKEND_TEST_CASE( "Testing wasm <simd_store16_lane_1_wasm>", "[simd_store16_lane_1_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_store16_lane.1.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_store16_lane_2_wasm>", "[simd_store16_lane_2_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_store16_lane.2.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_store16_lane_3_wasm>", "[simd_store16_lane_3_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_store16_lane.3.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

