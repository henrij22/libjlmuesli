// SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de
// SPDX-License-Identifier: GPL-3.0-or-later

#include <jlmuesli/smallstrain/registersmallstrain.hh>
#include <jlmuesli/util/common.hh>
#include <jlmuesli/util/utils.hh>

#include <muesli/muesli.h>

#include <jlcxx/jlcxx.hpp>

// Registered once on the family root; all of these are virtual in smallStrainMP, so they
// dispatch to the concrete material.
void registerSmallStrainBase(jlcxx::Module& mod) {
  using muesli::smallStrainMP;

  mod.add_type<muesli::smallStrainMaterial>("SmallStrainMaterial", jlcxx::julia_base_type<muesli::material>());

  mod.add_type<muesli::thPotentials>("ThPotentials")
      .method("chemical", [](const muesli::thPotentials& p) { return p.chemical; })
      .method("deviatoric", [](const muesli::thPotentials& p) { return p.deviatoric; })
      .method("dissipated", [](const muesli::thPotentials& p) { return p.dissipated; })
      .method("entropy", [](const muesli::thPotentials& p) { return p.entropy; })
      .method("freeEnergy", [](const muesli::thPotentials& p) { return p.freeEnergy; })
      .method("freeEntropy", [](const muesli::thPotentials& p) { return p.freeEntropy; })
      .method("internalEnergy", [](const muesli::thPotentials& p) { return p.internalEnergy; })
      .method("kineticPotential", [](const muesli::thPotentials& p) { return p.kineticPotential; })
      .method("minimizing", [](const muesli::thPotentials& p) { return p.minimizing; })
      .method("volumetric", [](const muesli::thPotentials& p) { return p.volumetric; });

  mod.add_type<smallStrainMP>("SmallStrainMP", jlcxx::julia_base_type<muesli::materialPoint>())
      // 3D response
      .method("contractWithDeviatoricTangent!", [](const smallStrainMP& mp, const ivector& v1, const ivector& v2,
                                                   itensor& T) { mp.contractWithDeviatoricTangent(v1, v2, T); })
      .method("contractWithTangent!", [](const smallStrainMP& mp, const ivector& v1, const ivector& v2,
                                         itensor& T) { mp.contractWithTangent(v1, v2, T); })
      .method("contractWithMixedTangent!",
              [](const smallStrainMP& mp, istensor& CM) { mp.contractWithMixedTangent(CM); })
      .method("dissipationTangent!", [](const smallStrainMP& mp, itensor4& D) { mp.dissipationTangent(D); })
      .method("plasticSlip", [](const smallStrainMP& mp) { return mp.plasticSlip(); })
      .method("shearStiffness", [](const smallStrainMP& mp) { return mp.shearStiffness(); })
      .method("tangentTensor!", [](const smallStrainMP& mp, itensor4& C) { mp.tangentTensor(C); })
      .method("tangentMatrix!",
              [](const smallStrainMP& mp, JuliaTensor out) {
                double C[6][6];
                mp.tangentMatrix(C);
                jlmuesli::matrix6ToColumnMajor(C, jlmuesli::checkedMatrix6Data(out));
              })
      .method("volumetricStiffness", [](const smallStrainMP& mp) { return mp.volumetricStiffness(); })

      // Energy
      .method("deviatoricEnergy", [](const smallStrainMP& mp) { return mp.deviatoricEnergy(); })
      .method("energyDissipationInStep", [](const smallStrainMP& mp) { return mp.energyDissipationInStep(); })
      .method("effectiveStoredEnergy", [](const smallStrainMP& mp) { return mp.effectiveStoredEnergy(); })
      .method("kineticPotential", [](const smallStrainMP& mp) { return mp.kineticPotential(); })
      .method("storedEnergy", [](const smallStrainMP& mp) { return mp.storedEnergy(); })
      .method("thermodynamicPotentials", [](const smallStrainMP& mp) { return mp.thermodynamicPotentials(); })
      .method("volumetricEnergy", [](const smallStrainMP& mp) { return mp.volumetricEnergy(); })

      // Stresses
      .method("pressure", [](const smallStrainMP& mp) { return mp.pressure(); })
      .method("stress!", [](const smallStrainMP& mp, istensor& sigma) { mp.stress(sigma); })
      .method("deviatoricStress!", [](const smallStrainMP& mp, istensor& sigma) { mp.deviatoricStress(sigma); })

      // Strains / states
      .method("getConvergedPlasticStrain",
              [](const smallStrainMP& mp) -> istensor { return mp.getConvergedPlasticStrain(); })
      .method("getCurrentPlasticStrain",
              [](const smallStrainMP& mp) -> istensor { return mp.getCurrentPlasticStrain(); })

      // Self test
      .method("testImplementation",
              [](const smallStrainMP& mp, bool testDE, bool testDDE) { return runSelfTest(mp, testDE, testDDE); })
      // Bookkeeping (commit/reset/get*State are on MaterialPoint)
      .method("updateCurrentState",
              [](smallStrainMP& mp, double t, const istensor& strain) { mp.updateCurrentState(t, strain); });
}
