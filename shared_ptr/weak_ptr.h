#pragma once

#include "control_block.h"

template <typename T>
struct shared_ptr;

template <typename T>
struct weak_ptr {
  weak_ptr() noexcept = default;
  weak_ptr(const weak_ptr& other) noexcept;
  weak_ptr(weak_ptr&& other) noexcept;
  weak_ptr& operator=(const weak_ptr& other) noexcept;
  weak_ptr& operator=(weak_ptr&& other) noexcept;
  weak_ptr(const shared_ptr<T>& other) noexcept;
  weak_ptr& operator=(const shared_ptr<T>& other) noexcept;
  ~weak_ptr() noexcept;

  shared_ptr<T> lock() const noexcept;

  void swap(weak_ptr& other) noexcept;

private:
  T* ptr = nullptr;
  control_block* cb_ptr = nullptr;

  weak_ptr(T* ptr, control_block* cb_ptr) noexcept;
};

template <typename T>
weak_ptr<T>::weak_ptr(const weak_ptr& other) noexcept
    : weak_ptr(other.ptr, other.cb_ptr) {}

template <typename T>
weak_ptr<T>::weak_ptr(weak_ptr&& other) noexcept {
  swap(other);
}

template <typename T>
weak_ptr<T>& weak_ptr<T>::operator=(const weak_ptr& other) noexcept {
  if (&other == this) {
    return *this;
  }
  auto copy = other;
  swap(copy);
  return *this;
}

template <typename T>
weak_ptr<T>& weak_ptr<T>::operator=(weak_ptr&& other) noexcept {
  if (&other == this) {
    return *this;
  }
  auto moved = std::move(other);
  swap(moved);
  return *this;
}

template <typename T>
weak_ptr<T>::weak_ptr(const shared_ptr<T>& other) noexcept
    : weak_ptr(other.ptr, other.cb_ptr) {}

template <typename T>
weak_ptr<T>& weak_ptr<T>::operator=(const shared_ptr<T>& other) noexcept {
  weak_ptr<T> copy = other;
  swap(copy);
  return *this;
}

template <typename T>
weak_ptr<T>::~weak_ptr() noexcept {
  if (cb_ptr) {
    cb_ptr->dec_weak();
  }
}

template <typename T>
shared_ptr<T> weak_ptr<T>::lock() const noexcept {
  if (cb_ptr == nullptr || cb_ptr->strong_cnt == 0) {
    return shared_ptr<T>();
  } else {
    return shared_ptr(ptr, cb_ptr);
  }
}

template <typename T>
void weak_ptr<T>::swap(weak_ptr& other) noexcept {
  std::swap(ptr, other.ptr);
  std::swap(cb_ptr, other.cb_ptr);
}

template <typename T>
weak_ptr<T>::weak_ptr(T* ptr, control_block* cb_ptr) noexcept
    : ptr(ptr), cb_ptr(cb_ptr) {
  if (cb_ptr) {
    cb_ptr->inc_weak();
  }
}
