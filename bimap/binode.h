#pragma once

#include "treap.h"

namespace bimap_impl {
struct left_tag;
struct right_tag;

template <typename Left, typename Right>
struct binode : node<Left, left_tag>, node<Right, right_tag> {
  binode(Left left, Right right, size_t rank) noexcept
      : node<Left, left_tag>(std::move(left), rank), node<Right, right_tag>(
                                                         std::move(right),
                                                         rank) {}

  template <typename Tag>
  const auto& as_node() const noexcept {
    if constexpr (std::is_same_v<Tag, left_tag>) {
      return static_cast<const node<Left, left_tag>&>(*this);
    } else {
      return static_cast<const node<Right, right_tag>&>(*this);
    }
  }

  template <typename Tag>
  const auto& half() const noexcept {
    return as_node<Tag>().key;
  }
};
} // namespace bimap_impl
