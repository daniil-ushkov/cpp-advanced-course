#pragma once
#include <functional>

#include "intrusive_list.h"

// Чтобы не было коллизий с UNIX-сигналами реализация вынесена в неймспейс, по
// той же причине изменено и название файла
namespace signals {

template <typename T>
struct signal;

template <typename... Args>
struct signal<void(Args...)> {
  using slot_t = std::function<void(Args...)>;

  struct signal_tag;

  using connection_base = intrusive::list_element<signal_tag>;

  struct connection : connection_base {
    connection() = default;

    connection(connection&& other) noexcept {
      other.update_ctx([this](emit_ctx* ctx) {
        ctx->it = connections_t::as_iterator(*this);
      });
      swap(*this, other);
    }

    connection& operator=(connection&& other) noexcept {
      if (&other == this) {
        return *this;
      }
      other.update_ctx([this](emit_ctx* ctx) {
        ctx->it = connections_t::as_iterator(*this);
      });
      swap(*this, other);
      return *this;
    }

    ~connection() noexcept {
      disconnect();
    }

    static void swap(connection& lhs, connection& rhs) noexcept {
      std::swap(static_cast<connection_base&>(lhs),
                static_cast<connection_base&>(rhs));
      std::swap(lhs.slot, rhs.slot);
      std::swap(lhs.sig, rhs.sig);
    }

    void disconnect() noexcept {
      if (!sig) {
        return;
      }
      update_ctx([](emit_ctx* ctx) { ++ctx->it; });
      connection_base::unlink();
      reset();
    }

    void reset() noexcept {
      slot = {};
      sig = nullptr;
    }

    friend signal;

  private:
    connection(slot_t slot, const signal* sig) noexcept
        : slot(slot), sig(sig) {}

    template <typename Modifier>
    void update_ctx(Modifier modifier) noexcept {
      if (!sig) {
        return;
      }
      auto* cur_ctx = sig->top_ctx;
      while (cur_ctx) {
        if (&*cur_ctx->it == this) {
          modifier(cur_ctx);
        }
        cur_ctx = cur_ctx->prev;
      }
    }

    slot_t slot;
    const signal* sig = nullptr;
  };

  using connections_t = intrusive::list<connection, signal_tag>;
  using connections_iterator_t = typename connections_t::iterator;

  struct emit_ctx {
    emit_ctx(const signal* sig) noexcept
        : sig(sig), it(sig->conns.begin()), prev(sig->top_ctx) {
      sig->top_ctx = this;
    }

    ~emit_ctx() noexcept {
      if (sig) {
        sig->top_ctx = prev;
      }
    }

    const signal* sig;
    connections_iterator_t it;
    emit_ctx* prev;
  };

  signal() = default;

  signal(signal const&) = delete;
  signal& operator=(signal const&) = delete;

  ~signal() noexcept {
    for (auto& conn : conns) {
      conn.reset();
    }
    auto* cur_ctx = top_ctx;
    while (cur_ctx) {
      cur_ctx->sig = nullptr;
      cur_ctx = cur_ctx->prev;
    }
  };

  connection connect(std::function<void(Args...)> slot) noexcept {
    connection conn(slot, this);
    conns.push_back(conn);
    return conn;
  };

  void operator()(Args... args) const {
    emit_ctx ctx(this);
    while (ctx.it != conns.end()) {
      auto cur_it = ctx.it;
      ++ctx.it;
      cur_it->slot(args...);
      if (!ctx.sig) {
        break;
      }
    }
  };

private:
  mutable connections_t conns;
  mutable emit_ctx* top_ctx = nullptr;
};

} // namespace signals
