// SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de
// SPDX-License-Identifier: GPL-3.0-or-later

#include "common.hh"
#include "utils.hh"

#include <jlcxx/jlcxx.hpp>

inline void registerIVector(jlcxx::Module& mod) {
  mod.add_type<ivector>("IVector")
      // Constructors
      .constructor([](const double* data) { return new ivector(data); })
      .constructor([](double a, double b, double c) { return new ivector(a, b, c); })
      .constructor([](double a, double b, double c) { return new ivector(a, b, c); })
      .constructor([](const JuliaVector vector) { return new ivector(vector.data()); })
      // getter (and setter)
      .method([](ivector& ivec, size_t i) { return ivec(i); });
}

inline void registerITensor(jlcxx::Module& mod) {
  mod.add_type<itensor>("ITensor")
      // Constructors
      .constructor([](const double data[3][3]) { return new itensor(data); })
      .constructor([](const ivector& a, const ivector& b, const ivector& c) { return new itensor(a, b, c); })
      .constructor([](double a00, double a01, double a02, double a10, const double a11, double a12, double a20,
                      double a21, double a22) { return new itensor(a00, a01, a02, a10, a11, a12, a20, a21, a22); })
      // .constructor([](const JuliaVector vector) { return new itensor(vector.data()); })
      // getter (and setter)
      .method([](itensor& iten, size_t i, size_t j) { return iten(i, j); });
}

inline void registerIsTensor(jlcxx::Module& mod) {
  mod.add_type<istensor>("IsTensor", jlcxx::julia_base_type<itensor>())
      // Constructors
      .constructor([](double t00, double t11, double t22, double t12, double t20, double t01) {
        return new istensor(t00, t11, t22, t12, t20, t01);
      })
      // .constructor([](const JuliaVector vector) { return new itensor(vector.data()); })
      // getter (and setter)
      .method([](itensor& iten, size_t i, size_t j) { return iten(i, j); });
}

inline void registerTensors(jlcxx::Module& mod) {
  registerIVector(mod);
  registerITensor(mod);
}
