// SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de
// SPDX-License-Identifier: GPL-3.0-or-later

#include "utils.hh"

void registerPropertyName(jlcxx::Module& mod) {
  // Shorthand
  using PN = muesli::propertyName;

  // Add the C++ enum to the Julia module, specifying it’s an enum type
  mod.add_bits<PN>("PropertyName", jlcxx::julia_type("CppEnum"));

  // clang-format off
  mod.set_const("PR_ALPHA",            PN::PR_ALPHA);
  mod.set_const("PR_BULK",             PN::PR_BULK);
  mod.set_const("PR_CHAR_LENGTH",      PN::PR_CHAR_LENGTH);
  mod.set_const("PR_CONDUCTIVITY",     PN::PR_CONDUCTIVITY);
  mod.set_const("PR_CP",               PN::PR_CP);
  mod.set_const("PR_CS",               PN::PR_CS);
  mod.set_const("PR_CREFL",            PN::PR_CREFL);
  mod.set_const("PR_CREFT",            PN::PR_CREFT);
  mod.set_const("PR_DIFFUSIVITY",      PN::PR_DIFFUSIVITY);
  mod.set_const("PR_ETA",              PN::PR_ETA);
  mod.set_const("PR_G_CRITICAL",       PN::PR_G_CRITICAL);
  mod.set_const("PR_GF",               PN::PR_GF);
  mod.set_const("PR_HEAT_SUPPLY",      PN::PR_HEAT_SUPPLY);
  mod.set_const("PR_HISO",             PN::PR_HISO);
  mod.set_const("PR_HKINE",            PN::PR_HKINE);
  mod.set_const("PR_ISOHARD",          PN::PR_ISOHARD);
  mod.set_const("PR_KINHARD",          PN::PR_KINHARD);
  mod.set_const("PR_LAMBDA",           PN::PR_LAMBDA);
  mod.set_const("PR_MASS_EXP",         PN::PR_MASS_EXP);
  mod.set_const("PR_MU",               PN::PR_MU);
  mod.set_const("PR_MUREFL",           PN::PR_MUREFL);
  mod.set_const("PR_MUREFT",           PN::PR_MUREFT);
  mod.set_const("PR_N",                PN::PR_N);
  mod.set_const("PR_NL",               PN::PR_NL);
  mod.set_const("PR_NT",               PN::PR_NT);
  mod.set_const("PR_NU",               PN::PR_NU);
  mod.set_const("PR_PLSTRESS_C",       PN::PR_PLSTRESS_C);
  mod.set_const("PR_POISSON",          PN::PR_POISSON);
  mod.set_const("PR_Q1_GURSON",        PN::PR_Q1_GURSON);
  mod.set_const("PR_Q2_GURSON",        PN::PR_Q2_GURSON);
  mod.set_const("PR_Rgas",             PN::PR_Rgas);
  mod.set_const("PR_R_Lemaitre",       PN::PR_R_Lemaitre);
  mod.set_const("PR_S_Lemaitre",       PN::PR_S_Lemaitre);
  mod.set_const("PR_Rinf_Hardening",   PN::PR_Rinf_Hardening);
  mod.set_const("PR_Rb_Hardening",     PN::PR_Rb_Hardening);
  mod.set_const("PR_akin_Hardening",   PN::PR_akin_Hardening);
  mod.set_const("PR_bkin_Hardening",   PN::PR_bkin_Hardening);
  mod.set_const("PR_SMAX",             PN::PR_SMAX);
  mod.set_const("PR_SRREF",            PN::PR_SRREF);
  mod.set_const("PR_TAYLOR_QUINNEY",   PN::PR_TAYLOR_QUINNEY);
  mod.set_const("PR_THERMAL_CAP",      PN::PR_THERMAL_CAP);
  mod.set_const("PR_THERMAL_EXP",      PN::PR_THERMAL_EXP);
  mod.set_const("PR_TREF",             PN::PR_TREF);
  mod.set_const("PR_VEXPONENT",        PN::PR_VEXPONENT);
  mod.set_const("PR_YIELD",            PN::PR_YIELD);
  mod.set_const("PR_YOUNG",            PN::PR_YOUNG);
  mod.set_const("PR_MU0L",             PN::PR_MU0L);
  mod.set_const("PR_MU0T",             PN::PR_MU0T);
  mod.set_const("PR_MUREF",            PN::PR_MUREF);
  // clang-format on
}
