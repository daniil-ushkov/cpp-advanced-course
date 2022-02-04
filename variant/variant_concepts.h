#pragma once

#include "variant_utils.h"

namespace v_impl {

// Copy constructor

template <typename... T>
concept all_copy_constructible = (std::is_copy_constructible_v<T> && ...);

template <typename... T>
concept all_trivially_copy_constructible = all_copy_constructible<T...> && (std::is_trivially_copy_constructible_v<T> && ...);

// Move constructor

template <typename... T>
concept all_move_constructible = (std::is_move_constructible_v<T> && ...);

template <typename... T>
concept all_trivially_move_constructible = all_move_constructible<T...> && (std::is_trivially_move_constructible_v<T> && ...);

// Converting constructor

template<typename U>
struct is_in_place_tag : std::false_type {};

template<size_t I>
struct is_in_place_tag<in_place_index_t<I>> : std::true_type {};

template<typename T>
struct is_in_place_tag<in_place_type_t<T>> : std::true_type {};

template<typename U>
inline constexpr bool is_in_place_tag_v = is_in_place_tag<U>::value;

// P0608R3

template<typename T, typename U>
concept no_narrowing = (!std::is_same_v<T, bool> || std::is_same_v<std::remove_cvref_t<U>, bool>)
&& requires(U&& t) {
  new T[1]{std::forward<U>(t)};
};

template<typename U, typename T, size_t Ind>
struct find_alternative_build_fun {
  static std::integral_constant<size_t, Ind> fun(T) requires(no_narrowing<T, U>);
};

template<typename U, typename Variant, typename Inds>
struct find_alternative_funs;

template<typename U, typename... Ts, size_t... Inds>
struct find_alternative_funs<U, variant<Ts...>, std::index_sequence<Inds...>> : find_alternative_build_fun<U, Ts, Inds>... {
  using find_alternative_build_fun<U, Ts, Inds>::fun...;
};

template<typename U, typename Variant>
using find_alternative_impl = decltype(find_alternative_funs<U, Variant,
                                       std::make_index_sequence<variant_size_v<Variant>>>::fun(std::declval<U>()));

template<typename U, typename Variant, typename = void>
struct find_alternative : std::integral_constant<size_t, not_found> {};

template<typename U, typename Variant>
struct find_alternative<U, Variant, std::void_t<find_alternative_impl<U, Variant>>>
    : find_alternative_impl<U, Variant> {};

template<typename U, typename Variant>
inline constexpr size_t find_alternative_v = find_alternative<U, Variant>::value;

// in_place_type constructor

template <typename U, typename... Ts>
struct count : std::integral_constant<size_t, 0> {};

template <typename U, typename... Ts>
struct count<U, U, Ts...> : std::integral_constant<size_t, count<U, Ts...>::value + 1> {};

template <typename U, typename T, typename... Ts>
struct count<U, T, Ts...> : std::integral_constant<size_t, count<U, Ts...>::value> {};

template <typename U, typename... Ts>
inline constexpr size_t count_v = count<U, Ts...>::value;

template <typename U, typename... Ts>
inline constexpr bool exactly_once = count_v<U, Ts...> == 1;

// Copy-assignment

template <typename... T>
concept all_copy_assignable = (std::is_copy_assignable_v<T> && ...);

template <typename... T>
concept all_trivially_copy_assignable = all_copy_assignable<T...> && (std::is_trivially_copy_assignable_v<T> && ...);

template <typename... T>
concept all_trivially_destructible = (std::is_trivially_destructible_v<T> && ...);

// Move-assignment

template <typename... T>
concept all_move_assignable = (std::is_move_assignable_v<T> && ...);

template <typename... T>
concept all_trivially_move_assignable = all_move_assignable<T...> && (std::is_trivially_move_assignable_v<T> && ...);

// Visitor

// is variant

template <typename Variant>
struct is_variant : std::false_type {};

template <typename... Ts>
struct is_variant<variant<Ts...>> : std::true_type {};

template <typename... Ts>
struct is_variant<const variant<Ts...>> : std::true_type {};

template <typename... Ts>
struct is_variant<volatile variant<Ts...>> : std::true_type {};

template <typename... Ts>
struct is_variant<const volatile variant<Ts...>> : std::true_type {};

template<typename Variant>
inline constexpr bool is_variant_v = is_variant<Variant>::value;

// has exactly one base which is a specialization of variant

template<typename... Ts>
std::true_type has_variant_base_fun(variant<Ts...>&);

template<typename... Ts>
std::true_type has_variant_base_fun(const variant<Ts...>&);

template <typename U>
using has_variant_base_impl = decltype(has_variant_base_fun(std::declval<U>()));

template <typename U, typename = void>
struct has_variant_base : std::false_type {};

template <typename U>
struct has_variant_base<U, std::void_t<has_variant_base_impl<U>>> : has_variant_base_impl<U> {};

template <typename U>
inline constexpr bool has_variant_base_v = has_variant_base<U>::value;
} // namespace v_impl
