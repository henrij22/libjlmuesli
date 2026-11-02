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

    @testset "Small strain: transversely isotropic elasticity, engineering constants" begin
        # Regression test for the engineering-constants overload of
        # ElasticTransverselyisotropicMaterial, added alongside the existing stiffness-vector
        # form. Unlike that vector form (see the note above about uninitialised entries), this
        # constructor zeroes the whole stiffness matrix before filling it in, so check() and
        # the stresses below are fully deterministic.
        E1, E2, v12, G23, G12 = 210000.0, 70000.0, 0.3, 25000.0, 30000.0
        mat = M.ElasticTransverselyisotropicMaterial(E1, E2, v12, G23, G12, 1.0)
        @test M.check(mat) isa Bool

        # Replicates muesli's own derivation in
        # elasticTransverselyisotropicMaterial::elasticTransverselyisotropicMaterial(name, E1,
        # E2, v12, G23, G12, rhox), to check the constructor end to end against the resulting
        # stresses rather than just that it runs.
        v21 = E2 / E1 * v12
        v23 = E2 / (2.0 * G23) - 1.0
        lam = (v12 * v21 + v23) / ((1.0 - v23 - 2.0 * v12 * v21) * (1.0 + v23)) * E2
        C11 = (1.0 - v23) / (1.0 - v23 - 2.0 * v12 * v21) * E1
        C12 = 2.0 * v12 * (lam + G23)   # == C13, by transverse symmetry about the x axis
        C22 = lam + 2.0 * G23           # == C33
        C23 = lam

        mp1 = M.ElasticTransverselyisotropicMP(mat)
        M.updateCurrentState(mp1, 1.0, M.Istensor([0.001 0.0 0.0; 0.0 0.0 0.0; 0.0 0.0 0.0]))
        σ1 = M.Istensor()
        M.stress!(mp1, σ1)
        got1 = mat3(σ1)
        @test got1[1, 1] ≈ C11 * 0.001 rtol = 1e-10
        @test got1[2, 2] ≈ C12 * 0.001 rtol = 1e-10
        @test got1[3, 3] ≈ C12 * 0.001 rtol = 1e-10

        mp2 = M.ElasticTransverselyisotropicMP(mat)
        M.updateCurrentState(mp2, 1.0, M.Istensor([0.0 0.0 0.0; 0.0 0.001 0.0; 0.0 0.0 0.0]))
        σ2 = M.Istensor()
        M.stress!(mp2, σ2)
        got2 = mat3(σ2)
        @test got2[2, 2] ≈ C22 * 0.001 rtol = 1e-10
        @test got2[3, 3] ≈ C23 * 0.001 rtol = 1e-10
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
            @test M.cxxCreateMaterialPoint(ortho) !== nothing

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

    @testset "Bulk transfer between tensors and Julia arrays" begin
        m = [1.0 2.0 3.0; 4.0 5.0 6.0; 7.0 8.0 9.0]

        @testset "toArray! copies without transposing" begin
            A = zeros(3, 3)
            M.toArray!(A, M.Itensor(m))
            @test A == m
        end

        @testset "fromArray! overwrites an existing tensor" begin
            t = M.Itensor(zeros(3, 3))
            M.fromArray!(t, m)
            @test mat3(t) == m
        end

        @testset "Ivector" begin
            v = M.Ivector([1.0, 2.0, 3.0])
            a = zeros(3)
            M.toArray!(a, v)
            @test a == [1.0, 2.0, 3.0]
            M.fromArray!(v, [4.0, 5.0, 6.0])
            @test [el(v, i) for i in 1:3] == [4.0, 5.0, 6.0]
        end

        @testset "Itensor4 keeps the first index fastest" begin
            a = reshape(collect(1.0:81.0), 3, 3, 3, 3)
            T = M.Itensor4(zeros(3, 3, 3, 3))
            M.fromArray!(T, a)
            @test ten4(T) == a
            out = zeros(3, 3, 3, 3)
            M.toArray!(out, T)
            @test out == a
        end

        @testset "Istensor stores the symmetric part" begin
            s = M.Istensor(zeros(3, 3))
            M.fromArray!(s, m)
            @test mat3(s) == (m + m') / 2
            out = zeros(3, 3)
            M.toArray!(out, s)
            @test out == (m + m') / 2
        end

        @testset "constructors take the symmetric part of a non-symmetric matrix" begin
            @test mat3(M.Istensor(m)) == (m + m') / 2
            @test mat3(M.Istensor(collect(m'))) == (m + m') / 2
        end

        @testset "the shape is checked, not just the element count" begin
            @test_throws Exception M.toArray!(zeros(1, 9), M.Itensor(m))
            @test_throws Exception M.toArray!(zeros(9, 1), M.Itensor(m))
            @test_throws Exception M.fromArray!(M.Itensor(m), zeros(1, 9))
            @test_throws Exception M.Itensor(zeros(1, 9))
            @test_throws Exception M.Itensor4(zeros(9, 9, 1, 1))
            @test_throws Exception M.toArray!(zeros(2), M.Ivector([1.0, 2.0, 3.0]))
        end

        @testset "the error names the expected shape" begin
            err = try
                M.Itensor(zeros(2, 2))
            catch e
                e
            end
            @test occursin("size 3x3", sprint(showerror, err))
        end
    end

    @testset "Properties shared by every material" begin
        smallstrain = (
            M.ElasticIsotropicMaterial(E, ν),
            M.SplasticMaterial(E, ν, 1.0, 0.0, 0.0, 200.0, 0.0, "mises"),
            M.GTN_Material(properties("young" => E, "poisson" => ν, "density" => 1.0,
                                      "q1" => 1.5, "q2" => 1.0, "yield" => 200.0)),
        )
        finitestrain = (
            M.NeoHookeMaterial(E, ν),
            M.SVKMaterial(E, ν),
            M.YeohMaterial(properties("young" => E, "poisson" => ν)),
        )

        for mat in (smallstrain..., finitestrain...)
            @testset "$(M.name(mat))" begin
                M.setDensity(mat, 7.85)
                @test M.density(mat) == 7.85

                M.setReferenceTemperature(mat, 293.0)
                @test M.referenceTemperature(mat) == 293.0

                @test !M.isVariational(mat)
                M.setVariational(mat)
                @test M.isVariational(mat)

                @test !isempty(String(M.name(mat)))
            end
        end

        @testset "a material point reports the density of its material" begin
            mat = M.ElasticIsotropicMaterial(E, ν)
            M.setDensity(mat, 3.0)
            @test M.density(M.ElasticIsotropicMP(mat)) == 3.0

            fmat = M.NeoHookeMaterial(E, ν)
            M.setDensity(fmat, 4.0)
            @test M.density(M.NeoHookeMP(fmat)) == 4.0
        end
    end

    @testset "createMaterialPoint returns the concrete point type" begin
        @test M.cxxCreateMaterialPoint(M.ElasticIsotropicMaterial(E, ν)) isa M.ElasticIsotropicMP
        @test M.cxxCreateMaterialPoint(M.NeoHookeMaterial(E, ν)) isa M.NeoHookeMP
        @test M.cxxCreateMaterialPoint(M.SVKMaterial(E, ν)) isa M.SVKMP

        mp = M.cxxCreateMaterialPoint(M.ElasticIsotropicMaterial(E, ν))
        ε = [0.001 0.0 0.0; 0.0 0.0 0.0; 0.0 0.0 0.0]
        M.updateCurrentState(mp, 1.0, M.Istensor(ε))
        σ = M.Istensor()
        M.stress!(mp, σ)
        @test mat3(σ) ≈ 2μ * ε + λ * ε[1, 1] * I3 rtol = 1e-10
    end

    @testset "Methods on the family roots reach every material" begin
        @testset "small strain" begin
            for mat in (M.ElasticIsotropicMaterial(E, ν),
                        M.SplasticMaterial(E, ν, 1.0, 0.0, 0.0, 200.0, 0.0, "mises"),
                        M.ViscoelasticMaterial(E, ν, 1.0, 1, [1.0], [1.0]))
                mp = M.cxxCreateMaterialPoint(mat)
                M.updateCurrentState(mp, 1.0, M.Istensor(zeros(3, 3)))
                M.commitCurrentState(mp)
                @test M.storedEnergy(mp) ≈ 0.0 atol = 1e-12
                @test M.getCurrentState(mp) isa M.InterfaceState
            end
        end

        @testset "finite strain" begin
            F = [1.02 0.01 0.0; 0.0 0.99 0.0; 0.0 0.0 1.0]
            for mat in (M.NeoHookeMaterial(E, ν), M.SVKMaterial(E, ν))
                mp = M.cxxCreateMaterialPoint(mat)
                M.updateCurrentState(mp, 1.0, M.Itensor(F))
                M.commitCurrentState(mp)
                @test mat3(M.convergedDeformationGradient(mp)) ≈ F
                @test M.storedEnergy(mp) > 0
            end
        end
    end

    @testset "Small strain: Voigt tangent matrix" begin
        mp = M.ElasticIsotropicMP(M.ElasticIsotropicMaterial(E, ν))
        M.updateCurrentState(mp, 1.0, M.Istensor(zeros(3, 3)))

        Cm = zeros(6, 6)
        M.tangentMatrix!(mp, Cm)

        # order 11, 22, 33, 23, 13, 12; the matrix acts on engineering shear strains
        expected = zeros(6, 6)
        for i in 1:3, j in 1:3
            expected[i, j] = i == j ? λ + 2μ : λ
        end
        for i in 4:6
            expected[i, i] = μ
        end
        @test Cm ≈ expected rtol = 1e-10

        @testset "agrees with the fourth-order tangent" begin
            C = M.Itensor4()
            M.tangentTensor!(mp, C)
            pairs = ((1, 1), (2, 2), (3, 3), (2, 3), (1, 3), (1, 2))
            for (a, (i, j)) in enumerate(pairs), (b, (k, l)) in enumerate(pairs)
                @test Cm[a, b] ≈ el(C, i, j, k, l) rtol = 1e-10
            end
        end

        @test_throws Exception M.tangentMatrix!(mp, zeros(3, 3))
    end

    @testset "Small strain: thermodynamic potentials" begin
        mp = M.ElasticIsotropicMP(M.ElasticIsotropicMaterial(E, ν))
        ε = [0.001 0.0002 0.0; 0.0002 -0.0005 0.0001; 0.0 0.0001 0.0003]
        M.updateCurrentState(mp, 1.0, M.Istensor(ε))

        tp = M.thermodynamicPotentials(mp)
        @test M.volumetric(tp) ≈ M.volumetricEnergy(mp) rtol = 1e-10
        @test M.deviatoric(tp) ≈ M.deviatoricEnergy(mp) rtol = 1e-10
        @test M.internalEnergy(tp) ≈ M.storedEnergy(mp) rtol = 1e-10
    end

    @testset "Finite strain: all tangent contractions at once" begin
        mp = M.NeoHookeMP(M.NeoHookeMaterial(E, ν))
        M.updateCurrentState(mp, 1.0, M.Itensor([1.05 0.02 0.0; 0.0 0.98 0.01; 0.0 0.0 1.01]))
        v1, v2 = M.Ivector([1.0, 0.5, -0.3]), M.Ivector([0.2, -1.0, 0.7])

        Tdev, Tmixed = M.Itensor(), M.Istensor()
        Tvol = M.contractWithAllTangents!(mp, v1, v2, Tdev, Tmixed)

        Tdev_ref, Tmixed_ref = M.Itensor(), M.Istensor()
        M.contractWithDeviatoricTangent!(mp, v1, v2, Tdev_ref)
        M.contractWithMixedTangent!(mp, Tmixed_ref)
        @test mat3(Tdev) ≈ mat3(Tdev_ref)
        @test mat3(Tmixed) ≈ mat3(Tmixed_ref)
        @test Tvol ≈ M.volumetricStiffness(mp)
    end

    @testset "Rate and temperature dependent plasticity" begin
        M.setFixedRandomSeed(true)
        stretch(λ) = [λ 0.0 0.0; 0.0 1.0 0.0; 0.0 0.0 1.0]

        function finite_response(mat, MP, λ, t)
            mp = MP(mat)
            M.updateCurrentState(mp, t, M.Itensor(stretch(λ)))
            P = M.Itensor()
            M.firstPiolaKirchhoffStress!(mp, P)
            return mat3(P)[1, 1], M.plasticSlip(mp)
        end

        @testset "types" begin
            @test M.JohnsonCookMaterial <: M.FiniteStrainMaterial
            @test M.JohnsonCookMP <: M.FiniteStrainMP
            @test M.ZerilliArmstrongMaterial <: M.FiniteStrainMaterial
            @test M.ZerilliArmstrongMP <: M.FiniteStrainMP
            @test M.ViscoplasticNLMaterial <: M.SmallStrainMaterial
            @test M.ViscoplasticNLMP <: M.SmallStrainMP
        end

        @testset "Johnson-Cook" begin
            jc(; T = 293.0) = properties("young" => 124e3, "poisson" => 0.34, "density" => 8.9e-9,
                                         "a" => 90.0, "b" => 292.0, "c" => 0.025, "n" => 0.31, "m" => 1.09,
                                         "edot0" => 1.0, "temp" => T, "modelreftemp" => 293.0, "melttemp" => 1356.0)
            mat = M.JohnsonCookMaterial(jc())
            @test M.check(mat)
            @test M.density(mat) ≈ 8.9e-9
            @test M.cxxCreateMaterialPoint(mat) isa M.JohnsonCookMP

            @testset "yielding" begin
                @test finite_response(mat, M.JohnsonCookMP, 1.0001, 1.0)[2] == 0
                @test finite_response(mat, M.JohnsonCookMP, 1.0, 1.0)[1] ≈ 0 atol = 1e-8
                @test finite_response(mat, M.JohnsonCookMP, 1.01, 1.0)[2] > 0
            end

            @testset "a faster load gives a higher stress" begin
                slow, medium, fast = (finite_response(mat, M.JohnsonCookMP, 1.05, t)[1] for t in (1e3, 1.0, 1e-3))
                @test fast > medium > slow
            end

            @testset "a higher temperature gives a lower stress" begin
                stress(T) = finite_response(M.JohnsonCookMaterial(jc(T = T)), M.JohnsonCookMP, 1.05, 1.0)[1]
                @test stress(293.0) > stress(600.0) > stress(1000.0)
            end

            @testset "MUESLI's own consistency test" begin
                # without the derivative of the energy, which MUESLI skips for this model too
                @test M.testImplementation(M.JohnsonCookMP(mat), false, true)
            end
        end

        @testset "Zerilli-Armstrong" begin
            za(; T = 293.0) = properties("young" => 124e3, "poisson" => 0.34, "density" => 8.9e-9,
                                         "c0" => 20.0, "c1" => 0.0, "c2" => 600.0, "c3" => 0.005, "c4" => 0.0003,
                                         "c5" => 0.0, "n" => 0.5, "temp" => T)
            mat = M.ZerilliArmstrongMaterial(za())
            @test M.check(mat)
            @testset "the density is the one that was set" begin
                # MUESLI's own density() returns a member no constructor initialises
                @test M.density(mat) ≈ 8.9e-9
            end
            @test M.cxxCreateMaterialPoint(mat) isa M.ZerilliArmstrongMP

            @testset "yielding" begin
                @test finite_response(mat, M.ZerilliArmstrongMP, 1.0001, 1.0)[2] == 0
                @test finite_response(mat, M.ZerilliArmstrongMP, 1.01, 1.0)[2] > 0
            end

            @testset "a faster load gives a higher stress" begin
                slow, medium, fast = (finite_response(mat, M.ZerilliArmstrongMP, 1.05, t)[1] for t in (1e3, 1.0, 1e-3))
                @test fast > medium > slow
            end

            @testset "a higher temperature gives a lower stress" begin
                stress(T) = finite_response(M.ZerilliArmstrongMaterial(za(T = T)), M.ZerilliArmstrongMP, 1.05, 1.0)[1]
                @test stress(200.0) > stress(293.0) > stress(500.0)
            end

            @testset "MUESLI's own consistency test" begin
                @test M.testImplementation(M.ZerilliArmstrongMP(mat), false, true)
            end
        end

        @testset "viscoplasticity with nonlinear hardening" begin
            function vp(; eta = 1.0)
                p = properties("young" => 210e3, "poisson" => 0.3, "density" => 7.8e-9,
                               "isotropich" => 1000.0, "isohardinf" => 100.0, "isohardzero" => 0.0,
                               "isoharddelta" => 1.0, "kinematich" => 0.0, "yieldstress" => 300.0,
                               "eta" => eta, "viscousexponent" => 1.0, "alpha" => 0.5)
                M.setString!(p, "plasticity", "mises")
                return p
            end
            mat = M.ViscoplasticNLMaterial(vp())
            @test M.check(mat)
            @testset "the density is the one that was set" begin
                @test M.density(mat) ≈ 7.8e-9
            end
            @test M.cxxCreateMaterialPoint(mat) isa M.ViscoplasticNLMP

            function small_response(ε11, t)
                mp = M.ViscoplasticNLMP(mat)
                M.updateCurrentState(mp, t, M.Istensor([ε11 0.0 0.0; 0.0 0.0 0.0; 0.0 0.0 0.0]))
                σ = M.Istensor()
                M.stress!(mp, σ)
                return mat3(σ)[1, 1], M.plasticSlip(mp)
            end

            @test small_response(1e-4, 1.0)[2] == 0
            @test small_response(2e-2, 1.0)[2] > 0
            @testset "a faster load gives a higher stress" begin
                slow, medium, fast = (small_response(2e-2, t)[1] for t in (1e3, 1.0, 1e-3))
                @test fast > medium ≥ slow
            end
            @test M.testImplementation(M.ViscoplasticNLMP(mat), true, true)
        end
    end

    @testset "Reduced models: bar, beam, shell and plane stress" begin
        G = E / (2 * (1 + ν))
        voigt = ((1, 1), (2, 2), (3, 3), (2, 3), (1, 3), (1, 2))

        # reduced point, 3D point it wraps, and the full strain/stress after the update
        function reduced_response(R, ε; method = nothing)
            mat = M.ElasticIsotropicMaterial(E, ν)
            mp3 = M.ElasticIsotropicMP(mat)
            rmp = method === nothing ? R(mp3) : R(mp3, method)
            ε3 = M.Istensor(ε)
            M.updateCurrentState!(rmp, 1.0, ε3)
            σ = M.Istensor()
            M.stress!(rmp, σ)
            C = zeros(6, 6)
            M.tangentMatrix!(rmp, C)
            GC.@preserve mp3 mat (mat3(ε3), mat3(σ), C)
        end

        ε = [1e-3 4e-4 2e-4; 4e-4 -5e-4 3e-4; 2e-4 3e-4 7e-4]

        @testset "bar: only the axial stress survives" begin
            ε1 = zeros(3, 3); ε1[1, 1] = 1e-3
            εf, σ, C = reduced_response(M.SbarMP, ε1)
            @test εf[2, 2] ≈ -ν * 1e-3 && εf[3, 3] ≈ -ν * 1e-3
            @test σ[1, 1] ≈ E * 1e-3
            @test count(>(1e-9), abs.(σ)) == 1
            @test C[1, 1] ≈ E
            @test count(>(1e-9), abs.(C)) == 1
        end

        @testset "plane stress and shell: S33 = 0" begin
            for R in (M.SplaneMP, M.SshellMP)
                εf, σ, C = reduced_response(R, ε)
                @test εf[3, 3] ≈ -ν / (1 - ν) * (ε[1, 1] + ε[2, 2])
                @test σ[3, 3] ≈ 0 atol = 1e-6
                @test C[1, 1] ≈ E / (1 - ν^2)
                @test C[1, 2] ≈ ν * E / (1 - ν^2)
            end
            # the plane model also drops the out-of-plane shears, the shell keeps them
            _, σp, Cp = reduced_response(M.SplaneMP, ε)
            _, σs, Cs = reduced_response(M.SshellMP, ε)
            @test σp[2, 3] ≈ 0 atol = 1e-6
            @test σp[1, 3] ≈ 0 atol = 1e-6
            @test σs[2, 3] ≈ 2G * ε[2, 3]
            @test Cp[4, 4] == 0 && Cs[4, 4] ≈ G
        end

        @testset "beam: S11 = S22 = S12 = 0" begin
            εf, σ, C = reduced_response(M.SbeamMP, ε)
            @test all(abs(σ[i, j]) < 1e-6 for (i, j) in ((1, 1), (2, 2), (1, 2)))
            @test σ[3, 3] ≈ E * ε[3, 3]
            @test C[3, 3] ≈ E && C[4, 4] ≈ G && C[5, 5] ≈ G
            @test C[1, 1] == 0
        end

        @testset "the prescribed strain components are not changed" begin
            εf, = reduced_response(M.SplaneMP, ε)
            for (i, j) in ((1, 1), (2, 2), (1, 2))
                @test εf[i, j] ≈ ε[i, j]
            end
        end

        @testset "nonlinear conjugate gradient gives the same answer" begin
            @test reduced_response(M.SbeamMP, ε; method = "nlcg")[2] ≈ reduced_response(M.SbeamMP, ε)[2] atol = 1e-6
        end

        @testset "inherited from the family root" begin
            mat = M.ElasticIsotropicMaterial(E, ν)
            mp3 = M.ElasticIsotropicMP(mat)
            rmp = M.SplaneMP(mp3)
            @test rmp isa M.ReducedSmallStrainMP
            M.updateCurrentState!(rmp, 1.0, M.Istensor(ε))
            @test M.storedEnergy(rmp) > 0
            @test M.density(rmp) == M.density(mat)
            M.commitCurrentState(rmp)
            @test M.testImplementation(rmp)
            GC.@preserve mp3 mat nothing
        end

        @testset "finite strain bar with a Saint Venant-Kirchhoff material" begin
            mat = M.SVKMaterial(properties("young" => E, "poisson" => ν))
            mp3 = M.SVKMP(mat)
            rmp = M.FbarMP(mp3)
            @test rmp isa M.ReducedFiniteStrainMP
            F = M.Itensor([1.1 0.0 0.0; 0.0 1.0 0.0; 0.0 0.0 1.0])
            M.updateCurrentState!(rmp, 1.0, F)
            S = M.Istensor()
            M.secondPiolaKirchhoffStress!(rmp, S)
            S = mat3(S)
            E11 = (1.1^2 - 1) / 2
            @test S[1, 1] ≈ E * E11
            @test all(abs(S[i, j]) < 1e-6 for (i, j) in voigt[2:end])
            # the solved transverse stretch is the one of uniaxial stress
            Fa = mat3(F)
            @test Fa[2, 2] ≈ Fa[3, 3] && Fa[2, 2] < 1
            C = zeros(6, 6)
            M.convectedTangentMatrix!(rmp, C)
            @test C[1, 1] ≈ E
            GC.@preserve mp3 mat nothing
        end

        @testset "finite strain shell, plane and beam" begin
            mat = M.NeoHookeMaterial(properties("young" => E, "poisson" => ν))
            F0 = [1.05 0.02 0.0; 0.01 0.97 0.0; 0.0 0.0 1.0]
            Fr = [1.05 0.02 0.01; 0.01 0.97 0.02; 0.0 0.01 0.98]
            for (R, zeros_) in ((M.FshellMP, ((3, 3),)),
                                (M.FplaneMP, ((3, 3), (2, 3), (1, 3))),
                                (M.FbeamMP, ((1, 1), (2, 2), (1, 2))))
                mp3 = M.NeoHookeMP(mat)
                rmp = R(mp3)
                F = M.Itensor(Fr)
                M.updateCurrentState!(rmp, 1.0, F)
                S = M.Istensor()
                M.secondPiolaKirchhoffStress!(rmp, S)
                S = mat3(S)
                @test all(abs(S[i, j]) < 1e-6 * E for (i, j) in zeros_)
                @test M.testImplementation(rmp, true, true)
                GC.@preserve mp3 nothing
            end
        end
    end

    @testset "Viscoplasticity takes Young's modulus and Poisson's ratio" begin
        E, ν = 210e3, 0.3
        mat = M.ViscoplasticMaterial(E, ν, 7.8e-9, 1000.0, 0.0, 300.0, "mises", 1.0, 0.0)
        @test M.check(mat)
        @test M.density(mat) ≈ 7.8e-9

        mp = M.ViscoplasticMP(mat)
        ε = 1e-4
        M.updateCurrentState(mp, 1.0, M.Istensor([ε 0.0 0.0; 0.0 -ν * ε 0.0; 0.0 0.0 -ν * ε]))
        σ = M.Istensor()
        M.stress!(mp, σ)
        @test mat3(σ)[1, 1] ≈ E * ε rtol = 1e-8
        @test M.plasticSlip(mp) == 0

        @test M.check(M.ViscoplasticMaterial(E, ν, 7.8e-9, 1000.0, 0.0, 300.0, "tresca", 1.0, 0.0))
        !Sys.isapple() && @test_throws Exception M.ViscoplasticMaterial(E, ν, 1.0, 0.0, 0.0, 300.0, "drucker", 1.0, 0.0)
    end
end
