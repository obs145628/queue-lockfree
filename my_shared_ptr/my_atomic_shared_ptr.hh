#pragma once

#include "my_shared_ptr.hh"

#include "spinlock.hh"

template <class T> class my_atomic_shared_ptr {
public:
  my_atomic_shared_ptr() = default;
  my_atomic_shared_ptr(const my_atomic_shared_ptr &) = delete;
  my_atomic_shared_ptr &operator=(const my_atomic_shared_ptr &) = delete;

  my_shared_ptr<T> load() {
    _sp.lock();
    my_shared_ptr<T> res = _ptr;
    _sp.unlock();
    return res;
  }

  bool compare_exchange(my_shared_ptr<T> &exp, my_shared_ptr<T> desired) {
    // Swaps used to make sure no refcount is ever decremented while holding the
    // lock (avoid calling free while holding lock)
    _sp.lock();

    if (_ptr == exp) {
      _ptr.swap(desired);
      _sp.unlock();
      return true;
    }

    my_shared_ptr<T> tmp = _ptr;
    tmp.swap(exp);
    _sp.unlock();
    return false;
  }

  operator bool() const { return _ptr; }

private:
  my_shared_ptr<T> _ptr;
  Spinlock _sp;
};
