#pragma once

struct bad_function_call : std::exception {};

namespace function_impl {
const size_t STORAGE_SIZE = sizeof(void*);
const size_t STORAGE_ALIGNMENT = alignof(void*);
using storage_t = std::aligned_storage_t<STORAGE_SIZE, STORAGE_ALIGNMENT>;

template <typename R, typename... Args>
struct operations {
  using deleter_t = void (*)(storage_t&);
  using invoker_t = R (*)(storage_t&, Args...);
  using copier_t = void (*)(storage_t&, storage_t&);
  using mover_t = void (*)(storage_t&, storage_t&);

  deleter_t deleter;
  invoker_t invoker;
  copier_t copier;
  mover_t mover;
};

template <typename T>
constexpr bool is_small_v =
    sizeof(T) <= STORAGE_SIZE &&
    alignof(T) <= STORAGE_ALIGNMENT && std::is_nothrow_move_assignable_v<T>
        && std::is_nothrow_move_constructible_v<T>;

template <typename R, typename... Args>
constexpr operations<R, Args...> empty_operations = {
    /*deleter*/ [](storage_t&) {
      // no operations
    },
    /*invoker*/ [](storage_t&, Args...) -> R { throw bad_function_call(); },
    /*copier*/
    [](storage_t&, storage_t&) {
      // no operations
    },
    /*mover*/
    [](storage_t&, storage_t&) {
      // no operations
    },
};

template <typename T, typename R, typename... Args>
constexpr operations<R, Args...> small_obj_operations = {
    /*deleter*/ [](storage_t& stg) {
      reinterpret_cast<T&>(stg).~T();
    },
    /*invoker*/
    [](storage_t& stg, Args... args) -> R {
      return reinterpret_cast<T&>(stg)(std::forward<Args>(args)...);
    },
    /*copier*/
    [](storage_t& src, storage_t& dst) {
      new (&dst) T(reinterpret_cast<T&>(src));
    },
    /*mover*/
    [](storage_t& src, storage_t& dst) {
      new (&dst) T(reinterpret_cast<T&&>(src));
    },
};

template <typename T, typename R, typename... Args>
constexpr operations<R, Args...> big_obj_operations = {
    /*deleter*/ [](storage_t& stg) { delete reinterpret_cast<T*&>(stg); },
    /*invoker*/
    [](storage_t& stg, Args... args) -> R {
      return (*reinterpret_cast<T*&>(stg))(std::forward<Args>(args)...);
    },
    /*copier*/
    [](storage_t& src, storage_t& dst) {
      reinterpret_cast<T*&>(dst) = new T(*reinterpret_cast<T*&>(src));
    },
    /*mover*/
    [](storage_t& src, storage_t& dst) {
      dst = src;
      reinterpret_cast<T*&>(src) = nullptr;
    },
};
} // namespace function_impl
