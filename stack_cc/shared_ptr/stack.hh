#pragma once

#include <atomic>
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
    new_head->next = std::atomic_load(&_head);

    while (
        !std::atomic_compare_exchange_weak(&_head, &new_head->next, new_head))
      continue;

    return std::shared_ptr<T>(new_head, &new_head->val);
  }

  std::shared_ptr<T> try_pop() {
    std::shared_ptr<Node> node = std::atomic_load(&_head);

    while (node &&
           !std::atomic_compare_exchange_weak(&_head, &node, node->next))
      continue;

    return std::shared_ptr<T>(node, &node->val);
  }

  std::shared_ptr<T> find(const T &val) {
    std::shared_ptr<Node> node = std::atomic_load(&_head);

    while (node && !(node->val == val))
      node = node->next;

    return std::shared_ptr<T>(node, &node->val);
  }

  // Here for debug / test, unreliable values in multithread env

  bool empty() const { return !_head; }

private:
  std::shared_ptr<Node> _head;
};
