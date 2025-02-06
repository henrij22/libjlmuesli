// SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>
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

inline const double* assertSizeAndExtractData(JuliaVector c, size_t expectedSize) {
  if (c.size() != expectedSize)
    throw std::invalid_argument("Input has to be a " + std::to_string(expectedSize) + " vector.");
  const double* data = c.data();
  return data;
}

inline const double* assertSizeAndExtractData(JuliaTensor c, std::pair<size_t, size_t> expectedSizes) {
  if (c.size() != expectedSizes.first * expectedSizes.second)
    throw std::invalid_argument("Input has to be a " + std::to_string(expectedSizes.first) + " x " +
                                std::to_string(expectedSizes.second) + " matrix.");
  const double* data = c.data();
  return data;
}

inline auto toIVector(JuliaVector vec) {
  const double* data = assertSizeAndExtractData(vec, 3);
  return ivector{vec.data()};
}

inline auto toITensor(JuliaTensor array) {
  // Validate that the input is 3x3
  const double* data = assertSizeAndExtractData(array, {3, 3});

  // Construct the itensor in row-major order:
  // Row 1: A[1,1], A[1,2], A[1,3]
  // Row 2: A[2,1], A[2,2], A[2,3]
  // Row 3: A[3,1], A[3,2], A[3,3]
  return itensor(data[0], data[3], data[6], // Row 1
                 data[1], data[4], data[7], // Row 2
                 data[2], data[5], data[8]  // Row 3
  );
}

inline istensor toIstensor(const JuliaTensor& array) {
  // 2) Extract the raw data pointer in row-major order: data[i * 3 + j].
  const double* data = assertSizeAndExtractData(array, {3, 3});

  // 3) Read the six unique components for the symmetric matrix.
  //    According to istensor constructor:
  //    istensor(const double t00, const double t11, const double t22,
  //             const double t12, const double t20, const double t01);
  //
  //    We assume the following mapping:
  //      a[0][0] = t00
  //      a[1][1] = t11
  //      a[2][2] = t22
  //      a[1][2] = a[2][1] = t12
  //      a[2][0] = a[0][2] = t20
  //      a[0][1] = a[1][0] = t01
  //
  //    So for row i, column j, the data index is i*3 + j.

  double t00 = data[0 * 3 + 0];
  double t11 = data[1 * 3 + 1];
  double t22 = data[2 * 3 + 2];
  double t12 = data[1 * 3 + 2];
  double t20 = data[2 * 3 + 0];
  double t01 = data[0 * 3 + 1];

  // 4) Construct and return the istensor.
  return istensor(t00, t11, t22, t12, t20, t01);
}
inline itensor4 toItensor4(const jlcxx::ArrayRef<double, 4>& array) {
  // Validate that the input array is 3x3x3x3 (3^4 = 81 elements)
  if (array.size() != 81)
    throw std::invalid_argument("Input has to be a 3x3x3x3 array.");

  // Create an empty itensor4
  itensor4 T;
  const double* data = array.data();

  // Populate the itensor4, using column-major indexing.
  // For a Julia array, the linear index is: i + 3*j + 9*k + 27*l
  for (size_t i = 0; i < 3; ++i)
    for (size_t j = 0; j < 3; ++j)
      for (size_t k = 0; k < 3; ++k)
        for (size_t l = 0; l < 3; ++l)
          T(i, j, k, l) = data[i + 3 * j + 9 * k + 27 * l];
  // T(i, j, k, l) = data[i * 27 + j * 9 + k * 3 + l];

  return T;
}
