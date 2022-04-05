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

BACKEND_TEST_CASE( "Testing wasm <simd_load8_lane_0_wasm>", "[simd_load8_lane_0_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_load8_lane.0.wasm");
   backend_t bkend( code, &wa );

   CHECK(bkend.call_with_return("env", "v128.load8_lane_0", UINT32_C(0), make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_1", UINT32_C(1), make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,1u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_2", UINT32_C(2), make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,2u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_3", UINT32_C(3), make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,3u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_4", UINT32_C(4), make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,4u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_5", UINT32_C(5), make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,0u,5u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_6", UINT32_C(6), make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,0u,0u,6u,0u,0u,0u,0u,0u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_7", UINT32_C(7), make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,0u,0u,0u,7u,0u,0u,0u,0u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_8", UINT32_C(8), make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,0u,0u,0u,0u,8u,0u,0u,0u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_9", UINT32_C(9), make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,0u,0u,0u,0u,0u,9u,0u,0u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_10", UINT32_C(10), make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,10u,0u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_11", UINT32_C(11), make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,11u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_12", UINT32_C(12), make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,12u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_13", UINT32_C(13), make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,13u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_14", UINT32_C(14), make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,14u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_15", UINT32_C(15), make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,15u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_0_offset_0", make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_1_offset_1", make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,1u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_2_offset_2", make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,2u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_3_offset_3", make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,3u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_4_offset_4", make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,4u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_5_offset_5", make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,0u,5u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_6_offset_6", make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,0u,0u,6u,0u,0u,0u,0u,0u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_7_offset_7", make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,0u,0u,0u,7u,0u,0u,0u,0u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_8_offset_8", make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,0u,0u,0u,0u,8u,0u,0u,0u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_9_offset_9", make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,0u,0u,0u,0u,0u,9u,0u,0u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_10_offset_10", make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,10u,0u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_11_offset_11", make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,11u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_12_offset_12", make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,12u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_13_offset_13", make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,13u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_14_offset_14", make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,14u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_15_offset_15", make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,15u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_0_align_1", UINT32_C(0), make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_1_align_1", UINT32_C(1), make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,1u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_2_align_1", UINT32_C(2), make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,2u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_3_align_1", UINT32_C(3), make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,3u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_4_align_1", UINT32_C(4), make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,4u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_5_align_1", UINT32_C(5), make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,0u,5u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_6_align_1", UINT32_C(6), make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,0u,0u,6u,0u,0u,0u,0u,0u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_7_align_1", UINT32_C(7), make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,0u,0u,0u,7u,0u,0u,0u,0u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_8_align_1", UINT32_C(8), make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,0u,0u,0u,0u,8u,0u,0u,0u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_9_align_1", UINT32_C(9), make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,0u,0u,0u,0u,0u,9u,0u,0u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_10_align_1", UINT32_C(10), make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,10u,0u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_11_align_1", UINT32_C(11), make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,11u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_12_align_1", UINT32_C(12), make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,12u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_13_align_1", UINT32_C(13), make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,13u,0u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_14_align_1", UINT32_C(14), make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,14u,0u));
   CHECK(bkend.call_with_return("env", "v128.load8_lane_15_align_1", UINT32_C(15), make_v128_i8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))->to_v128() == make_v128_i8(0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,15u));
}

BACKEND_TEST_CASE( "Testing wasm <simd_load8_lane_1_wasm>", "[simd_load8_lane_1_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_load8_lane.1.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_load8_lane_2_wasm>", "[simd_load8_lane_2_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_load8_lane.2.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_load8_lane_3_wasm>", "[simd_load8_lane_3_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_load8_lane.3.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}
