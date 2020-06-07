I try different implementations of a lock-free queue

# my_shared_ptr

For learning purposes, I made my own shared_ptr
I implemented all the features of std::shared_ptr (atomicity, weak_ptr, enable_shared_from_this, object allocated alongside or apart control block)
I only need a really small subset of these features for a lock-free linked list, but that was fun to do it all.

