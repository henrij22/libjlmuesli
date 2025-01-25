
#pragma once
#include <jlmuesli/util/common.hh>
#include <jlmuesli/util/helpers.hh>

#include <muesli/muesli.h>

#include <jlcxx/jlcxx.hpp>

template <typename Material, typename MaterialPoint, typename MaterialBase = muesli::smallStrainMaterial,
          typename MaterialPointBase = muesli::smallStrainMP>
auto registerSmallStrainMaterial(jlcxx::Module& mod, const std::string& name) {
  using jlcxx::arg;

  using JuliaTensor = jlcxx::ArrayRef<double, 2>;
  using JuliaVector = jlcxx::ArrayRef<double, 1>;

  std::string matName = name + "Material";
  std::string mpName  = name + "MP";

  auto mat = mod.add_type<Material>(matName, jlcxx::julia_base_type<MaterialBase>());
  mat.method("check", &Material::check);
  mat.method("print", [](Material& mat) { mat.print(std::cout); });
  mat.method("getProperty", &Material::getProperty);

  mat.constructor([](const MaterialProperties& properties) { return new Material{"Elastic", properties.multiMap()}; },
                  arg("properties"));

  auto mp = mod.add_type<MaterialPoint>(mpName, jlcxx::julia_base_type<MaterialPointBase>())
                .constructor([](const Material& sm) { return new MaterialPoint(sm); })
                // ----------------------------------------------------------------------
                // 3D response
                // ----------------------------------------------------------------------
                .method("contractWithDeviatoricTangent",
                        [](MaterialPoint& mp, JuliaVector v1, JuliaVector v2) {
                          itensor result;
                          mp.contractWithDeviatoricTangent(toIVector(v1), toIVector(v2), result);
                          return itensorToArrayRef(result);
                        })

                .method("contractWithTangent",
                        [](MaterialPoint& mp, JuliaVector v1, JuliaVector v2) {
                          itensor result;
                          mp.contractWithTangent(toIVector(v1), toIVector(v2), result);
                          return itensorToArrayRef(result);
                        })

                .method("dissipationTangent",
                        [](MaterialPoint& mp) {
                          itensor4 D;
                          mp.dissipationTangent(D);
                          // Convert itensor4 to a Julia-compatible array
                          return itensor4ToArrayRef(D);
                        })

                .method("plasticSlip", [](MaterialPoint& mp) { return mp.plasticSlip(); })

                .method("shearStiffness", [](MaterialPoint& mp) { return mp.shearStiffness(); })

                .method("tangentTensor",
                        [](MaterialPoint& mp) {
                          itensor4 C;
                          mp.tangentTensor(C);
                          return itensor4ToArrayRef(C);
                        })

                .method("tangentMatrix",
                        [](MaterialPoint& mp) {
                          // tangentMatrix expects a double[6][6] array
                          double C[6][6];
                          mp.tangentMatrix(C);

                          // Flatten into a 36-element std::vector (or whatever is
                          // most convenient for you to return to Julia)
                          std::vector<double> result(36);
                          for (int i = 0; i < 6; ++i) {
                            for (int j = 0; j < 6; ++j) {
                              result[i * 6 + j] = C[i][j];
                            }
                          }
                          return result; // Julia will see this as a Vector{Float64}
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

                .method("stress",
                        [](MaterialPoint& mp) {
                          istensor sigma;
                          mp.stress(sigma);
                          // Convert to array (6 components for a symmetric tensor, etc.)
                          return itensorToArrayRef(sigma);
                        })

                .method("deviatoricStress",
                        [](MaterialPoint& mp) {
                          istensor s;
                          mp.deviatoricStress(s);
                          return itensorToArrayRef(s);
                        })

                // ----------------------------------------------------------------------
                // Strains / States
                // ----------------------------------------------------------------------
                .method("getConvergedPlasticStrain",
                        [](MaterialPoint& mp) {
                          istensor epsp = mp.getConvergedPlasticStrain();
                          return itensorToArrayRef(epsp);
                        })

                .method("getCurrentPlasticStrain",
                        [](MaterialPoint& mp) {
                          istensor epsp = mp.getCurrentPlasticStrain();
                          return itensorToArrayRef(epsp);
                        })

                .method("getConvergedState", [](MaterialPoint& mp) { return mp.getConvergedState(); })

                .method("getCurrentState", [](MaterialPoint& mp) { return mp.getCurrentState(); })

                // ----------------------------------------------------------------------
                // Bookkeeping
                // ----------------------------------------------------------------------
                .method("commitCurrentState", [](MaterialPoint& mp) { mp.commitCurrentState(); })

                .method("resetCurrentState", [](MaterialPoint& mp) { mp.resetCurrentState(); })

                .method("updateCurrentState", [](MaterialPoint& mp, double t, JuliaTensor strain_array) {
                  istensor strain = toIStensor(strain_array);
                  mp.updateCurrentState(t, strain);
                });

  return std::make_tuple(mat, mp);
}
