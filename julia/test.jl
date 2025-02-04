# SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de
# SPDX-License-Identifier: MIT

using LinearAlgebra

module MuesliTest
using CxxWrap
@wrapmodule(()->joinpath("/workspaces/libjlmuesli/build/lib/", "libjlmuesli"))

function __init__()
    @initcxx
end

end

function toΛandμ(; Emod::AbstractFloat, ν::AbstractFloat)
    μ = Emod / (2 + 2 * ν)
    Λ = Emod * ν / ((1 + ν) * (1 - 2ν))
    Λ, μ
end

Emod = 1000.0
ν = 0.25
Λ, μ = toΛandμ(Emod = Emod, ν = ν)
K = Emod / (3 * (1 - 2ν))

mat = MuesliTest.ElasticIsotropicMaterial(Emod, ν)

mp_ = MuesliTest.createMaterialPoint(mat)

properties = MuesliTest.MaterialProperties()
MuesliTest.setProperty!(properties, "young", Emod)
MuesliTest.setProperty!(properties, "poisson", ν)
MuesliTest.getProperty(properties, "young")

using BenchmarkTools

vec = MuesliTest.Ivector(1.0, 2.0, 3.0)
vec = [1.0, 2.0, 3.0]
@benchmark MuesliTest.Itensor(vec, vec, vec)
T = [1.0 2.0 3.0
     4.0 5.0 6.0
     5.0 3.0 2.3]
@benchmark MuesliTest.Itensor(T)

Base.IndexStyle(::MuesliTest.Itensor) = IndexLinear()
Base.getindex(v::MuesliTest.Itensor, i::Int, j::Int) = MuesliTest.cxxgetindex(v, i, j)[]
function Base.setindex!(v::MuesliTest.Itensor, val, i::Int, j::Int)
    MuesliTest.cxxsetindex!(v, convert(Float64, val), i, j)
end

Base.IndexStyle(::MuesliTest.Itensor4) = IndexLinear()
function Base.getindex(v::MuesliTest.Itensor4, i::Int, j::Int, k::Int, l::Int)
    MuesliTest.cxxgetindex(v, i, j, k, l)[]
end
function Base.setindex!(v::MuesliTest.Itensor4, val, i::Int, j::Int, k::Int, l::Int)
    MuesliTest.cxxsetindex!(v, convert(Float64, val), i, j, k, l)
end

t = MuesliTest.Itensor(T)
t2 = MuesliTest.Istensor(T)

C = rand(3, 3, 3, 3);
ten4 = MuesliTest.Itensor4(C)

nothing
# function Base.show(io::IO, obj::MuesliTest.Itensor)
#     printObject(io, obj, multiline = false)
# end

# function Base.show(io::IO, mime::MIME"text/plain", obj::MuesliTest.Itensor)
#     multiline = get(io, :multiline, true)
#     printObject(io, obj, multiline = multiline)
# end

# function printObject(io::IO, obj::MuesliTest.Itensor; multiline::Bool)
#     if multiline
#         # print("ITensor")
#         # print("\n")
#         for i in 1:3
#             print("$(obj(i, 1)) $(obj(i, 2)) $(obj(i, 3))\n")
#         end
#     else
#         # print("ITensor")
#     end
# end

# t = MuesliTest.Itensor(T)

nothing
# MuesliTest.getProperty(mat, MuesliTest.PR_POISSON)

# mat2 = MuesliTest.ElasticIsotropicMaterial(properties)
# MuesliTest.print(mat2)

# MuesliTest.check(mat2)

# mp = MuesliTest.ElasticIsotropicMP(mat)

C = [0.600872 -0.179083 0
     -0.179083 0.859121 0
     0 0 1]

F_ = sqrt(C)
F = MuesliTest.Itensor(F_)


# tensors = MuesliTest.ArrayOfIsTensors()
# MuesliTest.push!(tensors, C)
# MuesliTest.clear!(tensors)
# Int(MuesliTest.size(tensors))

# using Tensors
# MuesliTest.updateCurrentState(mp, 1.0, C)

σ = MuesliTest.Istensor()
# ℂ = zeros(3, 3, 3, 3);

# MuesliTest.stress!(mp, σ)
# σ

# MuesliTest.storedEnergy(mp)
# MuesliTest.tangentTensor!(mp, ℂ)
# tovoigt(Array, SymmetricTensor{4, 3}(ℂ))

nh = MuesliTest.NeoHookeMaterial(properties)
nhmp = MuesliTest.NeoHookeMP(nh)
MuesliTest.updateCurrentState(nhmp, 0.0, F)
MuesliTest.secondPiolaKirchhoffStress!(nhmp, σ)


# σ
# MuesliTest.convectedTangent!(nhmp, ℂ)
# tovoigt(Array, SymmetricTensor{4, 3}(ℂ))

# svk = MuesliTest.SVKMaterial(properties)
# svkmp = MuesliTest.SVKMP(svk)
# MuesliTest.updateCurrentState(svkmp, 0.0, F)
# MuesliTest.secondPiolaKirchhoffStress(svkmp)
# tovoigt(SymmetricTensor{4, 3}(MuesliTest.convectedTangent(svkmp)))

# yeoh = MuesliTest.YeohMaterial(μ / 2, μ / 6, μ / 3, K, true)
# yeohmp = MuesliTest.YeohMP(yeoh)
# MuesliTest.updateCurrentState(yeohmp, 0.0, F)
# MuesliTest.secondPiolaKirchhoffStress(yeohmp)
# tovoigt(SymmetricTensor{4, 3}(MuesliTest.convectedTangent(yeohmp)))
