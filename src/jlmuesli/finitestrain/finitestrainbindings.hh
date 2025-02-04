// SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <jlmuesli/util/common.hh>
#include <jlmuesli/util/utils.hh>

#include <muesli/muesli.h>
#include <muesli/Utils/utils.h>

#include <jlcxx/jlcxx.hpp>

template <typename Material, typename MaterialPoint, typename MaterialBase = muesli::finiteStrainMaterial,
          typename MaterialPointBase = muesli::finiteStrainMP>
auto registerFiniteStrainMaterial(jlcxx::Module& mod, const std::string& name) {
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
      .method("CauchyStress!",
              [](MaterialPoint& mp, JuliaTensor T) {
                istensor sigma;
                mp.CauchyStress(sigma);
                itensorToArrayRef(sigma, T);
              })
      .method("energyMomentumTensor!",
              [](MaterialPoint& mp, JuliaTensor T) {
                itensor S;
                mp.energyMomentumTensor(S);
                itensorToArrayRef(S, T);
              })
      .method("firstPiolaKirchhoffStress!",
              [](MaterialPoint& mp, JuliaTensor T) {
                itensor P;
                mp.firstPiolaKirchhoffStress(P);
                itensorToArrayRef(P, T);
              })
      .method("firstPiolaKirchhoffStressNumerical!",
              [](MaterialPoint& mp, JuliaTensor T) {
                itensor P;
                mp.firstPiolaKirchhoffStressNumerical(P);
                itensorToArrayRef(P, T);
              })
      .method("KirchhoffStress!",
              [](MaterialPoint& mp, JuliaTensor T) {
                istensor tau;
                mp.KirchhoffStress(tau);
                itensorToArrayRef(tau, T);
              })
      .method("secondPiolaKirchhoffStress!",
              [](MaterialPoint& mp, JuliaTensor T) {
                istensor S;
                mp.secondPiolaKirchhoffStress(S);
                return itensorToArrayRef(S, T);
              })
      .method("secondPiolaKirchhoffStressNumerical!",
              [](MaterialPoint& mp, JuliaTensor T) {
                istensor S;
                mp.secondPiolaKirchhoffStressNumerical(S);
                return itensorToArrayRef(S, T);
              })

      // --- Elasticity tangents ---
      .method("convectedTangent!",
              [](MaterialPoint& mp, JuliaTensor4 T) {
                itensor4 c;
                mp.convectedTangent(c);
                return itensor4ToArrayRef(c, T);
              })
      .method("materialTangent!",
              [](MaterialPoint& mp, JuliaTensor4 T) {
                itensor4 c;
                mp.materialTangent(c);
                return itensor4ToArrayRef(c, T);
              })
      .method("spatialTangent!",
              [](MaterialPoint& mp, JuliaTensor4 T) {
                itensor4 c;
                mp.spatialTangent(c);
                itensor4ToArrayRef(c, T);
              })

      // --- Tangent contractions ---
      // .method("contractWithAllTangents",
      //         [](MaterialPoint& mp, jlcxx::ArrayRef<double, 1> v1, jlcxx::ArrayRef<double, 1> v2) {
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
      .method("contractWithConvectedTangent!",
              [](MaterialPoint& mp, jlcxx::ArrayRef<double, 1> v1, jlcxx::ArrayRef<double, 1> v2, JuliaTensor TT) {
                itensor T;
                mp.contractWithConvectedTangent(toIVector(v1), toIVector(v2), T);
                itensorToArrayRef(T, TT);
              })
      .method("contractWithSpatialTangent!",
              [](MaterialPoint& mp, jlcxx::ArrayRef<double, 1> v1, jlcxx::ArrayRef<double, 1> v2, JuliaTensor TT) {
                itensor T;
                mp.contractWithSpatialTangent(toIVector(v1), toIVector(v2), T);
                itensorToArrayRef(T, TT);
              })
      .method("contractWithDeviatoricTangent!",
              [](MaterialPoint& mp, jlcxx::ArrayRef<double, 1> v1, jlcxx::ArrayRef<double, 1> v2, JuliaTensor TT) {
                itensor T;
                mp.contractWithDeviatoricTangent(toIVector(v1), toIVector(v2), T);
                itensorToArrayRef(T, TT);
              })
      .method("contractWithMixedTangent!",
              [](MaterialPoint& mp, JuliaTensor T) {
                istensor CM;
                mp.contractWithMixedTangent(CM);
                itensorToArrayRef(CM, T);
              })
      .method("convectedTangentTimesSymmetricTensor!",
              [](MaterialPoint& mp, JuliaTensor M_in, JuliaTensor T) {
                istensor M = toIstensor(M_in);
                istensor CM;
                mp.convectedTangentTimesSymmetricTensor(M, CM);
                itensorToArrayRef(CM, T);
              })
      .method("volumetricStiffness", [](MaterialPoint& mp) { return mp.volumetricStiffness(); })

      // --- Bookkeeping ---
      .method("commitCurrentState", [](MaterialPoint& mp) { mp.commitCurrentState(); })
      .method("resetCurrentState", [](MaterialPoint& mp) { mp.resetCurrentState(); })
      .method("updateCurrentState",
              [](MaterialPoint& mp, double theTime, JuliaTensor F_in) {
                itensor F = toITensor(F_in);
                mp.updateCurrentState(theTime, F);
              })

      // --- Extract state ---
      // For convergedDeformationGradient, we have both const and non-const versions in C++.
      // We'll just expose one that returns the current content (read-only in Julia).
      .method("convergedDeformationGradient!",
              [](MaterialPoint& mp, JuliaTensor T) {
                itensor F = mp.convergedDeformationGradient();
                return itensorToArrayRef(F, T);
              })
      .method("deformationGradient",
              [](MaterialPoint& mp, JuliaTensor T) {
                itensor F = mp.deformationGradient();
                return itensorToArrayRef(F, T);
              })
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