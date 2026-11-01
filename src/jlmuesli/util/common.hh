// SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "convert.hh"
#include "supertypes.hh"

#include <array>
#include <cstddef>
#include <stdexcept>
#include <string>

#include <muesli/Math/mtensor.h>
#include <muesli/muesli.h>

#include <jlcxx/array.hpp>

#include <julia.h>

inline auto toMPM_Enu(double Emod, double nu) {
  auto mpm = muesli::materialProperties{};
  mpm.insert({"young", Emod});
  mpm.insert({"poisson", nu});
  return mpm;
}

using JuliaTensor  = jlcxx::ArrayRef<double, 2>;
using JuliaVector  = jlcxx::ArrayRef<double, 1>;
using JuliaTensor4 = jlcxx::ArrayRef<double, 4>;

namespace jlmuesli {

namespace detail {

  template <std::size_t N>
  std::string shapeString(const std::array<std::size_t, N>& dims) {
    std::string s;
    for (std::size_t d = 0; d < N; ++d)
      s += (d ? "x" : "") + std::to_string(dims[d]);
    return s;
  }

  /// A flat length check would accept a 1x9 or 9x1 matrix as a 3x3 tensor.
  template <std::size_t N>
  void requireShape(const jlcxx::ArrayRef<double, N>& array, const std::array<std::size_t, N>& expected) {
    bool ok = array.size() == [&] {
      std::size_t n = 1;
      for (auto e : expected)
        n *= e;
      return n;
    }();

#ifdef jl_array_dim
    // falls back to the length check above if a Julia version drops this macro
    if (ok) {
      for (std::size_t d = 0; d < N; ++d)
        ok = ok && jl_array_dim(array.wrapped(), d) == expected[d];
    }
#endif

    if (!ok)
      throw std::invalid_argument("Input has to be an array of size " + shapeString(expected) + ", got " +
                                  std::to_string(array.size()) + " element(s).");
  }

} // namespace detail

inline const double* checkedData(const JuliaVector& v) {
  detail::requireShape<1>(v, {kVectorSize});
  return v.data();
}
inline const double* checkedData(const JuliaTensor& t) {
  detail::requireShape<2>(t, {3, 3});
  return t.data();
}
inline const double* checkedData(const JuliaTensor4& T) {
  detail::requireShape<4>(T, {3, 3, 3, 3});
  return T.data();
}
inline double* checkedMatrix6Data(JuliaTensor& m) {
  detail::requireShape<2>(m, {6, 6});
  return m.data();
}
inline double* checkedData(JuliaVector& v) {
  detail::requireShape<1>(v, {kVectorSize});
  return v.data();
}
inline double* checkedData(JuliaTensor& t) {
  detail::requireShape<2>(t, {3, 3});
  return t.data();
}
inline double* checkedData(JuliaTensor4& T) {
  detail::requireShape<4>(T, {3, 3, 3, 3});
  return T.data();
}

} // namespace jlmuesli

// flat parameter vectors of the anisotropic materials; their length depends on the material
inline const double* assertSizeAndExtractData(const JuliaVector& c, std::size_t expectedSize) {
  if (c.size() != expectedSize)
    throw std::invalid_argument("Input has to be a vector of length " + std::to_string(expectedSize) + ", got " +
                                std::to_string(c.size()) + ".");
  return c.data();
}

inline ivector toIVector(const JuliaVector& v) { return jlmuesli::ivectorFromColumnMajor(jlmuesli::checkedData(v)); }

inline itensor toITensor(const JuliaTensor& a) { return jlmuesli::itensorFromColumnMajor(jlmuesli::checkedData(a)); }

inline istensor toIstensor(const JuliaTensor& a) { return jlmuesli::istensorFromColumnMajor(jlmuesli::checkedData(a)); }

inline itensor4 toItensor4(const JuliaTensor4& a) {
  return jlmuesli::itensor4FromColumnMajor(jlmuesli::checkedData(a));
}
