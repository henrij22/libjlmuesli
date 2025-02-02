// SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <jlmuesli/util/common.hh>
#include <jlmuesli/util/helpers.hh>

#include <muesli/muesli.h>

#include <jlcxx/jlcxx.hpp>

template <typename Material, typename MaterialPoint, bool registerConvergedState = true,
          typename MaterialBase = muesli::smallStrainMaterial, typename MaterialPointBase = muesli::smallStrainMP>
auto registerSmallStrainMaterial(jlcxx::Module& mod, const std::string& name) {
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

  auto mp = mod.add_type<MaterialPoint>(mpName, jlcxx::julia_base_type<MaterialPointBase>())
                .constructor([](const Material& sm) { return new MaterialPoint(sm); })
                // ----------------------------------------------------------------------
                // 3D response
                // ----------------------------------------------------------------------
                .method("contractWithDeviatoricTangent!",
                        [](MaterialPoint& mp, JuliaVector v1, JuliaVector v2, JuliaTensor T) {
                          itensor result;
                          mp.contractWithDeviatoricTangent(toIVector(v1), toIVector(v2), result);
                          itensorToArrayRef(result, T);
                        })

                .method("contractWithTangent!",
                        [](MaterialPoint& mp, JuliaVector v1, JuliaVector v2, JuliaTensor T) {
                          itensor result;
                          mp.contractWithTangent(toIVector(v1), toIVector(v2), result);
                          itensorToArrayRef(result, T);
                        })

                .method("dissipationTangent!",
                        [](MaterialPoint& mp, JuliaTensor4 T) {
                          itensor4 D;
                          mp.dissipationTangent(D);
                          itensor4ToArrayRef(D, T);
                        })

                .method("plasticSlip", [](MaterialPoint& mp) { return mp.plasticSlip(); })

                .method("shearStiffness", [](MaterialPoint& mp) { return mp.shearStiffness(); })

                .method("tangentTensor!",
                        [](MaterialPoint& mp, JuliaTensor4 T) {
                          itensor4 C;
                          mp.tangentTensor(C);
                          itensor4ToArrayRef(C, T);
                        })

                .method("tangentMatrix",
                        [](MaterialPoint& mp) {
                          // tangentMatrix expects a double[6][6] array
                          double C[6][6];
                          mp.tangentMatrix(C);

                          // Flatten into a 36-element std::vector
                          std::vector<double> result(36);
                          for (int i = 0; i < 6; ++i) {
                            for (int j = 0; j < 6; ++j) {
                              result[i * 6 + j] = C[i][j];
                            }
                          }
                          return result;
                        })

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

                .method("stress!",
                        [](MaterialPoint& mp, JuliaTensor T) {
                          istensor sigma;
                          mp.stress(sigma);
                          // Convert to array (6 components for a symmetric tensor, etc.)
                          itensorToArrayRef(sigma, T);
                        })

                .method("deviatoricStress!",
                        [](MaterialPoint& mp, JuliaTensor T) {
                          istensor sigma;
                          mp.deviatoricStress(sigma);
                          itensorToArrayRef(sigma, T);
                        })

                // ----------------------------------------------------------------------
                // Strains / States
                // ----------------------------------------------------------------------
                .method("getConvergedPlasticStrain!",
                        [](MaterialPoint& mp, JuliaTensor T) {
                          istensor epsp = mp.getConvergedPlasticStrain();
                          itensorToArrayRef(epsp, T);
                        })

                .method("getCurrentPlasticStrain!",
                        [](MaterialPoint& mp, JuliaTensor T) {
                          istensor epsp = mp.getCurrentPlasticStrain();
                          itensorToArrayRef(epsp, T);
                        })

                .method("getConvergedState", [](MaterialPoint& mp) { return mp.getConvergedState(); })

                .method("getCurrentState", [](MaterialPoint& mp) { return mp.getCurrentState(); })

                // ----------------------------------------------------------------------
                // Bookkeeping
                // ----------------------------------------------------------------------
                .method("commitCurrentState", [](MaterialPoint& mp) { mp.commitCurrentState(); })

                .method("resetCurrentState", [](MaterialPoint& mp) { mp.resetCurrentState(); })

                .method("updateCurrentState", [](MaterialPoint& mp, double t, JuliaTensor strain_array) {
                  istensor strain = toIstensor(strain_array);
                  mp.updateCurrentState(t, strain);
                });

  if constexpr (registerConvergedState) {
    mat.method("setConvergedState", [](MaterialPoint& mp, double theTime, jlcxx::ArrayRef<double, 2> strain_array) {
      istensor strain = toIstensor(strain_array);
      mp.setConvergedState(theTime, strain);
    });
  }

  return std::make_tuple(mat, mp);
}
