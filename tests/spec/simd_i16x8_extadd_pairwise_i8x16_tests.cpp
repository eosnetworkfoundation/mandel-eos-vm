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

BACKEND_TEST_CASE( "Testing wasm <simd_i16x8_extadd_pairwise_i8x16_0_wasm>", "[simd_i16x8_extadd_pairwise_i8x16_0_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_i16x8_extadd_pairwise_i8x16.0.wasm");
   backend_t bkend( code, &wa );

   CHECK(bkend.call_with_return("env", "i16x8.extadd_pairwise_i8x16_s", make_v128_i8(0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u))->to_v128() == make_v128_i16(0u,0u,0u,0u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "i16x8.extadd_pairwise_i8x16_s", make_v128_i8(1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u))->to_v128() == make_v128_i16(2u,2u,2u,2u,2u,2u,2u,2u));
   CHECK(bkend.call_with_return("env", "i16x8.extadd_pairwise_i8x16_s", make_v128_i8(255u,255u,255u,255u,255u,255u,255u,255u,255u,255u,255u,255u,255u,255u,255u,255u))->to_v128() == make_v128_i16(65534u,65534u,65534u,65534u,65534u,65534u,65534u,65534u));
   CHECK(bkend.call_with_return("env", "i16x8.extadd_pairwise_i8x16_s", make_v128_i8(126u,126u,126u,126u,126u,126u,126u,126u,126u,126u,126u,126u,126u,126u,126u,126u))->to_v128() == make_v128_i16(252u,252u,252u,252u,252u,252u,252u,252u));
   CHECK(bkend.call_with_return("env", "i16x8.extadd_pairwise_i8x16_s", make_v128_i8(129u,129u,129u,129u,129u,129u,129u,129u,129u,129u,129u,129u,129u,129u,129u,129u))->to_v128() == make_v128_i16(65282u,65282u,65282u,65282u,65282u,65282u,65282u,65282u));
   CHECK(bkend.call_with_return("env", "i16x8.extadd_pairwise_i8x16_s", make_v128_i8(128u,128u,128u,128u,128u,128u,128u,128u,128u,128u,128u,128u,128u,128u,128u,128u))->to_v128() == make_v128_i16(65280u,65280u,65280u,65280u,65280u,65280u,65280u,65280u));
   CHECK(bkend.call_with_return("env", "i16x8.extadd_pairwise_i8x16_s", make_v128_i8(127u,127u,127u,127u,127u,127u,127u,127u,127u,127u,127u,127u,127u,127u,127u,127u))->to_v128() == make_v128_i16(254u,254u,254u,254u,254u,254u,254u,254u));
   CHECK(bkend.call_with_return("env", "i16x8.extadd_pairwise_i8x16_s", make_v128_i8(255u,255u,255u,255u,255u,255u,255u,255u,255u,255u,255u,255u,255u,255u,255u,255u))->to_v128() == make_v128_i16(65534u,65534u,65534u,65534u,65534u,65534u,65534u,65534u));
   CHECK(bkend.call_with_return("env", "i16x8.extadd_pairwise_i8x16_u", make_v128_i8(0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u))->to_v128() == make_v128_i16(0u,0u,0u,0u,0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "i16x8.extadd_pairwise_i8x16_u", make_v128_i8(1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u))->to_v128() == make_v128_i16(2u,2u,2u,2u,2u,2u,2u,2u));
   CHECK(bkend.call_with_return("env", "i16x8.extadd_pairwise_i8x16_u", make_v128_i8(255u,255u,255u,255u,255u,255u,255u,255u,255u,255u,255u,255u,255u,255u,255u,255u))->to_v128() == make_v128_i16(510u,510u,510u,510u,510u,510u,510u,510u));
   CHECK(bkend.call_with_return("env", "i16x8.extadd_pairwise_i8x16_u", make_v128_i8(126u,126u,126u,126u,126u,126u,126u,126u,126u,126u,126u,126u,126u,126u,126u,126u))->to_v128() == make_v128_i16(252u,252u,252u,252u,252u,252u,252u,252u));
   CHECK(bkend.call_with_return("env", "i16x8.extadd_pairwise_i8x16_u", make_v128_i8(129u,129u,129u,129u,129u,129u,129u,129u,129u,129u,129u,129u,129u,129u,129u,129u))->to_v128() == make_v128_i16(258u,258u,258u,258u,258u,258u,258u,258u));
   CHECK(bkend.call_with_return("env", "i16x8.extadd_pairwise_i8x16_u", make_v128_i8(128u,128u,128u,128u,128u,128u,128u,128u,128u,128u,128u,128u,128u,128u,128u,128u))->to_v128() == make_v128_i16(256u,256u,256u,256u,256u,256u,256u,256u));
   CHECK(bkend.call_with_return("env", "i16x8.extadd_pairwise_i8x16_u", make_v128_i8(127u,127u,127u,127u,127u,127u,127u,127u,127u,127u,127u,127u,127u,127u,127u,127u))->to_v128() == make_v128_i16(254u,254u,254u,254u,254u,254u,254u,254u));
   CHECK(bkend.call_with_return("env", "i16x8.extadd_pairwise_i8x16_u", make_v128_i8(255u,255u,255u,255u,255u,255u,255u,255u,255u,255u,255u,255u,255u,255u,255u,255u))->to_v128() == make_v128_i16(510u,510u,510u,510u,510u,510u,510u,510u));
}

BACKEND_TEST_CASE( "Testing wasm <simd_i16x8_extadd_pairwise_i8x16_1_wasm>", "[simd_i16x8_extadd_pairwise_i8x16_1_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_i16x8_extadd_pairwise_i8x16.1.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_i16x8_extadd_pairwise_i8x16_2_wasm>", "[simd_i16x8_extadd_pairwise_i8x16_2_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_i16x8_extadd_pairwise_i8x16.2.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_i16x8_extadd_pairwise_i8x16_3_wasm>", "[simd_i16x8_extadd_pairwise_i8x16_3_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_i16x8_extadd_pairwise_i8x16.3.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_i16x8_extadd_pairwise_i8x16_4_wasm>", "[simd_i16x8_extadd_pairwise_i8x16_4_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_i16x8_extadd_pairwise_i8x16.4.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

