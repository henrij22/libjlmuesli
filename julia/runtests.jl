# SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de
# SPDX-License-Identifier: MIT
#
# Integration test suite for the libjlmuesli bindings.
#
# Usage:  julia --project=julia julia/runtests.jl <path_to_lib_dir>
#
# Loads the freshly built shared library directly, so it exercises the actual registration
# code rather than a released MuesliMaterialsWrapper_jll. This is the real coverage for the
# bindings -- the C++ unit tests in test/ only reach helpers that avoid the Julia runtime.
#
# If muesli itself is not on the loader path, prefix with
#   LD_PRELOAD=<prefix>/lib/libmuesli.so
# so the muesli you compiled against is the one that gets loaded. See the README.

using Test

const LIBDIR = if !isempty(ARGS)
    ARGS[1]
elseif haskey(ENV, "JLMUESLI_LIB")
    ENV["JLMUESLI_LIB"]
else
    error("Usage: julia --project=julia julia/runtests.jl <path_to_lib_dir>")
end

isdir(LIBDIR) || error("Library directory not found: $LIBDIR")

module M
using CxxWrap
@wrapmodule(() -> joinpath(Main.LIBDIR, "libjlmuesli"))
__init__() = @initcxx
end

# --- helpers ---------------------------------------------------------------------------

# The raw module has only cxxgetindex; the Base.getindex sugar lives in MuesliMaterials.jl.
el(t, idx...) = M.cxxgetindex(t, idx...)[]
mat3(t) = [el(t, i, j) for i in 1:3, j in 1:3]
ten4(t) = [el(t, i, j, k, l) for i in 1:3, j in 1:3, k in 1:3, l in 1:3]
δ(i, j) = i == j ? 1.0 : 0.0
const I3 = [1.0 0 0; 0 1 0; 0 0 1]

lame(E, ν) = (E * ν / ((1 + ν) * (1 - 2ν)), E / (2 * (1 + ν)))   # (λ, μ)

function properties(pairs...)
    p = M.MaterialProperties()
    for (k, v) in pairs
        M.setProperty!(p, k, v)
    end
    return p
end

const E, ν = 210000.0, 0.3
const λ, μ = lame(E, ν)

@testset "libjlmuesli" begin

    @testset "Tensor types" begin
        @testset "Ivector" begin
            v = [1.0, 2.0, 3.0]
            iv = M.Ivector(v)
            @test [M.cxxgetindex(iv, i)[] for i in 1:3] ≈ v

            # indices are 1-based
            @test M.cxxgetindex(iv, 1)[] == 1.0
            @test M.cxxgetindex(iv, 3)[] == 3.0

            M.cxxsetindex!(iv, 9.0, 2)
            @test M.cxxgetindex(iv, 2)[] == 9.0
        end

        @testset "Itensor round-trips a Julia matrix" begin
            # Julia is column-major, muesli's constructor is row-major -- this is the
            # conversion that has to line up.
            m = [1.0 2.0 3.0; 4.0 5.0 6.0; 7.0 8.0 9.0]
            t = M.Itensor(m)
            @test mat3(t) ≈ m

            # in particular it must not silently transpose
            @test el(t, 1, 2) == 2.0
            @test el(t, 2, 1) == 4.0
        end

        @testset "Istensor takes the symmetric part" begin
            s = [1.0 0.2 0.3; 0.2 2.0 0.4; 0.3 0.4 3.0]
            t = M.Istensor(s)
            @test mat3(t) ≈ s

            @testset "it is stored symmetrically" begin
                got = mat3(t)
                @test got ≈ got'
            end
        end

        @testset "Itensor4 round-trips a 3x3x3x3 array" begin
            a = reshape(collect(1.0:81.0), 3, 3, 3, 3)
            t = M.Itensor4(a)
            @test ten4(t) ≈ a
            @test el(t, 1, 2, 3, 1) == a[1, 2, 3, 1]
        end

        @testset "wrong sizes are rejected" begin
            @test_throws Exception M.Ivector([1.0, 2.0])
            @test_throws Exception M.Itensor(zeros(2, 2))
            @test_throws Exception M.Itensor4(zeros(3, 3, 3, 2))
        end
    end

    @testset "MaterialProperties" begin
        p = properties("young" => E, "poisson" => ν)
        @test M.getProperty(p, "young") == [E]
        @test M.getProperty(p, "poisson") == [ν]
        @test isempty(M.getProperty(p, "nonexistent"))

        @testset "repeated keys accumulate" begin
            q = properties("tau" => 1.0, "tau" => 2.0)
            @test sort(M.getProperty(q, "tau")) == [1.0, 2.0]
        end

        @testset "string options" begin
            q = M.MaterialProperties()
            M.setString!(q, "plasticity", "isotropic")
            @test M.hasKeyword(q, "plasticity")
            @test M.getString(q, "plasticity") == "isotropic"
            @test M.getString(q, "absent") == "NotFound"
        end
    end

    @testset "Small strain: linear isotropic elasticity" begin
        mat = M.ElasticIsotropicMaterial(E, ν)
        @test M.check(mat)

        mp = M.ElasticIsotropicMP(mat)
        ε = [0.001 0.0002 0.0; 0.0002 -0.0005 0.0001; 0.0 0.0001 0.0003]
        M.updateCurrentState(mp, 1.0, M.Istensor(ε))

        σ = M.Istensor()
        M.stress!(mp, σ)
        got = mat3(σ)

        @testset "stress matches the analytic law" begin
            expected = 2μ * ε + λ * (ε[1, 1] + ε[2, 2] + ε[3, 3]) * I3
            @test got ≈ expected rtol = 1e-12
        end

        @testset "stored energy is half the stress power" begin
            @test M.storedEnergy(mp) ≈ 0.5 * sum(got .* ε) rtol = 1e-10
        end

        @testset "tangent matches the analytic isotropic tensor" begin
            C = M.Itensor4()
            M.tangentTensor!(mp, C)
            expected = [λ * δ(i, j) * δ(k, l) + μ * (δ(i, k) * δ(j, l) + δ(i, l) * δ(j, k))
                        for i in 1:3, j in 1:3, k in 1:3, l in 1:3]
            @test ten4(C) ≈ expected rtol = 1e-10
        end

        @testset "pressure is minus a third of the trace" begin
            @test M.pressure(mp) ≈ -(got[1, 1] + got[2, 2] + got[3, 3]) / 3 rtol = 1e-10
        end

        @testset "zero strain gives zero stress" begin
            mp0 = M.ElasticIsotropicMP(mat)
            M.updateCurrentState(mp0, 1.0, M.Istensor(zeros(3, 3)))
            s0 = M.Istensor()
            M.stress!(mp0, s0)
            @test all(abs.(mat3(s0)) .< 1e-12)
            @test M.storedEnergy(mp0) ≈ 0.0 atol = 1e-12
        end

        @testset "response is linear in the strain" begin
            mpa = M.ElasticIsotropicMP(mat)
            M.updateCurrentState(mpa, 1.0, M.Istensor(2 .* ε))
            sa = M.Istensor()
            M.stress!(mpa, sa)
            @test mat3(sa) ≈ 2 .* got rtol = 1e-10
        end
    end

    @testset "The Julia type hierarchy mirrors the C++ one" begin
        # jlcxx has two independent views of inheritance: the Julia supertype declared by
        # add_type(..., julia_base_type<Base>()), and SuperType<T>, which cxxupcast uses to
        # reach inherited methods. They have to agree, or Julia dispatch and the C++ cast
        # chain disagree about what a type is.
        @test M.SmallStrainMaterial <: M.Material
        @test M.SmallStrainMP <: M.MaterialPoint
        @test M.ElasticIsotropicMaterial <: M.SmallStrainMaterial
        @test M.ElasticAnisotropicMaterial <: M.SmallStrainMaterial

        @testset "orthotropic families refine the anisotropic material, not the base" begin
            @test M.ElasticOrthotropicMaterial <: M.ElasticAnisotropicMaterial
            @test M.ElasticOrthotropicMP <: M.ElasticAnisotropicMP
            @test M.ElasticTransverselyisotropicMaterial <: M.ElasticAnisotropicMaterial
            @test M.ElasticTransverselyisotropicMP <: M.ElasticAnisotropicMP
        end

        @testset "damage models refine the damage base" begin
            for T in (M.GTN_Material, M.Gurson_Material, M.Lemaitre_Material, M.LemKin_Material)
                @test T <: M.SdamageMaterial
            end
            for T in (M.GTN_MP, M.Gurson_MP, M.Lemaitre_MP, M.LemKin_MP)
                @test T <: M.SdamageMP
            end
        end

        @testset "finite strain" begin
            @test M.FiniteStrainMaterial <: M.Material
            @test M.NeoHookeMaterial <: M.F_invariants
            @test M.YeohMaterial <: M.F_invariants
            @test M.SVKMaterial <: M.FiniteStrainMaterial
            @test M.NeoHookeMP <: M.FisotropicMP
        end

        @testset "inherited methods reach through the longer upcast chain" begin
            # What matters here is dispatch: a method registered on a base wrapper must be
            # callable on a type that now sits two levels below it. Only reachability is
            # asserted, not the values -- muesli's orthotropic constructors leave part of the
            # material uninitialised, so check() and the stresses are not reproducible from
            # one process to the next. See the note in the README.
            c9 = [210000.0, 210000.0, 210000.0, 80000.0, 80000.0, 80000.0, 0.3, 0.3, 0.3]
            ortho = M.ElasticOrthotropicMaterial(c9, 1.0)

            @test M.check(ortho) isa Bool                     # registered on the material
            @test M.createMaterialPoint(ortho) !== nothing

            mp = M.ElasticOrthotropicMP(ortho)
            M.updateCurrentState(mp, 1.0, M.Istensor([0.001 0.0 0.0; 0.0 0.0 0.0; 0.0 0.0 0.0]))
            σ = M.Istensor()
            @test (M.stress!(mp, σ); true)                    # registered on the MP
            @test M.storedEnergy(mp) isa Float64
        end
    end

    @testset "Damage materials build from a property map" begin
        # The property-map constructor used to be handed the registration prefix verbatim,
        # naming these materials "GTN_" rather than "GTN".
        props = properties("young" => E, "poisson" => ν, "density" => 1.0,
                           "q1" => 1.5, "q2" => 1.0, "yield" => 200.0)
        for (name, T) in (("GTN", M.GTN_Material), ("Gurson", M.Gurson_Material))
            @testset "$name" begin
                @test M.check(T(props))
            end
        end
    end

    @testset "Small strain: built from MaterialProperties" begin
        # The material name used to be hard-coded as "Elastic" for every material built
        # this way; both routes must give the same material.
        direct = M.ElasticIsotropicMaterial(E, ν)
        fromProps = M.ElasticIsotropicMaterial(properties("young" => E, "poisson" => ν))

        @test M.check(fromProps)
        @test M.getProperty(fromProps, M.PR_YOUNG) ≈ E
        @test M.getProperty(fromProps, M.PR_POISSON) ≈ ν
        @test M.getProperty(fromProps, M.PR_YOUNG) ≈ M.getProperty(direct, M.PR_YOUNG)

        ε = [0.001 0.0 0.0; 0.0 0.0 0.0; 0.0 0.0 0.0]
        stress_of(m) = begin
            mp = M.ElasticIsotropicMP(m)
            M.updateCurrentState(mp, 1.0, M.Istensor(ε))
            s = M.Istensor()
            M.stress!(mp, s)
            mat3(s)
        end
        @test stress_of(direct) ≈ stress_of(fromProps)
    end

    @testset "Finite strain: neo-Hookean" begin
        mat = M.NeoHookeMaterial(properties("young" => E, "poisson" => ν))
        @test M.check(mat)
        mp = M.NeoHookeMP(mat)

        @testset "the undeformed state is stress free" begin
            M.updateCurrentState(mp, 1.0, M.Itensor(I3))
            P = M.Itensor()
            M.firstPiolaKirchhoffStress!(mp, P)
            @test all(abs.(mat3(P)) .< 1e-8)
            @test M.storedEnergy(mp) ≈ 0.0 atol = 1e-8
        end

        @testset "analytic stress agrees with muesli's numerical derivative" begin
            F = [1.05 0.02 0.0; 0.0 0.98 0.01; 0.0 0.0 1.01]
            M.updateCurrentState(mp, 1.0, M.Itensor(F))

            P = M.Itensor()
            M.firstPiolaKirchhoffStress!(mp, P)
            Pn = M.Itensor()
            M.firstPiolaKirchhoffStressNumerical!(mp, Pn)
            @test mat3(P) ≈ mat3(Pn) rtol = 1e-5 atol = 1e-6
        end

        @testset "second Piola-Kirchhoff stress is symmetric and consistent with P" begin
            F = [1.05 0.02 0.0; 0.0 0.98 0.01; 0.0 0.0 1.01]
            M.updateCurrentState(mp, 1.0, M.Itensor(F))

            S = M.Istensor()
            M.secondPiolaKirchhoffStress!(mp, S)
            Sm = mat3(S)
            @test Sm ≈ Sm'

            P = M.Itensor()
            M.firstPiolaKirchhoffStress!(mp, P)
            @test mat3(P) ≈ F * Sm rtol = 1e-8      # P = F S
        end

        @testset "Kirchhoff and Cauchy stress differ by the Jacobian" begin
            F = [1.05 0.0 0.0; 0.0 0.98 0.0; 0.0 0.0 1.01]
            M.updateCurrentState(mp, 1.0, M.Itensor(F))

            τ = M.Istensor()
            M.KirchhoffStress!(mp, τ)
            σ = M.Istensor()
            M.CauchyStress!(mp, σ)

            J = F[1, 1] * F[2, 2] * F[3, 3]
            @test mat3(τ) ≈ J .* mat3(σ) rtol = 1e-8
        end

        @testset "the deformation gradient is stored as given" begin
            F = [1.05 0.02 0.0; 0.0 0.98 0.01; 0.0 0.0 1.01]
            M.updateCurrentState(mp, 1.0, M.Itensor(F))
            @test mat3(M.deformationGradient(mp)) ≈ F
        end
    end

    @testset "State bookkeeping" begin
        mat = M.ElasticIsotropicMaterial(E, ν)
        mp = M.ElasticIsotropicMP(mat)
        ε = [0.001 0.0 0.0; 0.0 0.0 0.0; 0.0 0.0 0.0]

        M.updateCurrentState(mp, 1.0, M.Istensor(ε))
        M.commitCurrentState(mp)

        @testset "materialState getters are 1-based" begin
            st = M.getCurrentState(mp)
            n = Int(M.getStensorSize(st))
            @test n ≥ 1
            @test M.getStensor(st, 1) !== nothing
            @test_throws Exception M.getStensor(st, 0)
            @test_throws Exception M.getStensor(st, n + 1)
        end

        @testset "resetCurrentState returns to the committed state" begin
            M.updateCurrentState(mp, 2.0, M.Istensor(5 .* ε))
            M.resetCurrentState(mp)
            s = M.Istensor()
            M.stress!(mp, s)
            expected = 2μ * ε + λ * ε[1, 1] * I3
            @test mat3(s) ≈ expected rtol = 1e-10
        end
    end

    @testset "Other hyperelastic materials build and respond" begin
        props = properties("young" => E, "poisson" => ν)
        for (name, matT, mpT) in (("SVK", M.SVKMaterial, M.SVKMP),
                                  ("Yeoh", M.YeohMaterial, M.YeohMP))
            @testset "$name" begin
                mat = matT(props)
                @test M.check(mat)
                mp = mpT(mat)
                M.updateCurrentState(mp, 1.0, M.Itensor(I3))
                P = M.Itensor()
                M.firstPiolaKirchhoffStress!(mp, P)
                @test all(isfinite, mat3(P))
            end
        end
    end
end
