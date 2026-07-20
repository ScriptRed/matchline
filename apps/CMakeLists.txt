add_executable(count count.cpp)
target_link_libraries(count PRIVATE matchline)

add_executable(replay replay.cpp)
target_link_libraries(replay PRIVATE matchline)
# ReferenceBook lives under tests/ (it's Phase 3's correctness oracle, not
# production code) — replay needs the repo root on its include path to
# reach it as "tests/reference_book.hpp".
target_include_directories(replay PRIVATE ${CMAKE_SOURCE_DIR})
