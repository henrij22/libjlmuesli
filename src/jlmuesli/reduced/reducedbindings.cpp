// SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de
// SPDX-License-Identifier: GPL-3.0-or-later

// Reduced (stress-constrained) material points: a bar, beam, shell or plane-stress model wrapped
// around a 3D material point. The caller prescribes the master strain components, the model solves
// for the others so that their stress vanishes (see MUESLI's reducedsmallstrain.cpp).
//
// A reduced point only holds a pointer to the 3D point it wraps. That point must outlive it;
// MuesliMaterials.jl takes care of this.

#include <sstream>

#include <jlmuesli/util/common.hh>
#include <jlmuesli/util/utils.hh>

#include <muesli/Finitestrain/reducedfinitestrain.h>
#include <muesli/muesli.h>
#include <muesli/Smallstrain/reducedsmallstrain.h>

#include <jlcxx/jlcxx.hpp>

namespace {

using muesli::rFiniteStrainMP;
using muesli::rSmallStrainMP;

template <typename Reduced, typename ThreeD>
void addReducedModel(jlcxx::Module& mod, const std::string& name) {
  mod.add_type<Reduced>(
         name, jlcxx::julia_base_type<
                   std::conditional_t<std::is_base_of_v<rSmallStrainMP, Reduced>, rSmallStrainMP, rFiniteStrainMP>>())
      .constructor([](ThreeD& mp) { return new Reduced(&mp); })
      .constructor([](ThreeD& mp, const std::string& method) { return new Reduced(&mp, method); });
}

} // namespace

void registerReducedModels(jlcxx::Module& mod) {
  using muesli::finiteStrainMP;
  using muesli::smallStrainMP;

  mod.add_type<rSmallStrainMP>("ReducedSmallStrainMP")
      .method("setRandom", [](rSmallStrainMP& mp) { mp.setRandom(); })
      .method("testImplementation",
              [](const rSmallStrainMP& mp) {
                std::ostringstream report;
                const bool ok = mp.testImplementation(report);
                if (!ok)
                  std::cout << report.str() << std::endl;
                return ok;
              })

      .method("deviatoricEnergy", [](const rSmallStrainMP& mp) { return mp.deviatoricEnergy(); })
      .method("energyDissipationInStep", [](const rSmallStrainMP& mp) { return mp.energyDissipationInStep(); })
      .method("effectiveStoredEnergy", [](const rSmallStrainMP& mp) { return mp.effectiveStoredEnergy(); })
      .method("storedEnergy", [](const rSmallStrainMP& mp) { return mp.storedEnergy(); })
      .method("thermodynamicPotentials", [](const rSmallStrainMP& mp) { return mp.thermodynamicPotentials(); })
      .method("volumetricEnergy", [](const rSmallStrainMP& mp) { return mp.volumetricEnergy(); })

      .method("pressure", [](const rSmallStrainMP& mp) { return mp.pressure(); })
      .method("stress!", [](const rSmallStrainMP& mp, istensor& sigma) { mp.stress(sigma); })
      .method("deviatoricStress!", [](const rSmallStrainMP& mp, istensor& s) { mp.deviatoricStress(s); })

      .method("tangentMatrix!",
              [](const rSmallStrainMP& mp, JuliaTensor out) {
                double C[6][6];
                mp.tangentMatrix(C);
                jlmuesli::matrix6ToColumnMajor(C, jlmuesli::checkedMatrix6Data(out));
              })

      .method("commitCurrentState", [](rSmallStrainMP& mp) { mp.commitCurrentState(); })
      .method("resetCurrentState", [](rSmallStrainMP& mp) { mp.resetCurrentState(); })
      // overwrites `strain` with the full 3D strain, including the solved components
      .method("updateCurrentState!",
              [](rSmallStrainMP& mp, double t, istensor& strain) { mp.updateCurrentState(t, strain); })
      .method("getCurrentStrain", [](const rSmallStrainMP& mp) -> istensor { return mp.getCurrentStrain(); })
      .method("getConvergedPlasticStrain",
              [](const rSmallStrainMP& mp) -> istensor { return mp.getConvergedPlasticStrain(); })
      .method("getCurrentPlasticStrain",
              [](const rSmallStrainMP& mp) -> istensor { return mp.getCurrentPlasticStrain(); })
      .method("getConvergedState", [](const rSmallStrainMP& mp) { return mp.getConvergedState(); })
      .method("getCurrentState", [](const rSmallStrainMP& mp) { return mp.getCurrentState(); })

      .method("density", [](const rSmallStrainMP& mp) { return mp.density(); })
      .method("plasticSlip", [](const rSmallStrainMP& mp) { return mp.plasticSlip(); })
      .method("waveVelocity", [](const rSmallStrainMP& mp) { return mp.waveVelocity(); });

  mod.add_type<rFiniteStrainMP>("ReducedFiniteStrainMP")
      .method("setRandom", [](rFiniteStrainMP& mp) { mp.setRandom(); })
      .method("testImplementation",
              [](const rFiniteStrainMP& mp, bool testDE, bool testDDE) {
                std::ostringstream report;
                const bool ok = mp.testImplementation(report, testDE, testDDE);
                if (!ok)
                  std::cout << report.str() << std::endl;
                return ok;
              })

      .method("energyDissipationInStep", [](const rFiniteStrainMP& mp) { return mp.energyDissipationInStep(); })
      .method("effectiveStoredEnergy", [](const rFiniteStrainMP& mp) { return mp.effectiveStoredEnergy(); })
      .method("storedEnergy", [](const rFiniteStrainMP& mp) { return mp.storedEnergy(); })

      .method("CauchyStress!", [](const rFiniteStrainMP& mp, istensor& sigma) { mp.CauchyStress(sigma); })
      .method("energyMomentumTensor!", [](const rFiniteStrainMP& mp, itensor& S) { mp.energyMomentumTensor(S); })
      .method("firstPiolaKirchhoffStress!",
              [](const rFiniteStrainMP& mp, itensor& P) { mp.firstPiolaKirchhoffStress(P); })
      .method("KirchhoffStress!", [](const rFiniteStrainMP& mp, istensor& tau) { mp.KirchhoffStress(tau); })
      .method("secondPiolaKirchhoffStress!",
              [](const rFiniteStrainMP& mp, istensor& S) { mp.secondPiolaKirchhoffStress(S); })

      .method("convectedTangentMatrix!",
              [](const rFiniteStrainMP& mp, JuliaTensor out) {
                double C[6][6];
                mp.convectedTangentMatrix(C);
                jlmuesli::matrix6ToColumnMajor(C, jlmuesli::checkedMatrix6Data(out));
              })
      .method("volumetricStiffness", [](const rFiniteStrainMP& mp) { return mp.volumetricStiffness(); })

      .method("commitCurrentState", [](rFiniteStrainMP& mp) { mp.commitCurrentState(); })
      .method("resetCurrentState", [](rFiniteStrainMP& mp) { mp.resetCurrentState(); })
      // overwrites `F` with the full 3D deformation gradient, including the solved components
      .method("updateCurrentState!", [](rFiniteStrainMP& mp, double t, itensor& F) { mp.updateCurrentState(t, F); })
      .method("deformationGradient", [](const rFiniteStrainMP& mp) -> itensor { return mp.deformationGradient(); })
      .method("getConvergedState", [](const rFiniteStrainMP& mp) { return mp.getConvergedState(); })
      .method("getCurrentState", [](const rFiniteStrainMP& mp) { return mp.getCurrentState(); })

      .method("density", [](const rFiniteStrainMP& mp) { return mp.density(); })
      .method("plasticSlip", [](const rFiniteStrainMP& mp) { return mp.plasticSlip(); })
      .method("waveVelocity", [](const rFiniteStrainMP& mp) { return mp.waveVelocity(); });

  addReducedModel<muesli::sbarMP, smallStrainMP>(mod, "SbarMP");
  addReducedModel<muesli::sbeamMP, smallStrainMP>(mod, "SbeamMP");
  addReducedModel<muesli::sshellMP, smallStrainMP>(mod, "SshellMP");
  addReducedModel<muesli::splaneMP, smallStrainMP>(mod, "SplaneMP");

  addReducedModel<muesli::fbarMP, finiteStrainMP>(mod, "FbarMP");
  addReducedModel<muesli::fbeamMP, finiteStrainMP>(mod, "FbeamMP");
  addReducedModel<muesli::fshellMP, finiteStrainMP>(mod, "FshellMP");
  addReducedModel<muesli::fplaneMP, finiteStrainMP>(mod, "FplaneMP");
}
