using FeaJiNL
using LinearAlgebra

module Muesli
using CxxWrap
@wrapmodule(()->joinpath("/home/henri/dev/libjlmuesli/build/lib/", "libjlmuesli"))

function __init__()
    @initcxx
end

end

Emod = 1000.0
ν = 0.25
Λ, μ = FeaJiNL.toΛandμ(Emod = Emod, ν = ν)
K = Emod / (3 * (1 - 2ν))

mat = Muesli.ElasticIsotropicMaterial(Emod, ν)

properties = Muesli.MaterialProperties()
Muesli.setProperty!(properties, "young", Emod)
Muesli.setProperty!(properties, "poisson", ν)
Muesli.getProperty(properties, "young")

Muesli.getProperty(mat, Muesli.PR_POISSON)

mat2 = Muesli.ElasticIsotropicMaterial(properties)
Muesli.print(mat2)

Muesli.check(mat2)

mp = Muesli.ElasticIsotropicMP(mat)

C = [0.600872 -0.179083 0
     -0.179083 0.859121 0
     0 0 1]

F = sqrt(C)

tensors = Muesli.ArrayOfIsTensors()
Muesli.push!(tensors, C)
Muesli.clear!(tensors)
Int(Muesli.size(tensors))

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
tovoigt(SymmetricTensor{4, 3}(Muesli.convectedTangent(nhmp)))

svk = Muesli.SVKMaterial(properties)
svkmp = Muesli.SVKMP(svk)
Muesli.updateCurrentState(svkmp, 0.0, F)
Muesli.secondPiolaKirchhoffStress(svkmp)
tovoigt(SymmetricTensor{4, 3}(Muesli.convectedTangent(svkmp)))

yeoh = Muesli.YeohMaterial(μ / 2, μ / 6, μ / 3, K, true)
yeohmp = Muesli.YeohMP(yeoh)
Muesli.updateCurrentState(yeohmp, 0.0, F)
Muesli.secondPiolaKirchhoffStress(yeohmp)
tovoigt(SymmetricTensor{4, 3}(Muesli.convectedTangent(yeohmp)))
