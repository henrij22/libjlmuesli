
#pragma once
#include <jlmuesli/util/common.hh>
#include <jlmuesli/util/helpers.hh>

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
      .method("dissipatedEnergyDF",
              [](MaterialPoint& mp) {
                itensor e = mp.dissipatedEnergyDF();
                return itensorToArrayRef(e);
              })
      .method("dissipatedEnergyDTheta", [](MaterialPoint& mp) { return mp.dissipatedEnergyDTheta(); })
      .method("kineticPotential", [](MaterialPoint& mp) { return mp.kineticPotential(); })
      .method("effectiveStoredEnergy", [](MaterialPoint& mp) { return mp.effectiveStoredEnergy(); })
      .method("storedEnergy", [](MaterialPoint& mp) { return mp.storedEnergy(); })

      // --- Stresses ---
      .method("CauchyStress",
              [](MaterialPoint& mp) {
                istensor sigma;
                mp.CauchyStress(sigma);
                return itensorToArrayRef(sigma);
              })
      .method("energyMomentumTensor",
              [](MaterialPoint& mp) {
                itensor S;
                mp.energyMomentumTensor(S);
                return itensorToArrayRef(S);
              })
      .method("firstPiolaKirchhoffStress",
              [](MaterialPoint& mp) {
                itensor P;
                mp.firstPiolaKirchhoffStress(P);
                return itensorToArrayRef(P);
              })
      .method("firstPiolaKirchhoffStressNumerical",
              [](MaterialPoint& mp) {
                itensor P;
                mp.firstPiolaKirchhoffStressNumerical(P);
                return itensorToArrayRef(P);
              })
      .method("KirchhoffStress",
              [](MaterialPoint& mp) {
                istensor tau;
                mp.KirchhoffStress(tau);
                return itensorToArrayRef(tau);
              })
      .method("secondPiolaKirchhoffStress",
              [](MaterialPoint& mp) {
                istensor S;
                mp.secondPiolaKirchhoffStress(S);
                return itensorToArrayRef(S);
              })
      .method("secondPiolaKirchhoffStressNumerical",
              [](MaterialPoint& mp) {
                istensor S;
                mp.secondPiolaKirchhoffStressNumerical(S);
                return itensorToArrayRef(S);
              })

      // --- Elasticity tangents ---
      .method("convectedTangent",
              [](MaterialPoint& mp) {
                itensor4 c;
                mp.convectedTangent(c);
                return itensor4ToArrayRef(c);
              })
      .method("materialTangent",
              [](MaterialPoint& mp) {
                itensor4 c;
                mp.materialTangent(c);
                return itensor4ToArrayRef(c);
              })
      .method("spatialTangent",
              [](MaterialPoint& mp) {
                itensor4 c;
                mp.spatialTangent(c);
                return itensor4ToArrayRef(c);
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
      .method("contractWithConvectedTangent",
              [](MaterialPoint& mp, jlcxx::ArrayRef<double, 1> v1, jlcxx::ArrayRef<double, 1> v2) {
                itensor T;
                mp.contractWithConvectedTangent(toIVector(v1), toIVector(v2), T);
                return itensorToArrayRef(T);
              })
      .method("contractWithSpatialTangent",
              [](MaterialPoint& mp, jlcxx::ArrayRef<double, 1> v1, jlcxx::ArrayRef<double, 1> v2) {
                itensor T;
                mp.contractWithSpatialTangent(toIVector(v1), toIVector(v2), T);
                return itensorToArrayRef(T);
              })
      .method("contractWithDeviatoricTangent",
              [](MaterialPoint& mp, jlcxx::ArrayRef<double, 1> v1, jlcxx::ArrayRef<double, 1> v2) {
                itensor T;
                mp.contractWithDeviatoricTangent(toIVector(v1), toIVector(v2), T);
                return itensorToArrayRef(T);
              })
      .method("contractWithMixedTangent",
              [](MaterialPoint& mp) {
                istensor CM;
                mp.contractWithMixedTangent(CM);
                return itensorToArrayRef(CM);
              })
      .method("convectedTangentTimesSymmetricTensor",
              [](MaterialPoint& mp, jlcxx::ArrayRef<double, 2> M_in) {
                istensor M = toIstensor(M_in);
                istensor CM;
                mp.convectedTangentTimesSymmetricTensor(M, CM);
                return itensorToArrayRef(CM);
              })
      .method("volumetricStiffness", [](MaterialPoint& mp) { return mp.volumetricStiffness(); })

      // --- Bookkeeping ---
      .method("commitCurrentState", [](MaterialPoint& mp) { mp.commitCurrentState(); })
      .method("resetCurrentState", [](MaterialPoint& mp) { mp.resetCurrentState(); })
      .method("updateCurrentState",
              [](MaterialPoint& mp, double theTime, jlcxx::ArrayRef<double, 2> F_in) {
                itensor F = toITensor(F_in);
                mp.updateCurrentState(theTime, F);
              })

      // --- Extract state ---
      // For convergedDeformationGradient, we have both const and non-const versions in C++.
      // We'll just expose one that returns the current content (read-only in Julia).
      .method("convergedDeformationGradient",
              [](MaterialPoint& mp) {
                // returns const itensor& or itensor&
                // We'll call the const version explicitly if needed.
                return itensorToArrayRef(mp.convergedDeformationGradient());
              })
      .method("deformationGradient", [](MaterialPoint& mp) { return itensorToArrayRef(mp.deformationGradient()); })
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