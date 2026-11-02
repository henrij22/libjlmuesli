# SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de SPDX-License-Identifier:
# GPL-3.0-or-later

add_subdirectory(jlmuesli)

set(JLMUESLI_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/jlmuesli)
set(JLMUESLI_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/jlmuesli)

file(GLOB_RECURSE JLMUESLI_HEADERS ${JLMUESLI_INCLUDE_DIR}/*.hh)

# Explicitly list the source files
set(JLMUESLI_SOURCES
    ${JLMUESLI_SOURCE_DIR}/muesli.cpp
    ${JLMUESLI_SOURCE_DIR}/util/helpers.cpp
    ${JLMUESLI_SOURCE_DIR}/util/materialbase.cpp
    ${JLMUESLI_SOURCE_DIR}/util/materialstate.cpp
    ${JLMUESLI_SOURCE_DIR}/util/tensors.cpp
    ${JLMUESLI_SOURCE_DIR}/util/propertynames.cpp
    ${JLMUESLI_SOURCE_DIR}/finitestrain/finitestrainbindings.cpp
    ${JLMUESLI_SOURCE_DIR}/smallstrain/smallstrainbindings.cpp
)

set_target_properties(${JLCXX_TARGET} PROPERTIES PUBLIC_HEADER "${JLMUESLI_HEADERS}")

add_library(jlmuesli SHARED ${JLMUESLI_SOURCES})

if(MINGW)
  # the templated jlcxx registration produces object files with more sections than COFF allows by default
  target_compile_options(jlmuesli PRIVATE -Wa,-mbig-obj)
  # Julia ships its own, older libstdc++ and this library is loaded next to it. Not
  # -static-libgcc: libmuesli would export its unwinder symbols and clash with ours.
  target_link_options(jlmuesli PRIVATE -static-libstdc++)
endif()
