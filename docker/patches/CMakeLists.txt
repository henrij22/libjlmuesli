# SPDX-FileCopyrightText: 2021-2024 The Ikarus Developers mueller@ibb.uni-stuttgart.de
# SPDX-License-Identifier: LGPL-3.0-or-later

# This file is intended to generate a CMake project for the Muesli material library, and is not part of said library

cmake_minimum_required(VERSION 3.10)

project(muesli)

include_directories(".")

file(GLOB SRC_FILES
  "muesli/*.cpp"
  "muesli/Damage/*.cpp"
  "muesli/Datadriven/*.cpp"
  "muesli/Diffusion/*.cpp"
  "muesli/Failure/*.cpp"
  "muesli/Fcoupled/*.cpp"
  "muesli/Finitestrain/*.cpp"
  "muesli/Fluid/*.cpp"
  "muesli/Interface/*.cpp"
  "muesli/Math/*.cpp"
  "muesli/Scoupled/*.cpp"
  "muesli/Smallstrain/*.cpp"
  "muesli/Thermal/*.cpp"
  "muesli/Utils/*.cpp"
)

file(GLOB HEADER_FILES
  "muesli/*.h"
  "muesli/Damage/*.h"
  "muesli/Datadriven/*.h"
  "muesli/Diffusion/*.h"
  "muesli/Failure/*.h"
  "muesli/Fcoupled/*.h"
  "muesli/Finitestrain/*.h"
  "muesli/Fluid/*.h"
  "muesli/Interface/*.h"
  "muesli/Math/*.h"
  "muesli/Scoupled/*.h"
  "muesli/Smallstrain/*.h"
  "muesli/Thermal/*.h"
  "muesli/Utils/*.h"
)

add_library(muesli ${SRC_FILES} ${HEADER_FILES})

install(TARGETS muesli EXPORT muesli-targets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

install(DIRECTORY muesli
  DESTINATION include/
  FILES_MATCHING PATTERN "*.h"
)
install(EXPORT muesli-targets
  FILE MuesliTargets.cmake
  DESTINATION lib/cmake/Muesli)

include(CMakePackageConfigHelpers)

# generate the config file that is includes the exports
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/MuesliConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/MuesliConfig.cmake"
  INSTALL_DESTINATION "lib/cmake/example"
  NO_SET_AND_CHECK_MACRO
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

# generate the version file for the config file
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/MuesliConfigVersion.cmake"
  VERSION "${muesli_VERSION_MAJOR}.${muesli_VERSION_MINOR}"
  COMPATIBILITY AnyNewerVersion
)

# Use -fPIC to generate position-independent code required for linking into shared libraries.
set_property(TARGET muesli PROPERTY POSITION_INDEPENDENT_CODE ON)

# install the configuration file
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/MuesliConfig.cmake
  DESTINATION lib/cmake/Muesli
)
