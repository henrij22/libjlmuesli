// SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de
// SPDX-License-Identifier: GPL-3.0-or-later

#include <jlmuesli/finitestrain/registerfinitestrain.hh>
#include <jlmuesli/util/common.hh>
#include <jlmuesli/util/utils.hh>

#include <muesli/muesli.h>

#include <jlcxx/jlcxx.hpp>

// Registered once on the family root; all of these are virtual in finiteStrainMP (or
// non-virtual and not hidden by any bound material), so they dispatch to the concrete material.
void registerFiniteStrainBase(jlcxx::Module& mod) {
  using jlcxx::julia_base_type;
  using muesli::finiteStrainMP;

  mod.add_type<muesli::finiteStrainMaterial>("FiniteStrainMaterial", julia_base_type<muesli::material>());
  mod.add_type<muesli::f_invariants>("F_invariants", julia_base_type<muesli::finiteStrainMaterial>());

  mod.add_type<finiteStrainMP>("FiniteStrainMP", julia_base_type<muesli::materialPoint>())
      .method("setRandom", [](finiteStrainMP& mp) { mp.setRandom(); })
      .method("setTemperature", [](finiteStrainMP& mp, double tt) { mp.setTemperature(tt); })

      // --- Energies ---
      .method("energyDissipationInStep", [](const finiteStrainMP& mp) { return mp.energyDissipationInStep(); })
      .method("dissipatedEnergyDF", [](const finiteStrainMP& mp) { return mp.dissipatedEnergyDF(); })
      .method("dissipatedEnergyDTheta", [](const finiteStrainMP& mp) { return mp.dissipatedEnergyDTheta(); })
      .method("kineticPotential", [](const finiteStrainMP& mp) { return mp.kineticPotential(); })
      .method("effectiveStoredEnergy", [](const finiteStrainMP& mp) { return mp.effectiveStoredEnergy(); })
      .method("storedEnergy", [](const finiteStrainMP& mp) { return mp.storedEnergy(); })

      // --- Stresses ---
      .method("CauchyStress!", [](const finiteStrainMP& mp, istensor& sigma) { mp.CauchyStress(sigma); })
      .method("energyMomentumTensor!", [](const finiteStrainMP& mp, itensor& S) { mp.energyMomentumTensor(S); })
      .method("firstPiolaKirchhoffStress!",
              [](const finiteStrainMP& mp, itensor& P) { mp.firstPiolaKirchhoffStress(P); })
      .method("firstPiolaKirchhoffStressNumerical!",
              [](const finiteStrainMP& mp, itensor& P) { mp.firstPiolaKirchhoffStressNumerical(P); })
      .method("KirchhoffStress!", [](const finiteStrainMP& mp, istensor& tau) { mp.KirchhoffStress(tau); })
      .method("secondPiolaKirchhoffStress!",
              [](const finiteStrainMP& mp, istensor& S) { mp.secondPiolaKirchhoffStress(S); })
      .method("secondPiolaKirchhoffStressNumerical!",
              [](const finiteStrainMP& mp, istensor& S) { mp.secondPiolaKirchhoffStressNumerical(S); })

      // --- Elasticity tangents ---
      .method("convectedTangent!", [](const finiteStrainMP& mp, itensor4& c) { mp.convectedTangent(c); })
      .method("materialTangent!", [](const finiteStrainMP& mp, itensor4& c) { mp.materialTangent(c); })
      .method("spatialTangent!", [](const finiteStrainMP& mp, itensor4& c) { mp.spatialTangent(c); })

      // --- Tangent contractions ---
      // TODO: contractWithAllTangents (fills Tdev, Tmixed and a scalar Tvol by reference)
      .method("contractWithConvectedTangent!", [](const finiteStrainMP& mp, const ivector& v1, const ivector& v2,
                                                  itensor& T) { mp.contractWithConvectedTangent(v1, v2, T); })
      .method("contractWithSpatialTangent!", [](const finiteStrainMP& mp, const ivector& v1, const ivector& v2,
                                                itensor& T) { mp.contractWithSpatialTangent(v1, v2, T); })
      .method("contractWithDeviatoricTangent!", [](const finiteStrainMP& mp, const ivector& v1, const ivector& v2,
                                                   itensor& T) { mp.contractWithDeviatoricTangent(v1, v2, T); })
      .method("contractWithMixedTangent!",
              [](const finiteStrainMP& mp, istensor& CM) { mp.contractWithMixedTangent(CM); })
      .method("convectedTangentTimesSymmetricTensor!",
              [](const finiteStrainMP& mp, const istensor& M, istensor& CM) {
                mp.convectedTangentTimesSymmetricTensor(M, CM);
              })
      .method("volumetricStiffness", [](const finiteStrainMP& mp) { return mp.volumetricStiffness(); })

      // --- Bookkeeping (commit/reset/get*State are on MaterialPoint) ---
      .method("updateCurrentState",
              [](finiteStrainMP& mp, double theTime, const itensor& F) { mp.updateCurrentState(theTime, F); })

      // --- Extract state (returned as copies) ---
      .method("convergedDeformationGradient",
              [](const finiteStrainMP& mp) -> itensor { return mp.convergedDeformationGradient(); })
      .method("deformationGradient", [](const finiteStrainMP& mp) -> itensor { return mp.deformationGradient(); })

      // --- Miscellaneous ---
      .method("plasticSlip", [](const finiteStrainMP& mp) { return mp.plasticSlip(); })
      .method("waveVelocity", [](const finiteStrainMP& mp) { return mp.waveVelocity(); })
      // TODO: parentMaterial (returns a const finiteStrainMaterial&)
      .method("getDamage", [](const finiteStrainMP& mp) { return mp.getDamage(); })
      .method("isFullyDamaged", [](const finiteStrainMP& mp) { return mp.isFullyDamaged(); });
}
