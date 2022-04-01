#pragma once

#include <cstdint>
#include <ostream>
#include <iomanip>

namespace eosio::vm {

   struct v128_t {
      std::uint64_t low,high;
      friend constexpr bool operator==(const v128_t& lhs, const v128_t& rhs) {
         return lhs.low == rhs.low && lhs.high == rhs.high;
      }
   };

   template<typename Ch, typename Traits>
   std::basic_ostream<Ch, Traits>& operator<<(std::basic_ostream<Ch, Traits>& os, v128_t val) {
      return os << std::hex << std::setfill('0') << std::setw(16) << val.high << std::setw(16) << val.low << std::dec << std::setfill(' ');
   }

}
