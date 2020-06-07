#pragma once

#include <atomic>

class Spinlock {
public:
  Spinlock() : _lock(0) {}

  void lock() {
    while (_lock.test_and_set(std::memory_order_acquire))
      continue;
  }

  void unlock() { _lock.clear(std::memory_order_release); }

private:
  std::atomic_flag _lock;
};
