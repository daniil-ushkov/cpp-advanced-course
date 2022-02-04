#pragma once

namespace bimap_impl {
struct node_base {
  node_base() = default;

  node_base(const node_base& other) = delete;
  node_base(node_base&& other) = delete;

  node_base& operator=(const node_base& other) = delete;
  node_base& operator=(node_base& other) = delete;

  void set_left(node_base* node) noexcept {
    left = node;

    if (node) {
      node->parent = this;
    }

    update_size();
  }

  void set_right(node_base* node) noexcept {
    right = node;

    if (node) {
      node->parent = this;
    }

    update_size();
  }

  static void unset(node_base* node) noexcept {
    if (!node) {
      return;
    }

    auto*& self_ptr =
        node->parent->left == node ? node->parent->left : node->parent->right;

    self_ptr = nullptr;
    node->parent->update_size();
    node->parent = nullptr;
  }

  node_base* next() noexcept {
    return const_cast<node_base*>(static_cast<const node_base*>(this)->next());
  }

  const node_base* next() const noexcept {
    if (right) {
      return right->min();
    }

    auto* cur = this;
    while (parent != nullptr && cur->parent->left != cur) {
      cur = cur->parent;
    }

    return cur->parent;
  }

  node_base* prev() noexcept {
    return const_cast<node_base*>(static_cast<const node_base*>(this)->prev());
  }

  const node_base* prev() const noexcept {
    if (left) {
      return left->max();
    }

    auto* cur = this;
    while (parent != nullptr && cur->parent->right != cur) {
      cur = cur->parent;
    }

    return cur->parent;
  }

  node_base* min() noexcept {
    return const_cast<node_base*>(static_cast<const node_base*>(this)->min());
  }

  const node_base* min() const noexcept {
    auto* cur = this;
    while (cur->left) {
      cur = cur->left;
    }

    return cur;
  }

  node_base* max() noexcept {
    return const_cast<node_base*>(static_cast<const node_base*>(this)->max());
  }

  const node_base* max() const noexcept {
    auto* cur = this;
    while (cur->right) {
      cur = cur->right;
    }

    return cur;
  }

  void update_size() noexcept {
    size_t left_size = left ? left->size : 0;
    size_t right_size = right ? right->size : 0;
    size = left_size + right_size + 1;
  }

  node_base* get_parent() noexcept {
    return parent;
  }

  const node_base* get_parent() const noexcept {
    return parent;
  }

  node_base* get_left() noexcept {
    return left;
  }

  const node_base* get_left() const noexcept {
    return left;
  }

  node_base* get_right() noexcept {
    return right;
  }

  const node_base* get_right() const noexcept {
    return right;
  }

  size_t get_size() const noexcept {
    return size;
  }

private:
  node_base* parent = nullptr;
  node_base* left = nullptr;
  node_base* right = nullptr;
  size_t size = 1;
};
} // namespace bimap_impl
