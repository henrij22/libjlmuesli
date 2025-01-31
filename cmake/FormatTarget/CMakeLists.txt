# SPDX-FileCopyrightText: 2021-2025 The Ikarus Developers mueller@ibb.uni-stuttgart.de
# SPDX-License-Identifier: LGPL-3.0-or-later

# partially taken from
# https://github.com/ikarus-project/ikarus/blob/main/cmake/FormatTarget/CMakeLists.txt

cmake_minimum_required(VERSION 3.18 FATAL_ERROR)
project(IkarusFormat)

if(ADD_FORMATTARGET)
  include(../CPM.cmake)
  CPMAddPackage(
    NAME Format.cmake
    VERSION 1.8.1
    GITHUB_REPOSITORY TheLartians/Format.cmake
    OPTIONS "FORMAT_SKIP_CMAKE NO" "CMAKE_FORMAT_EXCLUDE config.h.cmake"
  )
endif()
