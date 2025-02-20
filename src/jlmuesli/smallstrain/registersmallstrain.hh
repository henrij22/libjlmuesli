// SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <jlmuesli/util/common.hh>
#include <jlmuesli/util/utils.hh>

#include <muesli/muesli.h>
#include <muesli/Smallstrain/sdamage.h>

#include <jlcxx/jlcxx.hpp>

#include <julia.h>

// Forward declare
template <typename Material, typename MaterialPoint, bool registerConvergedState = true,
          typename MaterialBase = muesli::smallStrainMaterial, typename MaterialPointBase = muesli::smallStrainMP>
std::pair<jlcxx::TypeWrapper<Material>, jlcxx::TypeWrapper<MaterialPoint>> registerSmallStrainMaterial(
    jlcxx::Module& mod, const std::string& name);

inline void registerSmallStrainMaterials(jlcxx::Module& mod) {
  using jlcxx::arg;
  using jlcxx::julia_base_type;

  mod.add_type<muesli::smallStrainMaterial>("SmallStrainMaterial", julia_base_type<muesli::material>());
  mod.add_type<muesli::smallStrainMP>("SmallStrainMP", julia_base_type<muesli::materialPoint>());
  {
    using Material      = muesli::elasticIsotropicMaterial;
    using MaterialPoint = muesli::elasticIsotropicMP;

    auto [mat, mp] = registerSmallStrainMaterial<Material, MaterialPoint>(mod, "ElasticIsotropic");
    mat.constructor(
        [](double Emod, double nu, double rho = 1.0) { return new Material{"ElasticIsotropic", Emod, nu, rho}; },
        arg("Emod"), arg("nu"), arg("rho") = 1.0);
  }
  {
    using Material      = muesli::elasticAnisotropicMaterial;
    using MaterialPoint = muesli::elasticAnisotropicMP;

    auto [mat, mp] = registerSmallStrainMaterial<Material, MaterialPoint>(mod, "ElasticAnisotropic");
    mat.constructor(
        [](JuliaVector c, double rho = 1.0) {
          const double* data = assertSizeAndExtractData(c, 21);
          return new Material{"ElasticAnisotropic", data, rho};
        },
        arg("c"), arg("rho") = 1.0);
  }
  {
    using Material      = muesli::elasticOrthotropicMaterial;
    using MaterialPoint = muesli::elasticOrthotropicMP;

    auto [mat, mp] = registerSmallStrainMaterial<Material, MaterialPoint>(mod, "ElasticOrthotropic");
    mat.constructor(
        [](JuliaVector c, double rho = 1.0) {
          const double* data = assertSizeAndExtractData(c, 9);
          return new Material{"ElasticOrthotropic", data, rho};
        },
        arg("c"), arg("rho") = 1.0);
  }
  {
    using Material      = muesli::elasticTransverselyisotropicMaterial;
    using MaterialPoint = muesli::elasticTransverselyisotropicMP;

    auto [mat, mp] = registerSmallStrainMaterial<Material, MaterialPoint>(mod, "ElasticTransverselyisotropic");
    mat.constructor(
        [](JuliaVector c, double rho = 1.0) {
          const double* data = assertSizeAndExtractData(c, 6);
          return new Material{"ElasticTransverselyisotropic", data, rho};
        },
        arg("c"), arg("rho") = 1.0);
  }
  {
    using Material      = muesli::splasticMaterial;
    using MaterialPoint = muesli::splasticMP;

    auto [mat, mp] = registerSmallStrainMaterial<Material, MaterialPoint, false>(mod, "Splastic");

    mat.constructor([](double E, double nu, double rho, double Hiso, double Hkine, double yield, double xalpha,
                       const std::string& plasticityType) {
      return new Material{"Splastic", E, nu, rho, Hiso, Hkine, yield, xalpha, plasticityType};
    });

    mat.method("setConvergedState",
               [](MaterialPoint& mp, const double theTime, const istensor& strain, const double dg, const istensor& epn,
                  const double xin, const istensor& Xin) { mp.setConvergedState(theTime, strain, dg, epn, xin, Xin); });
  }
  {
    using Material      = muesli::viscoelasticMaterial;
    using MaterialPoint = muesli::viscoelasticMP;

    auto [mat, mp] = registerSmallStrainMaterial<Material, MaterialPoint, false>(mod, "Viscoelastic");

    mat.constructor([](double E, double nu, double rho, size_t nvisco, JuliaVector eta, JuliaVector tau) {
      return new Material{"Viscoelastic", E, nu, rho, nvisco, eta.data(), tau.data()};
    });
    mat.method("setConvergedState",
               [](MaterialPoint& mp, double theTime, const istensor& strain, ArrayOfTensorsT<istensor> epsv_arrays,
                  const istensor& epsdev, const double& theta) {
                 std::vector<istensor> epsv = epsv_arrays.tensors();
                 mp.setConvergedState(theTime, strain, epsv, epsdev, theta);
               });
  }
  {
    using Material      = muesli::viscoplasticMaterial;
    using MaterialPoint = muesli::viscoplasticMP;

    auto [mat, mp] = registerSmallStrainMaterial<Material, MaterialPoint, false>(mod, "Viscoplastic");

    mat.constructor([](double E, double nu, double rho, double Hiso, double Hkine, double yield,
                       const std::string& plasticityType, double eta, double alpha) {
      return new Material{"Viscoplastic", E, nu, rho, Hiso, Hkine, yield, plasticityType, eta, alpha};
    });

    mat.method("setConvergedState",
               [](MaterialPoint& mp, double theTime, double dg, const istensor& epn, double xin, const istensor& Xin,
                  const istensor& strain) { mp.setConvergedState(theTime, dg, epn, xin, Xin, strain); });
  }
  mod.add_type<muesli::sdamageMaterial>("SdamageMaterial", julia_base_type<muesli::smallStrainMaterial>());
  mod.add_type<muesli::sdamageMP>("SdamageMP", julia_base_type<muesli::smallStrainMP>());
  {
    using Material      = muesli::GTN_Material;
    using MaterialPoint = muesli::GTN_MP;

    auto [mat, mp] =
        registerSmallStrainMaterial<Material, MaterialPoint, false, muesli::sdamageMaterial, muesli::sdamageMP>(mod,
                                                                                                                "GTN_");
    mat.constructor([](double E, double nu, double rho, double q1, double q2, double yield) {
      return new Material{"GTN", E, nu, rho, q1, q2, yield};
    });
  }
  {
    using Material      = muesli::Gurson_Material;
    using MaterialPoint = muesli::Gurson_MP;

    auto [mat, mp] =
        registerSmallStrainMaterial<Material, MaterialPoint, false, muesli::sdamageMaterial, muesli::sdamageMP>(
            mod, "Gurson_");
    mat.constructor([](double E, double nu, double rho, double xRinf, double xRb, double yield) {
      return new Material{"Gurson", E, nu, rho, xRinf, xRb, yield};
    });
  }
  {
    using Material      = muesli::Lemaitre_Material;
    using MaterialPoint = muesli::Lemaitre_MP;

    auto [mat, mp] =
        registerSmallStrainMaterial<Material, MaterialPoint, false, muesli::sdamageMaterial, muesli::sdamageMP>(
            mod, "Lemaitre_");
    mat.constructor([](double E, double nu, double rho, double r, double s, double yield, double xR_inf, double xR_b) {
      return new Material{"Lemaitre", E, nu, rho, r, s, yield, xR_inf, xR_b};
    });
  }
  {
    using Material      = muesli::LemKin_Material;
    using MaterialPoint = muesli::LemKin_MP;

    auto [mat, mp] =
        registerSmallStrainMaterial<Material, MaterialPoint, false, muesli::sdamageMaterial, muesli::sdamageMP>(
            mod, "LemKin_");
    mat.constructor([](const double E, double nu, double rho, double r, double s, double yield, double xR_inf,
                       double xR_b, double xa,
                       double xb) { return new Material{"LemKin", E, nu, rho, r, s, yield, xR_inf, xR_b, xa, xb}; });
  }
}
