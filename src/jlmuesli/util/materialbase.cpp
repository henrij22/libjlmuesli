// SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de
// SPDX-License-Identifier: GPL-3.0-or-later

#include "common.hh"
#include "utils.hh"

#include <iostream>

#include <muesli/muesli.h>

// Registered once on the roots; derived wrappers inherit them through the SuperType chain in
// supertypes.hh.
void registerMaterialBase(jlcxx::Module& mod) {
  using muesli::material;
  using muesli::materialPoint;

  mod.add_type<material>("Material")
      .method("check", [](const material& m) { return m.check(); })
      .method("print",
              [](const material& m) {
                m.print(std::cout);
                std::cout << std::endl;
              })
      .method("name", [](const material& m) { return m.name(); })
      .method("getProperty", [](const material& m, muesli::propertyName p) { return m.getProperty(p); })
      .method("density", [](const material& m) { return m.density(); })
      .method("setDensity", [](material& m, double rho) { m.setDensity(rho); })
      .method("referenceTemperature", [](const material& m) { return m.referenceTemperature(); })
      .method("setReferenceTemperature", [](material& m, double theta) { m.setReferenceTemperature(theta); })
      .method("isVariational", [](const material& m) { return m.isVariational(); })
      .method("setVariational", [](material& m) { m.setVariational(); });

  mod.add_type<materialPoint>("MaterialPoint")
      .method("commitCurrentState", [](materialPoint& mp) { mp.commitCurrentState(); })
      .method("resetCurrentState", [](materialPoint& mp) { mp.resetCurrentState(); })
      .method("getConvergedState", [](const materialPoint& mp) { return mp.getConvergedState(); })
      .method("getCurrentState", [](const materialPoint& mp) { return mp.getCurrentState(); })
      .method("density", [](const materialPoint& mp) { return mp.density(); });
}
