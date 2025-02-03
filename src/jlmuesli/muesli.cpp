// SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de
// SPDX-License-Identifier: GPL-3.0-or-later

#include <jlmuesli/finitestrain/registerfinitestrain.hh>
#include <jlmuesli/smallstrain/registersmallstrain.hh>
#include <jlmuesli/util/common.hh>
#include <jlmuesli/util/utils.hh>

#include <muesli/muesli.h>

#include <jlcxx/jlcxx.hpp>
#include <jlcxx/type_conversion.hpp>

JLCXX_MODULE define_julia_module(jlcxx::Module& mod) {
  using jlcxx::arg;
  using jlcxx::julia_base_type;

  // Register utils
  registerHelpers(mod);
  registerMaterialState(mod);
  registerPropertyName(mod);
  // registerTensors(mod);

  // Register base classes (important?)
  mod.add_type<muesli::material>("Material");
  mod.add_type<muesli::materialPoint>("MaterialPoint");

  registerSmallStrainMaterials(mod);
  registerFiniteStrainMaterials(mod);
}