// SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de
// SPDX-License-Identifier: GPL-3.0-or-later

#include "utils.hh"
#include "common.hh"

void MultiMapWrapper::set(const std::string& key, double value) {
  m_map.insert({key, value});
}

std::vector<double> MultiMapWrapper::get(const std::string& key) const {
  std::vector<double> values;
  auto range = m_map.equal_range(key);
  for (auto it = range.first; it != range.second; ++it) {
    values.push_back(it->second);
  }
  return values;
}

const std::multimap<std::string, double>& MultiMapWrapper::multiMap() const {
  return m_map;
}

void MultiMapWrapper::setString(const std::string& key, const std::string& string) {
  auto option = key + " " + string;
  m_map.insert({option, 0.0});
}

std::string MultiMapWrapper::getString(const std::string& key) const {
  std::string value{"NotFound"};
  muesli::assignValue(m_map, key, value);
  return value;
}

bool MultiMapWrapper::hasKeyword(const std::string& key) const {
  return muesli::hasKeyword(m_map, key);
}

template <typename TensorType>
void ArrayOfTensorsT<TensorType>::pushTensor(JuliaTensor tensor) {
  if constexpr (std::is_same_v<TensorType, istensor>)
    tensors_.push_back(toIstensor(tensor));
  else
    tensors_.push_back(toITensor(tensor));
}

template <typename TensorType>
void ArrayOfTensorsT<TensorType>::clear() {
  tensors_.clear();
}

template <typename TensorType>
size_t ArrayOfTensorsT<TensorType>::size() const {
  return tensors_.size();
}

template <typename TensorType>
const std::vector<TensorType>& ArrayOfTensorsT<TensorType>::tensors() const {
  return tensors_;
}

template <typename TensorType>
void registerArrayOfTensorsT(jlcxx::Module& mod, const std::string& name) {
  using TensorWrapper = ArrayOfTensorsT<TensorType>;
  mod.add_type<TensorWrapper>(name)
      .template constructor<>()
      .method("push!", &TensorWrapper::pushTensor)
      .method("clear!", &TensorWrapper::clear)
      .method("size", &TensorWrapper::size);
}

void registerHelpers(jlcxx::Module& mod) {
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
