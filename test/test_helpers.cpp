// SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Unit tests for the pure C++ helpers in jlmuesli/util.
//
// Only helpers that do not touch the Julia runtime can be exercised here: anything taking a
// jlcxx::ArrayRef needs a live `jl_init`, and the bindings themselves only exist once the
// module is loaded from Julia. Those are covered by the Julia integration suite in
// julia/runtests.jl -- see the README.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <jlmuesli/util/common.hh>
#include <jlmuesli/util/utils.hh>

#include <muesli/muesli.h>

#include <algorithm>
#include <string>
#include <vector>

TEST_CASE("MaterialProperties stores and returns numeric properties") {
  MaterialProperties props;

  props.set("young", 210000.0);
  props.set("poisson", 0.3);

  const auto young = props.get("young");
  REQUIRE(young.size() == 1);
  CHECK(young[0] == doctest::Approx(210000.0));

  const auto poisson = props.get("poisson");
  REQUIRE(poisson.size() == 1);
  CHECK(poisson[0] == doctest::Approx(0.3));

  SUBCASE("an unknown key yields an empty result rather than throwing") {
    CHECK(props.get("not_a_property").empty());
  }
}

TEST_CASE("MaterialProperties keeps every value of a repeated key") {
  // The backing store is a multimap, so repeated keys accumulate. Several muesli
  // materials rely on that to pass a list of relaxation times.
  MaterialProperties props;
  props.set("tau", 1.0);
  props.set("tau", 2.0);
  props.set("tau", 4.0);

  auto values = props.get("tau");
  REQUIRE(values.size() == 3);
  std::sort(values.begin(), values.end());
  CHECK(values[0] == doctest::Approx(1.0));
  CHECK(values[1] == doctest::Approx(2.0));
  CHECK(values[2] == doctest::Approx(4.0));
}

TEST_CASE("MaterialProperties encodes string options the way muesli expects") {
  // muesli spells a flag as a bare "key value" entry in the property map rather than as a
  // key with a string value, so setString concatenates the two.
  MaterialProperties props;
  props.setString("plasticity", "isotropic");

  CHECK(props.hasKeyword("plasticity"));
  CHECK(props.getString("plasticity") == "isotropic");

  SUBCASE("a keyword that was never set is reported as absent") {
    CHECK_FALSE(props.hasKeyword("viscosity"));
    CHECK(props.getString("viscosity") == "NotFound");
  }
}

TEST_CASE("MaterialProperties exposes the underlying multimap for muesli constructors") {
  MaterialProperties props;
  props.set("young", 1.0);
  props.set("poisson", 0.25);

  const auto& raw = props.multiMap();
  CHECK(raw.size() == 2);
  CHECK(raw.count("young") == 1);
  CHECK(raw.count("poisson") == 1);
}

TEST_CASE("toMPM_Enu builds the young/poisson property map") {
  const auto mpm = toMPM_Enu(210000.0, 0.3);

  REQUIRE(mpm.count("young") == 1);
  REQUIRE(mpm.count("poisson") == 1);
  CHECK(mpm.find("young")->second == doctest::Approx(210000.0));
  CHECK(mpm.find("poisson")->second == doctest::Approx(0.3));
}

TEST_CASE("a material built from those properties agrees with the direct constructor") {
  // Guards the fix that stopped hard-coding the material name: whichever route is used, the
  // resulting material must report the same stiffness.
  const double E = 210000.0, nu = 0.3;

  muesli::elasticIsotropicMaterial direct{"ElasticIsotropic", E, nu, 1.0};
  muesli::elasticIsotropicMaterial fromProps{"ElasticIsotropic", toMPM_Enu(E, nu)};

  CHECK(direct.check());
  CHECK(fromProps.check());
  CHECK(direct.getProperty(muesli::PR_YOUNG) == doctest::Approx(fromProps.getProperty(muesli::PR_YOUNG)));
  CHECK(direct.getProperty(muesli::PR_POISSON) == doctest::Approx(fromProps.getProperty(muesli::PR_POISSON)));
}

TEST_CASE("muesliMaterialName strips the Julia naming suffix") {
  // The registration prefix doubles as the Julia type-name prefix, so the damage models carry
  // a trailing underscore ("GTN_" -> GTN_Material). MUESLI must not see it.
  CHECK(muesliMaterialName("GTN_") == "GTN");
  CHECK(muesliMaterialName("Gurson_") == "Gurson");
  CHECK(muesliMaterialName("Lemaitre_") == "Lemaitre");
  CHECK(muesliMaterialName("LemKin_") == "LemKin");

  SUBCASE("names without a suffix are untouched") {
    CHECK(muesliMaterialName("ElasticIsotropic") == "ElasticIsotropic");
    CHECK(muesliMaterialName("SVK") == "SVK");
    CHECK(muesliMaterialName("") == "");
  }
}

TEST_CASE("both construction routes give a material the same MUESLI name") {
  // Regression: the property-map constructor used to be handed the registration prefix
  // verbatim, so a GTN material built that way was named "GTN_" while the direct constructor
  // named it "GTN". The name is stored on the material and materialFactory looks materials up
  // by it, so the two routes have to agree.
  const double E = 210000.0, nu = 0.3, rho = 1.0, q1 = 1.5, q2 = 1.0, yield = 200.0;

  auto props = toMPM_Enu(E, nu);
  props.insert({"q1", q1});
  props.insert({"q2", q2});
  props.insert({"yield", yield});
  props.insert({"density", rho});

  muesli::GTN_Material direct{"GTN", E, nu, rho, q1, q2, yield};
  muesli::GTN_Material fromProps{muesliMaterialName("GTN_"), props};

  CHECK(direct.name() == "GTN");
  CHECK(fromProps.name() == "GTN");
  CHECK(direct.name() == fromProps.name());
}
