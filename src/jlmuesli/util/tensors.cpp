// SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de
// SPDX-License-Identifier: GPL-3.0-or-later

#include "common.hh"
#include "utils.hh"

#include <jlcxx/jlcxx.hpp>

namespace jlcxx {
template <>
struct SuperType<istensor>
{
  typedef itensor type;
};
} // namespace jlcxx

void registerIVector(jlcxx::Module& mod) {
  using jlcxx::cxxint_t;

  mod.add_type<ivector>("Ivector")
      // Constructors
      .constructor([](const double* data) { return new ivector(data); })
      .constructor([](double a, double b, double c) { return new ivector(a, b, c); })
      .constructor([](JuliaVector vector) { return new ivector(vector.data()); })
      // getter and setter (account for 0 based indexing in c++)
      .method("cxxgetindex", [](const ivector& v, cxxint_t i) -> const double& { return v(i - 1); })
      .method("cxxgetindex", [](ivector& v, cxxint_t i) -> double& { return v(i - 1); })
      .method("cxxsetindex!", [](ivector& v, const double& val, cxxint_t i) { v(i - 1) = val; });
  ;
}

void registerITensor(jlcxx::Module& mod) {
  using jlcxx::cxxint_t;
  mod.add_type<itensor>("Itensor")
      // Constructors
      // .constructor([](const double data[3][3]) { return new itensor(data); })
      .constructor([](const ivector& a, const ivector& b, const ivector& c) { return new itensor(a, b, c); })
      .constructor(
          [](JuliaVector a, JuliaVector b, JuliaVector c) { return new itensor(a.data(), b.data(), c.data()); })
      .constructor([](double a00, double a01, double a02, double a10, const double a11, double a12, double a20,
                      double a21, double a22) { return new itensor(a00, a01, a02, a10, a11, a12, a20, a21, a22); })
      .constructor([](JuliaTensor array) { return new itensor(toITensor(array)); })
      // getter and setter (account for 0 based indexing in c++)
      .method("cxxgetindex", [](const itensor& v, cxxint_t i, cxxint_t j) -> const double& { return v(i - 1, j - 1); })
      .method("cxxgetindex", [](itensor& v, cxxint_t i, cxxint_t j) -> double& { return v(i - 1, j - 1); })
      .method("cxxsetindex!", [](itensor& v, const double& val, cxxint_t i, cxxint_t j) { v(i - 1, j - 1) = val; });
}

void registerIsTensor(jlcxx::Module& mod) {
  mod.add_type<istensor>("Istensor", jlcxx::julia_base_type<itensor>())
      // Constructors
      .constructor([](double t00, double t11, double t22, double t12, double t20, double t01) {
        return new istensor(t00, t11, t22, t12, t20, t01);
      })
      .constructor([](JuliaTensor array) { return new istensor(toIstensor(array)); });
}

void registerITensor4(jlcxx::Module& mod) {
  using jlcxx::cxxint_t;

  mod.add_type<itensor4>("Itensor4")
      // Constructors
      // .constructor([](JuliaTensor4 array) { return new arrayRefToItensor4(array); })
      .constructor([](JuliaTensor4 array) { return new itensor4(arrayRefToItensor4(array)); })
      // getter and setter (account for 0 based indexing in c++)
      .method("cxxgetindex",
              [](const itensor4& v, cxxint_t i, cxxint_t j, cxxint_t k, cxxint_t l) -> const double& {
                return v(i - 1, j - 1, k - 1, l - 1);
              })
      .method("cxxgetindex",
              [](itensor4& v, cxxint_t i, cxxint_t j, cxxint_t k, cxxint_t l) -> double& {
                return v(i - 1, j - 1, k - 1, l - 1);
              })
      .method("cxxsetindex!", [](itensor4& v, const double& val, cxxint_t i, cxxint_t j, cxxint_t k, cxxint_t l) {
        v(i - 1, j - 1, k - 1, l - 1) = val;
      });
}

void registerTensors(jlcxx::Module& mod) {
  registerIVector(mod);
  registerITensor(mod);
  registerIsTensor(mod);
  registerITensor4(mod);
}
