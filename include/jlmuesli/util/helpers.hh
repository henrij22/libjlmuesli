#pragma once

#include "common.hh"

#include <cstddef>
#include <type_traits>

#include <muesli/Math/mtensor.h>
#include <muesli/Utils/utils.h>

#include <jlcxx/jlcxx.hpp>

struct MultiMapWrapper
{
  // Insert a (key, value) pair into the multimap
  void set(const std::string& key, double value) { m_map.insert({key, value}); }

  // Return *all* values matching the specified key as a vector
  std::vector<double> get(const std::string& key) const {
    std::vector<double> values;
    auto range = m_map.equal_range(key);
    for (auto it = range.first; it != range.second; ++it) {
      values.push_back(it->second);
    }
    return values;
  }

  auto& multiMap() const { return m_map; }

  // What kind of interface is that even?
  void setString(const std::string& key, const std::string& string) {
    auto option = key + " " + string;
    m_map.insert({option, 0.0});
  }

  // We return NotFound if key was not found
  std::string getString(const std::string& key) const {
    std::string value{"NotFound"};
    muesli::assignValue(m_map, key, value);
    return value;
  }

  bool hasKeyword(const std::string& key) const { return muesli::hasKeyword(m_map, key); }

private:
  std::multimap<std::string, double> m_map;
};

using MaterialProperties = MultiMapWrapper;

template <typename TensorType>
struct ArrayOfTensorsT
{
  using JuliaTensor = jlcxx::ArrayRef<double, 2>;

  void pushTensor(JuliaTensor tensor) {
    if constexpr (std::is_same_v<TensorType, istensor>)
      tensors_.push_back(toIstensor(tensor));
    else
      tensors_.push_back(toITensor(tensor));
  }

  void clear() { tensors_.clear(); }

  size_t size() const { return tensors_.size(); }

  auto& tensors() const { return tensors_; }

private:
  std::vector<TensorType> tensors_{};
};

template <typename TensorType>
void registerArrayOfTensorsT(jlcxx::Module& mod, const std::string& name) {
  using TensorWrapper = ArrayOfTensorsT<TensorType>;
  mod.add_type<TensorWrapper>(name)
      .template constructor<>()
      .method("push!", &TensorWrapper::pushTensor)
      .method("clear!", &TensorWrapper::clear)
      .method("size", &TensorWrapper::size);
}

inline void registerHelpers(jlcxx::Module& mod) {
  mod.add_type<MultiMapWrapper>("MaterialProperties")
      // Default constructor
      .constructor<>()
      .method("setProperty!", &MultiMapWrapper::set)
      .method("getProperty", &MultiMapWrapper::get)
      .method("setString!", &MultiMapWrapper::setString)
      .method("getString", &MultiMapWrapper::getString)
      .method("hasKeyword", &MultiMapWrapper::hasKeyword);

  registerArrayOfTensorsT<istensor>(mod, "ArrayOfIsTensors");
  registerArrayOfTensorsT<itensor>(mod, "ArrayOfITensors");
}