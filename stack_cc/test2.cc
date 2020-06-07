#include <atomic>
#include <cassert>
#include <catch2/catch.hpp>
#include <thread>
#include <vector>

#include "stack.hh"

namespace {
constexpr std::size_t ITEMS_COUNT = 1 * 1024 * 1024;
constexpr std::size_t THREADS_COUNT = 16;

Stack<int> g_stack;
std::atomic<bool> g_ready;
std::atomic<std::size_t> g_total;

constexpr int VAL_END = -1;

void runner_consume() {
  std::size_t th_total = 0;
  int prev = ITEMS_COUNT;
  while (!g_ready)
    continue;

  for (;;) {
    auto next = g_stack.try_pop();
    REQUIRE(next);
    if (*next == VAL_END)
      break;

    REQUIRE(*next < prev);
    prev = *next;
    ++th_total;
  }

  g_total += th_total;
}
} // namespace

TEST_CASE("fill stack, then N consumers") {

  g_total = 0;
  for (std::size_t i = 0; i < THREADS_COUNT; ++i)
    g_stack.push(VAL_END);
  for (std::size_t i = 0; i < ITEMS_COUNT; ++i)
    g_stack.push((int)i);

  g_ready = false;
  std::vector<std::thread> ths;
  for (std::size_t i = 0; i < THREADS_COUNT; ++i)
    ths.emplace_back(runner_consume);

  g_ready = true;
  for (auto &t : ths)
    t.join();

  REQUIRE(g_total == ITEMS_COUNT);
  REQUIRE(g_stack.empty());
}
