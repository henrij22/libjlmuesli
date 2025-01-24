add_library(jlmuesli SHARED
  muesli.cpp
)

include_directories(${MUESLI_INCLUDE_DIR})
target_link_libraries(jlmuesli ${MUESLI_LIB})
target_link_libraries(jlmuesli JlCxx::cxxwrap_julia)

install(TARGETS
jlmuesli
LIBRARY DESTINATION lib
ARCHIVE DESTINATION lib
RUNTIME DESTINATION lib)
