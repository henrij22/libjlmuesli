// SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de
// SPDX-License-Identifier: GPL-3.0-or-later

#include "utils.hh"

#include <muesli/Math/mtensor.h>

void registerInterfaceState(jlcxx::Module& mod) {
  using namespace muesli;

  mod.add_type<interfaceState>("InterfaceState")
      // Default constructor
      .constructor<>()

      // --------------------------------------------------------------------
      // getTime
      // --------------------------------------------------------------------
      .method("getTime", [](const interfaceState& ms) { return ms.theTime; })

      .method("getDouble", [](const interfaceState& ms) { return ms.theDouble; })

      // --------------------------------------------------------------------
      // getVector - size & single element
      // --------------------------------------------------------------------
      .method("getVectorSize", [](const interfaceState& ms) { return ms.theVector.size(); })
      .method("getVector",
              [](const interfaceState& ms, std::size_t i) -> ivector {
                // 1-based, like every other index in these bindings
                if (i < 1 || i > ms.theVector.size())
                  throw std::out_of_range("getVector: index " + std::to_string(i) + " out of range 1:" +
                                          std::to_string(ms.theVector.size()));
                return ms.theVector[i - 1];
              })

      // --------------------------------------------------------------------
      // getStensor - size & single element
      // --------------------------------------------------------------------
      .method("getStensorSize", [](const interfaceState& ms) { return ms.theStensor.size(); })
      .method("getStensor",
              [](const interfaceState& ms, std::size_t i) {
                // 1-based, like every other index in these bindings
                if (i < 1 || i > ms.theStensor.size())
                  throw std::out_of_range("getStensor: index " + std::to_string(i) + " out of range 1:" +
                                          std::to_string(ms.theStensor.size()));
                return ms.theStensor[i - 1];
              })

      // --------------------------------------------------------------------
      // getTensor - size & single element
      // --------------------------------------------------------------------
      .method("getTensorSize", [](const interfaceState& ms) { return ms.theTensor.size(); })
      .method("getTensor", [](const interfaceState& ms, std::size_t i) {
        // 1-based, like every other index in these bindings
        if (i < 1 || i > ms.theTensor.size())
          throw std::out_of_range("getTensor: index " + std::to_string(i) + " out of range 1:" +
                                  std::to_string(ms.theTensor.size()));
        return ms.theTensor[i - 1];
      });
}
