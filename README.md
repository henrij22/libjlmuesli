# libjlmuesli
[![Check build](https://github.com/henrij22/libjlmuesli/actions/workflows/check_build.yml/badge.svg)](https://github.com/henrij22/libjlmuesli/actions/workflows/check_build.yml)

Set your `CMAKE_PREFIX_PATH` to something like 

```
-DCMAKE_PREFIX_PATH=/home/user/.julia/artifacts/7a508f56099aa725e5f3cd1623d9a33e3787d163/
```

The path for `CMAKE_PREFIX_PATH` can be obtained from Julia using:

```julia
julia> using CxxWrap
julia> CxxWrap.prefix_path()
```

## Prerequisits

- muesli library from https://bitbucket.org/ignromero/muesli/src/master/
