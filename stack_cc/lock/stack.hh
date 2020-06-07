#pragma once

#include <memory>
#include <mutex>

template <class T> class Stack {

  struct Node {
    T val;
    std::shared_ptr<Node> next;

    Node(const T &val) : val(val) {}
  };

public:
  using ref_t = std::shared_ptr<T>;

  Stack() = default;

  Stack(const Stack &) = delete;
  Stack &operator=(const Stack &) = delete;

  std::shared_ptr<T> push(const T &val) {
    auto new_head = std::make_shared<Node>(val);

    std::lock_guard<std::mutex> lock(_mut);
    new_head->next = _head;
    _head = new_head;
    return std::shared_ptr<T>(new_head, &new_head->val);
  }

  std::shared_ptr<T> try_pop() {
    std::lock_guard<std::mutex> lock(_mut);
    if (!_head)
      return nullptr;

    std::shared_ptr<T> res(_head, &_head->val);
    _head = _head->next;
    return res;
  }

  std::shared_ptr<T> find(const T &val) {
    std::lock_guard<std::mutex> lock(_mut);

    auto node = _head;
    while (node && !(node->val == val))
      node = node->next;

    return std::shared_ptr<T>(node, &node->val);
  }

  // Here for debug / test, unreliable values in multithread env

  bool empty() const {
    std::lock_guard<std::mutex> lock(_mut);
    return !_head;
  }

private:
  mutable std::mutex _mut;
  std::shared_ptr<Node> _head;
};
