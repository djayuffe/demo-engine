#pragma once
#include "Geometry.hpp"
#include <cstdint>
#include <map>
#include <string_view>
struct SceneInfo { int id; std::string_view name; bool dynamic; double updateHz; std::string_view provenance; };
class SceneSystem {
public:
  SceneSystem();
  const SceneInfo& info(int id) const;
  int count() const;
  const Mesh3& mesh(int id,double seconds,uint64_t seed);
  static uint64_t tickFor(const SceneInfo& si,double seconds);
private:
  Polytope4 c600_,c120_;
  std::map<int,std::pair<uint64_t,Mesh3>> cache_;
};
