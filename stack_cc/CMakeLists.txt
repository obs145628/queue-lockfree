set(TEST_SRC
  test1.cc
  test2.cc
  test3.cc
  test4.cc
  test_destructor.cc
)

add_executable(utest_stack_cc_lock.bin ${TEST_SRC})
target_compile_definitions(utest_stack_cc_lock.bin PUBLIC -DIMPL_LOCK)
target_link_libraries(utest_stack_cc_lock.bin pthread catch_main)
add_dependencies(build-tests utest_stack_cc_lock.bin)

add_executable(utest_stack_cc_shared_ptr.bin ${TEST_SRC})
target_compile_definitions(utest_stack_cc_shared_ptr.bin PUBLIC -DIMPL_SHARED_PTR)
target_link_libraries(utest_stack_cc_shared_ptr.bin pthread catch_main)
add_dependencies(build-tests utest_stack_cc_shared_ptr.bin)

add_executable(utest_stack_cc_my_shared_ptr.bin ${TEST_SRC})
target_compile_definitions(utest_stack_cc_my_shared_ptr.bin PUBLIC -DIMPL_MY_SHARED_PTR)
target_link_libraries(utest_stack_cc_my_shared_ptr.bin pthread catch_main)
add_dependencies(build-tests utest_stack_cc_my_shared_ptr.bin)
