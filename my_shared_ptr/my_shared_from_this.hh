#pragma once

#include "my_shared_ptr.hh"
#include "my_weak_ptr.hh"

template <class T> class enable_my_shared_from_this {

  friend class my_shared_ptr<T>;

public:
  enable_my_shared_from_this() {}
  enable_my_shared_from_this(const enable_my_shared_from_this &) {}
  ~enable_my_shared_from_this() {}
  enable_my_shared_from_this &operator=(const enable_my_shared_from_this &) {
    return *this;
  }

  my_shared_ptr<T> shared_from_this() { return my_shared_ptr<T>{_this_weak}; }

  my_shared_ptr<const T> shared_from_this() const {
    return my_shared_ptr<const T>{_this_weak};
  }

private:
  my_weak_ptr<T> _this_weak;
};
