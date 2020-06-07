#include <catch2/catch.hpp>

#include <memory>

#include "my_shared_ptr.hh"
#include "my_weak_ptr.hh"

struct Base {
  int x;

  Base(int x) : x(x) {}
};

struct Child : public Base {
  int y;

  Child(int x, int y) : Base(x), y(y) {}
};

TEST_CASE("shared cons_des") {
  my_shared_ptr<int> x(new int(12));
  REQUIRE(x.get());
  REQUIRE(x.use_count() == 1);
}

TEST_CASE("shared constructor") {
  {
    my_shared_ptr<int> x;
    REQUIRE(!x.get());
    REQUIRE(!x.use_count());
  }

  {
    my_shared_ptr<int> x(nullptr);
    REQUIRE(!x.get());
    REQUIRE(!x.use_count());
  }

  {
    auto ptr = new int(3);
    my_shared_ptr<int> x(ptr);
    REQUIRE(x.get() == ptr);
    REQUIRE(x.use_count() == 1);
    REQUIRE(*x == 3);
    REQUIRE(&*x == ptr);
  }

  {
    auto ptr = new Child(4, 7);
    my_shared_ptr<Base> x(ptr);
    REQUIRE(x.get() == ptr);
    REQUIRE(x.use_count() == 1);
    REQUIRE(x->x == 4);
    REQUIRE(&*x == ptr);
  }

  {
    int *ptr_addr = nullptr;
    auto ptr = new int(3);

    {
      my_shared_ptr<int> x(ptr, [&ptr_addr](int *ptr) {
        delete ptr;
        ptr_addr = ptr;
      });
      REQUIRE(x.get() == ptr);
      REQUIRE(x.use_count() == 1);
      REQUIRE(*x == 3);
      REQUIRE(&*x == ptr);
    }

    REQUIRE(ptr == ptr_addr);
  }

  {
    auto ptr = new Child(4, 8);
    my_shared_ptr<Child> r1(ptr);
    REQUIRE(r1.use_count() == 1);

    {
      my_shared_ptr<Base> r2(r1);
      REQUIRE(r1.use_count() == 2);
      REQUIRE(r2.use_count() == 2);
      REQUIRE(r1.get() == ptr);
      REQUIRE(r2.get() == ptr);

      {
        my_shared_ptr<int> r3(r2, &r2->x);
        REQUIRE(r1.use_count() == 3);
        REQUIRE(r2.use_count() == 3);
        REQUIRE(r3.use_count() == 3);
        REQUIRE(r3.get() == &ptr->x);
        REQUIRE(*r3 == 4);
        *r3 = 7;
        REQUIRE(*r3 == 7);
      }

      REQUIRE(r1.use_count() == 2);
      REQUIRE(r2.use_count() == 2);
    }

    REQUIRE(r1.use_count() == 1);
    REQUIRE(ptr->x == 7);
  }

  {
    auto ptr = new int(4);
    my_shared_ptr<int> r1(ptr);
    REQUIRE(r1.use_count() == 1);

    {
      my_shared_ptr<int> r2(r1);
      REQUIRE(r1.use_count() == 2);
      REQUIRE(r2.use_count() == 2);
      REQUIRE(r1.get() == ptr);
      REQUIRE(r2.get() == ptr);
      REQUIRE(*r2 == 4);
      *r2 = 7;
      REQUIRE(*r2 == 7);
    }

    REQUIRE(r1.use_count() == 1);
    REQUIRE(*ptr == 7);
  }

  {
    auto ptr = new int(4);
    my_shared_ptr<int> r1(ptr);
    REQUIRE(r1.use_count() == 1);

    {
      my_shared_ptr<int> r2(std::move(r1));
      REQUIRE(r1.use_count() == 0);
      REQUIRE(r1.get() == nullptr);
      REQUIRE(r2.use_count() == 1);
      REQUIRE(r2.get() == ptr);
      REQUIRE(*r2 == 4);
      *r2 = 7;
      REQUIRE(*ptr == 7);
    }

    REQUIRE(r1.use_count() == 0);
  }

  {
    auto ptr = new Child(4, 8);
    my_shared_ptr<Child> r1(ptr);
    REQUIRE(r1.use_count() == 1);

    {
      my_shared_ptr<Base> r2(std::move(r1));
      REQUIRE(r1.use_count() == 0);
      REQUIRE(r1.get() == nullptr);
      REQUIRE(r2.use_count() == 1);
      REQUIRE(r2.get() == ptr);

      {
        auto tmp_x = &r2->x;
        my_shared_ptr<int> r3(std::move(r2), tmp_x);
        REQUIRE(r1.use_count() == 0);
        REQUIRE(r2.use_count() == 0);
        REQUIRE(r3.use_count() == 1);
        REQUIRE(r2.get() == nullptr);
        REQUIRE(r3.get() == &ptr->x);
        REQUIRE(*r3 == 4);
        *r3 = 7;
        REQUIRE(*r3 == 7);
        REQUIRE(ptr->x == 7);
      }

      REQUIRE(r1.use_count() == 0);
      REQUIRE(r2.use_count() == 0);
    }

    REQUIRE(r1.use_count() == 0);
  }
}

TEST_CASE("shared operator=") {

  {
    int p1_del = 0;
    auto p1 = new int(4);
    my_shared_ptr<int> r1(p1, [&p1_del](int *x) {
      delete x;
      ++p1_del;
    });
    REQUIRE(r1.use_count() == 1);

    {
      auto p2 = new int(4);
      my_shared_ptr<int> r2(p2);
      REQUIRE(r1.use_count() == 1);

      REQUIRE(p1_del == 0);
      r1 = r2;
      REQUIRE(p1_del == 1);
      REQUIRE(r1.use_count() == 2);
      REQUIRE(r2.use_count() == 2);
      REQUIRE(*r2 == 4);
      *r2 = 7;
      REQUIRE(*r1 == 7);
    }

    REQUIRE(r1.use_count() == 1);
  }

  {
    int p1_del = 0;
    auto p1 = new Base(6);
    my_shared_ptr<Base> r1(p1, [&p1_del](Base *x) {
      delete x;
      ++p1_del;
    });
    REQUIRE(r1.use_count() == 1);

    {
      auto p2 = new Child(4, 8);
      my_shared_ptr<Child> r2(p2);
      REQUIRE(r1.use_count() == 1);

      REQUIRE(p1_del == 0);
      r1 = r2;
      REQUIRE(p1_del == 1);
      REQUIRE(r1.use_count() == 2);
      REQUIRE(r2.use_count() == 2);
      REQUIRE(r2->x == 4);
      r2->x = 9;
      REQUIRE(r1->x == 9);
    }

    REQUIRE(r1.use_count() == 1);
  }

  {
    int p1_del = 0;
    auto p1 = new int(4);
    my_shared_ptr<int> r1(p1, [&p1_del](int *x) {
      delete x;
      ++p1_del;
    });
    REQUIRE(r1.use_count() == 1);

    {
      auto p2 = new int(6);
      my_shared_ptr<int> r2(p2);
      REQUIRE(r1.use_count() == 1);

      REQUIRE(p1_del == 0);
      r1 = std::move(r2);
      REQUIRE(p1_del == 1);
      REQUIRE(r1.use_count() == 1);
      REQUIRE(r2.use_count() == 0);
      REQUIRE(r2.get() == nullptr);
      REQUIRE(*r1 == 6);
      *r1 = 7;
      REQUIRE(*p2 == 7);
    }

    REQUIRE(r1.use_count() == 1);
  }

  {
    int p1_del = 0;
    auto p1 = new Base(6);
    my_shared_ptr<Base> r1(p1, [&p1_del](Base *x) {
      delete x;
      ++p1_del;
    });
    REQUIRE(r1.use_count() == 1);

    {
      auto p2 = new Child(4, 8);
      my_shared_ptr<Child> r2(p2);
      REQUIRE(r1.use_count() == 1);

      REQUIRE(p1_del == 0);
      r1 = std::move(r2);
      REQUIRE(p1_del == 1);
      REQUIRE(r1.use_count() == 1);
      REQUIRE(r2.use_count() == 0);
      REQUIRE(r2.get() == nullptr);
      REQUIRE(r1->x == 4);
      r1->x = 9;
      REQUIRE(p2->x == 9);
    }

    REQUIRE(r1.use_count() == 1);
  }
}

TEST_CASE("shared reset") {

  {
    int p1_del = 0;
    auto p1 = new int(4);
    my_shared_ptr<int> r1(p1, [&p1_del](int *x) {
      delete x;
      ++p1_del;
    });

    REQUIRE(p1_del == 0);
    r1.reset();
    REQUIRE(p1_del == 1);

    REQUIRE(r1.use_count() == 0);
    REQUIRE(r1.get() == nullptr);
  }

  {
    int p1_del = 0;
    auto p1 = new Base(6);
    my_shared_ptr<Base> r1(p1, [&p1_del](Base *x) {
      delete x;
      ++p1_del;
    });

    REQUIRE(p1_del == 0);
    auto p2 = new Child(8, 9);
    r1.reset(p2);
    REQUIRE(p1_del == 1);

    REQUIRE(r1.use_count() == 1);
    REQUIRE(r1.get() == p2);
  }

  {
    int p1_del = 0;
    auto p1 = new Base(6);
    my_shared_ptr<Base> r1(p1, [&p1_del](Base *x) {
      delete x;
      ++p1_del;
    });

    int p2_del = 0;
    REQUIRE(p1_del == 0);
    auto p2 = new Child(8, 9);
    r1.reset(p2, [&p2_del](Base *x) {
      delete x;
      ++p2_del;
    });
    REQUIRE(p1_del == 1);

    REQUIRE(r1.use_count() == 1);
    REQUIRE(r1.get() == p2);

    REQUIRE(p2_del == 0);
    r1.reset();
    REQUIRE(p2_del == 1);

    REQUIRE(r1.use_count() == 0);
    REQUIRE(r1.get() == nullptr);
  }
}

TEST_CASE("shared utils") {

  auto p1 = new int(4);
  my_shared_ptr<int> r1(p1);
  auto p2 = new int(5);
  my_shared_ptr<int> r2(p2);
  my_shared_ptr<int> r3;

  REQUIRE(r1.get() == p1);
  REQUIRE(r3.get() == nullptr);

  REQUIRE(&*r1 == p1);

  REQUIRE(r1.use_count() == 1);
  REQUIRE(r3.use_count() == 0);

  REQUIRE(r1);
  REQUIRE(!!r1);
  REQUIRE(!r3);

  REQUIRE(!r1.owner_before(r3));
  REQUIRE(r3.owner_before(r1));

  REQUIRE(r1 != r2);
  REQUIRE(r1 != r3);
  REQUIRE(r1 == r1);
  REQUIRE(r3 == r3);

  REQUIRE(r1 != nullptr);
  REQUIRE(nullptr != r1);
  REQUIRE(r3 == nullptr);
  REQUIRE(nullptr == r3);
}

TEST_CASE("weak contructor") {
  {
    my_weak_ptr<int> x;
    REQUIRE(x.use_count() == 0);
    (void)x;
  }

  {
    my_weak_ptr<int> x;
    my_weak_ptr<int> y(x);
    REQUIRE(x.use_count() == 0);
    REQUIRE(y.use_count() == 0);
  }

  {
    my_shared_ptr<int> s(new int(4));
    my_weak_ptr<int> r(s);
    REQUIRE(r.use_count() == 1);
    s.reset();
    REQUIRE(r.use_count() == 0);
    REQUIRE(!r.lock());
  }

  {
    my_shared_ptr<int> s(new int(4));
    my_weak_ptr<int> r1(s);

    my_weak_ptr<int> r2(r1);

    REQUIRE(r1.use_count() == 1);
    REQUIRE(r2.use_count() == 1);
    s.reset();
    REQUIRE(r1.use_count() == 0);
    REQUIRE(!r1.lock());
    REQUIRE(r2.use_count() == 0);
    REQUIRE(!r2.lock());
  }

  {
    my_shared_ptr<int> s(new int(4));
    my_weak_ptr<int> r1(s);

    my_weak_ptr<int> r2(std::move(r1));
    REQUIRE(r1.use_count() == 0);
    REQUIRE(r2.use_count() == 1);
  }

  {
    my_shared_ptr<Child> s(new Child(4, 8));
    my_weak_ptr<Child> r1(s);

    my_weak_ptr<Base> r2(r1);

    r2.lock()->x *= 3;
    REQUIRE(r1.lock()->x == 12);
    REQUIRE(s->x == 12);

    REQUIRE(r1.use_count() == 1);
    REQUIRE(r2.use_count() == 1);
    s.reset();
    REQUIRE(r1.use_count() == 0);
    REQUIRE(!r1.lock());
    REQUIRE(r2.use_count() == 0);
    REQUIRE(!r2.lock());
  }

  {
    my_shared_ptr<Child> s(new Child(4, 8));
    my_weak_ptr<Child> r1(s);

    my_weak_ptr<Base> r2(std::move(r1));

    r2.lock()->x *= 3;
    REQUIRE(s->x == 12);

    REQUIRE(r1.use_count() == 0);
    REQUIRE(r2.use_count() == 1);
  }
}

TEST_CASE("weak operator=") {

  {
    my_weak_ptr<int> x1;
    my_weak_ptr<int> x2;
    my_weak_ptr<int> y;
    y = x1;
    y = x2;
    REQUIRE(x1.use_count() == 0);
    REQUIRE(x2.use_count() == 0);
    REQUIRE(y.use_count() == 0);
  }

  {
    my_shared_ptr<int> s(new int(4));
    my_shared_ptr<int> s2;
    my_weak_ptr<int> r(s);
    REQUIRE(r.use_count() == 1);
    r = s2;
    REQUIRE(r.use_count() == 0);
    r = s;

    REQUIRE(r.use_count() == 1);
    s.reset();
    REQUIRE(r.use_count() == 0);
    REQUIRE(!r.lock());
  }

  {
    my_shared_ptr<int> s(new int(4));
    my_weak_ptr<int> r1(s);
    my_weak_ptr<int> r2;

    REQUIRE(r1.use_count() == 1);
    REQUIRE(r2.use_count() == 0);
    r2 = r1;

    REQUIRE(r1.use_count() == 1);
    REQUIRE(r2.use_count() == 1);
    s.reset();
    REQUIRE(r1.use_count() == 0);
    REQUIRE(!r1.lock());
    REQUIRE(r2.use_count() == 0);
    REQUIRE(!r2.lock());

    r1 = r2;
    REQUIRE(r1.use_count() == 0);
  }

  {
    my_shared_ptr<int> s(new int(4));
    my_weak_ptr<int> r1(s);
    my_weak_ptr<int> r2;

    REQUIRE(r1.use_count() == 1);
    REQUIRE(r2.use_count() == 0);

    r2 = std::move(r1);
    REQUIRE(r1.use_count() == 0);
    REQUIRE(r2.use_count() == 1);
  }

  {
    my_shared_ptr<Child> s(new Child(4, 8));
    my_weak_ptr<Child> r1(s);

    my_weak_ptr<Base> r2;

    REQUIRE(r1.use_count() == 1);
    REQUIRE(r2.use_count() == 0);

    r2 = r1;
    r2.lock()->x *= 3;
    REQUIRE(r1.lock()->x == 12);
    REQUIRE(s->x == 12);

    REQUIRE(r1.use_count() == 1);
    REQUIRE(r2.use_count() == 1);
    s.reset();
    REQUIRE(r1.use_count() == 0);
    REQUIRE(!r1.lock());
    REQUIRE(r2.use_count() == 0);
    REQUIRE(!r2.lock());
  }

  {
    my_shared_ptr<Child> s(new Child(4, 8));

    my_weak_ptr<Child> r1(s);
    my_weak_ptr<Base> r2;

    REQUIRE(r1.use_count() == 1);
    REQUIRE(r2.use_count() == 0);

    r2 = std::move(r1);

    r2.lock()->x *= 3;
    REQUIRE(s->x == 12);

    REQUIRE(r1.use_count() == 0);
    REQUIRE(r2.use_count() == 1);
  }
}

TEST_CASE("weak utils") {

  {
    my_weak_ptr<int> x;
    REQUIRE(x.use_count() == 0);
    x.reset();
    REQUIRE(x.use_count() == 0);
  }

  {
    my_shared_ptr<int> s(new int(4));
    my_weak_ptr<int> r1(s);
    REQUIRE(r1.use_count() == 1);
    r1.reset();
    REQUIRE(r1.use_count() == 0);
  }

  {
    my_shared_ptr<int> s(new int(4));
    my_weak_ptr<int> r1(s);
    REQUIRE(r1.use_count() == 1);
    REQUIRE(!r1.expired());
    r1.reset();
    REQUIRE(r1.use_count() == 0);
    REQUIRE(r1.expired());
  }

  {
    my_shared_ptr<int> s1(new int(4));
    my_weak_ptr<int> r(s1);
    REQUIRE(r.use_count() == 1);

    my_shared_ptr<int> s2(r);

    REQUIRE(r.use_count() == 2);
    REQUIRE(s1.use_count() == 2);
    REQUIRE(s2.use_count() == 2);

    s1.reset();
    REQUIRE(r.use_count() == 1);
    REQUIRE(s1.use_count() == 0);
    REQUIRE(s2.use_count() == 1);

    s2.reset();
    REQUIRE(r.use_count() == 0);
    REQUIRE(s1.use_count() == 0);
    REQUIRE(s2.use_count() == 0);

    r.reset();
    REQUIRE(r.use_count() == 0);
    REQUIRE(s1.use_count() == 0);
    REQUIRE(s2.use_count() == 0);
  }
}

TEST_CASE("weak basics make_shared") {

  {
    auto s = make_my_shared<int>(4);
    my_weak_ptr<int> r(s);
    REQUIRE(r.use_count() == 1);
    s.reset();
    REQUIRE(r.use_count() == 0);
    REQUIRE(!r.lock());
  }

  {
    auto s = make_my_shared<int>(4);
    my_weak_ptr<int> r1(s);

    my_weak_ptr<int> r2(r1);

    REQUIRE(r1.use_count() == 1);
    REQUIRE(r2.use_count() == 1);
    s.reset();
    REQUIRE(r1.use_count() == 0);
    REQUIRE(!r1.lock());
    REQUIRE(r2.use_count() == 0);
    REQUIRE(!r2.lock());
  }

  {
    auto s = make_my_shared<int>(4);
    my_weak_ptr<int> r1(s);

    my_weak_ptr<int> r2(std::move(r1));
    REQUIRE(r1.use_count() == 0);
    REQUIRE(r2.use_count() == 1);
  }

  {
    auto s = make_my_shared<Child>(4, 8);
    my_weak_ptr<Child> r1(s);

    my_weak_ptr<Base> r2(r1);

    r2.lock()->x *= 3;
    REQUIRE(r1.lock()->x == 12);
    REQUIRE(s->x == 12);

    REQUIRE(r1.use_count() == 1);
    REQUIRE(r2.use_count() == 1);
    s.reset();
    REQUIRE(r1.use_count() == 0);
    REQUIRE(!r1.lock());
    REQUIRE(r2.use_count() == 0);
    REQUIRE(!r2.lock());
  }

  {
    auto s = make_my_shared<Child>(4, 8);
    my_weak_ptr<Child> r1(s);

    my_weak_ptr<Base> r2(std::move(r1));

    r2.lock()->x *= 3;
    REQUIRE(s->x == 12);

    REQUIRE(r1.use_count() == 0);
    REQUIRE(r2.use_count() == 1);
  }

  {
    auto s = make_my_shared<int>(4);
    my_shared_ptr<int> s2;
    my_weak_ptr<int> r(s);
    REQUIRE(r.use_count() == 1);
    r = s2;
    REQUIRE(r.use_count() == 0);
    r = s;

    REQUIRE(r.use_count() == 1);
    s.reset();
    REQUIRE(r.use_count() == 0);
    REQUIRE(!r.lock());
  }

  {
    auto s = make_my_shared<int>(4);
    my_weak_ptr<int> r1(s);
    my_weak_ptr<int> r2;

    REQUIRE(r1.use_count() == 1);
    REQUIRE(r2.use_count() == 0);
    r2 = r1;

    REQUIRE(r1.use_count() == 1);
    REQUIRE(r2.use_count() == 1);
    s.reset();
    REQUIRE(r1.use_count() == 0);
    REQUIRE(!r1.lock());
    REQUIRE(r2.use_count() == 0);
    REQUIRE(!r2.lock());

    r1 = r2;
    REQUIRE(r1.use_count() == 0);
  }

  {
    auto s = make_my_shared<int>(4);
    my_weak_ptr<int> r1(s);
    my_weak_ptr<int> r2;

    REQUIRE(r1.use_count() == 1);
    REQUIRE(r2.use_count() == 0);

    r2 = std::move(r1);
    REQUIRE(r1.use_count() == 0);
    REQUIRE(r2.use_count() == 1);
  }

  {
    auto s = make_my_shared<Child>(4, 8);
    my_weak_ptr<Child> r1(s);

    my_weak_ptr<Base> r2;

    REQUIRE(r1.use_count() == 1);
    REQUIRE(r2.use_count() == 0);

    r2 = r1;
    r2.lock()->x *= 3;
    REQUIRE(r1.lock()->x == 12);
    REQUIRE(s->x == 12);

    REQUIRE(r1.use_count() == 1);
    REQUIRE(r2.use_count() == 1);
    s.reset();
    REQUIRE(r1.use_count() == 0);
    REQUIRE(!r1.lock());
    REQUIRE(r2.use_count() == 0);
    REQUIRE(!r2.lock());
  }

  {
    auto s = make_my_shared<Child>(4, 8);
    my_weak_ptr<Child> r1(s);
    my_weak_ptr<Base> r2;

    REQUIRE(r1.use_count() == 1);
    REQUIRE(r2.use_count() == 0);

    r2 = std::move(r1);

    r2.lock()->x *= 3;
    REQUIRE(s->x == 12);

    REQUIRE(r1.use_count() == 0);
    REQUIRE(r2.use_count() == 1);
  }

  {
    auto s = make_my_shared<int>(4);
    my_weak_ptr<int> r1(s);
    REQUIRE(r1.use_count() == 1);
    r1.reset();
    REQUIRE(r1.use_count() == 0);
  }

  {
    auto s = make_my_shared<int>(4);
    my_weak_ptr<int> r1(s);
    REQUIRE(r1.use_count() == 1);
    REQUIRE(!r1.expired());
    r1.reset();
    REQUIRE(r1.use_count() == 0);
    REQUIRE(r1.expired());
  }

  {
    auto s1 = make_my_shared<int>(4);
    my_weak_ptr<int> r(s1);
    REQUIRE(r.use_count() == 1);

    my_shared_ptr<int> s2(r);

    REQUIRE(r.use_count() == 2);
    REQUIRE(s1.use_count() == 2);
    REQUIRE(s2.use_count() == 2);

    s1.reset();
    REQUIRE(r.use_count() == 1);
    REQUIRE(s1.use_count() == 0);
    REQUIRE(s2.use_count() == 1);

    s2.reset();
    REQUIRE(r.use_count() == 0);
    REQUIRE(s1.use_count() == 0);
    REQUIRE(s2.use_count() == 0);

    r.reset();
    REQUIRE(r.use_count() == 0);
    REQUIRE(s1.use_count() == 0);
    REQUIRE(s2.use_count() == 0);
  }
}
