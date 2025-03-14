# SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de
# SPDX-License-Identifier: MIT

using LinearAlgebra

module MuesliTest
# path_to_lib = "/workspaces/libjlmuesli/build/lib/"
path_to_lib = "/home/henri/dev/libjlmuesli/build/lib/"

if (!isempty(ARGS))
    path_to_lib = ARGS[1]
end

using CxxWrap
@wrapmodule(()->joinpath(path_to_lib, "libjlmuesli"))

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

# t = MuesliTest.Itensor(T)

# MuesliTest.getProperty(mat, MuesliTest.PR_POISSON)

# mat2 = MuesliTest.ElasticIsotropicMaterial(properties)
# MuesliTest.print(mat2)

# MuesliTest.check(mat2)

# mp = MuesliTest.ElasticIsotropicMP(mat)

# C = [0.600872 -0.179083 0
#      -0.179083 0.859121 0
#      0 0 1]
# C = MuesliTest.Itensor(C)

# F_ = sqrt(C)
# F = MuesliTest.Itensor(F_)

# tensors = MuesliTest.ArrayOfIsTensors()
# MuesliTest.push!(tensors, C)
# MuesliTest.clear!(tensors)
# Int(MuesliTest.size(tensors))

# using Tensors
# MuesliTest.updateCurrentState(mp, 1.0, C)

# σ = MuesliTest.Istensor()
# ℂ = zeros(3, 3, 3, 3);

# MuesliTest.stress!(mp, σ)
# σ

# MuesliTest.storedEnergy(mp)
# MuesliTest.tangentTensor!(mp, ℂ)
# tovoigt(Array, SymmetricTensor{4, 3}(ℂ))

# nh = MuesliTest.NeoHookeMaterial(properties)
# nhmp = MuesliTest.NeoHookeMP(nh)
# MuesliTest.updateCurrentState(nhmp, 0.0, F)
# MuesliTest.secondPiolaKirchhoffStress!(nhmp, σ)

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
