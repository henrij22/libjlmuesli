// SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de
// SPDX-License-Identifier: GPL-3.0-or-later

#include <jlmuesli/finitestrain/registerfinitestrain.hh>
#include <jlmuesli/smallstrain/registersmallstrain.hh>
#include <jlmuesli/util/common.hh>
#include <jlmuesli/util/utils.hh>

#include <muesli/muesli.h>
#include <muesli/Utils/utils.h>

#include <jlcxx/jlcxx.hpp>
#include <jlcxx/type_conversion.hpp>

// reducedbindings.cpp
void registerReducedModels(jlcxx::Module& mod);

JLCXX_MODULE define_julia_module(jlcxx::Module& mod) {
  using jlcxx::arg;
  using jlcxx::julia_base_type;

  // Register utils
  registerTensors(mod);
  registerHelpers(mod);
  registerInterfaceState(mod);
  registerPropertyName(mod);

  // The root classes have to exist before any family that derives from them, and they carry
  // the methods every material and material point shares.
  registerMaterialBase(mod);

  // MUESLI's self test and setRandom draw random numbers; a fixed seed makes them reproducible
  mod.method("setFixedRandomSeed", [](bool fixed) { muesli::useFixedRandomSeed = fixed; });

  registerSmallStrainMaterials(mod);
  registerFiniteStrainMaterials(mod);
  registerReducedModels(mod);
}