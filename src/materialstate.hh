
#pragma once

#include "common.hh"

#include <muesli/Utils/utils.h>

#include <jlcxx/jlcxx.hpp>

inline void registerMaterialState(jlcxx::Module& mod) {
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
                // Convert ivector -> a Julia array
                return iVectorToArrayRef(ms.theVector[i]);
              })

      // --------------------------------------------------------------------
      // getStensor - size & single element
      // --------------------------------------------------------------------
      .method("getStensorSize", [](const materialState& ms) { return ms.theStensor.size(); })
      .method("getStensor",
              [](const materialState& ms, std::size_t i) {
                if (i >= ms.theStensor.size()) {
                  throw std::out_of_range("Index out of range in getStensor.");
                }
                return istensorToArrayRef(ms.theStensor[i]);
              })

      // --------------------------------------------------------------------
      // getTensor - size & single element
      // --------------------------------------------------------------------
      .method("getTensorSize", [](const materialState& ms) { return ms.theTensor.size(); })
      .method("getTensor", [](const materialState& ms, std::size_t i) {
        if (i >= ms.theTensor.size()) {
          throw std::out_of_range("Index out of range in getTensor.");
        }
        return itensorToArrayRef(ms.theTensor[i]);
      });
}
