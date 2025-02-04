// SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de
// SPDX-License-Identifier: GPL-3.0-or-later

#include <jlmuesli/finitestrain/registerfinitestrain.hh>
#include <jlmuesli/util/common.hh>
#include <jlmuesli/util/utils.hh>

#include <muesli/muesli.h>
#include <muesli/Utils/utils.h>

#include <jlcxx/jlcxx.hpp>

template <typename Material, typename MaterialPoint, typename MaterialBase, typename MaterialPointBase>
jlcxx::TypeWrapper<Material> registerFiniteStrainMaterial(jlcxx::Module& mod, const std::string& name) {
  std::string matName = name + "Material";
  std::string mpName  = name + "MP";

  using jlcxx::arg;

  auto mat = mod.add_type<Material>(matName, jlcxx::julia_base_type<MaterialBase>());
  mat.method("check", &Material::check);
  mat.method("print", [](Material& mat) {
    mat.print(std::cout);
    std::cout << std::endl;
  });
  mat.method("getProperty", &Material::getProperty);

  mat.constructor([](const MaterialProperties& properties) { return new Material{"Finite", properties.multiMap()}; },
                  arg("properties"));

  mod.add_type<MaterialPoint>(mpName, jlcxx::julia_base_type<MaterialPointBase>())
      .constructor([](const Material& fm) { return new MaterialPoint(fm); })

      // --- Implementation / Utilities ---
      // .method(
      //     "testImplementation",
      //     [](MaterialPoint& mp, bool testDE = true, bool testDDE = true) -> bool {
      //       // For the std::ostream& of=std::cout argument, we might ignore or redirect it
      //       // if you want to capture output. For now, we just call the default version.
      //       // If you want to specify a different stream, you'd need a more elaborate approach.
      //       return mp.testImplementation(std::cout, true, true);
      //     },
      //     jlcxx::arg("testDE") = true, jlcxx::arg("testDDE") = true)
      .method("setRandom", [](MaterialPoint& mp) { mp.setRandom(); })
      .method("setTemperature", [](MaterialPoint& mp, double tt) { mp.setTemperature(tt); })

      // --- Energies ---
      .method("energyDissipationInStep", [](MaterialPoint& mp) { return mp.energyDissipationInStep(); })
      .method("dissipatedEnergyDF!",
              [](MaterialPoint& mp, JuliaTensor T) {
                itensor e = mp.dissipatedEnergyDF();
                itensorToArrayRef(e, T);
              })
      .method("dissipatedEnergyDTheta", [](MaterialPoint& mp) { return mp.dissipatedEnergyDTheta(); })
      .method("kineticPotential", [](MaterialPoint& mp) { return mp.kineticPotential(); })
      .method("effectiveStoredEnergy", [](MaterialPoint& mp) { return mp.effectiveStoredEnergy(); })
      .method("storedEnergy", [](MaterialPoint& mp) { return mp.storedEnergy(); })

      // --- Stresses ---
      .method("CauchyStress!", [](MaterialPoint& mp, istensor& sigma) { mp.CauchyStress(sigma); })
      .method("energyMomentumTensor!", [](MaterialPoint& mp, itensor& S) { mp.energyMomentumTensor(S); })
      .method("firstPiolaKirchhoffStress!", [](MaterialPoint& mp, itensor& P) { mp.firstPiolaKirchhoffStress(P); })
      .method("firstPiolaKirchhoffStressNumerical!",
              [](MaterialPoint& mp, itensor& P) { mp.firstPiolaKirchhoffStressNumerical(P); })
      .method("KirchhoffStress!", [](MaterialPoint& mp, istensor& tau) { mp.KirchhoffStress(tau); })
      .method("secondPiolaKirchhoffStress!", [](MaterialPoint& mp, istensor& S) { mp.secondPiolaKirchhoffStress(S); })
      .method("secondPiolaKirchhoffStressNumerical!",
              [](MaterialPoint& mp, istensor& S) { mp.secondPiolaKirchhoffStressNumerical(S); })

      // --- Elasticity tangents ---
      .method("convectedTangent!", [](MaterialPoint& mp, itensor4& c) { mp.convectedTangent(c); })
      .method("materialTangent!", [](MaterialPoint& mp, itensor4& c) { mp.materialTangent(c); })
      .method("spatialTangent!", [](MaterialPoint& mp, itensor4& c) { mp.spatialTangent(c); })

      // --- Tangent contractions ---
      // .method("contractWithAllTangents",
      //         [](MaterialPoint& mp, JuliaVector v1, JuliaVector v2) {
      //           itensor Tdev;
      //           istensor Tmixed;
      //           double Tvol = 0.0;
      //           mp.contractWithAllTangents(toIVector(v1), toIVector(v2), Tdev, Tmixed, Tvol);
      //           // Return a tuple: (Tdev, Tmixed, Tvol)
      //           // Tdev is an itensor => array
      //           // Tmixed is an istensor => array
      //           // Tvol is double
      //           return std::make_tuple(itensorToArrayRef(Tdev), itensorToArrayRef(Tmixed), Tvol);
      //         })
      .method("contractWithConvectedTangent!", [](MaterialPoint& mp, const ivector& v1, const ivector& v2,
                                                  itensor& T) { mp.contractWithConvectedTangent(v1, v2, T); })
      .method("contractWithSpatialTangent!", [](MaterialPoint& mp, const ivector& v1, const ivector& v2,
                                                itensor& T) { mp.contractWithSpatialTangent(v1, v2, T); })
      .method("contractWithDeviatoricTangent!", [](MaterialPoint& mp, const ivector& v1, const ivector& v2,
                                                   itensor& T) { mp.contractWithDeviatoricTangent(v1, v2, T); })
      .method("contractWithMixedTangent!", [](MaterialPoint& mp, istensor& CM) { mp.contractWithMixedTangent(CM); })
      .method(
          "convectedTangentTimesSymmetricTensor!",
          [](MaterialPoint& mp, const istensor& M, istensor& CM) { mp.convectedTangentTimesSymmetricTensor(M, CM); })
      .method("volumetricStiffness", [](MaterialPoint& mp) { return mp.volumetricStiffness(); })

      // --- Bookkeeping ---
      .method("commitCurrentState", [](MaterialPoint& mp) { mp.commitCurrentState(); })
      .method("resetCurrentState", [](MaterialPoint& mp) { mp.resetCurrentState(); })
      .method("updateCurrentState",
              [](MaterialPoint& mp, double theTime, itensor F) { mp.updateCurrentState(theTime, F); })

      // --- Extract state ---
      // For convergedDeformationGradient, we have both const and non-const versions in C++.
      // We'll just expose one that returns a copy
      .method("convergedDeformationGradient",
              [](MaterialPoint& mp) -> itensor { return mp.convergedDeformationGradient(); })
      .method("deformationGradient", [](MaterialPoint& mp) -> itensor { return mp.deformationGradient(); })
      .method("getConvergedState", [](MaterialPoint& mp) { return mp.getConvergedState(); })
      .method("getCurrentState", [](MaterialPoint& mp) { return mp.getCurrentState(); })

      // --- Miscellaneous ---
      .method("density", [](MaterialPoint& mp) { return mp.density(); })
      .method("plasticSlip", [](MaterialPoint& mp) { return mp.plasticSlip(); })
      .method("waveVelocity", [](MaterialPoint& mp) { return mp.waveVelocity(); })
      // .method("parentMaterial",
      //         [](MaterialPoint& mp) -> const finiteStrainMaterial& {
      //           // returns const reference to finiteStrainMaterial
      //           // You may want to wrap finiteStrainMaterial as well.
      //           return mp.parentMaterial();
      //         })
      .method("getDamage", [](MaterialPoint& mp) { return mp.getDamage(); })
      .method("isFullyDamaged", [](MaterialPoint& mp) { return mp.isFullyDamaged(); });

  return mat;
}

// Explicitly instantiate templates
#define INSTANTIATE_FINITE_STRAIN_MATERIAL(Material, MaterialMP, MaterialBase, MPBase)                            \
  template jlcxx::TypeWrapper<Material> registerFiniteStrainMaterial<Material, MaterialMP, MaterialBase, MPBase>( \
      jlcxx::Module&, const std::string&);

// Use the macro for explicit instantiations
INSTANTIATE_FINITE_STRAIN_MATERIAL(muesli::neohookeanMaterial, muesli::neohookeanMP, muesli::f_invariants,
                                   muesli::fisotropicMP)
INSTANTIATE_FINITE_STRAIN_MATERIAL(muesli::svkMaterial, muesli::svkMP, muesli::finiteStrainMaterial,
                                   muesli::finiteStrainMP)
INSTANTIATE_FINITE_STRAIN_MATERIAL(muesli::mooneyMaterial, muesli::mooneyMP, muesli::f_invariants, muesli::fisotropicMP)
INSTANTIATE_FINITE_STRAIN_MATERIAL(muesli::arrudaboyceMaterial, muesli::arrudaboyceMP, muesli::f_invariants,
                                   muesli::fisotropicMP)
INSTANTIATE_FINITE_STRAIN_MATERIAL(muesli::yeohMaterial, muesli::yeohMP, muesli::f_invariants, muesli::fisotropicMP)
