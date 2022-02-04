#pragma once

#include <cstddef>
#include <memory>

#include "weak_ptr.h"

template <typename T>
struct shared_ptr {
  shared_ptr() noexcept = default;
  shared_ptr(std::nullptr_t) noexcept;

  template <typename U, typename D = std::default_delete<U>,
            typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
  shared_ptr(U* ptr, D&& d = D());

  template <typename U>
  shared_ptr(const shared_ptr<U>& other, T* ptr) noexcept;

  template<typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
  shared_ptr(const shared_ptr<U>& other) noexcept;

  shared_ptr(const shared_ptr& other) noexcept;
  shared_ptr(shared_ptr&& other) noexcept;
  shared_ptr& operator=(const shared_ptr& other) noexcept;
  shared_ptr& operator=(shared_ptr&& other) noexcept;
  ~shared_ptr() noexcept;

  T* get() const noexcept;
  operator bool() const noexcept;
  T& operator*() const noexcept;
  T* operator->() const noexcept;

  std::size_t use_count() const noexcept;
  void reset() noexcept;

  template <typename U, typename D = std::default_delete<U>>
  void reset(U* new_ptr, D&& d = D());

  void swap(shared_ptr& other) noexcept;

private:
  T* ptr = nullptr;
  control_block* cb_ptr = nullptr;

  shared_ptr(T* ptr, control_block* cb_ptr) noexcept;

  template <typename U>
  friend bool operator==(const shared_ptr<U>& lhs, const shared_ptr<U>& rhs);

  template <typename U>
  friend bool operator!=(const shared_ptr<U>& lhs, const shared_ptr<U>& rhs);

  template <typename U>
  friend bool operator==(const shared_ptr<U>& lhs, std::nullptr_t);

  template <typename U>
  friend bool operator!=(const shared_ptr<U>& lhs, std::nullptr_t);

  template <typename U>
  friend bool operator==(std::nullptr_t, const shared_ptr<U>& rhs);

  template <typename U>
  friend bool operator!=(std::nullptr_t, const shared_ptr<U>& rhs);

  template <typename U>
  friend struct weak_ptr;

  template <typename U>
  friend struct shared_ptr;

  template <typename U, typename... Args>
  friend shared_ptr<U> make_shared(Args&&... args);
};

template <typename T>
shared_ptr<T>::shared_ptr(std::nullptr_t) noexcept {}

template <typename T>
template <typename U, typename D, typename>
shared_ptr<T>::shared_ptr(U* ptr, D&& d) : ptr(ptr) {
  try {
    cb_ptr = new ptr_control_block<U, D>(ptr, std::forward<D>(d));
  } catch (...) {
    d(ptr);
    throw;
  }
}

template <typename T>
template <typename U>
shared_ptr<T>::shared_ptr(const shared_ptr<U>& other, T* ptr) noexcept
    : shared_ptr(ptr, other.cb_ptr) {}

template <typename T>
template<typename U, typename>
shared_ptr<T>::shared_ptr(const shared_ptr<U>& other) noexcept
    : shared_ptr(static_cast<T*>(other.ptr), other.cb_ptr) {}

template <typename T>
shared_ptr<T>::shared_ptr(const shared_ptr& other) noexcept
    : shared_ptr(other.ptr, other.cb_ptr) {}

template <typename T>
shared_ptr<T>::shared_ptr(shared_ptr&& other) noexcept {
  swap(other);
}

template <typename T>
shared_ptr<T>& shared_ptr<T>::operator=(const shared_ptr& other) noexcept {
  if (&other == this) {
    return *this;
  }
  auto copy = other;
  swap(copy);
  return *this;
}

template <typename T>
shared_ptr<T>& shared_ptr<T>::operator=(shared_ptr&& other) noexcept {
  if (&other == this) {
    return *this;
  }
  auto moved = std::move(other);
  swap(moved);
  return *this;
}

template <typename T>
shared_ptr<T>::~shared_ptr() noexcept {
  if (cb_ptr) {
    cb_ptr->dec_strong();
  }
}

template <typename T>
T* shared_ptr<T>::get() const noexcept {
  return ptr;
}

template <typename T>
shared_ptr<T>::operator bool() const noexcept {
  return ptr != nullptr;
}

template <typename T>
T& shared_ptr<T>::operator*() const noexcept {
  return *ptr;
}

template <typename T>
T* shared_ptr<T>::operator->() const noexcept {
  return ptr;
}

template <typename T>
std::size_t shared_ptr<T>::use_count() const noexcept {
  return cb_ptr ? cb_ptr->strong_cnt : 0;
}

template <typename T>
void shared_ptr<T>::reset() noexcept {
  auto other = shared_ptr();
  swap(other);
}

template <typename T>
template <typename U, typename D>
void shared_ptr<T>::reset(U* new_ptr, D&& d) {
  auto other = shared_ptr(new_ptr, std::forward<D>(d));
  swap(other);
}

template <typename T>
void shared_ptr<T>::swap(shared_ptr& other) noexcept {
  std::swap(ptr, other.ptr);
  std::swap(cb_ptr, other.cb_ptr);
}

template <typename T>
shared_ptr<T>::shared_ptr(T* ptr, control_block* cb_ptr) noexcept
    : ptr(ptr), cb_ptr(cb_ptr) {
  if (cb_ptr) {
    cb_ptr->inc_strong();
  }
}

template <typename U>
bool operator==(const shared_ptr<U>& lhs, const shared_ptr<U>& rhs) {
  return lhs.ptr == rhs.ptr;
}

template <typename U>
bool operator!=(const shared_ptr<U>& lhs, const shared_ptr<U>& rhs) {
  return lhs.ptr != rhs.ptr;
}

template <typename U>
bool operator==(const shared_ptr<U>& lhs, std::nullptr_t) {
  return lhs.ptr == nullptr;
}

template <typename U>
bool operator!=(const shared_ptr<U>& lhs, std::nullptr_t) {
  return lhs.ptr != nullptr;
}

template <typename U>
bool operator==(std::nullptr_t, const shared_ptr<U>& rhs) {
  return rhs.ptr == nullptr;
}

template <typename U>
bool operator!=(std::nullptr_t, const shared_ptr<U>& rhs) {
  return rhs.ptr != nullptr;
}

template <typename T, typename... Args>
shared_ptr<T> make_shared(Args&&... args) {
  auto cb_ptr = new inplace_control_block<T>(std::forward<Args>(args)...);
  return shared_ptr<T>(cb_ptr->get(), static_cast<control_block*>(cb_ptr));
}
