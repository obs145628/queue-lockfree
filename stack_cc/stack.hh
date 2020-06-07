#pragma once

#if defined(IMPL_LOCK)
#include "lock/stack.hh"

#elif defined(IMPL_SHARED_PTR)
#include "shared_ptr/stack.hh"

#elif defined(IMPL_MY_SHARED_PTR)
#include "my_shared_ptr/stack.hh"

#endif
