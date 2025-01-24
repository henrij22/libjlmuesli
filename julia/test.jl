using FeaJiNL
using LinearAlgebra

module Muesli
using CxxWrap
@wrapmodule(()->joinpath("/home/henri/dev/libjlmuesli/build/lib/", "libjlmuesli"))

function __init__()
    @initcxx
end

export ElasticIsotropicMaterial, ElasticIsotropicMP, NeoHookeMaterial, NeoHookeMP
export stress, storedEnergy, tangentTensor
end

mat = Muesli.ElasticIsotropicMaterial(1000, 0.25, 0.0)

properties = Muesli.MaterialProperties()
Muesli.set(properties, "young", 1000)
Muesli.set(properties, "poisson", 0.25)
Muesli.get(properties, "young")

mat2 = Muesli.ElasticIsotropicMaterial(properties)
Muesli.print(mat2)

Muesli.check(mat2)

mp = Muesli.ElasticIsotropicMP(mat)

C = [0.600872 -0.179083 0
     -0.179083 0.859121 0
     0 0 1]

F = sqrt(C)

using Tensors
Muesli.updateCurrentState(mp, 1.0, C)

Muesli.stress(mp)
Muesli.storedEnergy(mp)
CC = SymmetricTensor{4, 3}(Muesli.tangentTensor(mp));
tovoigt(Array, CC)

nh = Muesli.NeoHookeMaterial(properties)
nhmp = Muesli.NeoHookeMP(nh)
Muesli.updateCurrentState(nhmp, 0.0, F)
Muesli.secondPiolaKirchhoffStress(nhmp)

using BenchmarkTools
tovoigt(Array, SymmetricTensor{4, 3}(Muesli.convectedTangent(nhmp)))
