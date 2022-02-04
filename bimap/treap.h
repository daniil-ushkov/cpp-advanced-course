#include "node_base.h"
#include <random>

namespace bimap_impl {
template <typename Key, typename Tag>
struct node : node_base {
  explicit node(Key key, size_t rank) : key(std::move(key)), rank(rank) {}

  node* get_left_node() noexcept {
    return static_cast<node*>(node_base::get_left());
  }

  const node* get_left_node() const noexcept {
    return static_cast<const node*>(node_base::get_left());
  }

  node* get_right_node() noexcept {
    return static_cast<node*>(node_base::get_right());
  }

  const node* get_right_node() const noexcept {
    return static_cast<const node*>(node_base::get_right());
  }

  const Key key;
  const size_t rank;
};

template <typename Data, typename Key, typename CompareKey, typename Tag>
class treap : node_base, CompareKey {
public:
  using node_t = node<Key, Tag>;

  static_assert(std::is_base_of_v<node_t, Data>);

  struct const_iterator {
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = Data;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type&;

    const_iterator() noexcept = default;
    const_iterator(const node_base* cur) noexcept : cur(cur) {}

    const Data& operator*() const noexcept {
      return *static_cast<const Data*>(static_cast<const node_t*>(cur));
    }

    const Data* operator->() const noexcept {
      return static_cast<const Data*>(static_cast<const node_t*>(cur));
    }

    const_iterator& operator++() noexcept {
      cur = cur->next();
      return *this;
    }

    const_iterator operator++(int) noexcept {
      auto copy = *this;
      ++*this;
      return copy;
    }

    const_iterator& operator--() noexcept {
      cur = cur->prev();
      return *this;
    }

    const_iterator operator--(int) noexcept {
      auto copy = *this;
      --*this;
      return copy;
    }

    bool operator==(const const_iterator& other) const noexcept {
      return cur == other.cur;
    }

    bool operator!=(const const_iterator& other) const noexcept {
      return cur != other.cur;
    }

    const node_base* cur = nullptr;
  };

  explicit treap(CompareKey cmp = CompareKey()) : CompareKey(std::move(cmp)) {}

  treap(const treap& other) = delete;
  treap(treap&& other) noexcept {
    swap(*this, other);
  }

  treap& operator=(const treap& other) = delete;
  treap& operator=(treap&& other) noexcept {
    if (&other == this) {
      return *this;
    }

    swap(*this, other);

    return *this;
  }

  static void swap(treap& lhs, treap& rhs) noexcept {
    auto* lhs_root = lhs.root();
    auto* rhs_root = rhs.root();

    unset(lhs_root);
    unset(rhs_root);

    lhs.dummy()->set_left(rhs_root);
    rhs.dummy()->set_left(lhs_root);

    std::swap(static_cast<CompareKey&>(lhs), static_cast<CompareKey&>(rhs));
  }

  void insert(Data& data) noexcept {
    auto* data_node = static_cast<node_t*>(&data);
    auto* root_ = root();

    unset(root_);
    auto root_splitted = split(root_, data_node->key);

    root_ = merge(root_splitted.left, data_node);
    root_ = merge(root_, root_splitted.right);
    dummy()->set_left(root_);
  }

  void erase(const Key& key) noexcept {
    auto* root_ = root();
    unset(root_);
    auto root_splitted = split(root_, key);
    dummy()->set_left(merge(root_splitted.left, root_splitted.right));
  }

  const_iterator find(const Key& key) const noexcept {
    auto found = find_(key);
    return found.child ? found.child : dummy();
  }

  const_iterator lower_bound(const Key& key) const noexcept {
    auto found = find_(key);

    if (found.child) {
      return found.child;
    }

    if (found.is_left) {
      return found.parent;
    } else {
      return found.parent->next();
    }
  }

  const_iterator upper_bound(const Key& key) const noexcept {
    auto found = find_(key);

    if (found.is_left) {
      return found.parent;
    }

    if (found.child) {
      return found.child->next();
    } else {
      return found.parent->next();
    }
  }

  const_iterator begin() const noexcept {
    return dummy()->min();
  }

  const_iterator end() const noexcept {
    return dummy();
  }

  size_t size() const noexcept {
    return dummy()->get_size() - 1;
  }

  bool empty() const noexcept {
    return root() == nullptr;
  }

  static const treap& dummy_as_treap(const node_base& dummy) noexcept {
    return static_cast<const treap&>(dummy);
  }

private:
  static node_t* merge(node_t* lhs, node_t* rhs) noexcept {
    if (!lhs) {
      return rhs;
    }

    if (!rhs) {
      return lhs;
    }

    if (lhs->rank < rhs->rank) {
      auto* rhs_left = rhs->get_left_node();
      unset(rhs_left);
      rhs->set_left(merge(lhs, rhs_left));
      return rhs;
    } else {
      auto* lhs_right = lhs->get_right_node();
      unset(lhs_right);
      lhs->set_right(merge(lhs_right, rhs));
      return lhs;
    }
  }

  struct splitted {
    node_t* left;
    node_t* middle;
    node_t* right;
  };

  splitted split(node_t* node, const Key& key) noexcept {
    if (!node) {
      return {nullptr, nullptr, nullptr};
    }

    auto* n_left = node->get_left_node();
    auto* n_right = node->get_right_node();

    if (cmp(key, node->key)) {
      unset(n_left);

      auto n_left_splitted = split(n_left, key);
      n_left_splitted.right = merge(n_left_splitted.right, node);

      return n_left_splitted;
    } else if (cmp(node->key, key)) {
      unset(n_right);

      auto n_right_splitted = split(n_right, key);
      n_right_splitted.left = merge(node, n_right_splitted.left);

      return n_right_splitted;
    } else {
      unset(n_left);
      unset(n_right);

      return {n_left, node, n_right};
    }
  }

  // represents place where node was found
  struct found {
    const node_base* parent;
    const node_t* child;
    bool is_left; // we need it, because child can be nullptr.
  };

  found find_(const Key& key) const noexcept {
    found cur = {dummy(), root(), true};

    while (cur.child != nullptr) {
      if (cmp(key, cur.child->key)) {
        cur = {cur.child, cur.child->get_left_node(), true};
      } else if (cmp(cur.child->key, key)) {
        cur = {cur.child, cur.child->get_right_node(), false};
      } else {
        return cur;
      }
    }

    return cur;
  }

  bool cmp(const Key& lhs, const Key& rhs) const noexcept {
    return static_cast<const CompareKey&>(*this)(lhs, rhs);
  }

  node_base* dummy() noexcept {
    return static_cast<node_base*>(this);
  }

  const node_base* dummy() const noexcept {
    return static_cast<const node_base*>(this);
  }

  node_t* root() noexcept {
    return static_cast<node_t*>(static_cast<node_base&>(*this).get_left());
  }

  const node_t* root() const noexcept {
    return static_cast<const node_t*>(
        static_cast<const node_base&>(*this).get_left());
  }
};
} // namespace bimap_impl
