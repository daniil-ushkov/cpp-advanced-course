#include <cstddef>
#include <memory>

struct control_block {
  control_block(size_t strong_cnt, size_t weak_cnt);

  virtual ~control_block() noexcept = default;

  void inc_strong() noexcept;

  void dec_strong() noexcept;

  void inc_weak() noexcept;

  void dec_weak() noexcept;

  virtual void delete_data() noexcept = 0;

  size_t strong_cnt;
  size_t weak_cnt;
};

template <typename T, typename D>
struct ptr_control_block : control_block, D {
  template <typename D1>
  explicit ptr_control_block(T* ptr, D1&& d) noexcept;

  void delete_data() noexcept override;

  T* ptr;
};

template <typename T, typename D>
void ptr_control_block<T, D>::delete_data() noexcept {
  D::operator()(ptr);
}

template <typename T, typename D>
template <typename D1>
ptr_control_block<T, D>::ptr_control_block(T* ptr, D1&& d) noexcept
    : control_block(1, 1), D(std::forward<D>(d)), ptr(ptr) {}

template <typename T>
struct inplace_control_block : control_block {
  template <typename... Args>
  inplace_control_block(Args&&... args);

  void delete_data() noexcept override;

  T* get() noexcept;

  std::aligned_storage_t<sizeof(T), alignof(T)> storage;
};

template <typename T>
template <typename... Args>
inplace_control_block<T>::inplace_control_block(Args&&... args)
    // counters will be increased by shared_ptr(T* ptr, control_block* cb_ptr)
    // in make_shared
    : control_block(0, 0) {
  new (&storage) T(std::forward<Args>(args)...);
}

template <typename T>
void inplace_control_block<T>::delete_data() noexcept {
  get()->~T();
}
template <typename T>
T* inplace_control_block<T>::get() noexcept {
  return reinterpret_cast<T*>(&storage);
}
