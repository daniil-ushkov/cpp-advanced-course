#pragma once

#include <array>

#include "variant_utils.h"
#include "variant_concepts.h"
#include "variant_get.h"

namespace v_impl {
template <bool Indexed, typename Visitor, typename... Variants, size_t... Is>
constexpr auto build_vtable(std::index_sequence<Is...>) noexcept {
  if constexpr (Indexed) {
    struct indexed_dispatcher {
      static constexpr decltype(auto) dispatch(Visitor visitor, Variants...) {
        return std::forward<Visitor>(visitor)(in_place_index<Is>...);
      }
    };

    return &indexed_dispatcher::dispatch;
  } else {
    struct dispatcher {
      static constexpr decltype(auto) dispatch(Visitor visitor, Variants... variants) {
        return std::forward<Visitor>(visitor)(get<Is>(std::forward<Variants>(variants))...);
      }
    };

    return &dispatcher::dispatch;
  }
}

template <bool Indexed, typename Visitor, typename... Variants, size_t... Is, size_t... Js, typename... Ls>
constexpr auto build_vtable(std::index_sequence<Is...>, std::index_sequence<Js...>, Ls... ls) {
  return std::array{build_vtable<Indexed, Visitor, Variants...>(std::index_sequence<Is..., Js>{}, ls...)...};
}

template <bool Indexed, typename Visitor, typename... Variants>
constexpr auto build_vtable() {
  return build_vtable<Indexed, Visitor, Variants...>(
      std::index_sequence<>{}, std::make_index_sequence<variant_size_v<std::decay_t<Variants>>>{}...);
}

constexpr auto& at(auto& array) {
  return array;
}

template <typename Array, typename... Is>
constexpr auto& at(Array& array, size_t i, Is... is) {
  return at(array[i], is...);
}

template <bool Indexed, typename Visitor, typename... Variants>
constexpr auto vtable = v_impl::build_vtable<Indexed, Visitor, Variants...>();

template <typename Visitor, typename... Variants>
constexpr decltype(auto) visit_indexed(Visitor&& visitor, Variants&&... variants) {
  if ((variants.valueless_by_exception() || ...)) {
    throw bad_variant_access{};
  }

  return at(vtable<true, Visitor&&, Variants&&...>, variants.index()...)(std::forward<Visitor>(visitor),
                                                                           std::forward<Variants>(variants)...);
}

} // namespace v_impl

template <typename Visitor, typename... Variants>
constexpr decltype(auto) visit(Visitor&& visitor, Variants&&... variants)
    requires((v_impl::is_variant_v<std::remove_reference_t<Variants>>
           || v_impl::has_variant_base_v<Variants>) && ...) {
  using v_impl::at;
  using v_impl::vtable;

  if ((variants.valueless_by_exception() || ...)) {
    throw bad_variant_access{};
  }

  return at(vtable<false, Visitor&&, Variants&&...>, variants.index()...)(std::forward<Visitor>(visitor),
                                                                          std::forward<Variants>(variants)...);
}
