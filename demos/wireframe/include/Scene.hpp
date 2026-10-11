#pragma once
#include "Geometry.hpp"
#include <cstdint>
#include <future>
#include <map>
#include <string_view>
struct SceneInfo { int id; std::string_view name; bool dynamic; double updateHz; std::string_view provenance; };
class SceneSystem {
public:
  SceneSystem();
  const SceneInfo& info(int id) const;
  int count() const;
  // Synchronous: always returns the mesh for this exact tick (builds it if needed). Used by tests.
  const Mesh3& mesh(int id,double seconds,uint64_t seed);
  // For the render loop: never waits. Returns the newest finished mesh (at most one update tick old)
  // and builds the current tick on a worker thread. A scene that has never finished building returns
  // an empty mesh, and the caller keeps showing its previous one; prefetch() makes that rare.
  const Mesh3& meshAsync(int id,double seconds,uint64_t seed);
  // Starts building a scene that has never been built, without waiting (e.g. the next scene).
  void prefetch(int id,double seconds,uint64_t seed);
  static uint64_t tickFor(const SceneInfo& si,double seconds);
private:
  struct Slot { uint64_t tick=0; bool ready=false; Mesh3 mesh; std::future<Mesh3> job; uint64_t jobTick=0; };
  Mesh3 build(int id,uint64_t tick,uint64_t seed) const;
  static void collect(Slot& s);
  Polytope4 c600_,c120_;
  std::map<int,Slot> cache_;   // std::map: slots never move, so a returned reference stays valid
};
