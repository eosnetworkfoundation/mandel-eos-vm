#pragma once

#include <cstdint>

namespace eosio::vm {

   struct v128_t {
      std::uint64_t low,high;
      friend constexpr bool operator==(const v128_t& lhs, const v128_t& rhs) {
         return lhs.low == rhs.low && lhs.high == rhs.high;
      }
   };

}
