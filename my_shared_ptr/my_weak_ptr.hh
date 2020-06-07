#pragma once

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "control_block.hh"

template <class T> class my_shared_ptr;
template <class T> class my_weak_ptr;

template <class T> class my_weak_ptr {

  template <class Y> friend class my_shared_ptr;
  template <class Y> friend class my_weak_ptr;

  using element_type = T;

public:
  my_weak_ptr() : my_weak_ptr(nullptr, nullptr) {}

  my_weak_ptr(const my_weak_ptr &r) : my_weak_ptr(r._ptr, r._cb) {
    if (_cb)
      _cb->increment_weak();
  }

  template <class Y>
  my_weak_ptr(
      const my_weak_ptr<Y> &r,
      typename std::enable_if<std::is_convertible<Y *, T *>::value>::type * = 0)
      : my_weak_ptr(r._ptr, r._cb) {
    if (_cb)
      _cb->increment_weak();
  }

  template <class Y>
  my_weak_ptr(
      const my_shared_ptr<Y> &r,
      typename std::enable_if<std::is_convertible<Y *, T *>::value>::type * = 0)
      : my_weak_ptr(r._ptr, r._cb) {
    if (_cb)
      _cb->increment_weak();
  }

  my_weak_ptr(my_weak_ptr &&r) : my_weak_ptr(r._ptr, r._cb) {
    r._ptr = nullptr;
    r._cb = nullptr;
  }

  template <class Y>
  my_weak_ptr(
      my_weak_ptr<Y> &&r,
      typename std::enable_if<std::is_convertible<Y *, T *>::value>::type * = 0)
      : my_weak_ptr(r._ptr, r._cb) {
    r._ptr = nullptr;
    r._cb = nullptr;
  }

  ~my_weak_ptr() {
    if (_cb)
      _cb->decrement_weak();
  }

  my_weak_ptr &operator=(const my_weak_ptr &t) {
    my_weak_ptr{t}.swap(*this);
    return *this;
  }

  template <class Y> my_weak_ptr &operator=(const my_weak_ptr<Y> &t) {
    my_weak_ptr{t}.swap(*this);
    return *this;
  }

  template <class Y> my_weak_ptr &operator=(const my_shared_ptr<Y> &t) {
    my_weak_ptr{t}.swap(*this);
    return *this;
  }

  my_weak_ptr &operator=(my_weak_ptr &&t) {
    my_weak_ptr{std::move(t)}.swap(*this);
    return *this;
  }

  template <class Y> my_weak_ptr &operator=(my_weak_ptr<Y> &&t) {
    my_weak_ptr{std::move(t)}.swap(*this);
    return *this;
  }

  void reset() { my_weak_ptr{}.swap(*this); }

  std::size_t use_count() const { return _cb ? _cb->shared_count() : 0; }

  bool expired() const { return use_count() == 0; }

  void swap(my_weak_ptr &r) {
    std::swap(_ptr, r._ptr);
    std::swap(_cb, r._cb);
  }

  my_shared_ptr<T> lock() const {
    using raw_constructor = typename my_shared_ptr<T>::raw_constructor;
    auto ctrl = _cb && _cb->lock() ? _cb : nullptr;
    return my_shared_ptr<T>(raw_constructor{}, ctrl ? _ptr : nullptr, ctrl);
  }

private:
  T *_ptr;
  ControlBlock *_cb;

  my_weak_ptr(T *ptr, ControlBlock *cb) : _ptr(ptr), _cb(cb) {}
};
