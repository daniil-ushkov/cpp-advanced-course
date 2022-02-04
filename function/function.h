#pragma once

#include "operations.h"

template <typename F>
struct function;

template <typename R, typename... Args>
struct function<R(Args...)> {
  function() noexcept = default;

  function(function const& other) : ops(other.ops) {
    other.ops->copier(other.storage, storage);
  };

  function(function&& other) noexcept : ops(other.ops) {
    other.ops->mover(other.storage, storage);
  };

  template <typename T>
  function(T val) {
    if constexpr (function_impl::is_small_v<T>) {
      new (&storage) T(std::move(val));
      ops = &function_impl::small_obj_operations<T, R, Args...>;
    } else {
      reinterpret_cast<T*&>(storage) = new T(std::move(val));
      ops = &function_impl::big_obj_operations<T, R, Args...>;
    }
  };

  function& operator=(function const& rhs) {
    if (&rhs == this) {
      return *this;
    }
    auto copy = rhs;
    swap(copy);
    return *this;
  };

  function& operator=(function&& rhs) noexcept {
    if (&rhs == this) {
      return *this;
    }
    swap(rhs);
    return *this;
  };

  void swap(function& rhs) noexcept {
    function_impl::storage_t tmp;
    ops->mover(storage, tmp);
    rhs.ops->mover(rhs.storage, storage);
    ops->mover(tmp, rhs.storage);
    std::swap(ops, rhs.ops);
  }

  ~function() {
    ops->deleter(storage);
  };

  explicit operator bool() const noexcept {
    return ops != &function_impl::empty_operations<R, Args...>;
  };

  R operator()(Args... args) const {
    return ops->invoker(storage, std::forward<Args>(args)...);
  };

  template <typename T>
  T* target() noexcept {
    if constexpr (function_impl::is_small_v<T>) {
      return ops == &function_impl::small_obj_operations<T, R, Args...>
               ? reinterpret_cast<T*>(&storage)
               : nullptr;
    } else {
      return ops == &function_impl::big_obj_operations<T, R, Args...>
               ? reinterpret_cast<T*&>(storage)
               : nullptr;
    }
  };

  template <typename T>
  T const* target() const noexcept {
    if constexpr (function_impl::is_small_v<T>) {
      return ops == &function_impl::small_obj_operations<T, R, Args...>
               ? reinterpret_cast<T*>(&storage)
               : nullptr;
    } else {
      return ops == &function_impl::big_obj_operations<T, R, Args...>
               ? reinterpret_cast<T*&>(storage)
               : nullptr;
    }
  };

private:
  // `mutable` as we want store functions with non-const operator()
  mutable function_impl::storage_t storage;
  const function_impl::operations<R, Args...>* ops =
      &function_impl::empty_operations<R, Args...>;
};
