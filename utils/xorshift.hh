#pragma once

#include <cstdint>

class Xorshift {
  using val_t = std::uint64_t;

public:
  Xorshift(val_t seed) : _state(seed) {}

  val_t next() {
    auto x = _state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    return _state = x;
  }

  val_t next(val_t max) { return next() % max; }

  val_t next(val_t min, val_t max) { return next() % (max - min) + min; }

  template <class T> void shuffle(T *arr, std::size_t len) {
    for (std::size_t i = len; i > 0; --i)
      std::swap(arr[i - 1], arr[next(i)]);
  }

  template <class It> void fill(val_t min, val_t max, It beg, It end) {
    while (beg != end)
      *(beg++) = next(min, max);
  }
  template <class It> void fill(val_t max, It beg, It end) {
    fill(0, max, beg, end);
  }

private:
  val_t _state;
};
