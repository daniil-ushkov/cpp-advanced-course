#pragma once

#include <memory>
#include <type_traits>

struct nullopt_t {};

inline constexpr nullopt_t nullopt;

struct in_place_t {};

inline constexpr in_place_t in_place;

namespace optional_impl {
struct dummy_t {};

template <typename T, bool IsTrivial>
struct destructor_base {
  constexpr destructor_base() noexcept {};

  constexpr destructor_base(const T& value)
      : initialized(true), value(value) {}

  constexpr destructor_base(T&& value)
      : initialized(true), value(std::move(value)) {}

  template <typename... Args>
  explicit constexpr destructor_base(in_place_t, Args&&... args)
      : value(std::forward<Args>(args)...) {}

  ~destructor_base() {
    reset();
  }

  void reset() noexcept {
    if (initialized) {
      initialized = false;
      value.~T();
    }
  }

  bool initialized = false;
  union {
    dummy_t dummy{};
    T value;
  };
};

template <typename T>
struct destructor_base<T, true> {
  constexpr destructor_base() noexcept {};

  constexpr destructor_base(const T& value)
      : initialized(true), value(value) {}

  constexpr destructor_base(T&& value)
      : initialized(true), value(std::move(value)) {}

  template <typename... Args>
  explicit constexpr destructor_base(in_place_t, Args&&... args)
      : initialized(true), value(std::forward<Args>(args)...) {}

  ~destructor_base() = default;

  void reset() noexcept {
    initialized = false;
  }

  bool initialized = false;
  union {
    dummy_t dummy{};
    T value;
  };
};

template <typename T, bool IsTrivial>
struct copyable_base : destructor_base<T, std::is_trivially_destructible_v<T>> {
  using base = destructor_base<T, std::is_trivially_destructible_v<T>>;
  using base::base;

  constexpr copyable_base() noexcept = default;

  constexpr copyable_base(const copyable_base& other) noexcept(
      std::is_nothrow_copy_constructible_v<T>) {
    if (other.initialized) {
      this->initialized = other.initialized;
      new (&this->value) T(other.value);
    }
  }

  constexpr copyable_base& operator=(const copyable_base& other) noexcept(
      std::is_nothrow_copy_assignable_v<T>) {
    if (&other == this) {
      return *this;
    }

    if (this->initialized && other.initialized) {
      this->value = other.value;
      return *this;
    }

    this->reset();
    if (other.initialized) {
      new (&this->value) T(other.value);
      this->initialized = other.initialized;
    }

    return *this;
  }

  constexpr copyable_base(copyable_base&& other) noexcept {
    if (other.initialized) {
      this->initialized = other.initialized;
      new (&this->value) T(std::move(other.value));
    }
  }

  constexpr copyable_base& operator=(copyable_base&& other) noexcept {
    if (&other == this) {
      return *this;
    }

    if (this->initialized && other.initialized) {
      this->value = std::move(other.value);
      return *this;
    }

    this->reset();
    if (other.initialized) {
      this->initialized = other.initialized;
      new (&this->value) T(std::move(other.value));
    }

    return *this;
  }
};

template <typename T>
struct copyable_base<T, true>
    : destructor_base<T, std::is_trivially_destructible_v<T>> {
  using base = destructor_base<T, std::is_trivially_destructible_v<T>>;
  using base::base;
};

template <bool enable>
struct enable_copy_constructor {
  constexpr enable_copy_constructor() = default;

  constexpr enable_copy_constructor(const enable_copy_constructor&) = delete;
  constexpr enable_copy_constructor&
  operator=(const enable_copy_constructor&) = default;

  constexpr enable_copy_constructor(enable_copy_constructor&&) noexcept =
      default;
  constexpr enable_copy_constructor&
  operator=(enable_copy_constructor&&) noexcept = default;
};

template <>
struct enable_copy_constructor<true> {};

template <bool enable>
struct enable_copy_assignment {
  constexpr enable_copy_assignment() = default;

  constexpr enable_copy_assignment(const enable_copy_assignment&) = default;
  constexpr enable_copy_assignment&
  operator=(const enable_copy_assignment&) = delete;

  constexpr enable_copy_assignment(enable_copy_assignment&&) noexcept = default;
  constexpr enable_copy_assignment&
  operator=(enable_copy_assignment&&) noexcept = default;
};

template <>
struct enable_copy_assignment<true> {};

template <bool enable>
struct enable_move_constructor {
  constexpr enable_move_constructor() = default;

  constexpr enable_move_constructor(const enable_move_constructor&) = default;
  constexpr enable_move_constructor&
  operator=(const enable_move_constructor&) = default;

  constexpr enable_move_constructor(enable_move_constructor&&) noexcept =
      delete;
  constexpr enable_move_constructor&
  operator=(enable_move_constructor&&) noexcept = default;
};

template <>
struct enable_move_constructor<true> {};

template <bool enable>
struct enable_move_assignment {
  constexpr enable_move_assignment() = default;

  constexpr enable_move_assignment(const enable_move_assignment&) = default;
  constexpr enable_move_assignment&
  operator=(const enable_move_assignment&) = default;

  constexpr enable_move_assignment(enable_move_assignment&&) noexcept = default;
  constexpr enable_move_assignment&
  operator=(enable_move_assignment&&) noexcept = delete;
};

template <>
struct enable_move_assignment<true> {};
} // namespace optional_impl

template <typename T>
class optional
    : optional_impl::copyable_base<T, std::is_trivially_copyable_v<T>>,
      optional_impl::enable_copy_constructor<std::is_copy_constructible_v<T>>,
      optional_impl::enable_copy_assignment<std::is_copy_assignable_v<T>>,
      optional_impl::enable_move_constructor<std::is_move_constructible_v<T>>,
      optional_impl::enable_move_assignment<std::is_move_assignable_v<T>> {
public:
  using base = optional_impl::copyable_base<T, std::is_trivially_copyable_v<T>>;
  using base::base;
  using base::reset;

  constexpr optional() noexcept = default;

  constexpr optional(nullopt_t) noexcept {};

  constexpr operator bool() const noexcept {
    return this->initialized;
  }

  constexpr T& operator*() noexcept {
    return this->value;
  }

  constexpr T const& operator*() const noexcept {
    return this->value;
  }

  constexpr T* operator->() noexcept {
    return &this->value;
  }
  constexpr T const* operator->() const noexcept {
    return &this->value;
  }

  template <typename... Args>
  void emplace(Args&&... args) {
    this->reset();
    new (&this->value) T(std::forward<Args>(args)...);
    this->initialized = true;
  }

  template <typename T1>
  friend constexpr bool operator==(optional<T1> const& a,
                                   optional<T1> const& b);

  template <typename T1>
  friend constexpr bool operator!=(optional<T1> const& a,
                                   optional<T1> const& b);

  template <typename T1>
  friend constexpr bool operator<(optional<T1> const& a, optional<T1> const& b);

  template <typename T1>
  friend constexpr bool operator<=(optional<T1> const& a,
                                   optional<T1> const& b);

  template <typename T1>
  friend constexpr bool operator>(optional<T1> const& a, optional<T1> const& b);

  template <typename T1>
  friend constexpr bool operator>=(optional<T1> const& a,
                                   optional<T1> const& b);
};

template <typename T>
constexpr bool operator==(optional<T> const& a, optional<T> const& b) {
  return !(a < b) && !(b < a);
}

template <typename T>
constexpr bool operator!=(optional<T> const& a, optional<T> const& b) {
  return !(a == b);
}

template <typename T>
constexpr bool operator<(optional<T> const& a, optional<T> const& b) {
  if (!b) {
    return false;
  }

  if (!a) {
    return true;
  }

  return *a < *b;
}

template <typename T>
constexpr bool operator<=(optional<T> const& a, optional<T> const& b) {
  return a < b || a == b;
}

template <typename T>
constexpr bool operator>(optional<T> const& a, optional<T> const& b) {
  return !(a < b) && a != b;
}

template <typename T>
constexpr bool operator>=(optional<T> const& a, optional<T> const& b) {
  return !(a < b);
}
