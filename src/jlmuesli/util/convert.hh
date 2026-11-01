// SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Julia arrays are column-major (first index fastest), MUESLI tensors are row-major C arrays.
// Element (i, j, ...) of a Julia array maps to the muesli component t(i, j, ...) with the same
// 0-based indices; nothing is transposed:
//
//   rank 2:  data[i + 3 j]
//   rank 4:  data[i + 3 j + 9 k + 27 l]
//
// Pure pointer code with no jlcxx dependency, so it is unit tested in test/test_convert.cpp.

#include <cstddef>

#include <muesli/Math/mtensor.h>

namespace jlmuesli {

inline constexpr std::size_t kVectorSize  = 3;
inline constexpr std::size_t kTensorSize  = 9;
inline constexpr std::size_t kTensor4Size = 81;

inline void toColumnMajor(const ivector& v, double* out) {
  for (std::size_t i = 0; i < 3; ++i)
    out[i] = v(i);
}

inline void toColumnMajor(const itensor& t, double* out) {
  for (std::size_t j = 0; j < 3; ++j)
    for (std::size_t i = 0; i < 3; ++i)
      out[i + 3 * j] = t(i, j);
}

inline void toColumnMajor(const itensor4& t, double* out) {
  for (std::size_t l = 0; l < 3; ++l)
    for (std::size_t k = 0; k < 3; ++k)
      for (std::size_t j = 0; j < 3; ++j)
        for (std::size_t i = 0; i < 3; ++i)
          out[i + 3 * j + 9 * k + 27 * l] = t(i, j, k, l);
}

/// Voigt matrices are filled by muesli as `double[6][6]`.
inline void matrix6ToColumnMajor(const double (&C)[6][6], double* out) {
  for (std::size_t j = 0; j < 6; ++j)
    for (std::size_t i = 0; i < 6; ++i)
      out[i + 6 * j] = C[i][j];
}

inline ivector ivectorFromColumnMajor(const double* in) { return ivector{in[0], in[1], in[2]}; }

inline itensor itensorFromColumnMajor(const double* in) {
  // the itensor constructor takes row-major order
  return itensor(in[0], in[3], in[6], // row 1: A[1,1] A[1,2] A[1,3]
                 in[1], in[4], in[7], // row 2
                 in[2], in[5], in[8]  // row 3
  );
}

/// Stores the symmetric part (A + A') / 2, so the result does not depend on which triangle is read.
inline istensor istensorFromColumnMajor(const double* in) {
  const auto sym = [in](std::size_t i, std::size_t j) { return 0.5 * (in[i + 3 * j] + in[j + 3 * i]); };
  // constructor order: t00, t11, t22, t12, t20, t01
  return istensor(in[0], in[4], in[8], sym(1, 2), sym(2, 0), sym(0, 1));
}

inline itensor4 itensor4FromColumnMajor(const double* in) {
  itensor4 T;
  for (std::size_t l = 0; l < 3; ++l)
    for (std::size_t k = 0; k < 3; ++k)
      for (std::size_t j = 0; j < 3; ++j)
        for (std::size_t i = 0; i < 3; ++i)
          T(i, j, k, l) = in[i + 3 * j + 9 * k + 27 * l];
  return T;
}

inline void assignFromColumnMajor(ivector& v, const double* in) { v = ivectorFromColumnMajor(in); }
inline void assignFromColumnMajor(itensor& t, const double* in) { t = itensorFromColumnMajor(in); }
inline void assignFromColumnMajor(istensor& t, const double* in) { t = istensorFromColumnMajor(in); }
inline void assignFromColumnMajor(itensor4& T, const double* in) { T = itensor4FromColumnMajor(in); }

} // namespace jlmuesli
