add_executable(matchline_tests
  frame_walk_test.cpp
  parser_test.cpp
  reference_book_test.cpp
)
target_link_libraries(matchline_tests PRIVATE matchline GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(matchline_tests)

# libFuzzer ships with Clang, not GCC — only build this target when
# configured with Clang.
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_executable(fuzz_parser fuzz/fuzz_parser.cpp)
  target_link_libraries(fuzz_parser PRIVATE matchline)
  target_compile_options(fuzz_parser PRIVATE -fsanitize=fuzzer,address)
  target_link_options(fuzz_parser PRIVATE -fsanitize=fuzzer,address)
endif()
