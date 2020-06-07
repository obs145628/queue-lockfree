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

constexpr std::size_t ITEMS_COUNT = 512 * 1024;
constexpr std::size_t THREADS_COUNT = 16;
constexpr std::size_t ITEMS_PER_THREAD = ITEMS_COUNT / THREADS_COUNT;

static_assert(ITEMS_COUNT % THREADS_COUNT == 0);

struct ValCounter {
  std::atomic<int> n;
  char offset[64]; // to avoid false sharing

  ValCounter() : n(0) {}
};

std::unique_ptr<std::vector<ValCounter>> g_out;

Stack<int> g_stack;
std::atomic<int> g_ready;

using shared_t = Stack<int>::ref_t;

void runner_produce(const int *arr) {
  while (!g_ready)
    continue;

  for (std::size_t i = 0; i < ITEMS_PER_THREAD; ++i)
    g_stack.push(arr[i]);
}

void runner_consume() {
  while (!g_ready)
    continue;

  for (std::size_t i = 0; i < ITEMS_PER_THREAD; ++i) {
    shared_t next;
    while (!(next = g_stack.try_pop()))
      continue;
    ++((*g_out)[*next].n);
  }
}

void runner_prod_cons(const int *arr) {
  while (!g_ready)
    continue;

  for (std::size_t i = 0; i < ITEMS_PER_THREAD; ++i) {
    // Push
    g_stack.push(arr[i]);

    // Pop
    shared_t next;
    while (!(next = g_stack.try_pop()))
      continue;
    ++((*g_out)[*next].n);
  }
}

void run_test(bool duo) {
  g_ready = false;

  std::vector<int> input(ITEMS_COUNT);
  for (std::size_t i = 0; i < input.size(); ++i)
    input[i] = i;

  Xorshift xs(172847);
  xs.shuffle(&input[0], input.size());

  g_out = std::make_unique<std::vector<ValCounter>>(ITEMS_COUNT);

  std::vector<std::thread> ths;
  for (std::size_t i = 0; i < THREADS_COUNT; ++i) {
    if (duo) {
      ths.emplace_back(runner_produce, &input[i * ITEMS_PER_THREAD]);
      ths.emplace_back(runner_consume);
    } else
      ths.emplace_back(runner_prod_cons, &input[i * ITEMS_PER_THREAD]);
  }

  g_ready = true;
  for (auto &t : ths)
    t.join();

  REQUIRE(g_stack.empty());
  for (const auto &x : *g_out)
    REQUIRE(x.n == 1);
}

} // namespace

TEST_CASE("Test N consumer + N producer") { run_test(true); }

TEST_CASE("Test N consumer / producer") { run_test(false); }
