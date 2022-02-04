#pragma once

#include "variant_dtor_base.h"
#include "variant_traits.h"
#include "variant_utils.h"
#include "variant_concepts.h"
#include "variant_get.h"
#include "variant_visit.h"

template <typename... Ts>
class variant : v_impl::dtor_base<(std::is_trivially_destructible_v<Ts> && ...), Ts...> {
private:
  using base = v_impl::dtor_base<(std::is_trivially_destructible_v<Ts> && ...), Ts...>;
  using base::make_valueless;

public:
  // Default constructor

  template <typename Head = v_impl::nth_t<0, Ts...>>
  constexpr variant() noexcept(std::is_nothrow_default_constructible_v<Head>)
      requires(std::is_default_constructible_v<Head>)
      : base(in_place_index<0>) {}

  // Copy constructor

  constexpr variant(const variant& other) = delete;

  constexpr variant(const variant& other) noexcept requires(v_impl::all_trivially_copy_constructible<Ts...>) = default;

  constexpr variant(const variant& other) requires(v_impl::all_copy_constructible<Ts...>) : base(variant_npos) {
    if (other.valueless_by_exception()) {
      return;
    }

    v_impl::visit_indexed([this, &other]<size_t I>(in_place_index_t<I>){
      base::entries.template construct<I>(get<I>(other));
    }, other);

    base::index = other.index();
  }

  // Move constructor

  constexpr variant(variant&& other) = delete;

  constexpr variant(variant&& other) noexcept requires(v_impl::all_trivially_move_constructible<Ts...>) = default;

  constexpr variant(variant&& other)
      noexcept((std::is_nothrow_move_constructible_v<Ts> && ...))
      requires(v_impl::all_move_constructible<Ts...>)
      : base(variant_npos) {
    if (other.valueless_by_exception()) {
      return;
    }

    v_impl::visit_indexed([this, &other]<size_t I>(in_place_index_t<I>){
      base::entries.template construct<I>(get<I>(std::move(other)));
    }, other);

    base::index = other.index();
  }

  // Converting constructor

  template <typename U,
            size_t I = v_impl::find_alternative_v<U, variant>,
            typename Ti = v_impl::nth_t<I, Ts...>>
  constexpr variant(U&& u)
      noexcept(std::is_nothrow_constructible_v<Ti, U>)
      requires(sizeof...(Ts) > 0
            && !std::is_same_v<std::remove_cvref<U>, variant>
            && !v_impl::is_in_place_tag_v<std::remove_cvref<U>>
            && std::is_constructible_v<Ti, U>
            && !std::is_same_v<Ti, v_impl::nothing>
      ) : base(in_place_index<I>, std::forward<U>(u)) {}

  // in_place_type ctor

  template <typename U, typename... Args, size_t I = v_impl::find_first_v<U, Ts...>>
  constexpr variant(in_place_type_t<U>, Args&&... args)
      requires(v_impl::exactly_once<U, Ts...>
            && std::is_constructible_v<U, Args...>
      ) : base(in_place_index<I>, std::forward<Args>(args)...) {}

  // in_place_index ctor

  template <size_t I, typename... Args, typename Ti = v_impl::nth_t<I, Ts...>>
  constexpr variant(in_place_index_t<I> i, Args&&... args)
      requires(I < sizeof...(Ts)
            && std::is_constructible_v<Ti, Args...>
      ) : base(i, std::forward<Args>(args)...) {}


  // Copy-assignment

  constexpr variant& operator=(const variant& other) = delete;

  constexpr variant& operator=(const variant& other) noexcept
      requires(v_impl::all_trivially_copy_constructible<Ts...>
            && v_impl::all_trivially_copy_assignable<Ts...>
            && v_impl::all_trivially_destructible<Ts...>
      ) = default;

  constexpr variant& operator=(const variant& other)
      requires(v_impl::all_copy_constructible<Ts...>
            && v_impl::all_copy_assignable<Ts...>) {
    if (valueless_by_exception() && other.valueless_by_exception()) {
      return *this;
    }

    if (other.valueless_by_exception()) {
      make_valueless();
      return *this;
    }

    v_impl::visit_indexed([this, &other]<size_t I>(in_place_index_t<I>) {
      using Ti = v_impl::nth_t<I, Ts...>;

      if (index() == I) {
        get<I>(*this) = get<I>(other);
      } else if (std::is_nothrow_copy_constructible_v<Ti> || !std::is_nothrow_move_constructible_v<Ti>) {
        emplace<I>(get<I>(other));
      } else {
        this->operator=(variant(other));
      }
    }, other);

    return *this;
  }

  // Move-assignment

  constexpr variant& operator=(variant&& other) = delete;

  constexpr variant& operator=(variant&& other) noexcept
      requires(v_impl::all_trivially_move_constructible<Ts...>
            && v_impl::all_trivially_move_assignable<Ts...>
            && v_impl::all_trivially_destructible<Ts...>
      ) = default;

  constexpr variant& operator=(variant&& other)
      noexcept(((std::is_nothrow_move_constructible_v<Ts> &&
                 std::is_nothrow_move_assignable_v<Ts>) && ...))
      requires(v_impl::all_move_constructible<Ts...>
            && v_impl::all_move_assignable<Ts...>) {
    if (valueless_by_exception() && other.valueless_by_exception()) {
      return *this;
    }

    if (other.valueless_by_exception()) {
      make_valueless();
      return *this;
    }

    v_impl::visit_indexed([this, &other]<size_t I>(in_place_index_t<I>) {
      if (index() == I) {
        get<I>(*this) = std::move(get<I>(other));
      } else {
        emplace<I>(get<I>(std::move(other)));
      }
    }, other);

    return *this;
  }

  // Converting assignment

  template<typename U,
           size_t I = v_impl::find_alternative_v<U, variant>,
           typename Ti = v_impl::nth_t<I, Ts...>>
  constexpr variant& operator=(U&& u)
      noexcept(std::is_nothrow_assignable_v<Ti&, U> && std::is_nothrow_constructible_v<Ti, U>)
      requires(!std::is_same_v<std::remove_cvref<U>, variant>
            && std::is_assignable_v<Ti&, U>
            && std::is_constructible_v<Ti, U>
            && !std::is_same_v<Ti, v_impl::nothing>) {

    if (index() == I) {
      get<I>(*this) = std::forward<U>(u);
      return *this;
    }

    if (std::is_nothrow_constructible_v<Ti, U> || !std::is_nothrow_move_constructible_v<Ti>) {
      emplace<I>(std::forward<U>(u));
      return *this;
    }

    emplace<I>(Ti(std::forward<U>(u)));
    return *this;
  }

  // emplace by index

  template <size_t I, typename... Args, typename Ti = variant_alternative_t<I, variant>>
  variant_alternative_t<I, variant>& emplace(Args&&... args) requires(std::is_constructible_v<Ti, Args...>) {
    static_assert(I < sizeof...(Ts)); // cppref: It is a compile-time error if I is not less than sizeof...(Types)

    make_valueless();
    base::entries.template construct<I>(std::forward<Args>(args)...);
    base::index = I;
    return get<I>(*this);
  }

  // emplace by type

  template <typename U, typename... Args, size_t I = v_impl::find_first_v<U, Ts...>>
  U& emplace(Args&&... args) requires(std::is_constructible_v<U, Args...> && v_impl::exactly_once<U, Ts...>) {
    return emplace<I>(std::forward<Args>(args)...);
  }

  constexpr void swap(variant& other) noexcept(((std::is_nothrow_move_constructible_v<Ts> &&
                                                 std::is_nothrow_swappable_v<Ts>) && ...)) {
    if (valueless_by_exception() && other.valueless_by_exception()) {
      return;
    }

    if (index() == other.index()) {
      v_impl::visit_indexed([this, &other]<size_t I>(in_place_index_t<I>) {
        using std::swap;
        swap(get<I>(*this), get<I>(other));
      }, *this);
      return;
    }

    auto tmp = std::move(other);
    other = std::move(*this);
    *this = std::move(tmp);
  }

  constexpr size_t index() const noexcept {
    return base::index;
  }

  constexpr bool valueless_by_exception() const noexcept {
    return index() == variant_npos;
  }

  // comparison

  constexpr bool operator==(const variant& other) const noexcept {
    if (index() != other.index()) {
      return false;
    }

    if (other.valueless_by_exception()) {
      return true;
    }

    return v_impl::visit_indexed([this, &other]<size_t I>(in_place_index_t<I>){
      return get<I>(*this) == get<I>(other);
    }, *this);
  }

  constexpr bool operator!=(const variant& other) const noexcept {
    return !(*this == other);
  }

  constexpr bool operator<(const variant& other) const noexcept {
    if (other.valueless_by_exception()) {
      return false;
    }

    if (valueless_by_exception()) {
      return true;
    }

    return v_impl::visit_indexed([this, &other]<size_t I, size_t J>(in_place_index_t<I>, in_place_index_t<J>) {
      if (I < J) {
        return true;
      }

      if (I > J) {
        return false;
      }

      return get<I>(*this) < get<I>(other);
    }, *this, other);
  }

  constexpr bool operator>=(const variant& other) const noexcept {
    return !(*this < other);
  }

  constexpr bool operator<=(const variant& other) const noexcept {
    return (*this < other) || (*this == other);
  }

  constexpr bool operator>(const variant& other) const noexcept {
    return !(*this <= other);
  }

private:
  template <bool AllTriviallyDestructible, typename... Ts1>
  friend class v_impl::dtor_base;

  template <size_t I, typename... Ts1>
  friend constexpr variant_alternative_t<I, variant<Ts1...>>& get(variant<Ts1...> &v);

  template <size_t I, typename... Ts1>
  friend constexpr const variant_alternative_t<I, variant<Ts1...>>& get(const variant<Ts1...> &v);

  template <size_t I, typename... Ts1>
  friend constexpr variant_alternative_t<I, variant<Ts1...>>&& get(variant<Ts1...> &&v);

  template <size_t I, typename... Ts1>
  friend constexpr const variant_alternative_t<I, variant<Ts1...>>&& get(const variant<Ts1...> &&v);


  template <typename U, typename... Ts1>
  friend constexpr const U& get(const variant<Ts1...> &v);

  template <typename U, typename... Ts1>
  friend constexpr U& get(variant<Ts1...> &v);

  template <typename U, typename... Ts1>
  friend constexpr const U&& get(const variant<Ts1...> &&v);

  template <typename U, typename... Ts1>
  friend constexpr U&& get(variant<Ts1...> &&v);

  template <size_t I, typename... Ts1>
  friend constexpr std::add_pointer_t<const variant_alternative_t<I, variant<Ts1...>>> get_if(const variant<Ts1...>* pv) noexcept;

  template <size_t I, typename... Ts1>
  friend constexpr std::add_pointer_t<variant_alternative_t<I, variant<Ts1...>>> get_if(variant<Ts1...>* pv) noexcept;

  template <typename U, typename... Ts1>
  friend constexpr std::add_pointer_t<const U> get_if(const variant<Ts1...>* pv) noexcept;

  template <typename U, typename... Ts1>
  friend constexpr std::add_pointer_t<U> get_if(variant<Ts1...>* pv) noexcept;
};
