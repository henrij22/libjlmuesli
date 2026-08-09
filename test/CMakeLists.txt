# SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de SPDX-License-Identifier:
# GPL-3.0-or-later

include(FetchContent)

# Prefer an installed doctest if there is one, otherwise fetch it.
find_package(doctest QUIET)

if(NOT doctest_FOUND)
  message(STATUS "doctest not found, fetching it")
  FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.11
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(doctest)
  list(APPEND CMAKE_MODULE_PATH "${doctest_SOURCE_DIR}/scripts/cmake")
endif()

include(doctest)

add_executable(jlmuesli_tests test_helpers.cpp)

target_link_libraries(jlmuesli_tests PRIVATE jlmuesli doctest::doctest)
target_include_directories(jlmuesli_tests PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_compile_features(jlmuesli_tests PRIVATE cxx_std_17)

# Registers each TEST_CASE with CTest individually, so a failure names the case.
doctest_discover_tests(jlmuesli_tests)
