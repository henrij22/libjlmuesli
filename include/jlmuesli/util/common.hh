
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

using JuliaTensor = jlcxx::ArrayRef<double, 2>;
using JuliaVector = jlcxx::ArrayRef<double, 1>;

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

inline auto iVectorToArrayRef(ivector ivec) { return JuliaVector{ivec.components()}; }

inline auto toITensor(JuliaTensor array) {
  // Validate that the input is 3x3
  const double* data = assertSizeAndExtractData(array, {3, 3});

  // Construct the itensor using the 3x3 double array constructor
  return itensor(data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8]);
}

inline JuliaTensor itensorToArrayRef(const itensor& tensor) {
  // Cast jl_float64_type to jl_value_t* for jl_apply_array_type
  jl_value_t* array_type = jl_apply_array_type(reinterpret_cast<jl_value_t*>(jl_float64_type), 2);

  // Create a 3x3 Julia array
  jl_array_t* julia_array = jl_alloc_array_2d(array_type, 3, 3);

  // Access the data pointer using the updated jl_array_data macro
  double* data = jl_array_data(julia_array, double);

  // Populate the Julia array with data from the itensor
  for (size_t i = 0; i < 3; ++i)
    for (size_t j = 0; j < 3; ++j)
      data[i * 3 + j] = tensor(i, j); // Row-major order

  // Wrap the Julia array in a jlcxx::ArrayRef
  return JuliaTensor(julia_array);
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

inline JuliaTensor istensorToArrayRef(const istensor& T) {
  // 1) Allocate a 3×3 Julia array of Float64.
  //    Note: jl_apply_array_type() expects a jl_value_t*, so we cast jl_float64_type.
  jl_value_t* arr_type    = jl_apply_array_type(reinterpret_cast<jl_value_t*>(jl_float64_type), 2);
  jl_array_t* julia_array = jl_alloc_array_2d(arr_type, 3, 3);

  // 2) Get a pointer to the array data (row-major).
  //    In recent Julia versions, use jl_array_data(array, type).
  double* data_ptr = jl_array_data(julia_array, double);

  // 3) Copy the 3×3 entries from the istensor into the Julia array.
  //    We just do it element-by-element:
  data_ptr[0 * 3 + 0] = T(0, 0);
  data_ptr[0 * 3 + 1] = T(0, 1);
  data_ptr[0 * 3 + 2] = T(0, 2);
  data_ptr[1 * 3 + 0] = T(1, 0);
  data_ptr[1 * 3 + 1] = T(1, 1);
  data_ptr[1 * 3 + 2] = T(1, 2);
  data_ptr[2 * 3 + 0] = T(2, 0);
  data_ptr[2 * 3 + 1] = T(2, 1);
  data_ptr[2 * 3 + 2] = T(2, 2);

  // 4) Wrap it in a jlcxx::ArrayRef and return.
  return JuliaTensor(julia_array);
}

inline itensor4 arrayRefToItensor4(const jlcxx::ArrayRef<double, 4>& array) {
  // Validate that the input array is 3x3x3x3
  if (array.size() != 81)
    throw std::invalid_argument("Input ArrayRef must be a 3x3x3x3 array.");

  // Create an empty itensor4
  itensor4 T;

  // Populate the itensor4 from the ArrayRef
  const double* data = array.data();
  for (size_t i = 0; i < 3; ++i)
    for (size_t j = 0; j < 3; ++j)
      for (size_t k = 0; k < 3; ++k)
        for (size_t l = 0; l < 3; ++l)
          T(i, j, k, l) = data[i * 27 + j * 9 + k * 3 + l];

  return T;
}

inline jlcxx::ArrayRef<double, 4> itensor4ToArrayRef(const itensor4& T) {
  // Allocate a 4-dimensional Julia array of Float64
  jl_value_t* arr_type = jl_apply_array_type(reinterpret_cast<jl_value_t*>(jl_float64_type), 4);

  // Define the dimensions for the 4D array
  size_t dims[4] = {3, 3, 3, 3}; // 3x3x3x3

  // Allocate the 4D array
  jl_array_t* julia_array = jl_alloc_array_nd(arr_type, dims, 4);

  // Get a pointer to the Julia array data
  double* data_ptr = jl_array_data(julia_array, double);

  // Copy data from the itensor4 into the Julia array
  for (size_t i = 0; i < 3; ++i)
    for (size_t j = 0; j < 3; ++j)
      for (size_t k = 0; k < 3; ++k)
        for (size_t l = 0; l < 3; ++l)
          data_ptr[i * 27 + j * 9 + k * 3 + l] = T(i, j, k, l);

  // Wrap the Julia array in a jlcxx::ArrayRef and return
  return jlcxx::ArrayRef<double, 4>(julia_array);
}
