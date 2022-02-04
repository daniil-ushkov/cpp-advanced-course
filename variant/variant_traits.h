#pragma once

#include <cstddef>
#include <type_traits>

namespace v_impl {

// `nth` retrieves nth type from pack

struct nothing {};

template <size_t I, typename... Ts>
struct nth {
  using type = nothing;
};

template <size_t I, typename T, typename... Ts>
struct nth<I, T, Ts...> {
  using type = typename nth<I - 1, Ts...>::type;
};

template <typename T, typename... Ts>
struct nth<0, T, Ts...> {
  using type = T;
};

template <size_t I, typename... Ts>
using nth_t = typename nth<I, Ts...>::type;

// finds first type occurrence in pack

inline constexpr size_t not_found = -1;

template<typename U, typename... Ts>
struct find_first : std::integral_constant<size_t, not_found> {};

template<typename U, typename... Ts>
struct find_first<U, U, Ts...> : std::integral_constant<size_t, 0> {};

template<typename U, typename T, typename... Ts>
struct find_first<U, T, Ts...> : std::conditional_t<find_first<U, Ts...>::value == not_found,
                                                    std::integral_constant<size_t, not_found>,
                                                    std::integral_constant<size_t, find_first<U, Ts...>::value + 1>> {};

template<typename U, typename... Ts>
inline constexpr size_t find_first_v = find_first<U, Ts...>::value;
} // namespace v_impl
