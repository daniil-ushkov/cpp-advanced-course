#pragma once

#include "variant_utils.h"

namespace v_impl {
template <bool AllTriviallyDestructible, typename... Ts>
union recursive_union {};

template <typename T, typename... Ts>
union recursive_union<false, T, Ts...> {
  constexpr recursive_union() noexcept
      requires(std::is_trivially_default_constructible_v<T>
           && (std::is_trivially_default_constructible_v<Ts> && ...)) = default;

  constexpr recursive_union() noexcept {};

  template <typename... Us>
  constexpr recursive_union(in_place_index_t<0>, Us&&... us)
      : value(std::forward<Us>(us)...) {}

  template <size_t I, typename... Us>
  constexpr recursive_union(in_place_index_t<I>, Us&&... us)
      : other(in_place_index<I - 1>, std::forward<Us>(us)...) {}

  template <size_t I, typename... Args>
  constexpr void construct(Args&&... args) {
    if constexpr (I == 0) {
      new (const_cast<std::remove_cv_t<T>*>(std::addressof(value))) T(std::forward<Args>(args)...);
    } else {
      other.template construct<I - 1>(std::forward<Args>(args)...);
    }
  }

  template <size_t I>
  constexpr auto& get() {
    if constexpr (I == 0) {
      return value;
    } else {
      return other.template get<I - 1>();
    }
  }

  template <size_t I>
  constexpr const auto& get() const {
    if constexpr (I == 0) {
      return value;
    } else {
      return other.template get<I - 1>();
    }
  }

  constexpr ~recursive_union() {}

private:
  T value;
  recursive_union<(std::is_trivially_destructible_v<Ts> && ...), Ts...> other;
};

template <typename T, typename... Ts>
union recursive_union<true, T, Ts...> {
  constexpr recursive_union() noexcept
      requires(std::is_trivially_default_constructible_v<T>
               && (std::is_trivially_default_constructible_v<Ts> && ...)) = default;

  constexpr recursive_union() noexcept {};

  template <typename... Us>
  constexpr recursive_union(in_place_index_t<0>, Us&&... us)
      : value(std::forward<Us>(us)...) {}

  template <size_t I, typename... Us>
  constexpr recursive_union(in_place_index_t<I>, Us&&... us)
      : other(in_place_index<I - 1>, std::forward<Us>(us)...) {}

  template <size_t I, typename... Args>
  constexpr void construct(Args&&... args) {
    if constexpr (I == 0) {
      new (const_cast<std::remove_cv_t<T>*>(std::addressof(value))) T(std::forward<Args>(args)...);
    } else {
      other.template construct<I - 1>(std::forward<Args>(args)...);
    }
  }

  template <size_t I>
  constexpr auto& get() {
    if constexpr (I == 0) {
      return value;
    } else {
      return other.template get<I - 1>();
    }
  }

  template <size_t I>
  constexpr const auto& get() const {
    if constexpr (I == 0) {
      return value;
    } else {
      return other.template get<I - 1>();
    }
  }

  constexpr ~recursive_union() = default;

private:
  T value;
  recursive_union<(std::is_trivially_destructible_v<Ts> && ...), Ts...> other;
};
} // namespace variant_impl
