# SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de
# SPDX-License-Identifier: MIT

using LinearAlgebra

module MuesliTest
path_to_lib = "/workspaces/libjlmuesli/build/lib/"

if (!isempty(ARGS))
    path_to_lib = ARGS[1]
end

using CxxWrap
@wrapmodule(()->joinpath(path_to_lib, "libjlmuesli"))

function __init__()
    @initcxx
end

end

println("Test WrapModule successfull")
exit(0)
