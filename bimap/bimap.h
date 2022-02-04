#pragma once

#include <cstddef>
#include <stdexcept>

#include "binode.h"

template <typename Left, typename Right, typename CompareLeft = std::less<Left>,
          typename CompareRight = std::less<Right>>
class bimap {
public:
  using left_tag = bimap_impl::left_tag;
  using right_tag = bimap_impl::right_tag;

  using binode_t = bimap_impl::binode<Left, Right>;

  template <typename Tag, typename = void>
  struct traits;

  template <typename Dummy>
  struct traits<left_tag, Dummy> {
    using opposite = traits<right_tag>;
    using tag = left_tag;
    using half_t = Left;
    using compare_half_t = CompareLeft;
    using half_node_t = bimap_impl::node<Left, left_tag>;
    using half_tree_t =
        bimap_impl::treap<binode_t, Left, CompareLeft, left_tag>;
    using half_tree_iterator_t = typename half_tree_t::const_iterator;
  };

  template <typename Dummy>
  struct traits<right_tag, Dummy> {
    using opposite = traits<left_tag>;
    using tag = right_tag;
    using half_t = Right;
    using compare_half_t = CompareRight;
    using half_node_t = bimap_impl::node<Right, right_tag>;
    using half_tree_t =
        bimap_impl::treap<binode_t, Right, CompareRight, right_tag>;
    using half_tree_iterator_t = typename half_tree_t::const_iterator;
  };

  using left_t = typename traits<left_tag>::half_t;
  using right_t = typename traits<right_tag>::half_t;

  using left_node_t = typename traits<left_tag>::half_node_t;
  using right_node_t = typename traits<right_tag>::half_node_t;

  using left_tree_t = typename traits<left_tag>::half_tree_t;
  using right_tree_t = typename traits<right_tag>::half_tree_t;

  using left_tree_iterator_t = typename traits<left_tag>::half_tree_iterator_t;
  using right_tree_iterator_t =
      typename traits<right_tag>::half_tree_iterator_t;

  struct tree_pair : left_tree_t, right_tree_t {};

  template <typename From, typename To>
  static const To& opposite(const From& from) noexcept {
    return static_cast<const To&>(static_cast<const tree_pair&>(from));
  }

  template <typename Tag>
  class iterator_impl {
  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = typename traits<Tag>::half_t;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type&;

    iterator_impl(const binode_t& binode) noexcept : cur(&binode.template as_node<Tag>()) {}

    // Элемент на который сейчас ссылается итератор.
    // Разыменование итератора end_left() неопределено.
    // Разыменование невалидного итератора неопределено.
    value_type const& operator*() const {
      return cur->template half<Tag>();
    }

    value_type const* operator->() const {
      return &cur->template half<Tag>();
    }

    // Переход к следующему по величине left'у.
    // Инкремент итератора end_left() неопределен.
    // Инкремент невалидного итератора неопределен.
    iterator_impl& operator++() {
      ++cur;
      return *this;
    }

    iterator_impl operator++(int) {
      return cur++;
    }

    // Переход к предыдущему по величине left'у.
    // Декремент итератора begin_left() неопределен.
    // Декремент невалидного итератора неопределен.
    iterator_impl& operator--() {
      --cur;
      return *this;
    }

    iterator_impl operator--(int) {
      return cur--;
    }

    // left_iterator ссылается на левый элемент некоторой пары.
    // Эта функция возвращает итератор на правый элемент той же пары.
    // end_left().flip() возращает end_right().
    // end_right().flip() возвращает end_left().
    // flip() невалидного итератора неопределен.
    iterator_impl<typename traits<Tag>::opposite::tag> flip() const {
      if (cur.cur->get_parent() == nullptr) {
        auto& tree = half_tree_t::dummy_as_treap(*cur.cur);
        return opposite<half_tree_t, opposite_half_tree_t>(tree).end();
      }

      return *cur;
    }

    bool operator==(const iterator_impl& other) const noexcept {
      return cur == other.cur;
    }

    bool operator!=(const iterator_impl& other) const noexcept {
      return cur != other.cur;
    }

    friend bimap;

  private:
    using half_tree_t = typename traits<Tag>::half_tree_t;
    using opposite_half_tree_t = typename traits<Tag>::opposite::half_tree_t;

    using half_tree_iterator_t = typename traits<Tag>::half_tree_iterator_t;

    iterator_impl(half_tree_iterator_t cur) : cur(cur) {}

    half_tree_iterator_t cur;
  };

  using left_iterator = iterator_impl<left_tag>;
  using right_iterator = iterator_impl<right_tag>;

  // Создает bimap не содержащий ни одной пары.
  bimap(CompareLeft compare_left = CompareLeft(),
        CompareRight compare_right = CompareRight())
      : trees{left_tree_t(compare_left), right_tree_t(compare_right)} {}

  // Конструкторы от других и присваивания
  bimap(bimap const& other) {
    for (auto it = other.begin_left(); it != other.end_left(); ++it) {
      insert(*it, *it.flip());
    }
  }

  bimap(bimap&& other) noexcept = default;

  bimap& operator=(bimap const& other) {
    if (&other == this) {
      return *this;
    }

    auto copy = other;
    swap(copy);

    return *this;
  }

  bimap& operator=(bimap&& other) noexcept = default;

  void swap(bimap& other) noexcept {
    std::swap(tree<left_tag>(), other.tree<left_tag>());
    std::swap(tree<right_tag>(), other.tree<right_tag>());
  }

  // Деструктор. Вызывается при удалении объектов bimap.
  // Инвалидирует все итераторы ссылающиеся на элементы этого bimap
  // (включая итераторы ссылающиеся на элементы следующие за последними).
  ~bimap() noexcept {
    while (!empty()) {
      erase_left(--end_left());
    }
  }

  // Вставка пары (left, right), возвращает итератор на left.
  // Если такой left или такой right уже присутствуют в bimap, вставка не
  // производится и возвращается end_left().
  left_iterator insert(left_t left, right_t right) {
    if (tree<left_tag>().find(left) != tree<left_tag>().end()) {
      return end_left();
    }

    if (tree<right_tag>().find(right) != tree<right_tag>().end()) {
      return end_left();
    }

    auto* b = new binode_t(std::move(left), std::move(right), rand_rank());
    tree<left_tag>().insert(*b);
    tree<right_tag>().insert(*b);

    return *b;
  }

  // Удаляет элемент и соответствующий ему парный.
  // erase невалидного итератора неопределен.
  // erase(end_left()) и erase(end_right()) неопределены.
  // Пусть it ссылается на некоторый элемент e.
  // erase инвалидирует все итераторы ссылающиеся на e и на элемент парный к e.
  left_iterator erase_left(left_iterator it) {
    return erase_impl<left_tag>(it);
  }

  // Аналогично erase, но по ключу, удаляет элемент если он присутствует, иначе
  // не делает ничего Возвращает была ли пара удалена
  bool erase_left(left_t const& left) {
    return erase_impl<left_tag>(left);
  }

  right_iterator erase_right(right_iterator it) {
    return erase_impl<right_tag>(it);
  }

  bool erase_right(right_t const& right) {
    return erase_impl<right_tag>(right);
  }

  // erase от ренжа, удаляет [first, last), возвращает итератор на последний
  // элемент за удаленной последовательностью
  left_iterator erase_left(left_iterator first, left_iterator last) {
    return erase_impl<left_tag>(first, last);
  }

  right_iterator erase_right(right_iterator first, right_iterator last) {
    return erase_impl<right_tag>(first, last);
  }

  // Возвращает итератор по элементу. Если не найден - соответствующий end()
  left_iterator find_left(left_t const& left) const noexcept {
    return find_impl<left_tag>(left);
  }

  right_iterator find_right(right_t const& right) const noexcept {
    return find_impl<right_tag>(right);
  }

  // Возвращает противоположный элемент по элементу
  // Если элемента не существует -- бросает std::out_of_range
  right_t const& at_left(left_t const& key) const {
    return at_impl<left_tag>(key);
  }

  left_t const& at_right(right_t const& key) const {
    return at_impl<right_tag>(key);
  }

  // Возвращает противоположный элемент по элементу
  // Если элемента не существует, добавляет его в bimap и на противоположную
  // сторону кладет дефолтный элемент, ссылку на который и возвращает
  // Если дефолтный элемент уже лежит в противоположной паре - должен поменять
  // соответствующий ему элемент на запрашиваемый (смотри тесты)
  template <
      typename Right1 = right_t,
      typename = std::enable_if_t<std::is_default_constructible_v<Right1>>>
  right_t const& at_left_or_default(left_t const& key) {
    auto it = find_left(key);
    if (it != end_left()) {
      return *it.flip();
    } else {
      erase_right(right_t());
      return *insert(key, right_t()).flip();
    }
  }

  template <typename Left1 = left_t,
            typename = std::enable_if_t<std::is_default_constructible_v<Left1>>>
  left_t const& at_right_or_default(right_t const& key) {
    auto it = find_right(key);
    if (it != end_right()) {
      return *it.flip();
    } else {
      erase_left(left_t());
      return *insert(left_t(), key);
    }
  }

  // lower и upper bound'ы по каждой стороне
  // Возвращают итераторы на соответствующие элементы
  // Смотри std::lower_bound, std::upper_bound.
  left_iterator lower_bound_left(const left_t& left) const {
    return tree<left_tag>().lower_bound(left);
  }

  left_iterator upper_bound_left(const left_t& left) const {
    return tree<left_tag>().upper_bound(left);
  }

  right_iterator lower_bound_right(const right_t& right) const {
    return tree<right_tag>().lower_bound(right);
  }

  right_iterator upper_bound_right(const right_t& right) const {
    return tree<right_tag>().upper_bound(right);
  }

  // Возващает итератор на минимальный по порядку left.
  left_iterator begin_left() const {
    return begin_impl<left_tag>();
  }
  // Возващает итератор на следующий за последним по порядку left.
  left_iterator end_left() const {
    return end_impl<left_tag>();
  }

  // Возващает итератор на минимальный по порядку right.
  right_iterator begin_right() const {
    return begin_impl<right_tag>();
  }
  // Возващает итератор на следующий за последним по порядку right.
  right_iterator end_right() const {
    return end_impl<right_tag>();
  }

  // Проверка на пустоту
  bool empty() const noexcept {
    return tree<left_tag>().empty();
  }

  // Возвращает размер бимапы (кол-во пар)
  size_t size() const noexcept {
    return tree<left_tag>().size();
  }

  // операторы сравнения
  bool operator==(bimap const& other) const noexcept {
    if (size() != other.size()) {
      return false;
    }

    auto it_b = other.begin_left();
    for (auto it_a = begin_left(); it_a != end_left(); ++it_a) {
      if (!(*it_a == *it_b)) {
        return false;
      }

      if (!(*it_a.flip() == *it_b.flip())) {
        return false;
      }

      ++it_b;
    }

    return true;
  }

  bool operator!=(bimap const& other) const noexcept {
    return !(*this == other);
  }

private:
  template <typename Tag>
  auto erase_impl(iterator_impl<Tag> it) {
    auto next_it = it;
    ++next_it;

    tree<typename traits<Tag>::opposite::tag>().erase(*it.flip());
    tree<Tag>().erase(*it);

    const binode_t& node = *it.cur;
    delete &node;

    return next_it;
  }

  template <typename Tag, typename Half>
  bool erase_impl(const Half& value) {
    auto it = find_impl<Tag>(value);
    if (it == end_impl<Tag>()) {
      return false;
    }

    erase_impl<Tag>(it);
    return true;
  }

  template <typename Tag>
  auto erase_impl(iterator_impl<Tag> first, iterator_impl<Tag> last) {
    auto cur_it = first;
    while (cur_it != last) {
      auto copy = cur_it;
      ++cur_it;
      erase_impl<Tag>(copy);
    }

    return last;
  }

  template <typename Tag, typename Half>
  auto find_impl(const Half& value) const noexcept {
    return iterator_impl<Tag>(tree<Tag>().find(value));
  }

  template <typename Tag, typename Half>
  const auto& at_impl(const Half& key) const {
    auto it = find_impl<Tag>(key);
    if (it != end_impl<Tag>()) {
      return *it.flip();
    } else {
      throw std::out_of_range("bimap: out of range");
    }
  }

  template <typename Tag>
  auto begin_impl() const {
    return tree<Tag>().begin();
  }

  template <typename Tag>
  auto end_impl() const {
    return tree<Tag>().end();
  }

  template <typename Tag>
  auto& tree() noexcept {
    return static_cast<typename traits<Tag>::half_tree_t&>(trees);
  }

  template <typename Tag>
  const auto& tree() const noexcept {
    return static_cast<const typename traits<Tag>::half_tree_t&>(trees);
  }

  tree_pair trees;
  std::mt19937 rand_rank{std::random_device{}()};
};
