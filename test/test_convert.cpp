// SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Unit tests for the Julia <-> MUESLI layout conversion in jlmuesli/util/convert.hh.
//
// That header works on raw double pointers and never touches the Julia runtime, so the one
// place where a column-major Julia array becomes a row-major muesli tensor -- and where a
// mix-up silently transposes every matrix -- can be tested in plain C++.

#include <array>
#include <cstddef>
#include <doctest/doctest.h>

#include <jlmuesli/util/convert.hh>

using namespace jlmuesli;

namespace {

// The Julia matrix [1 2 3; 4 5 6; 7 8 9] as Julia stores it: column by column.
constexpr std::array<double, 9> kColumnMajor3 = {1, 4, 7, 2, 5, 8, 3, 6, 9};

// reshape(1.0:81.0, 3, 3, 3, 3) in Julia: the first index runs fastest.
std::array<double, 81> sequence4() {
  std::array<double, 81> a{};
  for (std::size_t n = 0; n < 81; ++n)
    a[n] = static_cast<double>(n + 1);
  return a;
}

} // namespace

TEST_CASE("itensorFromColumnMajor keeps Julia's (row, column) meaning") {
  const itensor t = itensorFromColumnMajor(kColumnMajor3.data());

  // A[i, j] in Julia is t(i - 1, j - 1) in muesli, nothing is transposed.
  CHECK(t(0, 0) == doctest::Approx(1.0));
  CHECK(t(0, 1) == doctest::Approx(2.0));
  CHECK(t(0, 2) == doctest::Approx(3.0));
  CHECK(t(1, 0) == doctest::Approx(4.0));
  CHECK(t(2, 0) == doctest::Approx(7.0));
  CHECK(t(2, 2) == doctest::Approx(9.0));
}

TEST_CASE("toColumnMajor inverts itensorFromColumnMajor") {
  const itensor t = itensorFromColumnMajor(kColumnMajor3.data());

  std::array<double, 9> out{};
  toColumnMajor(t, out.data());
  for (std::size_t n = 0; n < 9; ++n)
    CHECK(out[n] == doctest::Approx(kColumnMajor3[n]));
}

TEST_CASE("ivector round-trips") {
  const std::array<double, 3> in = {1.5, -2.5, 3.5};
  const ivector v                = ivectorFromColumnMajor(in.data());
  CHECK(v(0) == doctest::Approx(1.5));
  CHECK(v(1) == doctest::Approx(-2.5));
  CHECK(v(2) == doctest::Approx(3.5));

  std::array<double, 3> out{};
  toColumnMajor(v, out.data());
  CHECK(out == in);
}

TEST_CASE("istensorFromColumnMajor") {
  SUBCASE("a symmetric matrix is stored exactly") {
    // [1 .2 .3; .2 2 .4; .3 .4 3]
    const std::array<double, 9> in = {1.0, 0.2, 0.3, 0.2, 2.0, 0.4, 0.3, 0.4, 3.0};
    const istensor s               = istensorFromColumnMajor(in.data());

    std::array<double, 9> out{};
    toColumnMajor(s, out.data());
    for (std::size_t n = 0; n < 9; ++n)
      CHECK(out[n] == doctest::Approx(in[n]));
  }

  SUBCASE("a non-symmetric matrix is reduced to its symmetric part, whichever triangle") {
    // A = [1 2 3; 4 5 6; 7 8 9], so sym(A) = [1 3 5; 3 5 7; 5 7 9]
    const istensor s = istensorFromColumnMajor(kColumnMajor3.data());

    std::array<double, 9> out{};
    toColumnMajor(s, out.data());
    const std::array<double, 9> expected = {1, 3, 5, 3, 5, 7, 5, 7, 9};
    for (std::size_t n = 0; n < 9; ++n)
      CHECK(out[n] == doctest::Approx(expected[n]));
  }

  SUBCASE("the result does not depend on which triangle is larger") {
    // the transpose has the same symmetric part
    const std::array<double, 9> transposed = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    const istensor a                       = istensorFromColumnMajor(kColumnMajor3.data());
    const istensor b                       = istensorFromColumnMajor(transposed.data());

    std::array<double, 9> oa{}, ob{};
    toColumnMajor(a, oa.data());
    toColumnMajor(b, ob.data());
    CHECK(oa == ob);
  }
}

TEST_CASE("itensor4FromColumnMajor puts the first index on the fastest axis") {
  const auto in    = sequence4();
  const itensor4 T = itensor4FromColumnMajor(in.data());

  // Julia's reshape(1.0:81.0, 3, 3, 3, 3)[i, j, k, l] = i + 3(j-1) + 9(k-1) + 27(l-1)
  CHECK(T(0, 0, 0, 0) == doctest::Approx(1.0));
  CHECK(T(1, 0, 0, 0) == doctest::Approx(2.0));
  CHECK(T(0, 1, 0, 0) == doctest::Approx(4.0));
  CHECK(T(0, 0, 1, 0) == doctest::Approx(10.0));
  CHECK(T(0, 0, 0, 1) == doctest::Approx(28.0));
  CHECK(T(2, 2, 2, 2) == doctest::Approx(81.0));
  // an index combination that is not symmetric in any pair
  CHECK(T(0, 1, 2, 0) == doctest::Approx(1.0 + 3 * 1 + 9 * 2));
}

TEST_CASE("itensor4 round-trips, element for element") {
  const auto in    = sequence4();
  const itensor4 T = itensor4FromColumnMajor(in.data());

  std::array<double, 81> out{};
  toColumnMajor(T, out.data());
  CHECK(out == in);
}

TEST_CASE("assignFromColumnMajor overwrites in place") {
  itensor t = itensorFromColumnMajor(kColumnMajor3.data());

  const std::array<double, 9> other = {9, 8, 7, 6, 5, 4, 3, 2, 1};
  assignFromColumnMajor(t, other.data());

  std::array<double, 9> out{};
  toColumnMajor(t, out.data());
  CHECK(out == other);

  SUBCASE("an istensor stays symmetric") {
    istensor s;
    assignFromColumnMajor(s, kColumnMajor3.data());
    CHECK(s(0, 1) == doctest::Approx(s(1, 0)));
    CHECK(s(0, 2) == doctest::Approx(s(2, 0)));
    CHECK(s(1, 2) == doctest::Approx(s(2, 1)));
  }
}
