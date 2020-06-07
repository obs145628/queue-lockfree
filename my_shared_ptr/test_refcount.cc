#include <catch2/catch.hpp>

#include <vector>

#include "../utils/xorshift.hh"
#include "my_shared_ptr.hh"
#include "my_weak_ptr.hh"

namespace {

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

} // namespace

TEST_CASE("refcount simple shared") {
  auto r = make_my_shared<MyInt>(8);
  REQUIRE(r.get_raw_weak_count() == 1);
  check_before_delete();
  r.reset();
  check_after_delete();
}

TEST_CASE("refcount simple weak") {
  auto r = make_my_shared<MyInt>(8);
  my_weak_ptr<MyInt> s(r);
  REQUIRE(r.get_raw_weak_count() == 2);
  check_before_delete();
  r.reset();
  check_after_delete();
  REQUIRE(s.use_count() == 0);
}

TEST_CASE("refcount arrays") {

  auto r = make_my_shared<MyInt>(7);

  std::size_t arr_count = fast_rand(r) % 100;
  for (std::size_t i = 0; i < arr_count; ++i) {
    auto v = gen_arr(r);
    REQUIRE(r.use_count() == v.size() + 1);
    v.clear();
    REQUIRE(r.use_count() == 1);
  }

  REQUIRE(r.get_raw_weak_count() == 1);
  check_before_delete();
  r.reset();
  check_after_delete();
}

TEST_CASE("refcount arrays weak") {

  auto r = make_my_shared<MyInt>(7);

  std::size_t arr_count = fast_rand(r) % 100;
  for (std::size_t i = 0; i < arr_count; ++i) {
    auto v = gen_arr(r);
    auto weaks = gen_weak_arr(r);
    REQUIRE(r.use_count() == v.size() + 1);
    REQUIRE(r.get_raw_weak_count() == weaks.size() + 1);
    v.clear();
    weaks.clear();
    REQUIRE(r.use_count() == 1);
    REQUIRE(r.get_raw_weak_count() == 1);
  }

  REQUIRE(r.get_raw_weak_count() == 1);
  check_before_delete();
  r.reset();
  check_after_delete();
}
