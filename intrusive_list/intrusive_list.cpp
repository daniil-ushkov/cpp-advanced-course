#include "intrusive_list.h"

namespace intrusive {
list_element_base::list_element_base() : next(this), prev(this) {}

list_element_base::~list_element_base() {
  unlink();
}

bool list_element_base::is_single() noexcept {
  return next == this;
}

void list_element_base::unlink() noexcept {
  prev->next = next;
  next->prev = prev;
  next = this;
  prev = this;
}

void list_element_base::clear() noexcept {
  while (next != this) {
    next->unlink();
  }
}

void list_element_base::insert(list_element_base& other) noexcept {
  if (!other.is_single()) {
    other.unlink();
  }
  other.prev = prev;
  other.next = this;
  prev->next = &other;
  prev = &other;
}

void list_element_base::splice(list_element_base& first, list_element_base& second) {
  if (&first == &second) {
    return;
  }
  auto* first_ptr = &first;
  auto* second_inclusive_ptr = second.prev;

  first_ptr->prev->next = &second;
  second.prev = first_ptr->prev;

  first_ptr->prev = prev;
  second_inclusive_ptr->next = this;

  prev->next = first_ptr;
  prev = second_inclusive_ptr;
}
} // namespace intrusive