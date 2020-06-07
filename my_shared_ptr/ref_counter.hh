#pragma once

#include <atomic>
#include <cstddef>

class RefCounter {
public:
  RefCounter(std::size_t init) : _val(init) {}
  RefCounter(const RefCounter &) = delete;

  std::size_t count() const { return _val; }

  void increment() { ++_val; }

  bool decrement() { return --_val == 0; }

  bool lock() {
    if (_val == 0)
      return false;
    ++_val;
    return true;
  }

private:
  std::size_t _val;
};

class AtomicRefCounter {
public:
  AtomicRefCounter(std::size_t init) : _val(init) {}
  AtomicRefCounter(const AtomicRefCounter &) = delete;

  std::size_t count() const { return _val.load(std::memory_order_relaxed); }

  void increment() { _val.fetch_add(1, std::memory_order_relaxed); }

  bool decrement() { return _val.fetch_sub(1, std::memory_order_acq_rel) == 1; }

  bool lock() {
    // @TODO: LLVM libcxx implem use sequential consistency, but I think
    // memory_order_relaxed is enough For the same reason than increment
    // refcounter can be relaxed
    // Lock doesn't protect access to the object

    std::size_t count = _val.load(std::memory_order_relaxed);

    while (count)
      if (_val.compare_exchange_weak(count, count + 1,
                                     std::memory_order_relaxed,
                                     std::memory_order_relaxed))
        return true;

    return false;
  }

private:
  std::atomic<std::size_t> _val;
};
