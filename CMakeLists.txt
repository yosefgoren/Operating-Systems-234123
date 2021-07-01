cmake_minimum_required(VERSION 3.0.0)
project(malloc_3 VERSION 0.1.0)

include(CTest)
enable_testing()

add_executable(test_3 test_3.cpp)
add_executable(oldariel oldariel/test.cpp)
add_executable(newariel newariel/test.cpp)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
