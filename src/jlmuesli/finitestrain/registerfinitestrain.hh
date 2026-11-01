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

// finitestrainbindings.cpp: the family roots and everything they share
void registerFiniteStrainBase(jlcxx::Module& mod);

/// Registers `<name>Material` and `<name>MP`. `MaterialBase`/`MaterialPointBase` must be the
/// direct C++ base: the invariant-based family sits between the root and its materials.
template <typename Material, typename MaterialPoint, typename MaterialBase = muesli::finiteStrainMaterial,
          typename MaterialPointBase = muesli::finiteStrainMP>
std::pair<jlcxx::TypeWrapper<Material>, jlcxx::TypeWrapper<MaterialPoint>> addFiniteStrainMaterial(
    jlcxx::Module& mod, const std::string& name) {
  using jlcxx::arg;

  auto mat = mod.add_type<Material>(name + "Material", jlcxx::julia_base_type<MaterialBase>());
  mat.constructor([muesliName = muesliMaterialName(name)](
                      const MaterialProperties& properties) { return new Material{muesliName, properties.multiMap()}; },
                  arg("properties"));

  auto mp = mod.add_type<MaterialPoint>(name + "MP", jlcxx::julia_base_type<MaterialPointBase>())
                .constructor([](const Material& m) { return new MaterialPoint(m); });

  // Must follow the MP registration, since a method cannot return a type that does not exist yet.
  mat.method("cxxCreateMaterialPoint", [](const Material& m) { return jlcxx::create<MaterialPoint>(m); });

  return std::make_pair(mat, mp);
}

/// For materials whose whole history is F; the others register their own overload.
template <typename MaterialPoint>
void addDeformationOnlyConvergedState(jlcxx::TypeWrapper<MaterialPoint>& mp) {
  mp.method("setConvergedState",
            [](MaterialPoint& mp, double theTime, const itensor& F) { mp.setConvergedState(theTime, F); });
}

inline void registerFiniteStrainMaterials(jlcxx::Module& mod) {
  using jlcxx::arg;
  using jlcxx::julia_base_type;

  registerFiniteStrainBase(mod);
  mod.add_type<muesli::fisotropicMP>("FisotropicMP", julia_base_type<muesli::finiteStrainMP>());

  {
    using Material      = muesli::neohookeanMaterial;
    using MaterialPoint = muesli::neohookeanMP;
    auto [mat, mp] =
        addFiniteStrainMaterial<Material, MaterialPoint, muesli::f_invariants, muesli::fisotropicMP>(mod, "NeoHooke");
    addDeformationOnlyConvergedState(mp);
    mat.constructor([](double Emod, double nu, double rho = 1.0) { return new Material{"NeoHooke", Emod, nu, rho}; },
                    arg("Emod"), arg("nu"), arg("rho") = 1.0);
  }

  {
    using Material      = muesli::svkMaterial;
    using MaterialPoint = muesli::svkMP;
    auto [mat, mp]      = addFiniteStrainMaterial<Material, MaterialPoint>(mod, "SVK");
    addDeformationOnlyConvergedState(mp);
    mat.constructor([](double Emod, double nu) { return new Material{"SVK", toMPM_Enu(Emod, nu)}; }, arg("Emod"),
                    arg("nu"));
  }

  {
    using Material      = muesli::mooneyMaterial;
    using MaterialPoint = muesli::mooneyMP;
    auto [mat, mp] =
        addFiniteStrainMaterial<Material, MaterialPoint, muesli::f_invariants, muesli::fisotropicMP>(mod, "Mooney");
    addDeformationOnlyConvergedState(mp);
    mat.constructor(
        [](double alpha0, double alpha1, double alpha2, bool incompressible = true) {
          MaterialProperties properties{};
          properties.set("alpha0", alpha0);
          properties.set("alpha1", alpha1);
          properties.set("alpha2", alpha2);
          if (incompressible)
            properties.set("incompressible", 0);
          return new Material{"Mooney", properties.multiMap()};
        },
        arg("alpha0"), arg("alpha1"), arg("alpha2"), arg("incompressible") = true);
  }
  {
    using Material      = muesli::arrudaboyceMaterial;
    using MaterialPoint = muesli::arrudaboyceMP;
    auto [mat, mp]      = addFiniteStrainMaterial<Material, MaterialPoint, muesli::f_invariants, muesli::fisotropicMP>(
        mod, "ArrudaBoyce");
    addDeformationOnlyConvergedState(mp);
    mat.constructor([](double C1, double lambdam, double bulk,
                       bool compressible) { return new Material{"ArrudaBoyce", C1, lambdam, bulk, compressible}; },
                    arg("C1"), arg("lambdam"), arg("bulk"), arg("compressible"));
  }
  {
    using Material      = muesli::yeohMaterial;
    using MaterialPoint = muesli::yeohMP;
    auto [mat, mp] =
        addFiniteStrainMaterial<Material, MaterialPoint, muesli::f_invariants, muesli::fisotropicMP>(mod, "Yeoh");
    addDeformationOnlyConvergedState(mp);
    mat.constructor([](double C1, double C2, double C3, double bulk,
                       bool compressible) { return new Material{"Yeoh", C1, C2, C3, bulk, compressible}; },
                    arg("C1"), arg("C2"), arg("C3"), arg("bulk"), arg("compressible"));
  }
  {
    using Material      = muesli::fplasticMaterial;
    using MaterialPoint = muesli::fplasticMP;
    auto [mat, mp]      = addFiniteStrainMaterial<Material, MaterialPoint>(mod, "Fplastic");
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