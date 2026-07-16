add_executable(matchline_tests
  frame_walk_test.cpp
)
target_link_libraries(matchline_tests PRIVATE matchline GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(matchline_tests)
