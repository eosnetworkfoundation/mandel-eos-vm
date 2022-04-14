#pragma once

// temporarily use exceptions
#include <eosio/vm/exceptions.hpp>
#include <eosio/vm/repeat.hpp>

#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <limits>
#include <tuple>
#include <type_traits>
#include <variant>
#include <utility>

namespace eosio { namespace vm {

   // forward declaration
   template <typename... Alternatives>
   class variant;

   // implementation details
   namespace detail {

      constexpr std::size_t find_impl(std::initializer_list<bool> il) {
         std::size_t result = 0;
         for(auto iter = il.begin(), end = il.end(); iter != end && !*iter; ++iter, ++result) {}
         return result;
      }

      template <typename... Ts>
      constexpr std::size_t max_layout_size_v = std::max({sizeof(Ts)...});

      template <typename... Ts>
      constexpr std::size_t max_alignof_v = std::max({alignof(Ts)...});

      template <typename T, typename... Alternatives>
      constexpr bool is_valid_alternative_v = (... + (std::is_same_v<T, Alternatives>?1:0)) != 0;

      template <typename T, typename... Alternatives>
      constexpr std::size_t get_alternatives_index_v = find_impl({std::is_same_v<T, Alternatives>...});

      template <std::size_t I, typename... Alternatives>
      using get_alternative_t = std::tuple_element_t<I, std::tuple<Alternatives...>>;

      template <bool Valid, typename Ret>
      struct dispatcher;

      template <typename Ret>
      struct dispatcher<false, Ret> {
         template <std::size_t I, typename Vis, typename Var>
         static constexpr Ret _case(Vis&&, Var&&) {
            throw wasm_interpreter_exception("variant visit shouldn't be here");
         }
         template <std::size_t I, typename Vis, typename Var>
         static constexpr Ret _switch(Vis&&, Var&&) {
            throw wasm_interpreter_exception("variant visit shouldn't be here");
         }
      };

      template <typename Ret>
      struct dispatcher<true, Ret> {
         template <std::size_t I, typename Vis, typename Var>
         static constexpr Ret _case(Vis&& vis, Var&& var) {
            return std::invoke(std::forward<Vis>(vis), std::forward<Var>(var).template get<I>());
         }

         template <std::size_t I, typename Vis, typename Var>
         static constexpr Ret _switch(Vis&& vis, Var&& var) {
            constexpr std::size_t sz = std::decay_t<Var>::variant_size();
            switch (var.index()) {
#define C(n)                                                            \
               case I + n: {                                            \
                  return dispatcher<I + n < sz, Ret>::template _case<I + n>(std::forward<Vis>(vis), \
                                                                            std::forward<Var>(var)); \
               }
               EOS_VM_REPEAT_16(C)
#undef C
               default: {
                  return dispatcher<I + 16 < sz, Ret>::template _switch<I + 16>(std::forward<Vis>(vis),
                                                                                std::forward<Var>(var));
               }
            }
         }
      };

#define V_ELEM(N)                                                       \
      T##N _t##N;                                                       \
      constexpr variant_storage(const T##N& arg) : _t##N(arg) {}

      template<typename... T>
      union variant_storage;

      template<EOS_VM_ENUM_16(typename T), typename... T>
      union variant_storage<EOS_VM_ENUM_16(T), T...> {
         variant_storage() = default;
         EOS_VM_REPEAT_16(V_ELEM)
         template<typename A>
         constexpr variant_storage(const A& arg) : _tail{arg} {}
         variant_storage<T...> _tail;
      };

#define STORAGE(n)                                      \
      template<EOS_VM_ENUM_ ## n(typename T)>           \
      union variant_storage<EOS_VM_ENUM_ ## n(T)> {     \
         variant_storage() = default;                   \
         EOS_VM_REPEAT_ ## n(V_ELEM)                    \
      }

      STORAGE(1);
      STORAGE(2);
      STORAGE(3);
      STORAGE(4);
      STORAGE(5);
      STORAGE(6);
      STORAGE(7);
      STORAGE(8);
      STORAGE(9);
      STORAGE(10);
      STORAGE(11);
      STORAGE(12);
      STORAGE(13);
      STORAGE(14);
      STORAGE(15);
      STORAGE(16);

#undef STORAGE
#undef V_ELEM

      // Note: This method of organizing the getter requires O(n) total template
      // instantiations to instantiate the getters for every index, because
      // the memoized recursive call is shared.  Other methods require O(n^2)
      // instantiations.  This makes a big difference when there are 500
      // variant types.
      template<int I, typename Storage>
      constexpr decltype(auto) variant_storage_get_storage(Storage&& val) {
         if constexpr(I == 0) {
            return static_cast<Storage&&>(val);
         } else {
            return (variant_storage_get_storage<I - 1>(static_cast<Storage&&>(val))._tail);
         }
      }

      template<int I, typename Storage>
      constexpr decltype(auto) variant_storage_get(Storage& val) {
         decltype(auto) t = variant_storage_get_storage<I / 16>(val);
#define GET(n)                                  \
         if constexpr (I%16 == n) {             \
            return (t._t ## n);                 \
         } else
         EOS_VM_REPEAT_16(GET)
         {}
#undef GET
      }
      template<int I, typename Storage>
      constexpr decltype(auto) variant_storage_get(Storage&& val) {
         return std::move(variant_storage_get<I>(val));
      }
   } // namespace detail

   template <class Visitor, typename Variant>
   constexpr auto visit(Visitor&& vis, Variant&& var) {
      using Ret = decltype(std::invoke(std::forward<Visitor>(vis), var.template get<0>()));
      return detail::dispatcher<true, Ret>::template _switch<0>(std::forward<Visitor>(vis), std::forward<Variant>(var));
   }

   template <typename... Alternatives>
   class variant {
      static_assert(sizeof...(Alternatives) <= std::numeric_limits<uint16_t>::max()+1,
                    "eosio::vm::variant can only accept 65536 alternatives");
      static_assert((... && (std::is_trivially_copy_constructible_v<Alternatives> && std::is_trivially_move_constructible_v<Alternatives> &&
                    std::is_trivially_copy_assignable_v<Alternatives> && std::is_trivially_move_assignable_v<Alternatives> &&
                    std::is_trivially_destructible_v<Alternatives>)), "Variant requires trivial types");

    public:
      variant() = default;
      variant(const variant& other) = default;
      variant(variant&& other) = default;

      variant& operator=(const variant& other) = default;
      variant& operator=(variant&& other) = default;

      template <typename T, typename = std::enable_if_t<detail::is_valid_alternative_v<std::decay_t<T>, Alternatives...>>>
      constexpr variant(T&& alt) :
         _which(detail::get_alternatives_index_v<std::decay_t<T>, Alternatives...>),
         _storage(static_cast<T&&>(alt)) {
      }

      template <typename T,
                typename = std::enable_if_t<detail::is_valid_alternative_v<std::decay_t<T>, Alternatives...>>>
      constexpr variant& operator=(T&& alt) {
#if (defined(__GNUC__) && !defined(__clang__))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
         _storage = static_cast<T&&>(alt);
#pragma GCC diagnostic pop
#else
        _storage = static_cast<T&&>(alt);
#endif
         _which = detail::get_alternatives_index_v<std::decay_t<T>, Alternatives...>;
         return *this;
      }

      static inline constexpr size_t variant_size() { return sizeof...(Alternatives); }
      inline constexpr uint16_t      index() const { return _which; }

      template <size_t Index>
      inline constexpr const auto& get() const & {
         return detail::variant_storage_get<Index>(_storage);
      }

      template <typename Alt>
      inline constexpr const Alt& get() const & {
         return detail::variant_storage_get<detail::get_alternatives_index_v<Alt, Alternatives...>>(_storage);
      }

      template <size_t Index>
      inline constexpr const auto&& get() const && {
         return detail::variant_storage_get<Index>(std::move(_storage));
      }

      template <typename Alt>
      inline constexpr const Alt&& get() const && {
         return detail::variant_storage_get<detail::get_alternatives_index_v<Alt, Alternatives...>>(std::move(_storage));
      }

      template <size_t Index>
      inline constexpr auto&& get() && {
         return detail::variant_storage_get<Index>(std::move(_storage));
      }

      template <typename Alt>
      inline constexpr Alt&& get() && {
         return detail::variant_storage_get<detail::get_alternatives_index_v<Alt, Alternatives...>>(std::move(_storage));
      }

      template <size_t Index>
      inline constexpr auto& get() & {
         return detail::variant_storage_get<Index>(_storage);
      }

      template <typename Alt>
      inline constexpr Alt& get() & {
         return detail::variant_storage_get<detail::get_alternatives_index_v<Alt, Alternatives...>>(_storage);
      }

      template <typename Alt>
      inline constexpr bool is_a() const {
         return _which == detail::get_alternatives_index_v<Alt, Alternatives...>;
      }

    private:
      static constexpr size_t _sizeof  = detail::max_layout_size_v<Alternatives...>;
      static constexpr size_t _alignof = detail::max_alignof_v<Alternatives...>;
      uint16_t _which                  = 0;
      detail::variant_storage<Alternatives...> _storage;
   };

}} // namespace eosio::vm
