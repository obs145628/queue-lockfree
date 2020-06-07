#include <algorithm>
#include <atomic>
#include <cassert>
#include <thread>
#include <vector>

#include <catch2/catch.hpp>

#include "stack.hh"
#include "xorshift.hh"

#include <array>

namespace {

constexpr std::size_t ITEMS_COUNT = 256 * 1024;
constexpr std::size_t THREADS_COUNT = 4;
constexpr std::size_t FIND_COUNT = 1;
constexpr std::size_t ITEMS_PER_THREAD = ITEMS_COUNT / THREADS_COUNT;

static_assert(ITEMS_COUNT % THREADS_COUNT == 0);

struct ValCounter {
  std::atomic<int> n;
  char offset[64]; // to avoid false sharing

  ValCounter() : n(0) {}
};

std::unique_ptr<std::vector<ValCounter>> g_out;

constexpr std::size_t VAL_NONE = -1;

class Val {

public:
  Val(std::size_t pos) : _pos(pos) {}

  Val(const Val &v) : _pos(v._pos) { v._pos = VAL_NONE; }

  Val &operator=(const Val &) = delete;

  ~Val() {
    if (_pos != VAL_NONE)
      ++((*g_out)[_pos].n);
  }

private:
  mutable std::size_t _pos;

  friend bool operator==(const Val &a, const Val &b) {
    return a._pos == b._pos;
  }
};

Stack<Val> g_stack;
std::atomic<int> g_ready;
std::atomic<int> g_prod_finished;

template <bool keep_empty> void runner_produce(const std::size_t *arr) {
  while (!g_ready)
    continue;

  for (std::size_t i = 0; i < ITEMS_PER_THREAD; ++i) {
    if constexpr (keep_empty)
      while (g_stack.try_pop())
        continue;
    g_stack.push(Val{arr[i]});
  }
}

void runner_consume() {
  while (!g_ready)
    continue;

  while (!g_prod_finished) {
    auto next = g_stack.try_pop();
    if (!next)
      continue;
  }

  while (g_stack.try_pop())
    continue;
}

void runner_find() {
  while (!g_ready)
    continue;

    // Find in lock too slow to use it by all times
#ifdef IMPL_LOCK
  for (std::size_t i = 0; i < 10000; ++i) {
    auto val = g_stack.find(Val{VAL_NONE});
    REQUIRE(!val);
  }
  return;
#endif

  while (!g_prod_finished) {
    auto val = g_stack.find(Val{VAL_NONE});
    REQUIRE(!val);
  }
}

template <bool keep_empty> void run_test() {
  g_ready = false;
  g_prod_finished = false;

  std::vector<std::size_t> input(ITEMS_COUNT);
  for (std::size_t i = 0; i < input.size(); ++i)
    input[i] = i;

  Xorshift xs(172847);
  xs.shuffle(&input[0], input.size());

  g_out = std::make_unique<std::vector<ValCounter>>(ITEMS_COUNT);

  std::vector<std::thread> prods;
  std::vector<std::thread> ths;
  for (std::size_t i = 0; i < THREADS_COUNT; ++i) {
    prods.emplace_back(runner_produce<keep_empty>,
                       &input[i * ITEMS_PER_THREAD]);
    ths.emplace_back(runner_consume);
  }

  for (std::size_t i = 0; i < FIND_COUNT; ++i)
    ths.emplace_back(runner_find);

  g_ready = true;

  for (auto &t : prods)
    t.join();
  g_prod_finished = true;

  for (auto &t : ths)
    t.join();

  REQUIRE(g_stack.empty());
  for (const auto &x : *g_out)
    REQUIRE(x.n == 1);
}

} // namespace

TEST_CASE("Test destructor keep_empty=false") { run_test<false>(); }

TEST_CASE("Test destructor keep_empty=true") { run_test<true>(); }
