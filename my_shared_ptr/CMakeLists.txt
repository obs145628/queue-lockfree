set(TEST_SRC
  test1.cc
  test_refcount.cc
  test_refcount_multi.cc
  test_shared_from_this.cc
)
add_executable(utest_my_shared_ptr.bin ${TEST_SRC})
target_link_libraries(utest_my_shared_ptr.bin catch_main pthread)
