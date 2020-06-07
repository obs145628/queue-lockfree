#include <algorithm>
#include <array>
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

std::array<std::atomic<std::size_t>, THREADS_COUNT> g_counts;

void runner_produce(std::size_t tid) {
  while (!g_ready)
    continue;

  for (std::uint64_t i = 0; i < ITEMS_PER_THREAD; ++i)
    g_stack.push(i << 32 | tid);
}

void runner_consume() {
  while (!g_ready)
    continue;

  std::vector<std::size_t> counts(THREADS_COUNT, 0);

  std::vector<std::uint64_t> exp(THREADS_COUNT, ITEMS_PER_THREAD);
  for (;;) {
    auto next = g_stack.try_pop();
    if (!next)
      break;

    std::uint64_t tid = *next & 0xFFFFFFFF;
    std::uint64_t val = *next >> 32;
    REQUIRE(val < exp[tid]);
    exp[tid] = val;
    ++counts[tid];
  }

  for (std::size_t i = 0; i < THREADS_COUNT; ++i)
    g_counts[i] += counts[i];
}

} // namespace

TEST_CASE("N producers, then N consumers") {

  {
    g_ready = false;
    std::vector<std::thread> ths;
    for (std::size_t i = 0; i < THREADS_COUNT; ++i)
      ths.emplace_back(runner_produce, i);

    g_ready = true;
    for (auto &t : ths)
      t.join();
  }

  std::fill(g_counts.begin(), g_counts.end(), 0);

  {
    g_ready = false;
    std::vector<std::thread> ths;
    for (std::size_t i = 0; i < THREADS_COUNT; ++i)
      ths.emplace_back(runner_consume);

    g_ready = true;
    for (auto &t : ths)
      t.join();
  }

  for (std::size_t i = 0; i < THREADS_COUNT; ++i)
    REQUIRE(g_counts[i] == ITEMS_PER_THREAD);
  REQUIRE(g_stack.empty());
}
