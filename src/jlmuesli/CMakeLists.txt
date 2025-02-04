# SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de SPDX-License-Identifier:
# GPL-3.0-or-later

# add_library(jlmuesli SHARED muesli.cpp 
#                             util/helpers.cpp 
#                             util/materialstate.cpp
#                             util/tensors.cpp
#                             util/propertynames.cpp)





add_subdirectory(util)
add_subdirectory(smallstrain)
add_subdirectory(finitestrain)
