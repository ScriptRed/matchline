cmake_minimum_required(VERSION 3.25)
project(matchline CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

add_library(matchline INTERFACE)
target_include_directories(matchline INTERFACE ${CMAKE_SOURCE_DIR}/src)
target_compile_options(matchline INTERFACE
  -Wall -Wextra -Wpedantic -Wshadow -Wconversion)

option(MATCHLINE_SANITIZE "Build with ASan + UBSan" OFF)
if(MATCHLINE_SANITIZE)
  target_compile_options(matchline INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(matchline INTERFACE -fsanitize=address,undefined)
endif()

add_subdirectory(apps)
add_subdirectory(bench)
