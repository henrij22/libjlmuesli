# libjlmuesli

[![Check build](https://github.com/henrij22/libjlmuesli/actions/workflows/check_build.yml/badge.svg)](https://github.com/henrij22/libjlmuesli/actions/workflows/check_build.yml)

The C++ component of [MuesliMaterials.jl](https://github.com/henrij22/MuesliMaterials.jl) — a
[CxxWrap](https://github.com/JuliaInterop/CxxWrap.jl)-based binding layer over
[MUESLI](https://gitlab.com/ignacio.romero/muesli) (Material UnivErSal LIbrary), the
constitutive-model library developed at IMDEA Materials Institute.

Building this repository produces a single shared library, `libjlmuesli`, which Julia loads
with `@wrapmodule`. Cross-compiled binaries are released through
[MuesliMaterialsWrapper_jll.jl](https://github.com/henrij22/MuesliMaterialsWrapper_jll.jl); you
only need to build here when working on the bindings themselves.

The developer of this wrapper is not affiliated with IMDEA in any form.

## What it does

The library registers a curated subset of MUESLI with the Julia runtime: the tensor and vector
classes, the material property map, and the small-strain and finite-strain material families
together with their material points.

It is a translation layer, not a thin passthrough. The conventions it establishes are what make
the Julia side feel native:

- **1-based indexing** everywhere it is observable — tensor and vector components, and the
  `materialState` accessors, are all shifted on the way in and out.
- **Column-major input.** Julia matrices arrive column-major and MUESLI's constructors are
  row-major, so `util/convert.hh` reorders the components rather than transposing by accident.
  `Istensor` keeps the symmetric part of a non-symmetric input.
- **Bang methods** (`stress!`, `tangentTensor!`, `CauchyStress!`, …) for every MUESLI routine
  that writes into a preallocated tensor, matching the C++ out-parameter style.
- **PascalCase type names** — `itensor` becomes `Itensor`, `elasticIsotropicMaterial` becomes
  `ElasticIsotropicMaterial` — while function names are kept as they are.
- **State-changing calls keep no bang** (`setConvergedState`, `commitCurrentState`) because
  they advance the material point's own history rather than filling an argument.

### Layout

| Path | Contents |
| :-- | :-- |
| `src/jlmuesli/muesli.cpp` | module entry point, calls the register functions |
| `src/jlmuesli/util/convert.hh` | column-major ↔ row-major conversion on raw pointers, no Julia dependency |
| `src/jlmuesli/util/common.hh` | the same for `jlcxx::ArrayRef`, with shape checks |
| `src/jlmuesli/util/materialbase.cpp` | methods every material and material point shares |
| `src/jlmuesli/util/supertypes.hh` | `jlcxx::SuperType` specializations for the class hierarchy |
| `src/jlmuesli/util/` | tensors, material properties, material state, property names |
| `src/jlmuesli/smallstrain/` | small-strain materials: elastic, plastic, viscous, damage |
| `src/jlmuesli/finitestrain/` | finite-strain materials: hyperelastic and finite plasticity |
| `src/jlmuesli/reduced/` | reduced models: bar, beam, shell and plane stress on top of a 3D point |
| `test/` | C++ unit tests (doctest + CTest) |
| `patches/` | `cmakesupport.patch`, which adds a CMake project to MUESLI |
| `julia/` | Julia integration suite run against the freshly built library |

## Building

### Prerequisites

- A C++17 compiler and CMake ≥ 3.18
- **Eigen** (header-only), which MUESLI's tensor headers include — `libeigen3-dev` on Debian
  and Ubuntu
- **BLAS and LAPACK**, which MUESLI links against — `libopenblas-dev liblapack-dev`
- **MUESLI**, built and installed — see below
- **libcxxwrap-julia**, which ships with the `CxxWrap.jl` package. Ask Julia for its path from
  an environment that has CxxWrap installed — `julia/` in this repository is one:

  ```bash
  julia --project=julia -e 'using Pkg; Pkg.instantiate()'
  julia --project=julia -e 'using CxxWrap; println(CxxWrap.prefix_path())'
  ```

### Building MUESLI

MUESLI ships no build system of its own, so `patches/cmakesupport.patch` adds a CMake project
to it. That patch is the same one
[Yggdrasil](https://github.com/JuliaPackaging/Yggdrasil/blob/master/M/MuesliMaterials/build_tarballs.jl)
applies when building `MuesliMaterials_jll`.

**Yggdrasil is the source of truth for the pinned commit.** Match it locally, or you will be
developing against a different MUESLI than the released binaries. The same commit is pinned
here and in `.github/workflows/check_build.yml` as `MUESLI_COMMIT`; keep the three in step.
The current pin is `52df6390181aca4b2add1bbc9531c636b8b4160f` (MUESLI 2.0).

```bash
PREFIX=$HOME/dev/install

git clone https://gitlab.com/ignacio.romero/muesli.git
cd muesli
git checkout 52df6390181aca4b2add1bbc9531c636b8b4160f
git apply /path/to/libjlmuesli/patches/cmakesupport.patch

cmake -B builddir \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=$PREFIX \
  -DCMAKE_CXX_FLAGS="-I/usr/include/eigen3"
cmake --build builddir --parallel
cmake --install builddir
```

If Eigen is not installed system-wide, fetch the headers and point at those instead:

```bash
git clone --depth 1 https://gitlab.com/libeigen/eigen.git
cmake -S eigen -B eigen/build -DCMAKE_INSTALL_PREFIX=$PREFIX
cmake --build eigen/build --target install
# ... then -DCMAKE_CXX_FLAGS="-I$PREFIX/include/eigen3"
```

### Configure and build

```bash
PREFIX=$HOME/dev/install
CXXWRAP_PREFIX=$(julia --project=julia -e 'using CxxWrap; print(CxxWrap.prefix_path())')
JULIA_LIB=$(julia --startup-file=no -e 'using Libdl; print(abspath(Libdl.dlpath("libjulia")))')

cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$PREFIX;$CXXWRAP_PREFIX" \
  -DJulia_LIBRARY="$JULIA_LIB" \
  -DCMAKE_CXX_FLAGS="-I/usr/include/eigen3"
cmake --build build --parallel
```

The result is `build/lib/libjlmuesli.so` (`.dylib` on macOS, `.dll` on Windows).

`-DJulia_LIBRARY` is only needed for the **test executable**. The shared library links fine
without it, because a shared library may leave symbols undefined; an executable may not, and
`jlmuesli_tests` reaches `libjulia` transitively through `libcxxwrap_julia`. `JlCxxConfig` runs
`find_package(Julia)` without `REQUIRED`, so when the `julia` executable is not on CMake's
`PATH` the library path silently goes missing and only the tests fail to link. Passing it
explicitly sidesteps that; omit it and the tests are skipped with a warning.

### Options

| Option | Default | Meaning |
| :-- | :-- | :-- |
| `JLMUESLI_BUILD_TESTS` | `ON` | Build the C++ unit tests. Fetches doctest if it is not installed, so turn this off for a network-free build. |

## Testing

There are two suites, covering different things.

### C++ unit tests

```bash
ctest --test-dir build --output-on-failure
```

These cover the helpers that do not touch the Julia runtime — the material property map and
its string-option encoding, and the layout conversion in `util/convert.hh`. Everything else in this repository is CxxWrap registration code,
which cannot be exercised without a live Julia runtime, so it is covered by the suite below.
Add new cases to `test/test_helpers.cpp` or `test/test_convert.cpp`, or a new file listed in
`test/CMakeLists.txt`.

### Julia integration tests

This is the real coverage. It loads the library you just built and drives the bindings exactly
as MuesliMaterials.jl does:

```bash
julia --project=julia -e 'using Pkg; Pkg.instantiate()'
LD_LIBRARY_PATH=$HOME/dev/install/lib \
  julia --project=julia julia/runtests.jl build/lib
```

The path argument is the directory holding the shared library; alternatively set
`JLMUESLI_LIB`. `LD_LIBRARY_PATH` is only needed if MUESLI is installed somewhere the loader
does not search (`DYLD_LIBRARY_PATH` on macOS). This suite loads `libjlmuesli` directly, so
nothing else is competing to provide `libmuesli` — unlike the MuesliMaterials.jl case below.

Rather than assert against recorded numbers, the suite checks properties that must hold
whatever the implementation does: linear isotropic elasticity against the closed-form
`σ = 2με + λ tr(ε) I` and its analytic tangent, stored energy against ½σ:ε, the undeformed
state of a hyperelastic material being stress free, `P = F S`, `τ = J σ`, and MUESLI's own
analytic stresses against its numerical ones.

## Pointing MuesliMaterials.jl at a local build

`MuesliMaterials.jl` normally resolves the library through the released
`MuesliMaterialsWrapper_jll`. To run it against a library you built here, override the JLL with
a `LocalPreferences.toml` next to the `Project.toml` of the environment you are using:

```toml
[MuesliMaterialsWrapper_jll]
libjlmuesli_path = "/absolute/path/to/libjlmuesli/build/lib/libjlmuesli.so"
```

`JLLWrappers` then loads your library instead of the artifact.

> [!IMPORTANT]
> **MUESLI itself must match too.** `MuesliMaterials_jll` ships its own `libmuesli.so` and
> `dlopen`s it before your library is loaded. Once a library with that SONAME is in the
> process, the loader reuses it and ignores your `RUNPATH` — so a `libjlmuesli` compiled
> against your MUESLI headers ends up calling the artifact's MUESLI. The vtable layouts differ
> between MUESLI builds, so virtual calls land on the wrong method and the process segfaults
> somewhere unrelated (a `stress!` call arriving in `thermodynamicPotentials`, for instance).
>
> Force your build to win:
>
> ```bash
> LD_PRELOAD=$HOME/dev/install/lib/libmuesli.so julia --project=. -e 'using Pkg; Pkg.test()'
> ```
>
> On macOS use `DYLD_INSERT_LIBRARIES`. If you would rather avoid `LD_PRELOAD`, build against
> the artifact's MUESLI instead — `MuesliMaterials_jll.artifact_dir` ships `include/muesli` and
> `lib/cmake/Muesli`, though its exported CMake target hard-codes a BinaryBuilder path for
> OpenBLAS that has to be worked around.

Other notes:

- The path must be the **shared library file itself**, absolute, including the extension.
- The preference is read at precompilation time; Julia notices a change and recompiles.
- Each environment needs its own file, `docs/` included.
- Add `LocalPreferences.toml` to the consuming repository's `.gitignore` — it holds an absolute
  path from your machine.
- Delete it once the JLL is released with your changes, so the environment goes back to the
  published binary.

## Reduced models

A reduced point wraps a 3D material point for an element that only controls part of the strain: the
caller prescribes the *master* strain components, the model solves for the remaining ones so that
their stress is zero, and the tangent is condensed statically onto the master block,
`C* = C_mm - C_mz C_zz⁻¹ C_zm`. Formulation and references are in MUESLI's `reducedsmallstrain.cpp`
and `reducedfinitestrain.cpp`.

| Small strain | Finite strain | Master components | Zero stress | Use |
| :-- | :-- | :-- | :-- | :-- |
| `SbarMP` | `FbarMP` | 11 | 22, 33, 23, 13, 12 | uniaxial stress |
| `SbeamMP` | `FbeamMP` | 33, 23, 13 | 11, 22, 12 | beam along axis 3 |
| `SshellMP` | `FshellMP` | 11, 22, 23, 13, 12 | 33 | shell, plane stress with transverse shear |
| `SplaneMP` | `FplaneMP` | 11, 22, 12 | 33, 23, 13 | plane stress |

- The finite-strain models work in the Green-Lagrange strain `E = (FᵀF - I)/2` and its work-conjugate
  second Piola-Kirchhoff stress, the small-strain ones in the strain and the Cauchy stress.
- `Reduced…MP(point)` takes the 3D point; `Reduced…MP(point, "nlcg")` selects the nonlinear conjugate
  gradient solver instead of Newton-Raphson, which is more robust for plasticity.
- `updateCurrentState!(mp, t, ε)` (small) and `updateCurrentState!(mp, t, F)` (finite) **overwrite the
  argument** with the full 3D strain or deformation gradient, including the solved components.
- `tangentMatrix!` (small) and `convectedTangentMatrix!` (finite) fill a 6×6 matrix in Voigt order
  `[11, 22, 33, 23, 13, 12]` with the *engineering* tangent, `dS = C dE` with `γ = 2ε` for the shears.
  Only the master block is non-zero.
- The reduced point keeps a pointer to the 3D point, which must outlive it (MuesliMaterials.jl
  ensures this). The shared interface is on `ReducedSmallStrainMP` and `ReducedFiniteStrainMP`.
- The `SuperType` entries point straight at the root: the intermediate `reduced1z/3z/5z` classes are
  not bound, and `static_cast` reaches the root through them.
- MUESLI prints `Error in back linesearch` from its nonlinear conjugate gradient solver when the
  problem is already quadratic, as it is for linear elasticity. The result is correct.

## Working on the bindings

A new material is one `addSmallStrainMaterial` / `addFiniteStrainMaterial` call plus its
constructors in `registersmallstrain.hh` / `registerfinitestrain.hh`. The interface shared by a
family is registered once on its root in `smallstrainbindings.cpp` / `finitestrainbindings.cpp`,
and what every material has in common in `util/materialbase.cpp`; a material only repeats a
method when its signature differs (a history-carrying `setConvergedState`, for example).

A material point only holds a C++ reference to its material, so it must not outlive it. The raw
`...MP(material)` constructors and `cxxCreateMaterialPoint` do nothing to prevent that;
MuesliMaterials.jl wraps them and keeps the material alive. The bindings are not meant to be used
without that package.

To move a tensor to or from Julia, prefer the bulk methods `toArray!` and `fromArray!` over
`cxxgetindex`: one call per tensor instead of one per element.

A material whose `density()` returns a member no MUESLI constructor initialises (the Zerilli-Armstrong
and the nonlinear viscoplastic material do) is instantiated through `WithBaseDensity`, a subclass
that returns the density the `"density"` property sets. Specialise `ConcreteMaterial` for it.

The materials built from a `MaterialProperties` map are not given MUESLI's positional constructors
where those are broken: the Johnson-Cook one ignores its melting temperature, and its property map
constructor overwrites `edot0` with `ageparam`, so that key is never set.

The positional `Viscoplastic` constructor takes the Lamé constants and zeroes the viscous exponent and the reference
rate, so the binding keeps its `(E, nu, …)` signature but builds the material from a property map.

MUESLI's own consistency check of a material point is available as `testImplementation(mp, testDE,
testDDE)`. MUESLI skips the energy derivative (`testDE`) for the Johnson-Cook and Zerilli-Armstrong
models, since they are dissipative, and the tests do the same. Call `setFixedRandomSeed(true)`
first so that the random state it draws is reproducible.

Not bound, because MUESLI 2.0's own `test()` fails for them or the class is a stub:

| Class | Why |
| :-- | :-- |
| `FplasticRateDependent` | MUESLI's own test fails (zero stress, `inf` and `nan` errors) |
| `FLemaitre` | a copy of `fplastic` without damage parameters; its self test fails |

Points worth remembering when adding a binding:

- Shift every index a user can observe: `i - 1` on the way in.
- A lambda's parameter list and its `arg(...)` specifiers must agree in arity. A mismatch
  compiles silently and registers a Julia method that cannot be called correctly.
- Register a method on the wrapper of the type it belongs to. `mat.method(...)` with a lambda
  taking a material *point* happens to work, because jlcxx keys off the lambda signature, but
  it is misleading.
- **Every wrapped class with a base needs a `jlcxx::SuperType` specialization** in
  `util/supertypes.hh`, naming its *direct* C++ base. `julia_base_type<T>()` static_asserts on
  it, and `cxxupcast` uses it to reach inherited methods — a missing one fails to compile, a
  wrong one corrupts memory at runtime.
- Generic lambdas (`[](auto& x, ...)`) cannot be wrapped; spell out the concrete type.

After changing a binding, run both suites and add a case to `julia/runtests.jl`.

## License

GPL-3.0-or-later. See [LICENSE](LICENSE).
