cmake_minimum_required(VERSION 3.18)

project(Jlmuesli)

set(CMAKE_CXX_STANDARD 20)          
set(CMAKE_CXX_STANDARD_REQUIRED ON) 

set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_library(MUESLI_LIB NAMES muesli PATHS /usr/local/lib)
find_path(MUESLI_INCLUDE_DIR NAMES muesli PATHS /usr/local/include)


set(CMAKE_PREFIX_PATH "/root/.julia/artifacts/7a508f56099aa725e5f3cd1623d9a33e3787d163" ${CMAKE_PREFIX_PATH})

find_package(JlCxx)
get_target_property(JlCxx_location JlCxx::cxxwrap_julia LOCATION)
get_filename_component(JlCxx_location ${JlCxx_location} DIRECTORY)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib;${JlCxx_location}")

message(STATUS "Found JlCxx at ${JlCxx_location}")

add_subdirectory(src)
