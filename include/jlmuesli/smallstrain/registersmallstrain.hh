#pragma once

#include "smallstrainbindings.hh"

#include <vector>

#include <jlmuesli/util/common.hh>

#include <muesli/muesli.h>
#include <muesli/Smallstrain/sdamage.h>

#include <jlcxx/array.hpp>
#include <jlcxx/jlcxx.hpp>

#include <julia.h>

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

    mat.method("setConvergedState", [](MaterialPoint& mp, double theTime, JuliaTensor strain_array, double dg,
                                       JuliaTensor epn_array, double xin, JuliaTensor Xin_array) {
      istensor strain = toIstensor(strain_array);
      istensor epn    = toIstensor(epn_array);
      istensor Xin    = toIstensor(Xin_array);
      mp.setConvergedState(theTime, strain, dg, epn, xin, Xin);
    });
  }
  {
    using Material      = muesli::viscoelasticMaterial;
    using MaterialPoint = muesli::viscoelasticMP;

    auto [mat, mp] = registerSmallStrainMaterial<Material, MaterialPoint, false>(mod, "Viscoelastic");

    mat.constructor([](double E, double nu, double rho, size_t nvisco, JuliaVector eta, JuliaVector tau) {
      return new Material{"Splastic", E, nu, rho, nvisco, eta.data(), tau.data()};
    });
    mat.method("setConvergedState", [](MaterialPoint& mp, double theTime, JuliaTensor strain_array,
                                       ArrayOfTensorsT<istensor> epsv_arrays, JuliaTensor epsdev_array, double theta) {
      istensor strain            = toIstensor(strain_array);
      istensor epsdev            = toIstensor(epsdev_array);
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

    mat.method("setConvergedState", [](MaterialPoint& mp, const double theTime, const double dg, JuliaTensor epn_array,
                                       const double xin, JuliaTensor Xin_array, JuliaTensor strain_array) {
      istensor strain = toIstensor(strain_array);
      istensor epn    = toIstensor(epn_array);
      istensor Xin    = toIstensor(Xin_array);
      mp.setConvergedState(theTime, dg, epn, xin, Xin, strain);
    });
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
