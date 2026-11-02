# Load each DLL of the chain on its own: Julia's error for a failed dlopen does not say which
# dependency was the problem.
using Libdl

for name in ARGS
    try
        dlopen(name)
        println("ok   ", name)
    catch err
        println("FAIL ", name, ": ", sprint(showerror, err))
    end
end
println("\nloaded now:")
foreach(println, filter(p -> occursin(r"(mingw64|muesli|cxxwrap|openblas|gfortran|libgcc|libstdc|winpthread)"i, p), dllist()))
