#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include <eosio/vm/allocator.hpp>
#include <eosio/vm/stack_elem.hpp>
#include <eosio/vm/utils.hpp>

struct type_converter32 {
   union {
      uint32_t ui;
      float    f;
   } _data;
   type_converter32(uint32_t n) { _data.ui = n; }
   uint32_t to_ui() const { return _data.ui; }
   float    to_f() const { return _data.f; }
};

struct type_converter64 {
   union {
      uint64_t ui;
      double   f;
   } _data;
   type_converter64(uint64_t n) { _data.ui = n; }
   uint64_t to_ui() const { return _data.ui; }
   double   to_f() const { return _data.f; }
};

// C++20: using std::bit_cast;
template<typename T, typename U>
T bit_cast(const U& u) {
   static_assert(sizeof(T) == sizeof(U), "bitcast requires identical sizes.");
   T result;
   std::memcpy(&result, &u, sizeof(T));
   return result;
}

template<typename... T>
eosio::vm::v128_t make_v128_i8(T... x) {
   static_assert(sizeof...(T) == 16);
   uint8_t a[16] = {static_cast<uint8_t>(x)...};
   eosio::vm::v128_t result;
   memcpy(&result, &a, 16);
   return result;
}

template<typename... T>
eosio::vm::v128_t make_v128_i16(T... x) {
   static_assert(sizeof...(T) == 8);
   uint16_t a[8] = {static_cast<uint16_t>(x)...};
   eosio::vm::v128_t result;
   memcpy(&result, &a, 16);
   return result;
}

template<typename... T>
eosio::vm::v128_t make_v128_i32(T... x) {
   static_assert(sizeof...(T) == 4);
   uint32_t a[4] = {static_cast<uint32_t>(x)...};
   eosio::vm::v128_t result;
   memcpy(&result, &a, 16);
   return result;
}

template<typename... T>
eosio::vm::v128_t make_v128_i64(T... x) {
   static_assert(sizeof...(T) == 2);
   uint64_t a[2] = {static_cast<uint64_t>(x)...};
   eosio::vm::v128_t result;
   memcpy(&result, &a, 16);
   return result;
}

template<typename... T>
eosio::vm::v128_t make_v128_f32(T... x) {
   static_assert(sizeof...(T) == 4);
   uint32_t a[4] = {static_cast<uint32_t>(x)...};
   eosio::vm::v128_t result;
   memcpy(&result, &a, 16);
   return result;
}

template<typename... T>
eosio::vm::v128_t make_v128_f64(T... x) {
   static_assert(sizeof...(T) == 2);
   uint64_t a[2] = {static_cast<uint64_t>(x)...};
   eosio::vm::v128_t result;
   memcpy(&result, &a, 16);
   return result;
}

struct nan_arithmetic_t {};

inline std::ostream& operator<<(std::ostream& os, nan_arithmetic_t) {
   return os << "nan:arithmetic";
}

inline bool operator==(uint32_t arg, nan_arithmetic_t) {
   return (arg & 0x7fc00000u) == 0x7fc00000u;
}

inline bool operator==(uint64_t arg, nan_arithmetic_t) {
   return (arg & 0x7ff8000000000000u) == 0x7ff8000000000000u;
}

struct nan_canonical_t {};

inline std::ostream& operator<<(std::ostream& os, nan_canonical_t) {
   return os << "nan:canonical";
}

inline bool operator==(uint32_t arg, nan_canonical_t) {
   return (arg & 0x7fffffffu) == 0x7fc00000u;
}

inline bool operator==(uint64_t arg, nan_canonical_t) {
   return (arg & 0x7fffffffffffffffu) == 0x7ff8000000000000u;
}

template<typename... T>
struct v128_matcher {
   v128_matcher(T... t) : lanes(t...) {}
   std::tuple<T...> lanes;
};

template<typename... T>
std::ostream& operator<<(std::ostream& os, v128_matcher<T...> m) {
   os << "[";
   os << std::get<0>(m.lanes);
   os << "," << std::get<1>(m.lanes);
   if constexpr (sizeof... (T) > 2) {
      os << "," << std::get<2>(m.lanes);
      os << "," << std::get<3>(m.lanes);
   }
   os << "]";
   return os;
}

template<int N>
auto split_v128(eosio::vm::v128_t);

template<>
inline auto split_v128<2>(eosio::vm::v128_t arg) {
   std::uint64_t result[2];
   memcpy(&result, &arg, sizeof(arg));
   return std::tuple(result[0], result[1]);
}

template<>
inline auto split_v128<4>(eosio::vm::v128_t arg) {
   std::uint32_t result[4];
   memcpy(&result, &arg, sizeof(arg));
   return std::tuple(result[0], result[1], result[2], result[3]);
}

template<typename... T>
bool operator==(eosio::vm::v128_t vec, v128_matcher<T...> pattern) {
   return split_v128<sizeof...(T)>(vec) == pattern.lanes;
}

inline bool check_nan(const std::optional<eosio::vm::operand_stack_elem>& v) {
   return visit(eosio::vm::overloaded{[](eosio::vm::i32_const_t){ return false; },
                                      [](eosio::vm::i64_const_t){ return false; },
                                      [](eosio::vm::f32_const_t f) { return std::isnan(f.data.f); },
                                      [](eosio::vm::f64_const_t f) { return std::isnan(f.data.f); },
                                      [](eosio::vm::v128_const_t){ return false; }}, *v);
}

inline eosio::vm::wasm_allocator* get_wasm_allocator() {
   static eosio::vm::wasm_allocator alloc;
   return &alloc;
}

#if 0
#define BACKEND_TEST_CASE(name, tags) \
  TEMPLATE_TEST_CASE(name, tags, eosio::vm::interpreter, eosio::vm::jit)
#else
#define BACKEND_TEST_CASE(name, tags) \
  TEMPLATE_TEST_CASE(name, tags, eosio::vm::jit)
#endif
