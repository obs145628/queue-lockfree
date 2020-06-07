#pragma once

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "control_block.hh"

template <class T> class my_shared_ptr;
template <class T> class my_weak_ptr;
template <class T> class enable_my_shared_from_this;

template <class T> class my_shared_ptr {

  template <class Y> friend class my_shared_ptr;
  template <class Y> friend class my_weak_ptr;

  struct raw_constructor {};

public:
  using element_type = T;

  my_shared_ptr() : my_shared_ptr(raw_constructor{}, nullptr, nullptr) {}

  my_shared_ptr(std::nullptr_t)
      : my_shared_ptr(raw_constructor{}, nullptr, nullptr) {}

  template <class Y>
  explicit my_shared_ptr(
      Y *ptr,
      typename std::enable_if<std::is_convertible<Y *, T *>::value>::type * = 0)
      : my_shared_ptr(ptr, std::default_delete<Y>{}) {}

  template <class Y, class Deleter>
  my_shared_ptr(
      Y *ptr, Deleter deleter,
      typename std::enable_if<std::is_convertible<Y *, T *>::value>::type * = 0)
      : my_shared_ptr(raw_constructor{}, ptr,
                      new ControledPtr<Y, Deleter>(ptr, std::move(deleter))) {

    constexpr bool has_weak_this =
        std::is_convertible<T *, enable_my_shared_from_this<T> *>::value;
    _build_weak_this<has_weak_this>();
  }

  template <class Y>
  my_shared_ptr(const my_shared_ptr<Y> &r, T *ptr)
      : my_shared_ptr(raw_constructor{}, ptr, r._cb) {
    if (_cb)
      _cb->increment_shared();
  }

  template <class Y>
  my_shared_ptr(my_shared_ptr<Y> &&r, T *ptr)
      : my_shared_ptr(raw_constructor{}, ptr, r._cb) {
    r._ptr = nullptr;
    r._cb = nullptr;
  }

  my_shared_ptr(const my_shared_ptr &r) : my_shared_ptr(r, r._ptr) {}

  template <class Y>
  my_shared_ptr(const my_shared_ptr<Y> &r) : my_shared_ptr(r, r._ptr) {}

  my_shared_ptr(my_shared_ptr &&r)
      : my_shared_ptr(raw_constructor{}, r._ptr, r._cb) {
    r._ptr = nullptr;
    r._cb = nullptr;
  }

  template <class Y>
  my_shared_ptr(
      my_shared_ptr<Y> &&r,
      typename std::enable_if<std::is_convertible<Y *, T *>::value>::type * = 0)
      : my_shared_ptr(raw_constructor{}, r._ptr, r._cb) {
    r._ptr = nullptr;
    r._cb = nullptr;
  }

  template <class Y>
  explicit my_shared_ptr(
      const my_weak_ptr<Y> &r,
      typename std::enable_if<std::is_convertible<Y *, T *>::value>::type * = 0)
      : my_shared_ptr(raw_constructor{}, r._ptr,
                      r._cb && r._cb->lock() ? r._cb : nullptr) {
    if (!_cb) {
      throw std::runtime_error{"bad_weak_ptr"};
    }
  }

  ~my_shared_ptr() {
    if (_cb)
      _cb->decrement_shared();
  }

  my_shared_ptr &operator=(const my_shared_ptr &r) {
    my_shared_ptr{r}.swap(*this);
    return *this;
  }

  template <class Y> my_shared_ptr &operator=(const my_shared_ptr<Y> &r) {
    my_shared_ptr{r}.swap(*this);
    return *this;
  }

  my_shared_ptr &operator=(my_shared_ptr &&r) {
    my_shared_ptr{std::move(r)}.swap(*this);
    return *this;
  }

  template <class Y> my_shared_ptr &operator=(my_shared_ptr<Y> &&r) {
    my_shared_ptr{std::move(r)}.swap(*this);
    return *this;
  }

  void reset() { my_shared_ptr{}.swap(*this); }

  template <class Y>
  void
  reset(Y *ptr,
        typename std::enable_if<std::is_convertible<Y *, T *>::value>::type * =
            0) {
    reset(ptr, std::default_delete<Y>{});
  }

  template <class Y, class Deleter>
  void
  reset(Y *ptr, Deleter deleter,
        typename std::enable_if<std::is_convertible<Y *, T *>::value>::type * =
            0) {
    my_shared_ptr{ptr, deleter}.swap(*this);
  }

  void swap(my_shared_ptr &r) {
    std::swap(_ptr, r._ptr);
    std::swap(_cb, r._cb);
  }

  T *get() const { return _ptr; }
  T &operator*() const { return *get(); }
  T *operator->() const { return get(); }

  std::size_t use_count() const { return _cb ? _cb->shared_count() : 0; }

  operator bool() const { return get() != nullptr; }

  template <class Y> bool owner_before(const my_shared_ptr<Y> &other) const {
    return _cb < other._cb;
  }

  template <class... Args>
  static my_shared_ptr wrapper_make_shared(Args &&... args) {
    auto ctrl = new ControledInplace<T>(std::forward<Args>(args)...);
    auto res = my_shared_ptr{raw_constructor(), ctrl->get_ptr(), ctrl};

    constexpr bool has_weak_this =
        std::is_convertible<T *, enable_my_shared_from_this<T> *>::value;

    res.template _build_weak_this<has_weak_this>();
    return res;
  }

  // Used for debug purposes only
  std::size_t get_raw_shared_count() const { return _cb->shared_count(); }
  std::size_t get_raw_weak_count() const { return _cb->weak_count(); }

private:
  T *_ptr;
  ControlBlock *_cb;

  my_shared_ptr(const raw_constructor &, T *ptr, ControlBlock *cb)
      : _ptr(ptr), _cb(cb) {}

  template <bool has_weak_this> void _build_weak_this() {}

  template <> void _build_weak_this<true>() {
    if (!use_count())
      return;

    auto ptr = static_cast<enable_my_shared_from_this<T> *>(_ptr);
    ptr->_this_weak = my_shared_ptr<T>(*this);
  }
};

template <class T, class U>
bool operator==(const my_shared_ptr<T> &r1, const my_shared_ptr<U> &r2) {
  return r1.get() == r2.get();
}

template <class T, class U>
bool operator!=(const my_shared_ptr<T> &r1, const my_shared_ptr<U> &r2) {
  return !(r1 == r2);
}

template <class T> bool operator==(const my_shared_ptr<T> &r1, std::nullptr_t) {
  return !r1;
}

template <class T> bool operator!=(const my_shared_ptr<T> &r1, std::nullptr_t) {
  return !(r1 == nullptr);
}

template <class T> bool operator==(std::nullptr_t, const my_shared_ptr<T> &r1) {
  return r1 == nullptr;
}

template <class T> bool operator!=(std::nullptr_t, const my_shared_ptr<T> &r1) {
  return !(r1 == nullptr);
}

template <class T, class... Args>
my_shared_ptr<T> make_my_shared(Args &&... args) {
  return my_shared_ptr<T>::wrapper_make_shared(std::forward<Args>(args)...);
}
