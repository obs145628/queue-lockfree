#include <catch2/catch.hpp>

#include <memory>

#include "my_shared_from_this.hh"
#include "my_shared_ptr.hh"
#include "my_weak_ptr.hh"

class Foo : public enable_my_shared_from_this<Foo> {

public:
  Foo(int x) : _x(x) {}

  int &get_x() { return _x; }
  const int &get_x() const { return _x; }

private:
  int _x;
};

TEST_CASE("shared_from_this shared new") {

  auto r1 = my_shared_ptr<Foo>(new Foo(23));
  REQUIRE(r1.use_count() == 1);

  {
    auto r2 = r1->shared_from_this();
    REQUIRE(r1.use_count() == 2);
    REQUIRE(r2.use_count() == 2);
  }

  REQUIRE(r1.use_count() == 1);
}

TEST_CASE("shared_from_this make_shared") {

  auto r1 = make_my_shared<Foo>(23);
  REQUIRE(r1.use_count() == 1);

  {
    auto r2 = r1->shared_from_this();
    REQUIRE(r1.use_count() == 2);
    REQUIRE(r2.use_count() == 2);
  }

  REQUIRE(r1.use_count() == 1);
}

TEST_CASE("shared_from_this local") {
  Foo r1(23);
  REQUIRE_THROWS(r1.shared_from_this());
}
