#pragma once

#include <iterator>

namespace intrusive {
struct list_element_base {
  list_element_base();
  ~list_element_base();

  list_element_base(list_element_base const &) = delete;

  list_element_base(list_element_base &&other) noexcept {
    other.insert(*this);
    other.unlink();
  };

  list_element_base &operator=(list_element_base const &) = delete;

  list_element_base &operator=(list_element_base &&other) noexcept {
    if (&other == this) {
      return *this;
    }
    other.insert(*this);
    other.unlink();
    return *this;
  };

  void unlink() noexcept;
  void clear() noexcept;
  void insert(list_element_base &other) noexcept;
  void splice(list_element_base &first, list_element_base &second);

  list_element_base *next;
  list_element_base *prev;
};

struct default_tag;

template<typename Tag = default_tag>
struct list_element : private list_element_base {
  using list_element_base::unlink;

  template<typename T, typename Tag1>
  friend T &to_T(list_element_base &elem);

  template<typename T, typename Tag1>
  friend list_element_base &to_base(T &elem);
};

template<typename T, typename Tag = default_tag>
T &to_T(list_element_base &elem) {
  return static_cast<T &>(static_cast<list_element<Tag> &>(elem));
}

template<typename T, typename Tag = default_tag>
list_element_base &to_base(T &elem) {
  return static_cast<list_element<Tag> &>(elem);
}

template<typename T, typename Tag = default_tag>
const T &to_T(const list_element_base &elem) {
  return static_cast<T &>(static_cast<list_element<Tag> &>(elem));
}

template<typename T, typename Tag = default_tag>
const list_element_base &to_base(const T &elem) {
  return static_cast<list_element<Tag> &>(elem);
}

template<typename T, typename Tag = default_tag>
struct list {
  static_assert(std::is_base_of_v<list_element<Tag>, T>,
                "You should derive from list_element");

  template<typename TT, typename Tag1 = default_tag>
  struct list_iterator {
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = std::remove_const_t<TT>;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type *;
    using reference = value_type &;

    list_iterator() = default;

    template<typename TTT>
    list_iterator(
        list_iterator<TTT, Tag1> other,
        std::enable_if_t<std::is_same_v<std::remove_const_t<TT>, TTT> ||
            std::is_const_v<TT>> * = nullptr)
        : current(other.current) {}

    TT &operator*() const noexcept {
      return to_T<TT, Tag1>(*current);
    };

    TT *operator->() const noexcept {
      return &to_T<TT, Tag1>(*current);
    };

    list_iterator &operator++() noexcept {
      current = current->next;
      return *this;
    };

    list_iterator &operator--() noexcept {
      current = current->prev;
      return *this;
    };

    list_iterator operator++(int) noexcept {
      auto iterator_copy = *this;
      current = current->next;
      return iterator_copy;
    };

    list_iterator operator--(int) noexcept {
      auto iterator_copy = *this;
      current = current->prev;
      return iterator_copy;
    };

    bool operator==(list_iterator const &rhs) const noexcept {
      return current == rhs.current;
    };

    bool operator!=(list_iterator const &rhs) const noexcept {
      return current != rhs.current;
    };

   private:
    explicit list_iterator(list_element_base *current) : current(current) {}

    explicit list_iterator(const list_element_base *current)
        : current(const_cast<list_element_base *>(current)) {}

    template<typename TTT, typename Tag2>
    friend
    struct list;

    list_element_base *current;
  };

  using iterator = list_iterator<T, Tag>;
  using const_iterator = list_iterator<const T, Tag>;

  list() noexcept {};

  list(list const &) = delete;

  list(list &&other) noexcept {
    if (other.empty()) {
      return;
    }

    auto *next = other.dummy.next;
    other.dummy.unlink();
    next->insert(dummy);
  }

  ~list() noexcept {
    dummy.clear();
  }

  list &operator=(list const &) = delete;

  list &operator=(list &&other) noexcept {
    if (&other == this) {
      return *this;
    }

    dummy.clear();

    if (other.empty()) {
      return *this;
    }

    auto *next = other.dummy.next;
    other.dummy.unlink();
    next->insert(dummy);

    return *this;
  }

  void clear() noexcept {
    dummy.clear();
  };

  void push_back(T &other) noexcept {
    dummy.insert(to_base<T, Tag>(other));
  };

  void pop_back() noexcept {
    dummy.prev->unlink();
  };

  T &back() noexcept {
    return to_T<T, Tag>(*dummy.prev);
  };

  T const &back() const noexcept {
    return to_T<const T, Tag>(*dummy.prev);
  };

  void push_front(T &other) noexcept {
    dummy.next->insert(to_base<T, Tag>(other));
  };

  void pop_front() noexcept {
    dummy.next->unlink();
  };

  T &front() noexcept {
    return to_T<T, Tag>(*dummy.next);
  };

  T const &front() const noexcept {
    return to_T<const T, Tag>(*dummy.next);
  };

  bool empty() const noexcept {
    return dummy.next == &dummy;
  };

  iterator begin() noexcept {
    return iterator(dummy.next);
  };

  const_iterator begin() const noexcept {
    return const_iterator(dummy.next);
  };

  iterator end() noexcept {
    return iterator(&dummy);
  };

  const_iterator end() const noexcept {
    return const_iterator(&dummy);
  };

  iterator insert(const_iterator pos, T &rhs) noexcept {
    auto *rhs_base = &to_base<T, Tag>(rhs);
    rhs_base->unlink();
    pos.current->insert(*rhs_base);
    return iterator(rhs_base);
  };

  iterator erase(const_iterator pos) noexcept {
    auto *next = pos.current->next;
    pos.current->unlink();
    return iterator(next);
  };

  void splice(const_iterator pos, list &, const_iterator first,
              const_iterator last) noexcept {
    pos.current->splice(*first.current, *last.current);
  };

  static iterator as_iterator(T& other) noexcept {
    return iterator(&to_base<T, Tag>(other));
  }

  static const_iterator as_iterator(const T& other) noexcept {
    return const_iterator(&to_base<T, Tag>(other));
  }

 private:
  list_element_base dummy;
};
} // namespace intrusive
