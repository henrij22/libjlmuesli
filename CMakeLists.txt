# SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de SPDX-License-Identifier:
# GPL-3.0-or-later

cmake_minimum_required(VERSION 3.18)

project(Jlmuesli)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(JlCxx REQUIRED)
find_package(Muesli REQUIRED)

get_target_property(JlCxx_location JlCxx::cxxwrap_julia LOCATION)
get_filename_component(JlCxx_location ${JlCxx_location} DIRECTORY)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib;${JlCxx_location}")

message(STATUS "Found JlCxx at ${JlCxx_location}")

option(JLMUESLI_BUILD_TESTS "Build the C++ unit tests (requires doctest, fetched if absent)" ON)

# A cross build cannot run the resulting test binary, and the BinaryBuilder sandbox that
# produces the JLL has no network for FetchContent to pull doctest with. Turn the tests
# off there rather than making every release recipe remember to pass the flag.
if(JLMUESLI_BUILD_TESTS AND CMAKE_CROSSCOMPILING)
  message(STATUS "Cross-compiling: not building the C++ tests")
  set(JLMUESLI_BUILD_TESTS OFF)
endif()

add_subdirectory(src)
add_subdirectory(cmake)

target_include_directories(jlmuesli PUBLIC "${CMAKE_SOURCE_DIR}/src")

target_link_libraries(jlmuesli muesli JlCxx::cxxwrap_julia)

if(JLMUESLI_BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()

# On Windows the DLL is a RUNTIME artefact, not a LIBRARY one (the import .a is the
# ARCHIVE). It has to land in bin/, which is where BinaryBuilder's ${libdir} points on
# mingw and where the JLL looks for it. Installing it to lib/ instead is what forced the
# build recipe to move the .dll by hand afterwards.
install(
  TARGETS jlmuesli
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
)
