#include "control_block.h"

control_block::control_block(size_t strong_cnt, size_t weak_cnt)
    : strong_cnt(strong_cnt), weak_cnt(weak_cnt) {}

void control_block::inc_strong() noexcept {
  ++strong_cnt;
  ++weak_cnt;
}

void control_block::dec_strong() noexcept {
  --strong_cnt;
  if (strong_cnt == 0) {
    delete_data();
  }
  dec_weak();
}

void control_block::inc_weak() noexcept {
  ++weak_cnt;
}

void control_block::dec_weak() noexcept {
  --weak_cnt;
  if (weak_cnt == 0) {
    delete this;
  }
}
