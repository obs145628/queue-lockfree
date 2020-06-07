#pragma once

#include <type_traits>
#include <utility>

#include "ref_counter.hh"

class ControlBlock {
public:
  ControlBlock() : _shared_count(1), _weak_count(1) {}

  virtual ~ControlBlock() = default;

  void increment_shared() { _shared_count.increment(); }

  void decrement_shared() {
    bool dead = _shared_count.decrement();
    if (dead) {
      _on_0_shared();
      decrement_weak();
    }
  }

  void increment_weak() { _weak_count.increment(); }

  void decrement_weak() {
    bool dead = _weak_count.decrement();
    if (dead)
      _on_0_weak();
  }

  std::size_t shared_count() const { return _shared_count.count(); }

  std::size_t weak_count() const { return _weak_count.count(); }

  bool lock() { return _shared_count.lock(); }

protected:
  virtual void _on_0_shared() = 0;
  virtual void _on_0_weak() = 0;

private:
  AtomicRefCounter _shared_count;
  AtomicRefCounter _weak_count;
};

template <class T, class Deleter> class ControledPtr : public ControlBlock {
public:
  ControledPtr(T *ptr, Deleter deleter)
      : _ptr(ptr), _deleter(std::move(deleter)) {}

  void _on_0_shared() override { _deleter(_ptr); }

  void _on_0_weak() override { delete this; }

private:
  T *_ptr;
  Deleter _deleter;
};

template <class T> class ControledInplace : public ControlBlock {
public:
  template <class... Args> ControledInplace(Args &&... args) {
    new (get_ptr()) T(std::forward<Args>(args)...);
  }

  void _on_0_shared() override { get_ptr()->~T(); }

  void _on_0_weak() override { delete this; }

  T *get_ptr() { return reinterpret_cast<T *>(&_data); }

private:
  typename std::aligned_storage<sizeof(T), alignof(T)>::type _data;
};
