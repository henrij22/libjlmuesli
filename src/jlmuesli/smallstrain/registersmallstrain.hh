// SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <jlmuesli/util/common.hh>
#include <jlmuesli/util/utils.hh>

#include <muesli/muesli.h>
#include <muesli/Smallstrain/sdamage.h>

#include <jlcxx/jlcxx.hpp>

#include <julia.h>

// viscoplasticnlMaterial::density() returns a member that no constructor initialises
template <>
struct ConcreteMaterial<muesli::viscoplasticnlMaterial>
{
  using type = WithBaseDensity<muesli::viscoplasticnlMaterial>;
};

// viscoplasticMaterial::density() returns a member that only the positional constructor initialises
template <>
struct ConcreteMaterial<muesli::viscoplasticMaterial>
{
  using type = WithBaseDensity<muesli::viscoplasticMaterial>;
};

// smallstrainbindings.cpp: the family roots and everything they share
void registerSmallStrainBase(jlcxx::Module& mod);

/// Registers `<name>Material` and `<name>MP`. `MaterialBase`/`MaterialPointBase` must be the
/// direct C++ base, which is not the family root for the orthotropic and transversely
/// isotropic materials.
template <typename Material, typename MaterialPoint, typename MaterialBase = muesli::smallStrainMaterial,
          typename MaterialPointBase = muesli::smallStrainMP>
std::pair<jlcxx::TypeWrapper<Material>, jlcxx::TypeWrapper<MaterialPoint>> addSmallStrainMaterial(
    jlcxx::Module& mod, const std::string& name) {
  using jlcxx::arg;

  auto mat = mod.add_type<Material>(name + "Material", jlcxx::julia_base_type<MaterialBase>());
  mat.constructor(
      [muesliName = muesliMaterialName(name)](const MaterialProperties& properties) {
        return static_cast<Material*>(new typename ConcreteMaterial<Material>::type{muesliName, properties.multiMap()});
      },
      arg("properties"));

  auto mp = mod.add_type<MaterialPoint>(name + "MP", jlcxx::julia_base_type<MaterialPointBase>())
                .constructor([](const Material& m) { return new MaterialPoint(m); });

  // Not muesli's createMaterialPoint: that returns a raw base pointer Julia would never free.
  // Must follow the MP registration, since a method cannot return a type that does not exist yet.
  // The cxx prefix marks it as raw: MuesliMaterials.jl wraps it to keep the material alive.
  mat.method("cxxCreateMaterialPoint", [](const Material& m) { return jlcxx::create<MaterialPoint>(m); });

  return std::make_pair(mat, mp);
}

/// For materials whose whole history is the strain; the others register their own overload.
template <typename MaterialPoint>
void addStrainOnlyConvergedState(jlcxx::TypeWrapper<MaterialPoint>& mp) {
  mp.method("setConvergedState",
            [](MaterialPoint& mp, double theTime, const istensor& strain) { mp.setConvergedState(theTime, strain); });
}

inline void registerSmallStrainMaterials(jlcxx::Module& mod) {
  using jlcxx::arg;
  using jlcxx::julia_base_type;

  registerSmallStrainBase(mod);

  {
    using Material      = muesli::elasticIsotropicMaterial;
    using MaterialPoint = muesli::elasticIsotropicMP;

    auto [mat, mp] = addSmallStrainMaterial<Material, MaterialPoint>(mod, "ElasticIsotropic");
    addStrainOnlyConvergedState(mp);
    mat.constructor(
        [](double Emod, double nu, double rho = 1.0) { return new Material{"ElasticIsotropic", Emod, nu, rho}; },
        arg("Emod"), arg("nu"), arg("rho") = 1.0);
  }
  {
    using Material      = muesli::elasticAnisotropicMaterial;
    using MaterialPoint = muesli::elasticAnisotropicMP;

    auto [mat, mp] = addSmallStrainMaterial<Material, MaterialPoint>(mod, "ElasticAnisotropic");
    addStrainOnlyConvergedState(mp);
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

    // derives from elasticAnisotropicMaterial in C++, not straight from smallStrainMaterial
    auto [mat, mp] = addSmallStrainMaterial<Material, MaterialPoint, muesli::elasticAnisotropicMaterial,
                                            muesli::elasticAnisotropicMP>(mod, "ElasticOrthotropic");
    addStrainOnlyConvergedState(mp);
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

    // likewise derives from elasticAnisotropicMaterial
    auto [mat, mp] = addSmallStrainMaterial<Material, MaterialPoint, muesli::elasticAnisotropicMaterial,
                                            muesli::elasticAnisotropicMP>(mod, "ElasticTransverselyisotropic");
    addStrainOnlyConvergedState(mp);
    mat.constructor(
        [](JuliaVector c, double rho = 1.0) {
          const double* data = assertSizeAndExtractData(c, 6);
          return new Material{"ElasticTransverselyisotropic", data, rho};
        },
        arg("c"), arg("rho") = 1.0);
    // the engineering-constants overload, with the axis of symmetry along x: E1 is the
    // axial modulus, E2 the transverse one, v12 the axial-transverse Poisson's ratio, and
    // G23/G12 the transverse and axial-transverse shear moduli
    mat.constructor(
        [](double E1, double E2, double v12, double G23, double G12, double rho = 1.0) {
          return new Material{"ElasticTransverselyisotropic", E1, E2, v12, G23, G12, rho};
        },
        arg("E1"), arg("E2"), arg("v12"), arg("G23"), arg("G12"), arg("rho") = 1.0);
  }
  {
    using Material      = muesli::splasticMaterial;
    using MaterialPoint = muesli::splasticMP;

    auto [mat, mp] = addSmallStrainMaterial<Material, MaterialPoint>(mod, "Splastic");

    mat.constructor([](double E, double nu, double rho, double Hiso, double Hkine, double yield, double xalpha,
                       const std::string& plasticityType) {
      return new Material{"Splastic", E, nu, rho, Hiso, Hkine, yield, xalpha, plasticityType};
    });

    mp.method("setConvergedState",
              [](MaterialPoint& mp, const double theTime, const istensor& strain, const double dg, const istensor& epn,
                 const double xin, const istensor& Xin) { mp.setConvergedState(theTime, strain, dg, epn, xin, Xin); });
  }
  {
    using Material      = muesli::viscoelasticMaterial;
    using MaterialPoint = muesli::viscoelasticMP;

    auto [mat, mp] = addSmallStrainMaterial<Material, MaterialPoint>(mod, "Viscoelastic");

    mat.constructor([](double E, double nu, double rho, size_t nvisco, JuliaVector eta, JuliaVector tau) {
      return new Material{"Viscoelastic", E, nu, rho, nvisco, eta.data(), tau.data()};
    });
    mp.method("setConvergedState",
              [](MaterialPoint& mp, double theTime, const istensor& strain, const ArrayOfTensorsT<istensor>& epsv,
                 const istensor& epsdev,
                 const double& theta) { mp.setConvergedState(theTime, strain, epsv.tensors(), epsdev, theta); });
  }
  {
    using Material      = muesli::viscoplasticMaterial;
    using MaterialPoint = muesli::viscoplasticMP;

    auto [mat, mp] = addSmallStrainMaterial<Material, MaterialPoint>(mod, "Viscoplastic");

    // The positional MUESLI constructor reads (λ, μ) and zeroes the viscous exponent and the reference rate,
    // so the material is built from properties, like the other forms.
    mat.constructor([](double E, double nu, double rho, double Hiso, double Hkine, double yield,
                       const std::string& plasticityType, double eta, double alpha) {
      if (plasticityType != "mises" && plasticityType != "von_mises" && plasticityType != "tresca")
        throw std::invalid_argument("Viscoplastic supports the plasticity types \"mises\" and \"tresca\", got \"" +
                                    plasticityType + "\".");

      MaterialProperties properties;
      properties.set("young", E);
      properties.set("poisson", nu);
      properties.set("density", rho);
      properties.set("isotropich", Hiso);
      properties.set("kinematich", Hkine);
      properties.set("yieldstress", yield);
      properties.set("eta", eta);
      properties.set("alpha", alpha);
      properties.set("viscousexponent", 1.0);
      properties.set("srateref", 1.0);
      properties.setString("model", plasticityType == "tresca" ? "tresca" : "von_mises");
      return static_cast<Material*>(new
                                    typename ConcreteMaterial<Material>::type{"Viscoplastic", properties.multiMap()});
    });

    mp.method("setConvergedState",
              [](MaterialPoint& mp, double theTime, double dg, const istensor& epn, double xin, const istensor& Xin,
                 const istensor& strain) { mp.setConvergedState(theTime, dg, epn, xin, Xin, strain); });
  }
  mod.add_type<muesli::sdamageMaterial>("SdamageMaterial", julia_base_type<muesli::smallStrainMaterial>());
  mod.add_type<muesli::sdamageMP>("SdamageMP", julia_base_type<muesli::smallStrainMP>());
  {
    using Material      = muesli::GTN_Material;
    using MaterialPoint = muesli::GTN_MP;

    auto [mat, mp] =
        addSmallStrainMaterial<Material, MaterialPoint, muesli::sdamageMaterial, muesli::sdamageMP>(mod, "GTN_");
    mat.constructor([](double E, double nu, double rho, double q1, double q2, double yield) {
      return new Material{"GTN", E, nu, rho, q1, q2, yield};
    });
  }
  {
    using Material      = muesli::Gurson_Material;
    using MaterialPoint = muesli::Gurson_MP;

    auto [mat, mp] =
        addSmallStrainMaterial<Material, MaterialPoint, muesli::sdamageMaterial, muesli::sdamageMP>(mod, "Gurson_");
    mat.constructor([](double E, double nu, double rho, double xRinf, double xRb, double yield) {
      return new Material{"Gurson", E, nu, rho, xRinf, xRb, yield};
    });
  }
  {
    using Material      = muesli::Lemaitre_Material;
    using MaterialPoint = muesli::Lemaitre_MP;

    auto [mat, mp] =
        addSmallStrainMaterial<Material, MaterialPoint, muesli::sdamageMaterial, muesli::sdamageMP>(mod, "Lemaitre_");
    mat.constructor([](double E, double nu, double rho, double r, double s, double yield, double xR_inf, double xR_b) {
      return new Material{"Lemaitre", E, nu, rho, r, s, yield, xR_inf, xR_b};
    });
  }
  {
    using Material      = muesli::LemKin_Material;
    using MaterialPoint = muesli::LemKin_MP;

    auto [mat, mp] =
        addSmallStrainMaterial<Material, MaterialPoint, muesli::sdamageMaterial, muesli::sdamageMP>(mod, "LemKin_");
    mat.constructor([](const double E, double nu, double rho, double r, double s, double yield, double xR_inf,
                       double xR_b, double xa,
                       double xb) { return new Material{"LemKin", E, nu, rho, r, s, yield, xR_inf, xR_b, xa, xb}; });
  }

  addSmallStrainMaterial<muesli::viscoplasticnlMaterial, muesli::viscoplasticnlMP>(mod, "ViscoplasticNL");
}
