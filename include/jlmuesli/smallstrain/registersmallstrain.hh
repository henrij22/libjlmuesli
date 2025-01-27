#pragma once

#include "smallstrainbindings.hh"

#include <vector>

#include <jlmuesli/util/common.hh>

#include <muesli/muesli.h>

#include <jlcxx/array.hpp>
#include <jlcxx/jlcxx.hpp>

#include <julia.h>

inline void registerSmallStrainMaterials(jlcxx::Module& mod) {
  using jlcxx::arg;
  using jlcxx::julia_base_type;
  using JuliaTensor = jlcxx::ArrayRef<double, 2>;
  using JuliaVector = jlcxx::ArrayRef<double, 1>;

  mod.add_type<muesli::smallStrainMaterial>("SmallStrainMaterial", julia_base_type<muesli::material>());
  mod.add_type<muesli::smallStrainMP>("SmallStrainMP", julia_base_type<muesli::materialPoint>());
  {
    using Material      = muesli::elasticIsotropicMaterial;
    using MaterialPoint = muesli::elasticIsotropicMP;

    auto [mat, mp] = registerSmallStrainMaterial<Material, MaterialPoint>(mod, "ElasticIsotropic");
    mat.constructor(
        [](double Emod, double nu, double rho = 1.0) { return new Material{"ElasticIsotropic", Emod, nu, rho}; },
        arg("Emod"), arg("nu"), arg("rho") = 1.0);

    mat.method("setConvergedState", [](MaterialPoint& mp, double theTime, jlcxx::ArrayRef<double, 2> strain_array) {
      istensor strain = toIstensor(strain_array);
      mp.setConvergedState(theTime, strain);
    });
  }
  {
    using Material      = muesli::splasticMaterial;
    using MaterialPoint = muesli::splasticMP;

    auto [mat, mp] = registerSmallStrainMaterial<Material, MaterialPoint>(mod, "Splastic");

    mat.constructor([](const std::string& name, double E, double nu, double rho, double Hiso, double Hkine,
                       double yield, double xalpha, const std::string& plasticityType) {
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

    auto [mat, mp] = registerSmallStrainMaterial<Material, MaterialPoint>(mod, "Viscoelastic");

    mat.constructor(
        [](const std::string& name, double E, double nu, double rho, size_t nvisco, JuliaVector eta, JuliaVector tau) {
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

    auto [mat, mp] = registerSmallStrainMaterial<Material, MaterialPoint>(mod, "Viscoplastic");

    mat.constructor([](const std::string& name, double E, double nu, double rho, double Hiso, double Hkine,
                       double yield, const std::string& plasticityType, double eta, double alpha) {
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
}