// SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de
// SPDX-License-Identifier: GPL-3.0-or-later

#include "common.hh"
#include "utils.hh"

void registerMaterialState(jlcxx::Module& mod) {
  using namespace muesli;

  mod.add_type<materialState>("materialState")
      // Default constructor
      .constructor<>()

      // --------------------------------------------------------------------
      // getTime
      // --------------------------------------------------------------------
      .method("getTime", [](const materialState& ms) { return ms.theTime; })

      .method("getDouble", [](const materialState& ms) { return ms.theDouble; })

      // --------------------------------------------------------------------
      // getVector - size & single element
      // --------------------------------------------------------------------
      .method("getVectorSize", [](const materialState& ms) { return ms.theVector.size(); })
      .method("getVector",
              [](const materialState& ms, std::size_t i) {
                if (i >= ms.theVector.size()) {
                  throw std::out_of_range("Index out of range in getVector.");
                }
                return iVectorToArrayRef(ms.theVector[i]);
              })

      // --------------------------------------------------------------------
      // getStensor - size & single element
      // --------------------------------------------------------------------
      .method("getStensorSize", [](const materialState& ms) { return ms.theStensor.size(); })
      .method("getStensor!",
              [](const materialState& ms, std::size_t i, JuliaTensor T) {
                if (i >= ms.theStensor.size()) {
                  throw std::out_of_range("Index out of range in getStensor.");
                }
                return itensorToArrayRef(ms.theStensor[i], T);
              })

      // --------------------------------------------------------------------
      // getTensor - size & single element
      // --------------------------------------------------------------------
      .method("getTensorSize", [](const materialState& ms) { return ms.theTensor.size(); })
      .method("getTensor!", [](const materialState& ms, std::size_t i, JuliaTensor T) {
        if (i >= ms.theTensor.size()) {
          throw std::out_of_range("Index out of range in getTensor.");
        }
        return itensorToArrayRef(ms.theTensor[i], T);
      });
}
