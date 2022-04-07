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

BACKEND_TEST_CASE( "Testing wasm <simd_bitwise_0_wasm>", "[simd_bitwise_0_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_bitwise.0.wasm");
   backend_t bkend( code, &wa );

   CHECK(bkend.call_with_return("env", "not", make_v128_i32(0u,0u,0u,0u))->to_v128() == make_v128_i32(4294967295u,4294967295u,4294967295u,4294967295u));
   CHECK(bkend.call_with_return("env", "not", make_v128_i32(4294967295u,4294967295u,4294967295u,4294967295u))->to_v128() == make_v128_i32(0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "not", make_v128_i32(4294967295u,0u,4294967295u,0u))->to_v128() == make_v128_i32(0u,4294967295u,0u,4294967295u));
   CHECK(bkend.call_with_return("env", "not", make_v128_i32(0u,4294967295u,0u,4294967295u))->to_v128() == make_v128_i32(4294967295u,0u,4294967295u,0u));
   CHECK(bkend.call_with_return("env", "not", make_v128_i32(1431655765u,1431655765u,1431655765u,1431655765u))->to_v128() == make_v128_i32(2863311530u,2863311530u,2863311530u,2863311530u));
   CHECK(bkend.call_with_return("env", "not", make_v128_i32(3435973836u,3435973836u,3435973836u,3435973836u))->to_v128() == make_v128_i32(858993459u,858993459u,858993459u,858993459u));
   CHECK(bkend.call_with_return("env", "not", make_v128_i32(1234567890u,1234567890u,1234567890u,1234567890u))->to_v128() == make_v128_i32(3060399405u,3060399405u,3060399405u,3060399405u));
   CHECK(bkend.call_with_return("env", "not", make_v128_i32(305419896u,305419896u,305419896u,305419896u))->to_v128() == make_v128_i32(3989547399u,3989547399u,3989547399u,3989547399u));
   CHECK(bkend.call_with_return("env", "and", make_v128_i32(0u,0u,4294967295u,4294967295u), make_v128_i32(0u,4294967295u,0u,4294967295u))->to_v128() == make_v128_i32(0u,0u,0u,4294967295u));
   CHECK(bkend.call_with_return("env", "and", make_v128_i32(0u,0u,0u,0u), make_v128_i32(0u,0u,0u,0u))->to_v128() == make_v128_i32(0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "and", make_v128_i32(0u,0u,0u,0u), make_v128_i32(4294967295u,4294967295u,4294967295u,4294967295u))->to_v128() == make_v128_i32(0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "and", make_v128_i32(0u,0u,0u,0u), make_v128_i32(4294967295u,4294967295u,4294967295u,4294967295u))->to_v128() == make_v128_i32(0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "and", make_v128_i32(1u,1u,1u,1u), make_v128_i32(1u,1u,1u,1u))->to_v128() == make_v128_i32(1u,1u,1u,1u));
   CHECK(bkend.call_with_return("env", "and", make_v128_i32(255u,255u,255u,255u), make_v128_i32(85u,85u,85u,85u))->to_v128() == make_v128_i32(85u,85u,85u,85u));
   CHECK(bkend.call_with_return("env", "and", make_v128_i32(255u,255u,255u,255u), make_v128_i32(128u,128u,128u,128u))->to_v128() == make_v128_i32(128u,128u,128u,128u));
   CHECK(bkend.call_with_return("env", "and", make_v128_i32(2863311530u,2863311530u,2863311530u,2863311530u), make_v128_i32(10u,128u,5u,165u))->to_v128() == make_v128_i32(10u,128u,0u,160u));
   CHECK(bkend.call_with_return("env", "and", make_v128_i32(4294967295u,4294967295u,4294967295u,4294967295u), make_v128_i32(1431655765u,1431655765u,1431655765u,1431655765u))->to_v128() == make_v128_i32(1431655765u,1431655765u,1431655765u,1431655765u));
   CHECK(bkend.call_with_return("env", "and", make_v128_i32(4294967295u,4294967295u,4294967295u,4294967295u), make_v128_i32(2863311530u,2863311530u,2863311530u,2863311530u))->to_v128() == make_v128_i32(2863311530u,2863311530u,2863311530u,2863311530u));
   CHECK(bkend.call_with_return("env", "and", make_v128_i32(4294967295u,4294967295u,4294967295u,4294967295u), make_v128_i32(0u,0u,0u,0u))->to_v128() == make_v128_i32(0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "and", make_v128_i32(1431655765u,1431655765u,1431655765u,1431655765u), make_v128_i32(21845u,65535u,22015u,24575u))->to_v128() == make_v128_i32(21845u,21845u,21845u,21845u));
   CHECK(bkend.call_with_return("env", "and", make_v128_i32(1234567890u,1234567890u,1234567890u,1234567890u), make_v128_i32(1234567890u,1234567890u,1234567890u,1234567890u))->to_v128() == make_v128_i32(1234567890u,1234567890u,1234567890u,1234567890u));
   CHECK(bkend.call_with_return("env", "and", make_v128_i32(305419896u,305419896u,305419896u,305419896u), make_v128_i32(2427178479u,2427178479u,2427178479u,2427178479u))->to_v128() == make_v128_i32(270550120u,270550120u,270550120u,270550120u));
   CHECK(bkend.call_with_return("env", "or", make_v128_i32(0u,0u,4294967295u,4294967295u), make_v128_i32(0u,4294967295u,0u,4294967295u))->to_v128() == make_v128_i32(0u,4294967295u,4294967295u,4294967295u));
   CHECK(bkend.call_with_return("env", "or", make_v128_i32(0u,0u,0u,0u), make_v128_i32(0u,0u,0u,0u))->to_v128() == make_v128_i32(0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "or", make_v128_i32(0u,0u,0u,0u), make_v128_i32(4294967295u,4294967295u,4294967295u,4294967295u))->to_v128() == make_v128_i32(4294967295u,4294967295u,4294967295u,4294967295u));
   CHECK(bkend.call_with_return("env", "or", make_v128_i32(0u,0u,0u,0u), make_v128_i32(4294967295u,4294967295u,4294967295u,4294967295u))->to_v128() == make_v128_i32(4294967295u,4294967295u,4294967295u,4294967295u));
   CHECK(bkend.call_with_return("env", "or", make_v128_i32(1u,1u,1u,1u), make_v128_i32(1u,1u,1u,1u))->to_v128() == make_v128_i32(1u,1u,1u,1u));
   CHECK(bkend.call_with_return("env", "or", make_v128_i32(255u,255u,255u,255u), make_v128_i32(85u,85u,85u,85u))->to_v128() == make_v128_i32(255u,255u,255u,255u));
   CHECK(bkend.call_with_return("env", "or", make_v128_i32(255u,255u,255u,255u), make_v128_i32(128u,128u,128u,128u))->to_v128() == make_v128_i32(255u,255u,255u,255u));
   CHECK(bkend.call_with_return("env", "or", make_v128_i32(2863311530u,2863311530u,2863311530u,2863311530u), make_v128_i32(10u,128u,5u,165u))->to_v128() == make_v128_i32(2863311530u,2863311530u,2863311535u,2863311535u));
   CHECK(bkend.call_with_return("env", "or", make_v128_i32(4294967295u,4294967295u,4294967295u,4294967295u), make_v128_i32(1431655765u,1431655765u,1431655765u,1431655765u))->to_v128() == make_v128_i32(4294967295u,4294967295u,4294967295u,4294967295u));
   CHECK(bkend.call_with_return("env", "or", make_v128_i32(4294967295u,4294967295u,4294967295u,4294967295u), make_v128_i32(2863311530u,2863311530u,2863311530u,2863311530u))->to_v128() == make_v128_i32(4294967295u,4294967295u,4294967295u,4294967295u));
   CHECK(bkend.call_with_return("env", "or", make_v128_i32(4294967295u,4294967295u,4294967295u,4294967295u), make_v128_i32(0u,0u,0u,0u))->to_v128() == make_v128_i32(4294967295u,4294967295u,4294967295u,4294967295u));
   CHECK(bkend.call_with_return("env", "or", make_v128_i32(1431655765u,1431655765u,1431655765u,1431655765u), make_v128_i32(21845u,65535u,22015u,24575u))->to_v128() == make_v128_i32(1431655765u,1431699455u,1431655935u,1431658495u));
   CHECK(bkend.call_with_return("env", "or", make_v128_i32(1234567890u,1234567890u,1234567890u,1234567890u), make_v128_i32(1234567890u,1234567890u,1234567890u,1234567890u))->to_v128() == make_v128_i32(1234567890u,1234567890u,1234567890u,1234567890u));
   CHECK(bkend.call_with_return("env", "or", make_v128_i32(305419896u,305419896u,305419896u,305419896u), make_v128_i32(2427178479u,2427178479u,2427178479u,2427178479u))->to_v128() == make_v128_i32(2462048255u,2462048255u,2462048255u,2462048255u));
   CHECK(bkend.call_with_return("env", "xor", make_v128_i32(0u,0u,4294967295u,4294967295u), make_v128_i32(0u,4294967295u,0u,4294967295u))->to_v128() == make_v128_i32(0u,4294967295u,4294967295u,0u));
   CHECK(bkend.call_with_return("env", "xor", make_v128_i32(0u,0u,0u,0u), make_v128_i32(0u,0u,0u,0u))->to_v128() == make_v128_i32(0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "xor", make_v128_i32(0u,0u,0u,0u), make_v128_i32(4294967295u,4294967295u,4294967295u,4294967295u))->to_v128() == make_v128_i32(4294967295u,4294967295u,4294967295u,4294967295u));
   CHECK(bkend.call_with_return("env", "xor", make_v128_i32(0u,0u,0u,0u), make_v128_i32(4294967295u,4294967295u,4294967295u,4294967295u))->to_v128() == make_v128_i32(4294967295u,4294967295u,4294967295u,4294967295u));
   CHECK(bkend.call_with_return("env", "xor", make_v128_i32(1u,1u,1u,1u), make_v128_i32(1u,1u,1u,1u))->to_v128() == make_v128_i32(0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "xor", make_v128_i32(255u,255u,255u,255u), make_v128_i32(85u,85u,85u,85u))->to_v128() == make_v128_i32(170u,170u,170u,170u));
   CHECK(bkend.call_with_return("env", "xor", make_v128_i32(255u,255u,255u,255u), make_v128_i32(128u,128u,128u,128u))->to_v128() == make_v128_i32(127u,127u,127u,127u));
   CHECK(bkend.call_with_return("env", "xor", make_v128_i32(2863311530u,2863311530u,2863311530u,2863311530u), make_v128_i32(10u,128u,5u,165u))->to_v128() == make_v128_i32(2863311520u,2863311402u,2863311535u,2863311375u));
   CHECK(bkend.call_with_return("env", "xor", make_v128_i32(4294967295u,4294967295u,4294967295u,4294967295u), make_v128_i32(1431655765u,1431655765u,1431655765u,1431655765u))->to_v128() == make_v128_i32(2863311530u,2863311530u,2863311530u,2863311530u));
   CHECK(bkend.call_with_return("env", "xor", make_v128_i32(4294967295u,4294967295u,4294967295u,4294967295u), make_v128_i32(2863311530u,2863311530u,2863311530u,2863311530u))->to_v128() == make_v128_i32(1431655765u,1431655765u,1431655765u,1431655765u));
   CHECK(bkend.call_with_return("env", "xor", make_v128_i32(4294967295u,4294967295u,4294967295u,4294967295u), make_v128_i32(0u,0u,0u,0u))->to_v128() == make_v128_i32(4294967295u,4294967295u,4294967295u,4294967295u));
   CHECK(bkend.call_with_return("env", "xor", make_v128_i32(1431655765u,1431655765u,1431655765u,1431655765u), make_v128_i32(21845u,65535u,22015u,24575u))->to_v128() == make_v128_i32(1431633920u,1431677610u,1431634090u,1431636650u));
   CHECK(bkend.call_with_return("env", "xor", make_v128_i32(1234567890u,1234567890u,1234567890u,1234567890u), make_v128_i32(1234567890u,1234567890u,1234567890u,1234567890u))->to_v128() == make_v128_i32(0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "xor", make_v128_i32(305419896u,305419896u,305419896u,305419896u), make_v128_i32(2427178479u,2427178479u,2427178479u,2427178479u))->to_v128() == make_v128_i32(2191498135u,2191498135u,2191498135u,2191498135u));
   CHECK(bkend.call_with_return("env", "bitselect", make_v128_i32(2863311530u,2863311530u,2863311530u,2863311530u), make_v128_i32(3149642683u,3149642683u,3149642683u,3149642683u), make_v128_i32(1123141u,4027580415u,269557793u,3148528554u))->to_v128() == make_v128_i32(3148528314u,2881137322u,2880093114u,2864425659u));
   CHECK(bkend.call_with_return("env", "bitselect", make_v128_i32(2863311530u,2863311530u,2863311530u,2863311530u), make_v128_i32(3149642683u,3149642683u,3149642683u,3149642683u), make_v128_i32(0u,0u,0u,0u))->to_v128() == make_v128_i32(3149642683u,3149642683u,3149642683u,3149642683u));
   CHECK(bkend.call_with_return("env", "bitselect", make_v128_i32(2863311530u,2863311530u,2863311530u,2863311530u), make_v128_i32(3149642683u,3149642683u,3149642683u,3149642683u), make_v128_i32(286331153u,286331153u,286331153u,286331153u))->to_v128() == make_v128_i32(2863311530u,2863311530u,2863311530u,2863311530u));
   CHECK(bkend.call_with_return("env", "bitselect", make_v128_i32(2863311530u,2863311530u,2863311530u,2863311530u), make_v128_i32(3149642683u,3149642683u,3149642683u,3149642683u), make_v128_i32(19088743u,2309737967u,4275878552u,1985229328u))->to_v128() == make_v128_i32(3132799674u,3132799674u,2880154539u,2880154539u));
   CHECK(bkend.call_with_return("env", "bitselect", make_v128_i32(2863311530u,2863311530u,2863311530u,2863311530u), make_v128_i32(1431655765u,1431655765u,1431655765u,1431655765u), make_v128_i32(19088743u,2309737967u,4275878552u,1985229328u))->to_v128() == make_v128_i32(1417023538u,3707672762u,2877943757u,587294533u));
   CHECK(bkend.call_with_return("env", "bitselect", make_v128_i32(2863311530u,2863311530u,2863311530u,2863311530u), make_v128_i32(1431655765u,1431655765u,1431655765u,1431655765u), make_v128_i32(1431655765u,2863311530u,0u,4294967295u))->to_v128() == make_v128_i32(0u,4294967295u,1431655765u,2863311530u));
   CHECK(bkend.call_with_return("env", "bitselect", make_v128_i32(1234567890u,1234567890u,1234567890u,1234567890u), make_v128_i32(3060399406u,3060399406u,3060399406u,3060399406u), make_v128_i32(3455045103u,3455045103u,3455045103u,3455045103u))->to_v128() == make_v128_i32(2072391874u,2072391874u,2072391874u,2072391874u));
   CHECK(bkend.call_with_return("env", "bitselect", make_v128_i32(305419896u,305419896u,305419896u,305419896u), make_v128_i32(2427178479u,2427178479u,2427178479u,2427178479u), make_v128_i32(3455045103u,3455045103u,3455045103u,3455045103u))->to_v128() == make_v128_i32(270812264u,270812264u,270812264u,270812264u));
   CHECK(bkend.call_with_return("env", "andnot", make_v128_i32(0u,0u,4294967295u,4294967295u), make_v128_i32(0u,4294967295u,0u,4294967295u))->to_v128() == make_v128_i32(0u,0u,4294967295u,0u));
   CHECK(bkend.call_with_return("env", "andnot", make_v128_i32(0u,0u,0u,0u), make_v128_i32(0u,0u,0u,0u))->to_v128() == make_v128_i32(0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "andnot", make_v128_i32(0u,0u,0u,0u), make_v128_i32(4294967295u,4294967295u,4294967295u,4294967295u))->to_v128() == make_v128_i32(0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "andnot", make_v128_i32(0u,0u,0u,0u), make_v128_i32(4294967295u,4294967295u,4294967295u,4294967295u))->to_v128() == make_v128_i32(0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "andnot", make_v128_i32(1u,1u,1u,1u), make_v128_i32(1u,1u,1u,1u))->to_v128() == make_v128_i32(0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "andnot", make_v128_i32(255u,255u,255u,255u), make_v128_i32(85u,85u,85u,85u))->to_v128() == make_v128_i32(170u,170u,170u,170u));
   CHECK(bkend.call_with_return("env", "andnot", make_v128_i32(255u,255u,255u,255u), make_v128_i32(128u,128u,128u,128u))->to_v128() == make_v128_i32(127u,127u,127u,127u));
   CHECK(bkend.call_with_return("env", "andnot", make_v128_i32(2863311530u,2863311530u,2863311530u,2863311530u), make_v128_i32(10u,128u,5u,165u))->to_v128() == make_v128_i32(2863311520u,2863311402u,2863311530u,2863311370u));
   CHECK(bkend.call_with_return("env", "andnot", make_v128_i32(4294967295u,4294967295u,4294967295u,4294967295u), make_v128_i32(1431655765u,1431655765u,1431655765u,1431655765u))->to_v128() == make_v128_i32(2863311530u,2863311530u,2863311530u,2863311530u));
   CHECK(bkend.call_with_return("env", "andnot", make_v128_i32(4294967295u,4294967295u,4294967295u,4294967295u), make_v128_i32(2863311530u,2863311530u,2863311530u,2863311530u))->to_v128() == make_v128_i32(1431655765u,1431655765u,1431655765u,1431655765u));
   CHECK(bkend.call_with_return("env", "andnot", make_v128_i32(4294967295u,4294967295u,4294967295u,4294967295u), make_v128_i32(0u,0u,0u,0u))->to_v128() == make_v128_i32(4294967295u,4294967295u,4294967295u,4294967295u));
   CHECK(bkend.call_with_return("env", "andnot", make_v128_i32(1431655765u,1431655765u,1431655765u,1431655765u), make_v128_i32(21845u,65535u,22015u,24575u))->to_v128() == make_v128_i32(1431633920u,1431633920u,1431633920u,1431633920u));
   CHECK(bkend.call_with_return("env", "andnot", make_v128_i32(1234567890u,1234567890u,1234567890u,1234567890u), make_v128_i32(1234567890u,1234567890u,1234567890u,1234567890u))->to_v128() == make_v128_i32(0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "andnot", make_v128_i32(305419896u,305419896u,305419896u,305419896u), make_v128_i32(2427178479u,2427178479u,2427178479u,2427178479u))->to_v128() == make_v128_i32(34869776u,34869776u,34869776u,34869776u));
   CHECK(bkend.call_with_return("env", "not", make_v128_f32(4290772992u,4290772992u,4290772992u,4290772992u))->to_v128() == make_v128_f32(4194303u,4194303u,4194303u,4194303u));
   CHECK(bkend.call_with_return("env", "not", make_v128_f32(2143289344u,2143289344u,2143289344u,2143289344u))->to_v128() == make_v128_f32(2151677951u,2151677951u,2151677951u,2151677951u));
   CHECK(bkend.call_with_return("env", "not", make_v128_f32(4286578688u,4286578688u,4286578688u,4286578688u))->to_v128() == make_v128_i32(8388607u,8388607u,8388607u,8388607u));
   CHECK(bkend.call_with_return("env", "not", make_v128_f32(2139095040u,2139095040u,2139095040u,2139095040u))->to_v128() == make_v128_i32(2155872255u,2155872255u,2155872255u,2155872255u));
   CHECK(bkend.call_with_return("env", "and", make_v128_f32(4290772992u,4290772992u,4290772992u,4290772992u), make_v128_f32(4290772992u,4290772992u,4290772992u,4290772992u))->to_v128() == make_v128_i32(4290772992u,4290772992u,4290772992u,4290772992u));
   CHECK(bkend.call_with_return("env", "and", make_v128_f32(4290772992u,4290772992u,4290772992u,4290772992u), make_v128_f32(2143289344u,2143289344u,2143289344u,2143289344u))->to_v128() == make_v128_f32(2143289344u,2143289344u,2143289344u,2143289344u));
   CHECK(bkend.call_with_return("env", "and", make_v128_f32(4290772992u,4290772992u,4290772992u,4290772992u), make_v128_f32(4286578688u,4286578688u,4286578688u,4286578688u))->to_v128() == make_v128_f32(4286578688u,4286578688u,4286578688u,4286578688u));
   CHECK(bkend.call_with_return("env", "and", make_v128_f32(4290772992u,4290772992u,4290772992u,4290772992u), make_v128_f32(2139095040u,2139095040u,2139095040u,2139095040u))->to_v128() == make_v128_f32(2139095040u,2139095040u,2139095040u,2139095040u));
   CHECK(bkend.call_with_return("env", "and", make_v128_f32(2143289344u,2143289344u,2143289344u,2143289344u), make_v128_f32(2143289344u,2143289344u,2143289344u,2143289344u))->to_v128() == make_v128_f32(2143289344u,2143289344u,2143289344u,2143289344u));
   CHECK(bkend.call_with_return("env", "and", make_v128_f32(2143289344u,2143289344u,2143289344u,2143289344u), make_v128_f32(4286578688u,4286578688u,4286578688u,4286578688u))->to_v128() == make_v128_f32(2139095040u,2139095040u,2139095040u,2139095040u));
   CHECK(bkend.call_with_return("env", "and", make_v128_f32(2143289344u,2143289344u,2143289344u,2143289344u), make_v128_f32(2139095040u,2139095040u,2139095040u,2139095040u))->to_v128() == make_v128_f32(2139095040u,2139095040u,2139095040u,2139095040u));
   CHECK(bkend.call_with_return("env", "and", make_v128_f32(4286578688u,4286578688u,4286578688u,4286578688u), make_v128_f32(4286578688u,4286578688u,4286578688u,4286578688u))->to_v128() == make_v128_f32(4286578688u,4286578688u,4286578688u,4286578688u));
   CHECK(bkend.call_with_return("env", "and", make_v128_f32(4286578688u,4286578688u,4286578688u,4286578688u), make_v128_f32(2139095040u,2139095040u,2139095040u,2139095040u))->to_v128() == make_v128_f32(2139095040u,2139095040u,2139095040u,2139095040u));
   CHECK(bkend.call_with_return("env", "and", make_v128_f32(2139095040u,2139095040u,2139095040u,2139095040u), make_v128_f32(2139095040u,2139095040u,2139095040u,2139095040u))->to_v128() == make_v128_f32(2139095040u,2139095040u,2139095040u,2139095040u));
   CHECK(bkend.call_with_return("env", "or", make_v128_f32(4290772992u,4290772992u,4290772992u,4290772992u), make_v128_f32(4290772992u,4290772992u,4290772992u,4290772992u))->to_v128() == make_v128_i32(4290772992u,4290772992u,4290772992u,4290772992u));
   CHECK(bkend.call_with_return("env", "or", make_v128_f32(4290772992u,4290772992u,4290772992u,4290772992u), make_v128_f32(2143289344u,2143289344u,2143289344u,2143289344u))->to_v128() == make_v128_i32(4290772992u,4290772992u,4290772992u,4290772992u));
   CHECK(bkend.call_with_return("env", "or", make_v128_f32(4290772992u,4290772992u,4290772992u,4290772992u), make_v128_f32(4286578688u,4286578688u,4286578688u,4286578688u))->to_v128() == make_v128_i32(4290772992u,4290772992u,4290772992u,4290772992u));
   CHECK(bkend.call_with_return("env", "or", make_v128_f32(4290772992u,4290772992u,4290772992u,4290772992u), make_v128_f32(2139095040u,2139095040u,2139095040u,2139095040u))->to_v128() == make_v128_i32(4290772992u,4290772992u,4290772992u,4290772992u));
   CHECK(bkend.call_with_return("env", "or", make_v128_f32(2143289344u,2143289344u,2143289344u,2143289344u), make_v128_f32(2143289344u,2143289344u,2143289344u,2143289344u))->to_v128() == make_v128_f32(2143289344u,2143289344u,2143289344u,2143289344u));
   CHECK(bkend.call_with_return("env", "or", make_v128_f32(2143289344u,2143289344u,2143289344u,2143289344u), make_v128_f32(4286578688u,4286578688u,4286578688u,4286578688u))->to_v128() == make_v128_i32(4290772992u,4290772992u,4290772992u,4290772992u));
   CHECK(bkend.call_with_return("env", "or", make_v128_f32(2143289344u,2143289344u,2143289344u,2143289344u), make_v128_f32(2139095040u,2139095040u,2139095040u,2139095040u))->to_v128() == make_v128_f32(2143289344u,2143289344u,2143289344u,2143289344u));
   CHECK(bkend.call_with_return("env", "or", make_v128_f32(4286578688u,4286578688u,4286578688u,4286578688u), make_v128_f32(4286578688u,4286578688u,4286578688u,4286578688u))->to_v128() == make_v128_f32(4286578688u,4286578688u,4286578688u,4286578688u));
   CHECK(bkend.call_with_return("env", "or", make_v128_f32(4286578688u,4286578688u,4286578688u,4286578688u), make_v128_f32(2139095040u,2139095040u,2139095040u,2139095040u))->to_v128() == make_v128_f32(4286578688u,4286578688u,4286578688u,4286578688u));
   CHECK(bkend.call_with_return("env", "or", make_v128_f32(2139095040u,2139095040u,2139095040u,2139095040u), make_v128_f32(2139095040u,2139095040u,2139095040u,2139095040u))->to_v128() == make_v128_f32(2139095040u,2139095040u,2139095040u,2139095040u));
   CHECK(bkend.call_with_return("env", "xor", make_v128_f32(4290772992u,4290772992u,4290772992u,4290772992u), make_v128_f32(4290772992u,4290772992u,4290772992u,4290772992u))->to_v128() == make_v128_f32(0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "xor", make_v128_f32(4290772992u,4290772992u,4290772992u,4290772992u), make_v128_f32(2143289344u,2143289344u,2143289344u,2143289344u))->to_v128() == make_v128_f32(2147483648u,2147483648u,2147483648u,2147483648u));
   CHECK(bkend.call_with_return("env", "xor", make_v128_f32(4290772992u,4290772992u,4290772992u,4290772992u), make_v128_f32(4286578688u,4286578688u,4286578688u,4286578688u))->to_v128() == make_v128_i32(4194304u,4194304u,4194304u,4194304u));
   CHECK(bkend.call_with_return("env", "xor", make_v128_f32(4290772992u,4290772992u,4290772992u,4290772992u), make_v128_f32(2139095040u,2139095040u,2139095040u,2139095040u))->to_v128() == make_v128_i32(2151677952u,2151677952u,2151677952u,2151677952u));
   CHECK(bkend.call_with_return("env", "xor", make_v128_f32(2143289344u,2143289344u,2143289344u,2143289344u), make_v128_f32(2143289344u,2143289344u,2143289344u,2143289344u))->to_v128() == make_v128_f32(0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "xor", make_v128_f32(2143289344u,2143289344u,2143289344u,2143289344u), make_v128_f32(4286578688u,4286578688u,4286578688u,4286578688u))->to_v128() == make_v128_i32(2151677952u,2151677952u,2151677952u,2151677952u));
   CHECK(bkend.call_with_return("env", "xor", make_v128_f32(2143289344u,2143289344u,2143289344u,2143289344u), make_v128_f32(2139095040u,2139095040u,2139095040u,2139095040u))->to_v128() == make_v128_i32(4194304u,4194304u,4194304u,4194304u));
   CHECK(bkend.call_with_return("env", "xor", make_v128_f32(4286578688u,4286578688u,4286578688u,4286578688u), make_v128_f32(4286578688u,4286578688u,4286578688u,4286578688u))->to_v128() == make_v128_f32(0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "xor", make_v128_f32(4286578688u,4286578688u,4286578688u,4286578688u), make_v128_f32(2139095040u,2139095040u,2139095040u,2139095040u))->to_v128() == make_v128_i32(2147483648u,2147483648u,2147483648u,2147483648u));
   CHECK(bkend.call_with_return("env", "xor", make_v128_f32(2139095040u,2139095040u,2139095040u,2139095040u), make_v128_f32(2139095040u,2139095040u,2139095040u,2139095040u))->to_v128() == make_v128_f32(0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "bitselect", make_v128_f32(4290772992u,4290772992u,4290772992u,4290772992u), make_v128_f32(4290772992u,4290772992u,4290772992u,4290772992u), make_v128_f32(1327867302u,1327867302u,1327867302u,1327867302u))->to_v128() == make_v128_i32(4290772992u,4290772992u,4290772992u,4290772992u));
   CHECK(bkend.call_with_return("env", "bitselect", make_v128_f32(4290772992u,4290772992u,4290772992u,4290772992u), make_v128_f32(2143289344u,2143289344u,2143289344u,2143289344u), make_v128_f32(1327867302u,1327867302u,1327867302u,1327867302u))->to_v128() == make_v128_f32(2143289344u,2143289344u,2143289344u,2143289344u));
   CHECK(bkend.call_with_return("env", "bitselect", make_v128_f32(4290772992u,4290772992u,4290772992u,4290772992u), make_v128_f32(4286578688u,4286578688u,4286578688u,4286578688u), make_v128_f32(1327867302u,1327867302u,1327867302u,1327867302u))->to_v128() == make_v128_f32(4286578688u,4286578688u,4286578688u,4286578688u));
   CHECK(bkend.call_with_return("env", "bitselect", make_v128_f32(4290772992u,4290772992u,4290772992u,4290772992u), make_v128_f32(2139095040u,2139095040u,2139095040u,2139095040u), make_v128_f32(1327867302u,1327867302u,1327867302u,1327867302u))->to_v128() == make_v128_f32(2139095040u,2139095040u,2139095040u,2139095040u));
   CHECK(bkend.call_with_return("env", "bitselect", make_v128_f32(2143289344u,2143289344u,2143289344u,2143289344u), make_v128_f32(2143289344u,2143289344u,2143289344u,2143289344u), make_v128_f32(1327867302u,1327867302u,1327867302u,1327867302u))->to_v128() == make_v128_f32(2143289344u,2143289344u,2143289344u,2143289344u));
   CHECK(bkend.call_with_return("env", "bitselect", make_v128_f32(2143289344u,2143289344u,2143289344u,2143289344u), make_v128_f32(4286578688u,4286578688u,4286578688u,4286578688u), make_v128_f32(1327867302u,1327867302u,1327867302u,1327867302u))->to_v128() == make_v128_f32(4286578688u,4286578688u,4286578688u,4286578688u));
   CHECK(bkend.call_with_return("env", "bitselect", make_v128_f32(2143289344u,2143289344u,2143289344u,2143289344u), make_v128_f32(2139095040u,2139095040u,2139095040u,2139095040u), make_v128_f32(1327867302u,1327867302u,1327867302u,1327867302u))->to_v128() == make_v128_f32(2139095040u,2139095040u,2139095040u,2139095040u));
   CHECK(bkend.call_with_return("env", "bitselect", make_v128_f32(4286578688u,4286578688u,4286578688u,4286578688u), make_v128_f32(4286578688u,4286578688u,4286578688u,4286578688u), make_v128_f32(1327867302u,1327867302u,1327867302u,1327867302u))->to_v128() == make_v128_f32(4286578688u,4286578688u,4286578688u,4286578688u));
   CHECK(bkend.call_with_return("env", "bitselect", make_v128_f32(4286578688u,4286578688u,4286578688u,4286578688u), make_v128_f32(2139095040u,2139095040u,2139095040u,2139095040u), make_v128_f32(1327867302u,1327867302u,1327867302u,1327867302u))->to_v128() == make_v128_f32(2139095040u,2139095040u,2139095040u,2139095040u));
   CHECK(bkend.call_with_return("env", "bitselect", make_v128_f32(2139095040u,2139095040u,2139095040u,2139095040u), make_v128_f32(2139095040u,2139095040u,2139095040u,2139095040u), make_v128_f32(1327867302u,1327867302u,1327867302u,1327867302u))->to_v128() == make_v128_f32(2139095040u,2139095040u,2139095040u,2139095040u));
   CHECK(bkend.call_with_return("env", "andnot", make_v128_f32(4290772992u,4290772992u,4290772992u,4290772992u), make_v128_f32(4290772992u,4290772992u,4290772992u,4290772992u))->to_v128() == make_v128_i32(0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "andnot", make_v128_f32(4290772992u,4290772992u,4290772992u,4290772992u), make_v128_f32(2143289344u,2143289344u,2143289344u,2143289344u))->to_v128() == make_v128_f32(2147483648u,2147483648u,2147483648u,2147483648u));
   CHECK(bkend.call_with_return("env", "andnot", make_v128_f32(4290772992u,4290772992u,4290772992u,4290772992u), make_v128_f32(4286578688u,4286578688u,4286578688u,4286578688u))->to_v128() == make_v128_i32(4194304u,4194304u,4194304u,4194304u));
   CHECK(bkend.call_with_return("env", "andnot", make_v128_f32(4290772992u,4290772992u,4290772992u,4290772992u), make_v128_f32(2139095040u,2139095040u,2139095040u,2139095040u))->to_v128() == make_v128_i32(2151677952u,2151677952u,2151677952u,2151677952u));
   CHECK(bkend.call_with_return("env", "andnot", make_v128_f32(2143289344u,2143289344u,2143289344u,2143289344u), make_v128_f32(2143289344u,2143289344u,2143289344u,2143289344u))->to_v128() == make_v128_f32(0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "andnot", make_v128_f32(2143289344u,2143289344u,2143289344u,2143289344u), make_v128_f32(4286578688u,4286578688u,4286578688u,4286578688u))->to_v128() == make_v128_i32(4194304u,4194304u,4194304u,4194304u));
   CHECK(bkend.call_with_return("env", "andnot", make_v128_f32(2143289344u,2143289344u,2143289344u,2143289344u), make_v128_f32(2139095040u,2139095040u,2139095040u,2139095040u))->to_v128() == make_v128_i32(4194304u,4194304u,4194304u,4194304u));
   CHECK(bkend.call_with_return("env", "andnot", make_v128_f32(4286578688u,4286578688u,4286578688u,4286578688u), make_v128_f32(4286578688u,4286578688u,4286578688u,4286578688u))->to_v128() == make_v128_f32(0u,0u,0u,0u));
   CHECK(bkend.call_with_return("env", "andnot", make_v128_f32(4286578688u,4286578688u,4286578688u,4286578688u), make_v128_f32(2139095040u,2139095040u,2139095040u,2139095040u))->to_v128() == make_v128_i32(2147483648u,2147483648u,2147483648u,2147483648u));
   CHECK(bkend.call_with_return("env", "andnot", make_v128_f32(2139095040u,2139095040u,2139095040u,2139095040u), make_v128_f32(2139095040u,2139095040u,2139095040u,2139095040u))->to_v128() == make_v128_i32(0u,0u,0u,0u));
}

BACKEND_TEST_CASE( "Testing wasm <simd_bitwise_1_wasm>", "[simd_bitwise_1_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_bitwise.1.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_bitwise_10_wasm>", "[simd_bitwise_10_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_bitwise.10.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_bitwise_11_wasm>", "[simd_bitwise_11_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_bitwise.11.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_bitwise_12_wasm>", "[simd_bitwise_12_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_bitwise.12.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_bitwise_13_wasm>", "[simd_bitwise_13_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_bitwise.13.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_bitwise_14_wasm>", "[simd_bitwise_14_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_bitwise.14.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_bitwise_15_wasm>", "[simd_bitwise_15_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_bitwise.15.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_bitwise_16_wasm>", "[simd_bitwise_16_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_bitwise.16.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_bitwise_17_wasm>", "[simd_bitwise_17_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_bitwise.17.wasm");
   backend_t bkend( code, &wa );

   CHECK(!bkend.call_with_return("env", "v128.not-in-block"));
   CHECK(!bkend.call_with_return("env", "v128.and-in-block"));
   CHECK(!bkend.call_with_return("env", "v128.or-in-block"));
   CHECK(!bkend.call_with_return("env", "v128.xor-in-block"));
   CHECK(!bkend.call_with_return("env", "v128.bitselect-in-block"));
   CHECK(!bkend.call_with_return("env", "v128.andnot-in-block"));
   CHECK(!bkend.call_with_return("env", "nested-v128.not"));
   CHECK(!bkend.call_with_return("env", "nested-v128.and"));
   CHECK(!bkend.call_with_return("env", "nested-v128.or"));
   CHECK(!bkend.call_with_return("env", "nested-v128.xor"));
   CHECK(!bkend.call_with_return("env", "nested-v128.bitselect"));
   CHECK(!bkend.call_with_return("env", "nested-v128.andnot"));
   CHECK(!bkend.call_with_return("env", "as-param"));
}

BACKEND_TEST_CASE( "Testing wasm <simd_bitwise_18_wasm>", "[simd_bitwise_18_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_bitwise.18.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_bitwise_19_wasm>", "[simd_bitwise_19_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_bitwise.19.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_bitwise_2_wasm>", "[simd_bitwise_2_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_bitwise.2.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_bitwise_20_wasm>", "[simd_bitwise_20_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_bitwise.20.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_bitwise_21_wasm>", "[simd_bitwise_21_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_bitwise.21.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_bitwise_22_wasm>", "[simd_bitwise_22_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_bitwise.22.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_bitwise_23_wasm>", "[simd_bitwise_23_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_bitwise.23.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_bitwise_24_wasm>", "[simd_bitwise_24_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_bitwise.24.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_bitwise_25_wasm>", "[simd_bitwise_25_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_bitwise.25.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_bitwise_26_wasm>", "[simd_bitwise_26_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_bitwise.26.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_bitwise_27_wasm>", "[simd_bitwise_27_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_bitwise.27.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_bitwise_28_wasm>", "[simd_bitwise_28_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_bitwise.28.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_bitwise_29_wasm>", "[simd_bitwise_29_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_bitwise.29.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_bitwise_3_wasm>", "[simd_bitwise_3_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_bitwise.3.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_bitwise_4_wasm>", "[simd_bitwise_4_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_bitwise.4.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_bitwise_5_wasm>", "[simd_bitwise_5_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_bitwise.5.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_bitwise_6_wasm>", "[simd_bitwise_6_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_bitwise.6.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_bitwise_7_wasm>", "[simd_bitwise_7_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_bitwise.7.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_bitwise_8_wasm>", "[simd_bitwise_8_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_bitwise.8.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_bitwise_9_wasm>", "[simd_bitwise_9_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_bitwise.9.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

