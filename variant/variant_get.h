#pragma once

#include "variant_utils.h"

// by index

template <size_t I, typename... Ts>
constexpr variant_alternative_t<I, variant<Ts...>>& get(variant<Ts...> &v) {
  if (v.index() != I) {
    throw bad_variant_access{};
  }

  return v.entries.template get<I>();
}

template <size_t I, typename... Ts>
constexpr const variant_alternative_t<I, variant<Ts...>>& get(const variant<Ts...> &v) {
  return get<I>(const_cast<variant<Ts...>&>(v));
}

template <size_t I, typename... Ts>
constexpr variant_alternative_t<I, variant<Ts...>>&& get(variant<Ts...> &&v) {
  if (v.index() != I) {
    throw bad_variant_access{};
  }

  return std::move(get<I>(v));
}

template <size_t I, typename... Ts>
constexpr const variant_alternative_t<I, variant<Ts...>>&& get(const variant<Ts...> &&v) {
  return get<I>(const_cast<variant<Ts...>&&>(v));
}

// by type

template <typename U, typename... Ts>
constexpr const U& get(const variant<Ts...> &v) {
  return get<v_impl::find_first<U, Ts...>::value>(v);
}

template <typename U, typename... Ts>
constexpr U& get(variant<Ts...> &v) {
  return get<v_impl::find_first<U, Ts...>::value>(v);
}

template <typename U, typename... Ts>
constexpr const U&& get(const variant<Ts...> &&v) {
  return get<v_impl::find_first<U, Ts...>::value>(v);
}

template <typename U, typename... Ts>
constexpr U&& get(variant<Ts...> &&v) {
  return get<v_impl::find_first<U, Ts...>::value>(v);
}

// get_if

template <size_t I, typename... Ts>
constexpr std::add_pointer_t<variant_alternative_t<I, variant<Ts...>>> get_if(variant<Ts...>* pv) noexcept {
  if (!pv || pv->index() != I) {
    return nullptr;
  }

  return std::addressof(get<I>(*pv));
}

template <size_t I, typename... Ts>
constexpr std::add_pointer_t<const variant_alternative_t<I, variant<Ts...>>> get_if(const variant<Ts...>* pv) noexcept {
  return get_if<I>(const_cast<variant<Ts...>*>(pv));
}

template <typename U, typename... Ts>
constexpr std::add_pointer_t<const U> get_if(const variant<Ts...>* pv) noexcept {
  return get_if<v_impl::find_first_v<U, Ts...>>(pv);
}

template <typename U, typename... Ts>
constexpr std::add_pointer_t<U> get_if(variant<Ts...>* pv) noexcept {
  return get_if<v_impl::find_first_v<U, Ts...>>(pv);
}
