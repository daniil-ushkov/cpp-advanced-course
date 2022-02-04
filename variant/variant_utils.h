#pragma once

#include <stdexcept>

#include "variant_traits.h"

template<typename... Ts>
class variant;

// in_place tags

template<size_t I>
struct in_place_index_t {};

template<size_t I>
inline constexpr in_place_index_t<I> in_place_index;

template<typename T>
struct in_place_type_t {};

template<typename T>
inline constexpr in_place_type_t<T> in_place_type;

inline constexpr size_t variant_npos = -1;

// variant_size

template<typename Variant>
struct variant_size {};

template<typename... Ts>
struct variant_size<variant<Ts...>> : std::integral_constant<size_t, sizeof...(Ts)> {};

template<typename... Ts>
struct variant_size<const variant<Ts...>> : std::integral_constant<size_t, sizeof...(Ts)> {};

template<typename Variant>
inline constexpr size_t variant_size_v = variant_size<Variant>::value;

// variant_alternative

template<size_t I, typename Variant>
struct variant_alternative;

template <size_t I, typename... Ts>
struct variant_alternative<I, variant<Ts...>> {
  using type = v_impl::nth_t<I, Ts...>;
};

template <size_t I, typename... Ts>
struct variant_alternative<I, const variant<Ts...>> {
  using type = std::add_const_t<v_impl::nth_t<I, Ts...>>;
};

template <size_t I, typename... Ts>
struct variant_alternative<I, volatile variant<Ts...>> {
  using type = std::add_volatile_t<v_impl::nth_t<I, Ts...>>;
};

template <size_t I, typename... Ts>
struct variant_alternative<I, const volatile variant<Ts...>> {
  using type = std::add_cv_t<v_impl::nth_t<I, Ts...>>;
};

template <size_t I, typename Variant>
using variant_alternative_t = typename variant_alternative<I, Variant>::type;

// holds_alternative

template<typename U, typename... Ts>
constexpr bool holds_alternative(const variant<Ts...>& v) noexcept {
  return v_impl::find_first_v<U, Ts...> == v.index();
}

// bad_variant_access

struct bad_variant_access : std::exception {};
