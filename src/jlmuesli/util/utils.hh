// SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <muesli/muesli.h>

#include <jlcxx/jlcxx.hpp>

// helpers.cpp
struct MultiMapWrapper
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

// materialstate.cpp
void registerMaterialState(jlcxx::Module& mod);

// propertynames.cpp
void registerPropertyName(jlcxx::Module& mod);

// tensors.cpp
void registerTensors(jlcxx::Module& mod);