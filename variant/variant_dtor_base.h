#pragma once

#include "variant_recursive_union.h"
#include "variant_visit.h"

namespace v_impl {
// we need it as concepts doesn't work for destructors in clang
template <bool AllTriviallyDestructible, typename... Ts>
class dtor_base {};

template <typename... Ts>
class dtor_base<false, Ts...> {
public:
  constexpr dtor_base() = default;

  constexpr dtor_base(size_t index) : index(index) {}

  template <size_t I, typename... Us>
  constexpr dtor_base(in_place_index_t<I> ind, Us&&... us)
      : index(I), entries(ind, std::forward<Us>(us)...) {}

  constexpr ~dtor_base() {
    auto& this_variant = static_cast<variant<Ts...>&>(*this);
    if (!this_variant.valueless_by_exception()) {
      visit([]<typename T>(T& value) { value.~T(); }, this_variant);
    }
    index = variant_npos;
  }

  void make_valueless() {
    auto& this_variant = static_cast<variant<Ts...>&>(*this);
    if (index != variant_npos) {
      visit([]<typename T>(T& value) { value.~T(); }, this_variant);
    }
    index = variant_npos;
  }

protected:
  size_t index = 0;
  recursive_union<false, Ts...> entries;
};

template <typename... Ts>
class dtor_base<true, Ts...> {
public:
  constexpr dtor_base() = default;

  constexpr dtor_base(size_t index) : index(index) {}

  template <size_t I, typename... Us>
  constexpr dtor_base(in_place_index_t<I> ind, Us&&... us)
      : index(I), entries(ind, std::forward<Us>(us)...) {}

  constexpr ~dtor_base() = default;

  void make_valueless() {
    index = variant_npos;
  }

protected:
  size_t index = 0;
  recursive_union<true, Ts...> entries;
};
} // namespace v_impl
