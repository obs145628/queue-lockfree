#include <atomic>
#include <cassert>
#include <catch2/catch.hpp>
#include <thread>
#include <vector>

#include "stack.hh"

namespace {
constexpr std::uint64_t ITEMS_COUNT = 1 * 1024 * 1024;
constexpr std::uint64_t THREADS_COUNT = 16;
constexpr std::uint64_t ITEMS_PER_THREAD = ITEMS_COUNT / THREADS_COUNT;
static_assert(ITEMS_COUNT % THREADS_COUNT == 0);

Stack<std::uint64_t> g_stack;
std::atomic<bool> g_ready;

void runner_produce(std::size_t tid) {
  while (!g_ready)
    continue;

  for (std::uint64_t i = 0; i < ITEMS_PER_THREAD; ++i)
    g_stack.push(i << 32 | tid);
}
} // namespace

TEST_CASE("N producers, then pop stack") {

  g_ready = false;
  std::vector<std::thread> ths;
  for (std::size_t i = 0; i < THREADS_COUNT; ++i)
    ths.emplace_back(runner_produce, i);

  g_ready = true;
  for (auto &t : ths)
    t.join();

  std::vector<std::uint64_t> exp(THREADS_COUNT, ITEMS_PER_THREAD);
  for (std::uint64_t i = 0; i < ITEMS_COUNT; ++i) {
    auto next = g_stack.try_pop();
    REQUIRE(next);
    std::uint64_t tid = *next & 0xFFFFFFFF;
    std::uint64_t val = *next >> 32;
    REQUIRE(--exp[tid] == val);
  }

  for (auto x : exp)
    REQUIRE(x == 0);
  REQUIRE(g_stack.empty());
}
