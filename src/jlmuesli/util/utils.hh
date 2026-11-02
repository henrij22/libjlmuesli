// SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <iostream>
#include <sstream>
#include <string>

#include <muesli/muesli.h>

#include <jlcxx/jlcxx.hpp>

/// MUESLI's own consistency check of a material point (stress against the derivative of the
/// energy, tangent against the derivative of the stress). Its report is only printed on failure.
template <typename MaterialPoint>
bool runSelfTest(const MaterialPoint& mp, bool testDE, bool testDDE) {
  std::ostringstream report;
  const bool ok = mp.testImplementation(report, testDE, testDDE);
  if (!ok)
    std::cout << report.str() << std::endl;
  return ok;
}

/// The class an instance of `Material` is actually built from. Normally the class itself.
template <typename Material>
struct ConcreteMaterial
{
  using type = Material;
};

/// A subclass that returns the density held by `muesli::material`, which the "density" property
/// sets. The classes it replaces override `density()` to return a member no constructor initialises.
template <typename Material>
struct WithBaseDensity : Material
{
  using Material::Material;
  double density() const override { return muesli::material::density(); }
};

/// The MUESLI material name corresponding to a wrapper registration prefix.
///
/// `registerSmallStrainMaterial`/`registerFiniteStrainMaterial` take one string that is used
/// as the prefix of the Julia type names (`<name>Material`, `<name>MP`). For the damage models
/// that prefix carries a trailing underscore -- "GTN_" gives `GTN_Material` -- purely so the
/// Julia names read well. MUESLI must not see that underscore: its own constructors are called
/// with "GTN", the name is stored on the material and materialFactory looks materials up by it,
/// so passing "GTN_" would make the property-map route disagree with the direct constructors.
inline std::string muesliMaterialName(const std::string& wrapperName) {
  std::string name = wrapperName;
  while (!name.empty() && name.back() == '_')
    name.pop_back();
  return name;
}

// A DLL exports only what is marked once any symbol is (jlcxx marks its entry point), and the C++
// tests link against this library.
#if defined(_WIN32)
  #if defined(jlmuesli_EXPORTS)
    #define JLMUESLI_API __declspec(dllexport)
  #else
    #define JLMUESLI_API __declspec(dllimport)
  #endif
#else
  #define JLMUESLI_API
#endif

// helpers.cpp
struct JLMUESLI_API MultiMapWrapper
{
  // Insert a (key, value) pair into the multimap
  void set(const std::string& key, double value);

  // Return *all* values matching the specified key as a vector
  std::vector<double> get(const std::string& key) const;

  const std::multimap<std::string, double>& multiMap() const;

  // What kind of interface is that even?
  void setString(const std::string& key, const std::string& string);

  // We return NotFound if key was not found
  std::string getString(const std::string& key) const;

  bool hasKeyword(const std::string& key) const;

private:
  std::multimap<std::string, double> m_map;
};

using MaterialProperties = MultiMapWrapper;

template <typename TensorType>
struct ArrayOfTensorsT
{
  using JuliaTensor = jlcxx::ArrayRef<double, 2>;

  void pushTensor(JuliaTensor tensor);

  void clear();

  size_t size() const;

  const std::vector<TensorType>& tensors() const;

private:
  std::vector<TensorType> tensors_{};
};

template <typename TensorType>
void registerArrayOfTensorsT(jlcxx::Module& mod, const std::string& name);

void registerHelpers(jlcxx::Module& mod);

// materialbase.cpp
void registerMaterialBase(jlcxx::Module& mod);

// materialstate.cpp
void registerInterfaceState(jlcxx::Module& mod);

// propertynames.cpp
void registerPropertyName(jlcxx::Module& mod);

// tensors.cpp
void registerTensors(jlcxx::Module& mod);