// SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de
// SPDX-License-Identifier: GPL-3.0-or-later

#include <jlmuesli/smallstrain/registersmallstrain.hh>
#include <jlmuesli/util/common.hh>
#include <jlmuesli/util/utils.hh>

#include <muesli/muesli.h>

#include <jlcxx/jlcxx.hpp>

template <typename Material, typename MaterialPoint, bool registerConvergedState, typename MaterialBase,
          typename MaterialPointBase>
std::pair<jlcxx::TypeWrapper<Material>, jlcxx::TypeWrapper<MaterialPoint>> registerSmallStrainMaterial(
    jlcxx::Module& mod, const std::string& name) {
  using jlcxx::arg;

  std::string matName = name + "Material";
  std::string mpName  = name + "MP";

  auto mat = mod.add_type<Material>(matName, jlcxx::julia_base_type<MaterialBase>());
  mat.method("check", &Material::check);
  mat.method("print", [](Material& mat) {
    mat.print(std::cout);
    std::cout << std::endl;
  });
  mat.method("getProperty", &Material::getProperty);

  mat.constructor([](const MaterialProperties& properties) { return new Material{"Elastic", properties.multiMap()}; },
                  arg("properties"));

  mat.method("createMaterialPoint", &Material::createMaterialPoint);

  auto mp =
      mod.add_type<MaterialPoint>(mpName, jlcxx::julia_base_type<MaterialPointBase>())
          .constructor([](const Material& sm) { return new MaterialPoint(sm); })
          // ----------------------------------------------------------------------
          // 3D response
          // ----------------------------------------------------------------------
          .method("contractWithDeviatoricTangent!", [](MaterialPoint& mp, const ivector& v1, const ivector& v2,
                                                       itensor& T) { mp.contractWithDeviatoricTangent(v1, v2, T); })

          .method("contractWithTangent!", [](MaterialPoint& mp, const ivector& v1, const ivector& v2,
                                             itensor& T) { mp.contractWithTangent(v1, v2, T); })
          .method("contractWithMixedTangent!", [](MaterialPoint& mp, istensor& CM) { mp.contractWithMixedTangent(CM); })

          .method("dissipationTangent!", [](MaterialPoint& mp, itensor4& D) { mp.dissipationTangent(D); })

          .method("plasticSlip", [](MaterialPoint& mp) { return mp.plasticSlip(); })

          .method("shearStiffness", [](MaterialPoint& mp) { return mp.shearStiffness(); })

          .method("tangentTensor!", [](MaterialPoint& mp, itensor4& C) { mp.tangentTensor(C); })

          // .method("tangentMatrix",
          //         [](MaterialPoint& mp) {
          //           // NOT IMPLEMENTED
          //         })

          .method("volumetricStiffness", [](MaterialPoint& mp) { return mp.volumetricStiffness(); })

          // ----------------------------------------------------------------------
          // Energy
          // ----------------------------------------------------------------------
          .method("deviatoricEnergy", [](MaterialPoint& mp) { return mp.deviatoricEnergy(); })

          .method("energyDissipationInStep", [](MaterialPoint& mp) { return mp.energyDissipationInStep(); })

          .method("effectiveStoredEnergy", [](MaterialPoint& mp) { return mp.effectiveStoredEnergy(); })

          .method("kineticPotential", [](MaterialPoint& mp) { return mp.kineticPotential(); })

          .method("storedEnergy", [](MaterialPoint& mp) { return mp.storedEnergy(); })

          // .method("thermodynamicPotentials",
          //         [](MaterialPoint& mp) {
          //           // thPotentials is a custom return type; you need to ensure
          //           // you have a jlcxx wrapper or to define how it is converted.
          //           return mp.thermodynamicPotentials();
          //         })

          .method("volumetricEnergy", [](MaterialPoint& mp) { return mp.volumetricEnergy(); })

          // ----------------------------------------------------------------------
          // Stresses
          // ----------------------------------------------------------------------
          .method("pressure", [](MaterialPoint& mp) { return mp.pressure(); })

          .method("stress!", [](MaterialPoint& mp, istensor& sigma) { mp.stress(sigma); })

          .method("deviatoricStress!", [](MaterialPoint& mp, istensor& sigma) { mp.deviatoricStress(sigma); })

          // ----------------------------------------------------------------------
          // Strains / States
          // ----------------------------------------------------------------------
          .method("getConvergedPlasticStrain",
                  [](MaterialPoint& mp) -> istensor { return mp.getConvergedPlasticStrain(); })

          .method("getCurrentPlasticStrain", [](MaterialPoint& mp) -> istensor { return mp.getCurrentPlasticStrain(); })

          .method("getConvergedState", [](MaterialPoint& mp) { return mp.getConvergedState(); })

          .method("getCurrentState", [](MaterialPoint& mp) { return mp.getCurrentState(); })

          // ----------------------------------------------------------------------
          // Bookkeeping
          // ----------------------------------------------------------------------
          .method("commitCurrentState", [](MaterialPoint& mp) { mp.commitCurrentState(); })

          .method("resetCurrentState", [](MaterialPoint& mp) { mp.resetCurrentState(); })

          .method("updateCurrentState",
                  [](MaterialPoint& mp, double t, const istensor& strain) { mp.updateCurrentState(t, strain); });

  if constexpr (registerConvergedState) {
    mat.method("setConvergedState", [](MaterialPoint& mp, double theTime, const istensor& strain) {
      mp.setConvergedState(theTime, strain);
    });
  }

  return std::make_pair(mat, mp);
}

#define INSTANTIATE_SMALL_STRAIN_MATERIAL_BASE(Material, MaterialMP, MaterialBase, MPBase) \
  template std::pair<jlcxx::TypeWrapper<Material>, jlcxx::TypeWrapper<MaterialMP>>         \
  registerSmallStrainMaterial<Material, MaterialMP, false, MaterialBase, MPBase>(jlcxx::Module&, const std::string&);

#define INSTANTIATE_SMALL_STRAIN_MATERIAL_F(Material, MaterialMP)                  \
  template std::pair<jlcxx::TypeWrapper<Material>, jlcxx::TypeWrapper<MaterialMP>> \
  registerSmallStrainMaterial<Material, MaterialMP, false>(jlcxx::Module&, const std::string&);

#define INSTANTIATE_SMALL_STRAIN_MATERIAL(Material, MaterialMP)                    \
  template std::pair<jlcxx::TypeWrapper<Material>, jlcxx::TypeWrapper<MaterialMP>> \
  registerSmallStrainMaterial<Material, MaterialMP>(jlcxx::Module&, const std::string&);

// Use the macro for explicit instantiations
INSTANTIATE_SMALL_STRAIN_MATERIAL(muesli::elasticIsotropicMaterial, muesli::elasticIsotropicMP)
INSTANTIATE_SMALL_STRAIN_MATERIAL(muesli::elasticAnisotropicMaterial, muesli::elasticAnisotropicMP)
INSTANTIATE_SMALL_STRAIN_MATERIAL(muesli::elasticOrthotropicMaterial, muesli::elasticOrthotropicMP)
INSTANTIATE_SMALL_STRAIN_MATERIAL(muesli::elasticTransverselyisotropicMaterial, muesli::elasticTransverselyisotropicMP)
INSTANTIATE_SMALL_STRAIN_MATERIAL_F(muesli::splasticMaterial, muesli::splasticMP)
INSTANTIATE_SMALL_STRAIN_MATERIAL_F(muesli::viscoelasticMaterial, muesli::viscoelasticMP)
INSTANTIATE_SMALL_STRAIN_MATERIAL_F(muesli::viscoplasticMaterial, muesli::viscoplasticMP)
INSTANTIATE_SMALL_STRAIN_MATERIAL_BASE(muesli::GTN_Material, muesli::GTN_MP, muesli::sdamageMaterial, muesli::sdamageMP)
INSTANTIATE_SMALL_STRAIN_MATERIAL_BASE(muesli::Gurson_Material, muesli::Gurson_MP, muesli::sdamageMaterial,
                                       muesli::sdamageMP)
INSTANTIATE_SMALL_STRAIN_MATERIAL_BASE(muesli::Lemaitre_Material, muesli::Lemaitre_MP, muesli::sdamageMaterial,
                                       muesli::sdamageMP)
INSTANTIATE_SMALL_STRAIN_MATERIAL_BASE(muesli::LemKin_Material, muesli::LemKin_MP, muesli::sdamageMaterial,
                                       muesli::sdamageMP)
