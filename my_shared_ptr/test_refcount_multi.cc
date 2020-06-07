#include <catch2/catch.hpp>

#include <atomic>
#include <thread>
#include <vector>

#include "../utils/xorshift.hh"
#include "my_shared_ptr.hh"
#include "my_weak_ptr.hh"

namespace {

constexpr std::size_t NB_THREADS = 16;
constexpr std::size_t NB_ITERS = 50000;

class MyInt {
public:
  explicit MyInt(int x) : _x(x) {}

  ~MyInt() { ++_delete; }

  int &x() { return _x; }

  static std::size_t get_delete() { return _delete; }

private:
  int _x;
  static std::size_t _delete;
};

std::size_t MyInt::_delete = 0;

static std::size_t _exp_delete = 0;

void check_before_delete() { REQUIRE(_exp_delete == MyInt::get_delete()); }

void check_after_delete() {
  ++_exp_delete;
  REQUIRE(_exp_delete == MyInt::get_delete());
}

template <class T> std::size_t fast_rand(const T &data) {
  Xorshift rng{reinterpret_cast<std::size_t>(&data)};
  return rng.next();
}

std::vector<my_shared_ptr<MyInt>> gen_arr(my_shared_ptr<MyInt> r) {
  std::vector<my_weak_ptr<MyInt>> weaks;
  std::vector<my_shared_ptr<MyInt>> res;
  res.push_back(nullptr);
  std::size_t count = fast_rand(res[0]) % 64;
  res.pop_back();

  for (std::size_t i = 0; i < count; ++i) {
    res.push_back(r);
    if (fast_rand(res.back()) % 4 == 0)
      weaks.push_back(my_weak_ptr<MyInt>(r));
    else if (fast_rand(res.back()) % 16 == 0)
      weaks.clear();
  }
  return res;
}

std::vector<my_weak_ptr<MyInt>> gen_weak_arr(my_shared_ptr<MyInt> r) {
  std::vector<my_shared_ptr<MyInt>> others{r};
  std::vector<my_weak_ptr<MyInt>> res;
  std::size_t count = fast_rand(others[0]) % 64;

  for (std::size_t i = 0; i < count; ++i) {
    res.push_back(my_weak_ptr<MyInt>(r));
    if (fast_rand(res.back()) % 4 == 0)
      others.push_back(r);
    else if (fast_rand(res.back()) % 16 == 0)
      others.clear();
  }
  return res;
}

void shared_ops(my_shared_ptr<MyInt> r) {
  auto v = gen_arr(r);
  auto weaks = gen_weak_arr(r);

  v = {r};
  weaks = {my_weak_ptr<MyInt>{r}};
}

std::atomic<bool> g_ready;

void thread_fun(std::vector<my_shared_ptr<MyInt>> vec) {
  while (!g_ready)
    continue;

  Xorshift rng(fast_rand(vec.front()));

  for (std::size_t i = 0; i < NB_ITERS; ++i)
    shared_ops(vec[rng.next() % vec.size()]);
}

} // namespace

TEST_CASE("refcount multi") {

  std::vector<my_shared_ptr<MyInt>> arr;
  for (std::size_t i = 0; i < 64; ++i)
    arr.push_back(make_my_shared<MyInt>(i));

  g_ready = false;
  std::vector<std::thread> ths;
  for (std::size_t i = 0; i < NB_THREADS; ++i)
    ths.emplace_back(thread_fun, arr);

  g_ready = true;
  for (auto &t : ths)
    t.join();

  for (auto &r : arr) {
    REQUIRE(r.get_raw_weak_count() == 1);
    check_before_delete();
    r.reset();
    check_after_delete();
  }
}

namespace {

// Test to try and detect lock race
// That happens when 1 weak_ptr calls lock() while the last shared_ptr (for this
// object) is destroyed at the same time
// The risk is that the shared_ptr is destroyed (both control block + data),
// but the weak is transformed into a shared_ptr that has dangling pointers

struct Foo {
  static std::atomic<int> deleted;

  int x;

  Foo(int x) : x(x) { deleted = 0; }

  ~Foo() { ++deleted; }
};

std::atomic<int> Foo::deleted{};

std::atomic<my_shared_ptr<Foo> *> g_foo;

std::atomic<bool> g_foo_weak;

constexpr std::size_t RACE_ITERS = 1600000;

void read_race() {

  for (std::size_t i = 0; i < RACE_ITERS; ++i) {

    while (!g_foo)
      continue;
    my_weak_ptr<Foo> x(*g_foo);
    g_foo_weak = true;
    g_foo = nullptr;

    while (x.lock())
      continue;
    g_foo_weak = false;
  }
}

void write_race() {

  for (std::size_t i = 0; i < RACE_ITERS; ++i) {

    auto obj = make_my_shared<Foo>(2);
    g_foo = &obj;
    while (g_foo)
      continue;

    obj.reset();

    while (g_foo_weak)
      continue;

    REQUIRE(obj.use_count() == 0);
    REQUIRE(Foo::deleted.load() == 1);
  }
}

} // namespace

TEST_CASE("refcount multi lock race") {

  g_foo = nullptr;

  std::thread t1(read_race);
  std::thread t2(write_race);

  t1.join();
  t2.join();
}
