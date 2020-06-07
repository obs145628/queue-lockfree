#pragma once

#include "../../my_shared_ptr/my_atomic_shared_ptr.hh"

template <class T> class Stack {

  struct Node {
    T val;
    my_shared_ptr<Node> next;

    Node(const T &val) : val(val) {}
  };

public:
  using ref_t = my_shared_ptr<T>;

  Stack() = default;

  Stack(const Stack &) = delete;
  Stack &operator=(const Stack &) = delete;

  my_shared_ptr<T> push(const T &val) {
    auto new_head = make_my_shared<Node>(val);
    new_head->next = _head.load();

    while (!_head.compare_exchange(new_head->next, new_head))
      continue;

    return my_shared_ptr<T>(new_head, &new_head->val);
  }

  my_shared_ptr<T> try_pop() {
    my_shared_ptr<Node> node = _head.load();

    while (node && !_head.compare_exchange(node, node->next))
      continue;

    return my_shared_ptr<T>(node, &node->val);
  }

  my_shared_ptr<T> find(const T &val) {
    my_shared_ptr<Node> node = _head.load();

    while (node && !(node->val == val))
      node = node->next;

    return my_shared_ptr<T>(node, &node->val);
  }

  // Here for debug / test, unreliable values in multithread env

  bool empty() const { return !_head; }

private:
  my_atomic_shared_ptr<Node> _head;
};
