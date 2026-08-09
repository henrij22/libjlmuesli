// SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// jlcxx::SuperType specializations for every wrapped muesli class that has a base class.
//
// Two things depend on these:
//
//   * `jlcxx::julia_base_type<T>()` static_asserts that `SuperType<T>` is specialized, so
//     without them the bindings do not compile against current libcxxwrap-julia at all.
//   * `cxxupcast` uses them to reach a base class method through a derived pointer. A
//     missing or wrong specialization means every inherited method fails at runtime with
//     "No upcast for type ...".
//
// Each entry must name the *direct* C++ base, so that the chain of static_casts jlcxx
// builds matches the real class hierarchy. They are collected here rather than scattered
// across the registration files so the hierarchy can be checked against muesli in one place.

#include <muesli/muesli.h>

#include <muesli/Finitestrain/arrudaboyce.h>
#include <muesli/Finitestrain/fisotropic.h>
#include <muesli/Finitestrain/fplastic.h>
#include <muesli/Finitestrain/mooney.h>
#include <muesli/Smallstrain/sdamage.h>

#include <jlcxx/jlcxx.hpp>

#define JLMUESLI_SUPERTYPE(Derived, Base)     \
  template <>                                 \
  struct SuperType<Derived>                   \
  {                                           \
    using type = Base;                        \
  }

namespace jlcxx {

// --- small strain ------------------------------------------------------------------
JLMUESLI_SUPERTYPE(muesli::smallStrainMaterial, muesli::material);
JLMUESLI_SUPERTYPE(muesli::smallStrainMP, muesli::materialPoint);

JLMUESLI_SUPERTYPE(muesli::elasticIsotropicMaterial, muesli::smallStrainMaterial);
JLMUESLI_SUPERTYPE(muesli::elasticIsotropicMP, muesli::smallStrainMP);

JLMUESLI_SUPERTYPE(muesli::elasticAnisotropicMaterial, muesli::smallStrainMaterial);
JLMUESLI_SUPERTYPE(muesli::elasticAnisotropicMP, muesli::smallStrainMP);

// orthotropic and transversely isotropic refine the anisotropic material, not the base
JLMUESLI_SUPERTYPE(muesli::elasticOrthotropicMaterial, muesli::elasticAnisotropicMaterial);
JLMUESLI_SUPERTYPE(muesli::elasticOrthotropicMP, muesli::elasticAnisotropicMP);

JLMUESLI_SUPERTYPE(muesli::elasticTransverselyisotropicMaterial, muesli::elasticAnisotropicMaterial);
JLMUESLI_SUPERTYPE(muesli::elasticTransverselyisotropicMP, muesli::elasticAnisotropicMP);

JLMUESLI_SUPERTYPE(muesli::splasticMaterial, muesli::smallStrainMaterial);
JLMUESLI_SUPERTYPE(muesli::splasticMP, muesli::smallStrainMP);

JLMUESLI_SUPERTYPE(muesli::viscoelasticMaterial, muesli::smallStrainMaterial);
JLMUESLI_SUPERTYPE(muesli::viscoelasticMP, muesli::smallStrainMP);

JLMUESLI_SUPERTYPE(muesli::viscoplasticMaterial, muesli::smallStrainMaterial);
JLMUESLI_SUPERTYPE(muesli::viscoplasticMP, muesli::smallStrainMP);

// --- small strain damage -----------------------------------------------------------
JLMUESLI_SUPERTYPE(muesli::sdamageMaterial, muesli::smallStrainMaterial);
JLMUESLI_SUPERTYPE(muesli::sdamageMP, muesli::smallStrainMP);

JLMUESLI_SUPERTYPE(muesli::GTN_Material, muesli::sdamageMaterial);
JLMUESLI_SUPERTYPE(muesli::GTN_MP, muesli::sdamageMP);

JLMUESLI_SUPERTYPE(muesli::Gurson_Material, muesli::sdamageMaterial);
JLMUESLI_SUPERTYPE(muesli::Gurson_MP, muesli::sdamageMP);

JLMUESLI_SUPERTYPE(muesli::Lemaitre_Material, muesli::sdamageMaterial);
JLMUESLI_SUPERTYPE(muesli::Lemaitre_MP, muesli::sdamageMP);

JLMUESLI_SUPERTYPE(muesli::LemKin_Material, muesli::sdamageMaterial);
JLMUESLI_SUPERTYPE(muesli::LemKin_MP, muesli::sdamageMP);

// --- finite strain -----------------------------------------------------------------
JLMUESLI_SUPERTYPE(muesli::finiteStrainMaterial, muesli::material);
JLMUESLI_SUPERTYPE(muesli::finiteStrainMP, muesli::materialPoint);

// f_invariants is the invariant-based hyperelastic family; fisotropicMP is its point
JLMUESLI_SUPERTYPE(muesli::f_invariants, muesli::finiteStrainMaterial);
JLMUESLI_SUPERTYPE(muesli::fisotropicMP, muesli::finiteStrainMP);

JLMUESLI_SUPERTYPE(muesli::neohookeanMaterial, muesli::f_invariants);
JLMUESLI_SUPERTYPE(muesli::neohookeanMP, muesli::fisotropicMP);

JLMUESLI_SUPERTYPE(muesli::yeohMaterial, muesli::f_invariants);
JLMUESLI_SUPERTYPE(muesli::yeohMP, muesli::fisotropicMP);

JLMUESLI_SUPERTYPE(muesli::mooneyMaterial, muesli::f_invariants);
JLMUESLI_SUPERTYPE(muesli::mooneyMP, muesli::fisotropicMP);

JLMUESLI_SUPERTYPE(muesli::arrudaboyceMaterial, muesli::f_invariants);
JLMUESLI_SUPERTYPE(muesli::arrudaboyceMP, muesli::fisotropicMP);

// svk and fplastic derive straight from the finite strain base, not from f_invariants
JLMUESLI_SUPERTYPE(muesli::svkMaterial, muesli::finiteStrainMaterial);
JLMUESLI_SUPERTYPE(muesli::svkMP, muesli::finiteStrainMP);

JLMUESLI_SUPERTYPE(muesli::fplasticMaterial, muesli::finiteStrainMaterial);
JLMUESLI_SUPERTYPE(muesli::fplasticMP, muesli::finiteStrainMP);

} // namespace jlcxx

#undef JLMUESLI_SUPERTYPE
