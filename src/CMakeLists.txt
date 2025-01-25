add_library(jlmuesli SHARED
  muesli.cpp
)

# target_include_directories(jlmuesli
#     PUBLIC 
#         # This makes the headers visible while building (the source tree)
#         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
#         # This makes the headers visible after installing (the install tree)
#         $<INSTALL_INTERFACE:include> 
# )


