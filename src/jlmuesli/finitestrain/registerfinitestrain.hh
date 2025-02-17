// SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// #include "finitestrainbindings.hh"

#include <jlmuesli/util/common.hh>
#include <jlmuesli/util/utils.hh>

#include <muesli/Finitestrain/fplastic.h>
#include <muesli/Finitestrain/reducedfinitestrain.h>
#include <muesli/Math/mtensor.h>
#include <muesli/muesli.h>

#include <jlcxx/jlcxx.hpp>

// Forward declare
template <typename Material, typename MaterialPoint, typename MaterialBase = muesli::finiteStrainMaterial,
          typename MaterialPointBase = muesli::finiteStrainMP, bool registerConvergedState = true>
std::pair<jlcxx::TypeWrapper<Material>, jlcxx::TypeWrapper<MaterialPoint>> registerFiniteStrainMaterial(
    jlcxx::Module& mod, const std::string& name);

inline void registerFiniteStrainMaterials(jlcxx::Module& mod) {
  using jlcxx::arg;
  using jlcxx::julia_base_type;

  mod.add_type<muesli::finiteStrainMaterial>("FiniteStrainMaterial", julia_base_type<muesli::material>());
  mod.add_type<muesli::f_invariants>("F_invariants", julia_base_type<muesli::finiteStrainMaterial>());

  mod.add_type<muesli::finiteStrainMP>("FiniteStrainMP", julia_base_type<muesli::materialPoint>());
  mod.add_type<muesli::fisotropicMP>("FisotropicMP", julia_base_type<muesli::finiteStrainMP>());

  {
    using Material      = muesli::neohookeanMaterial;
    using MaterialPoint = muesli::neohookeanMP;
    auto [mat, mp] = registerFiniteStrainMaterial<Material, MaterialPoint, muesli::f_invariants, muesli::fisotropicMP>(
        mod, "NeoHooke");
    mat.constructor([](double Emod, double nu, double rho = 1.0) { return new Material{"NeoHooke", Emod, nu, rho}; },
                    arg("Emod"), arg("nu"), arg("rho") = 1.0);
  }

  {
    using Material      = muesli::svkMaterial;
    using MaterialPoint = muesli::svkMP;
    auto [mat, mp]      = registerFiniteStrainMaterial<Material, MaterialPoint>(mod, "SVK");
    mat.constructor([](double Emod, double nu) { return new Material{"SVK", toMPM_Enu(Emod, nu)}; }, arg("Emod"),
                    arg("nu"));
  }

  {
    using Material      = muesli::mooneyMaterial;
    using MaterialPoint = muesli::mooneyMP;
    auto [mat, mp] = registerFiniteStrainMaterial<Material, MaterialPoint, muesli::f_invariants, muesli::fisotropicMP>(
        mod, "Mooney");
    mat.constructor(
        [](double alpha0, double alpha1, double alpha2, bool incompressible = true) {
          MaterialProperties properties{};
          properties.set("alpha0", alpha0);
          properties.set("alpha1", alpha1);
          properties.set("alpha2", alpha2);
          return new Material{"Mooney", properties.multiMap()};
        },
        arg("alpha0"), arg("alpha1"), arg("alpha2"), arg("incompressible") = true);
  }
  {
    using Material      = muesli::arrudaboyceMaterial;
    using MaterialPoint = muesli::arrudaboyceMP;
    auto [mat, mp] = registerFiniteStrainMaterial<Material, MaterialPoint, muesli::f_invariants, muesli::fisotropicMP>(
        mod, "ArrudaBoyce");
    mat.constructor([](double C1, double lambdam, double bulk,
                       bool compressible) { return new Material{"ArrudaBoyce", C1, lambdam, bulk, compressible}; },
                    arg("C1"), arg("lambdam"), arg("bulk"), arg("compressible"));
  }
  {
    using Material      = muesli::yeohMaterial;
    using MaterialPoint = muesli::yeohMP;
    auto [mat, mp] =
        registerFiniteStrainMaterial<Material, MaterialPoint, muesli::f_invariants, muesli::fisotropicMP>(mod, "Yeoh");
    mat.constructor([](double C1, double C2, double C3, double bulk,
                       bool compressible) { return new Material{"Yeoh", C1, C2, C3, bulk, compressible}; },
                    arg("C1"), arg("C2"), arg("C3"), arg("bulk"), arg("compressible"));
  }
  {
    using Material      = muesli::fplasticMaterial;
    using MaterialPoint = muesli::fplasticMP;
    auto [mat, mp]      = registerFiniteStrainMaterial<Material, MaterialPoint, muesli::finiteStrainMaterial,
                                                       muesli::finiteStrainMP, false>(mod, "Fplastic");
    mat.constructor(
        [](double E, double nu, double Hiso, double Hkine, double Y0, double Yinf, double Yexp, double soft) {
          MaterialProperties properties;
          properties.set("young", E);
          properties.set("poisson", nu);
          properties.set("isotropich", Hiso);
          properties.set("kinematich", Hkine);
          properties.set("yieldstress", Y0);
          properties.set("yieldinf", Yinf);
          properties.set("hardexp", Yexp);
          properties.set("softening", soft);

          return new Material{"Fplastic", properties.multiMap()};
        },
        arg("E"), arg("nu"), arg("Hiso"), arg("Hkine"), arg("Y0"), arg("Yinf"), arg("Yexp"), arg("soft"));
    mp.method("setConvergedState",
              [](MaterialPoint& mp, double theTime, const itensor& F, double iso, const ivector& kine,
                 const istensor& be) { mp.setConvergedState(theTime, F, iso, kine, be); });
  }
}