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

BACKEND_TEST_CASE( "Testing wasm <simd_f64x2_rounding_0_wasm>", "[simd_f64x2_rounding_0_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_f64x2_rounding.0.wasm");
   backend_t bkend( code, &wa );

   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(0u,0u))->to_v128() == make_v128_f64(0u,0u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(9223372036854775808u,9223372036854775808u))->to_v128() == make_v128_f64(9223372036854775808u,9223372036854775808u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(4503599627370496u,4503599627370496u))->to_v128() == make_v128_f64(4607182418800017408u,4607182418800017408u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(9227875636482146304u,9227875636482146304u))->to_v128() == make_v128_f64(9223372036854775808u,9223372036854775808u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(4602678819172646912u,4602678819172646912u))->to_v128() == make_v128_f64(4607182418800017408u,4607182418800017408u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(13826050856027422720u,13826050856027422720u))->to_v128() == make_v128_f64(9223372036854775808u,9223372036854775808u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(4607182418800017408u,4607182418800017408u))->to_v128() == make_v128_f64(4607182418800017408u,4607182418800017408u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(13830554455654793216u,13830554455654793216u))->to_v128() == make_v128_f64(13830554455654793216u,13830554455654793216u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(4618760256179416344u,4618760256179416344u))->to_v128() == make_v128_f64(4619567317775286272u,4619567317775286272u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(13842132293034192152u,13842132293034192152u))->to_v128() == make_v128_f64(13841813454723219456u,13841813454723219456u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(9218868437227405311u,9218868437227405311u))->to_v128() == make_v128_f64(9218868437227405311u,9218868437227405311u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(18442240474082181119u,18442240474082181119u))->to_v128() == make_v128_f64(18442240474082181119u,18442240474082181119u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(1u,1u))->to_v128() == make_v128_f64(4607182418800017408u,4607182418800017408u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(1u,1u))->to_v128() == make_v128_f64(4607182418800017408u,4607182418800017408u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(9218868437227405312u,9218868437227405312u))->to_v128() == make_v128_f64(9218868437227405312u,9218868437227405312u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(18442240474082181120u,18442240474082181120u))->to_v128() == make_v128_f64(18442240474082181120u,18442240474082181120u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(4728057454347157504u,4728057454347157504u))->to_v128() == make_v128_f64(4728057454347157504u,4728057454347157504u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(5012481849648092922u,5012481849648092922u))->to_v128() == make_v128_f64(5012481849648092922u,5012481849648092922u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(5012481849648092922u,5012481849648092922u))->to_v128() == make_v128_f64(5012481849648092922u,5012481849648092922u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(4443687238071173905u,4443687238071173905u))->to_v128() == make_v128_f64(4607182418800017408u,4607182418800017408u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(4728057454347157504u,4728057454347157504u))->to_v128() == make_v128_f64(4728057454347157504u,4728057454347157504u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(5012481849648092922u,5012481849648092922u))->to_v128() == make_v128_f64(5012481849648092922u,5012481849648092922u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(5012481849648092922u,5012481849648092922u))->to_v128() == make_v128_f64(5012481849648092922u,5012481849648092922u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(4443687238071173905u,4443687238071173905u))->to_v128() == make_v128_f64(4607182418800017408u,4607182418800017408u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(4728057454347986008u,4728057454347986008u))->to_v128() == make_v128_f64(4728057454414266368u,4728057454414266368u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(5012481849648991189u,5012481849648991189u))->to_v128() == make_v128_f64(5012481849648991189u,5012481849648991189u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(5012481849648991189u,5012481849648991189u))->to_v128() == make_v128_f64(5012481849648991189u,5012481849648991189u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(4443687238071938066u,4443687238071938066u))->to_v128() == make_v128_f64(4607182418800017408u,4607182418800017408u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(4968090884938317023u,4968090884938317023u))->to_v128() == make_v128_f64(4968090884938317023u,4968090884938317023u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(5053659277858356447u,5053659277858356447u))->to_v128() == make_v128_f64(5053659277858356447u,5053659277858356447u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(5053659277858356447u,5053659277858356447u))->to_v128() == make_v128_f64(5053659277858356447u,5053659277858356447u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(4882522492018277599u,4882522492018277599u))->to_v128() == make_v128_f64(4882522492018277599u,4882522492018277599u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(4968090884938317023u,4968090884938317023u))->to_v128() == make_v128_f64(4968090884938317023u,4968090884938317023u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(5053659277858356447u,5053659277858356447u))->to_v128() == make_v128_f64(5053659277858356447u,5053659277858356447u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(5053659277858356447u,5053659277858356447u))->to_v128() == make_v128_f64(5053659277858356447u,5053659277858356447u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(4882522492018277599u,4882522492018277599u))->to_v128() == make_v128_f64(4882522492018277599u,4882522492018277599u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(4968090884938317023u,4968090884938317023u))->to_v128() == make_v128_f64(4968090884938317023u,4968090884938317023u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(5053659277858356447u,5053659277858356447u))->to_v128() == make_v128_f64(5053659277858356447u,5053659277858356447u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(5053659277858356447u,5053659277858356447u))->to_v128() == make_v128_f64(5053659277858356447u,5053659277858356447u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(4882522492018277599u,4882522492018277599u))->to_v128() == make_v128_f64(4882522492018277599u,4882522492018277599u));
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(9221120237041090560u,9221120237041090560u))->to_v128() == v128_matcher{nan_canonical_t{},nan_canonical_t{}});
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(18444492273895866368u,18444492273895866368u))->to_v128() == v128_matcher{nan_canonical_t{},nan_canonical_t{}});
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(9219994337134247936u,9219994337134247936u))->to_v128() == v128_matcher{nan_arithmetic_t{},nan_arithmetic_t{}});
   CHECK(bkend.call_with_return("env", "f64x2.ceil", make_v128_f64(18443366373989023744u,18443366373989023744u))->to_v128() == v128_matcher{nan_arithmetic_t{},nan_arithmetic_t{}});
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(0u,0u))->to_v128() == make_v128_f64(0u,0u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(9223372036854775808u,9223372036854775808u))->to_v128() == make_v128_f64(9223372036854775808u,9223372036854775808u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(4503599627370496u,4503599627370496u))->to_v128() == make_v128_f64(0u,0u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(9227875636482146304u,9227875636482146304u))->to_v128() == make_v128_f64(13830554455654793216u,13830554455654793216u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(4602678819172646912u,4602678819172646912u))->to_v128() == make_v128_f64(0u,0u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(13826050856027422720u,13826050856027422720u))->to_v128() == make_v128_f64(13830554455654793216u,13830554455654793216u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(4607182418800017408u,4607182418800017408u))->to_v128() == make_v128_f64(4607182418800017408u,4607182418800017408u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(13830554455654793216u,13830554455654793216u))->to_v128() == make_v128_f64(13830554455654793216u,13830554455654793216u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(4618760256179416344u,4618760256179416344u))->to_v128() == make_v128_f64(4618441417868443648u,4618441417868443648u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(13842132293034192152u,13842132293034192152u))->to_v128() == make_v128_f64(13842939354630062080u,13842939354630062080u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(9218868437227405311u,9218868437227405311u))->to_v128() == make_v128_f64(9218868437227405311u,9218868437227405311u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(18442240474082181119u,18442240474082181119u))->to_v128() == make_v128_f64(18442240474082181119u,18442240474082181119u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(1u,1u))->to_v128() == make_v128_f64(0u,0u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(1u,1u))->to_v128() == make_v128_f64(0u,0u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(9218868437227405312u,9218868437227405312u))->to_v128() == make_v128_f64(9218868437227405312u,9218868437227405312u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(18442240474082181120u,18442240474082181120u))->to_v128() == make_v128_f64(18442240474082181120u,18442240474082181120u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(4728057454347157504u,4728057454347157504u))->to_v128() == make_v128_f64(4728057454347157504u,4728057454347157504u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(5012481849648092922u,5012481849648092922u))->to_v128() == make_v128_f64(5012481849648092922u,5012481849648092922u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(5012481849648092922u,5012481849648092922u))->to_v128() == make_v128_f64(5012481849648092922u,5012481849648092922u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(4443687238071173905u,4443687238071173905u))->to_v128() == make_v128_f64(0u,0u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(4728057454347157504u,4728057454347157504u))->to_v128() == make_v128_f64(4728057454347157504u,4728057454347157504u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(5012481849648092922u,5012481849648092922u))->to_v128() == make_v128_f64(5012481849648092922u,5012481849648092922u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(5012481849648092922u,5012481849648092922u))->to_v128() == make_v128_f64(5012481849648092922u,5012481849648092922u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(4443687238071173905u,4443687238071173905u))->to_v128() == make_v128_f64(0u,0u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(4728057454347986008u,4728057454347986008u))->to_v128() == make_v128_f64(4728057454347157504u,4728057454347157504u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(5012481849648991189u,5012481849648991189u))->to_v128() == make_v128_f64(5012481849648991189u,5012481849648991189u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(5012481849648991189u,5012481849648991189u))->to_v128() == make_v128_f64(5012481849648991189u,5012481849648991189u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(4443687238071938066u,4443687238071938066u))->to_v128() == make_v128_f64(0u,0u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(4968090884938317023u,4968090884938317023u))->to_v128() == make_v128_f64(4968090884938317023u,4968090884938317023u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(5053659277858356447u,5053659277858356447u))->to_v128() == make_v128_f64(5053659277858356447u,5053659277858356447u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(5053659277858356447u,5053659277858356447u))->to_v128() == make_v128_f64(5053659277858356447u,5053659277858356447u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(4882522492018277599u,4882522492018277599u))->to_v128() == make_v128_f64(4882522492018277599u,4882522492018277599u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(4968090884938317023u,4968090884938317023u))->to_v128() == make_v128_f64(4968090884938317023u,4968090884938317023u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(5053659277858356447u,5053659277858356447u))->to_v128() == make_v128_f64(5053659277858356447u,5053659277858356447u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(5053659277858356447u,5053659277858356447u))->to_v128() == make_v128_f64(5053659277858356447u,5053659277858356447u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(4882522492018277599u,4882522492018277599u))->to_v128() == make_v128_f64(4882522492018277599u,4882522492018277599u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(4968090884938317023u,4968090884938317023u))->to_v128() == make_v128_f64(4968090884938317023u,4968090884938317023u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(5053659277858356447u,5053659277858356447u))->to_v128() == make_v128_f64(5053659277858356447u,5053659277858356447u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(5053659277858356447u,5053659277858356447u))->to_v128() == make_v128_f64(5053659277858356447u,5053659277858356447u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(4882522492018277599u,4882522492018277599u))->to_v128() == make_v128_f64(4882522492018277599u,4882522492018277599u));
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(9221120237041090560u,9221120237041090560u))->to_v128() == v128_matcher{nan_canonical_t{},nan_canonical_t{}});
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(18444492273895866368u,18444492273895866368u))->to_v128() == v128_matcher{nan_canonical_t{},nan_canonical_t{}});
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(9219994337134247936u,9219994337134247936u))->to_v128() == v128_matcher{nan_arithmetic_t{},nan_arithmetic_t{}});
   CHECK(bkend.call_with_return("env", "f64x2.floor", make_v128_f64(18443366373989023744u,18443366373989023744u))->to_v128() == v128_matcher{nan_arithmetic_t{},nan_arithmetic_t{}});
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(0u,0u))->to_v128() == make_v128_f64(0u,0u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(9223372036854775808u,9223372036854775808u))->to_v128() == make_v128_f64(9223372036854775808u,9223372036854775808u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(4503599627370496u,4503599627370496u))->to_v128() == make_v128_f64(0u,0u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(9227875636482146304u,9227875636482146304u))->to_v128() == make_v128_f64(9223372036854775808u,9223372036854775808u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(4602678819172646912u,4602678819172646912u))->to_v128() == make_v128_f64(0u,0u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(13826050856027422720u,13826050856027422720u))->to_v128() == make_v128_f64(9223372036854775808u,9223372036854775808u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(4607182418800017408u,4607182418800017408u))->to_v128() == make_v128_f64(4607182418800017408u,4607182418800017408u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(13830554455654793216u,13830554455654793216u))->to_v128() == make_v128_f64(13830554455654793216u,13830554455654793216u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(4618760256179416344u,4618760256179416344u))->to_v128() == make_v128_f64(4618441417868443648u,4618441417868443648u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(13842132293034192152u,13842132293034192152u))->to_v128() == make_v128_f64(13841813454723219456u,13841813454723219456u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(9218868437227405311u,9218868437227405311u))->to_v128() == make_v128_f64(9218868437227405311u,9218868437227405311u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(18442240474082181119u,18442240474082181119u))->to_v128() == make_v128_f64(18442240474082181119u,18442240474082181119u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(1u,1u))->to_v128() == make_v128_f64(0u,0u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(1u,1u))->to_v128() == make_v128_f64(0u,0u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(9218868437227405312u,9218868437227405312u))->to_v128() == make_v128_f64(9218868437227405312u,9218868437227405312u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(18442240474082181120u,18442240474082181120u))->to_v128() == make_v128_f64(18442240474082181120u,18442240474082181120u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(4728057454347157504u,4728057454347157504u))->to_v128() == make_v128_f64(4728057454347157504u,4728057454347157504u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(5012481849648092922u,5012481849648092922u))->to_v128() == make_v128_f64(5012481849648092922u,5012481849648092922u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(5012481849648092922u,5012481849648092922u))->to_v128() == make_v128_f64(5012481849648092922u,5012481849648092922u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(4443687238071173905u,4443687238071173905u))->to_v128() == make_v128_f64(0u,0u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(4728057454347157504u,4728057454347157504u))->to_v128() == make_v128_f64(4728057454347157504u,4728057454347157504u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(5012481849648092922u,5012481849648092922u))->to_v128() == make_v128_f64(5012481849648092922u,5012481849648092922u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(5012481849648092922u,5012481849648092922u))->to_v128() == make_v128_f64(5012481849648092922u,5012481849648092922u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(4443687238071173905u,4443687238071173905u))->to_v128() == make_v128_f64(0u,0u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(4728057454347986008u,4728057454347986008u))->to_v128() == make_v128_f64(4728057454347157504u,4728057454347157504u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(5012481849648991189u,5012481849648991189u))->to_v128() == make_v128_f64(5012481849648991189u,5012481849648991189u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(5012481849648991189u,5012481849648991189u))->to_v128() == make_v128_f64(5012481849648991189u,5012481849648991189u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(4443687238071938066u,4443687238071938066u))->to_v128() == make_v128_f64(0u,0u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(4968090884938317023u,4968090884938317023u))->to_v128() == make_v128_f64(4968090884938317023u,4968090884938317023u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(5053659277858356447u,5053659277858356447u))->to_v128() == make_v128_f64(5053659277858356447u,5053659277858356447u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(5053659277858356447u,5053659277858356447u))->to_v128() == make_v128_f64(5053659277858356447u,5053659277858356447u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(4882522492018277599u,4882522492018277599u))->to_v128() == make_v128_f64(4882522492018277599u,4882522492018277599u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(4968090884938317023u,4968090884938317023u))->to_v128() == make_v128_f64(4968090884938317023u,4968090884938317023u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(5053659277858356447u,5053659277858356447u))->to_v128() == make_v128_f64(5053659277858356447u,5053659277858356447u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(5053659277858356447u,5053659277858356447u))->to_v128() == make_v128_f64(5053659277858356447u,5053659277858356447u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(4882522492018277599u,4882522492018277599u))->to_v128() == make_v128_f64(4882522492018277599u,4882522492018277599u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(4968090884938317023u,4968090884938317023u))->to_v128() == make_v128_f64(4968090884938317023u,4968090884938317023u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(5053659277858356447u,5053659277858356447u))->to_v128() == make_v128_f64(5053659277858356447u,5053659277858356447u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(5053659277858356447u,5053659277858356447u))->to_v128() == make_v128_f64(5053659277858356447u,5053659277858356447u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(4882522492018277599u,4882522492018277599u))->to_v128() == make_v128_f64(4882522492018277599u,4882522492018277599u));
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(9221120237041090560u,9221120237041090560u))->to_v128() == v128_matcher{nan_canonical_t{},nan_canonical_t{}});
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(18444492273895866368u,18444492273895866368u))->to_v128() == v128_matcher{nan_canonical_t{},nan_canonical_t{}});
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(9219994337134247936u,9219994337134247936u))->to_v128() == v128_matcher{nan_arithmetic_t{},nan_arithmetic_t{}});
   CHECK(bkend.call_with_return("env", "f64x2.trunc", make_v128_f64(18443366373989023744u,18443366373989023744u))->to_v128() == v128_matcher{nan_arithmetic_t{},nan_arithmetic_t{}});
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(0u,0u))->to_v128() == make_v128_f64(0u,0u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(9223372036854775808u,9223372036854775808u))->to_v128() == make_v128_f64(9223372036854775808u,9223372036854775808u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(4503599627370496u,4503599627370496u))->to_v128() == make_v128_f64(0u,0u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(9227875636482146304u,9227875636482146304u))->to_v128() == make_v128_f64(9223372036854775808u,9223372036854775808u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(4602678819172646912u,4602678819172646912u))->to_v128() == make_v128_f64(0u,0u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(13826050856027422720u,13826050856027422720u))->to_v128() == make_v128_f64(9223372036854775808u,9223372036854775808u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(4607182418800017408u,4607182418800017408u))->to_v128() == make_v128_f64(4607182418800017408u,4607182418800017408u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(13830554455654793216u,13830554455654793216u))->to_v128() == make_v128_f64(13830554455654793216u,13830554455654793216u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(4618760256179416344u,4618760256179416344u))->to_v128() == make_v128_f64(4618441417868443648u,4618441417868443648u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(13842132293034192152u,13842132293034192152u))->to_v128() == make_v128_f64(13841813454723219456u,13841813454723219456u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(9218868437227405311u,9218868437227405311u))->to_v128() == make_v128_f64(9218868437227405311u,9218868437227405311u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(18442240474082181119u,18442240474082181119u))->to_v128() == make_v128_f64(18442240474082181119u,18442240474082181119u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(1u,1u))->to_v128() == make_v128_f64(0u,0u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(1u,1u))->to_v128() == make_v128_f64(0u,0u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(9218868437227405312u,9218868437227405312u))->to_v128() == make_v128_f64(9218868437227405312u,9218868437227405312u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(18442240474082181120u,18442240474082181120u))->to_v128() == make_v128_f64(18442240474082181120u,18442240474082181120u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(4728057454347157504u,4728057454347157504u))->to_v128() == make_v128_f64(4728057454347157504u,4728057454347157504u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(5012481849648092922u,5012481849648092922u))->to_v128() == make_v128_f64(5012481849648092922u,5012481849648092922u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(5012481849648092922u,5012481849648092922u))->to_v128() == make_v128_f64(5012481849648092922u,5012481849648092922u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(4443687238071173905u,4443687238071173905u))->to_v128() == make_v128_f64(0u,0u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(4728057454347157504u,4728057454347157504u))->to_v128() == make_v128_f64(4728057454347157504u,4728057454347157504u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(5012481849648092922u,5012481849648092922u))->to_v128() == make_v128_f64(5012481849648092922u,5012481849648092922u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(5012481849648092922u,5012481849648092922u))->to_v128() == make_v128_f64(5012481849648092922u,5012481849648092922u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(4443687238071173905u,4443687238071173905u))->to_v128() == make_v128_f64(0u,0u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(4728057454347986008u,4728057454347986008u))->to_v128() == make_v128_f64(4728057454347157504u,4728057454347157504u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(5012481849648991189u,5012481849648991189u))->to_v128() == make_v128_f64(5012481849648991189u,5012481849648991189u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(5012481849648991189u,5012481849648991189u))->to_v128() == make_v128_f64(5012481849648991189u,5012481849648991189u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(4443687238071938066u,4443687238071938066u))->to_v128() == make_v128_f64(0u,0u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(4968090884938317023u,4968090884938317023u))->to_v128() == make_v128_f64(4968090884938317023u,4968090884938317023u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(5053659277858356447u,5053659277858356447u))->to_v128() == make_v128_f64(5053659277858356447u,5053659277858356447u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(5053659277858356447u,5053659277858356447u))->to_v128() == make_v128_f64(5053659277858356447u,5053659277858356447u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(4882522492018277599u,4882522492018277599u))->to_v128() == make_v128_f64(4882522492018277599u,4882522492018277599u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(4968090884938317023u,4968090884938317023u))->to_v128() == make_v128_f64(4968090884938317023u,4968090884938317023u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(5053659277858356447u,5053659277858356447u))->to_v128() == make_v128_f64(5053659277858356447u,5053659277858356447u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(5053659277858356447u,5053659277858356447u))->to_v128() == make_v128_f64(5053659277858356447u,5053659277858356447u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(4882522492018277599u,4882522492018277599u))->to_v128() == make_v128_f64(4882522492018277599u,4882522492018277599u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(4968090884938317023u,4968090884938317023u))->to_v128() == make_v128_f64(4968090884938317023u,4968090884938317023u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(5053659277858356447u,5053659277858356447u))->to_v128() == make_v128_f64(5053659277858356447u,5053659277858356447u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(5053659277858356447u,5053659277858356447u))->to_v128() == make_v128_f64(5053659277858356447u,5053659277858356447u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(4882522492018277599u,4882522492018277599u))->to_v128() == make_v128_f64(4882522492018277599u,4882522492018277599u));
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(9221120237041090560u,9221120237041090560u))->to_v128() == v128_matcher{nan_canonical_t{},nan_canonical_t{}});
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(18444492273895866368u,18444492273895866368u))->to_v128() == v128_matcher{nan_canonical_t{},nan_canonical_t{}});
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(9219994337134247936u,9219994337134247936u))->to_v128() == v128_matcher{nan_arithmetic_t{},nan_arithmetic_t{}});
   CHECK(bkend.call_with_return("env", "f64x2.nearest", make_v128_f64(18443366373989023744u,18443366373989023744u))->to_v128() == v128_matcher{nan_arithmetic_t{},nan_arithmetic_t{}});
}

BACKEND_TEST_CASE( "Testing wasm <simd_f64x2_rounding_17_wasm>", "[simd_f64x2_rounding_17_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_f64x2_rounding.17.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_f64x2_rounding_18_wasm>", "[simd_f64x2_rounding_18_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_f64x2_rounding.18.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_f64x2_rounding_19_wasm>", "[simd_f64x2_rounding_19_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_f64x2_rounding.19.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_f64x2_rounding_20_wasm>", "[simd_f64x2_rounding_20_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_f64x2_rounding.20.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_f64x2_rounding_21_wasm>", "[simd_f64x2_rounding_21_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_f64x2_rounding.21.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_f64x2_rounding_22_wasm>", "[simd_f64x2_rounding_22_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_f64x2_rounding.22.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_f64x2_rounding_23_wasm>", "[simd_f64x2_rounding_23_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_f64x2_rounding.23.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <simd_f64x2_rounding_24_wasm>", "[simd_f64x2_rounding_24_wasm_tests]" ) {
   using backend_t = backend<standalone_function_t, TestType>;
   auto code = read_wasm( std::string(wasm_directory) + "simd_f64x2_rounding.24.wasm");
   CHECK_THROWS_AS(backend_t(code, nullptr), std::exception);
}

