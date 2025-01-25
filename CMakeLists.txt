cmake_minimum_required(VERSION 3.18)

project(Jlmuesli)

set(CMAKE_CXX_STANDARD 17)          
set(CMAKE_CXX_STANDARD_REQUIRED ON) 

set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# set(CMAKE_PREFIX_PATH "/home/henri/.julia/artifacts/7a508f56099aa725e5f3cd1623d9a33e3787d163/"  "/usr/local/lib/" ${CMAKE_PREFIX_PATH})

find_package(JlCxx REQUIRED)
find_package(Muesli REQUIRED)

get_target_property(JlCxx_location JlCxx::cxxwrap_julia LOCATION)
get_target_property(muesli_location muesli LOCATION)
get_filename_component(JlCxx_location ${JlCxx_location} DIRECTORY)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib;${JlCxx_location}")


message(STATUS "Found JlCxx at ${JlCxx_location}")
message(STATUS "Found Muesli at ${muesli_location}")

add_subdirectory(src)

target_include_directories(jlmuesli
  PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
)

add_subdirectory(include)

target_link_libraries(jlmuesli muesli)
target_link_libraries(jlmuesli JlCxx::cxxwrap_julia)

install(TARGETS
jlmuesli
LIBRARY DESTINATION lib
ARCHIVE DESTINATION lib
RUNTIME DESTINATION lib)

