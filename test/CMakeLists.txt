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

# The test executable links jlmuesli, which pulls in libcxxwrap_julia and therefore libjulia.
# Unlike a shared library, an executable has to resolve those symbols at link time, so libjulia
# must be on the link line even though these tests never call into the Julia runtime.
#
# JlCxxConfig runs find_package(Julia) without REQUIRED, so when the julia executable is not on
# CMake's PATH, Julia_LIBRARY silently ends up empty: the library still builds, and only the
# test executable fails, with "undefined reference to jl_top_module@JL_LIBJULIA_...".
if(NOT Julia_LIBRARY OR NOT EXISTS "${Julia_LIBRARY}")
  message(
    WARNING
      "Julia library not found (Julia_LIBRARY='${Julia_LIBRARY}') -- skipping the C++ tests.\n"
      "Make sure the julia executable is on PATH when configuring, or pass it explicitly:\n"
      "  -DJulia_LIBRARY=$(julia -e 'using Libdl; print(abspath(Libdl.dlpath(\"libjulia\")))')\n"
      "Configure with -DJLMUESLI_BUILD_TESTS=OFF to silence this."
  )
  return()
endif()

message(STATUS "Building C++ tests against ${Julia_LIBRARY}")

add_executable(jlmuesli_tests test_helpers.cpp)

target_link_libraries(jlmuesli_tests PRIVATE jlmuesli doctest::doctest "${Julia_LIBRARY}")
target_include_directories(jlmuesli_tests PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_compile_features(jlmuesli_tests PRIVATE cxx_std_17)

# so the loader finds libjulia and libcxxwrap_julia when ctest runs the binary
get_filename_component(_jlmuesli_julia_libdir "${Julia_LIBRARY}" DIRECTORY)
set_target_properties(
  jlmuesli_tests PROPERTIES BUILD_RPATH "${_jlmuesli_julia_libdir};${JlCxx_location}"
)

# Registers each TEST_CASE with CTest individually, so a failure names the case.
doctest_discover_tests(jlmuesli_tests)
