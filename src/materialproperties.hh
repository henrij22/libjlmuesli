#pragma once

#include <map>
#include <string>
#include <vector>

#include <jlcxx/jlcxx.hpp>

class MultiMapWrapper
{
public:
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

private:
  std::multimap<std::string, double> m_map;
};

  using MaterialProperties = MultiMapWrapper;


inline void registerMaterialProperties(jlcxx::Module& mod) {
  mod.add_type<MultiMapWrapper>("MaterialProperties")
      // Default constructor
      .constructor<>()

      // Bind set(key, value)
      .method("set", &MultiMapWrapper::set)

      // Bind get(key) -> vector<double>
      .method("get", &MultiMapWrapper::get);
}
