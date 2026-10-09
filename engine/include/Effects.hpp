#pragma once
#include "Geometry.hpp"
#include <cstdint>
#include <string_view>

// Per-frame audio/time state fed to effects. bass/mid/treble/beat in [0,1],
// driven by the scene's synthetic pulse or the real music analyser.
struct EffectContext {
  float time=0.f, amount=1.f, bass=0.f, mid=0.f, treble=0.f, beat=0.f;
  uint32_t seed=0;
};
struct EffectStage {
  int effect=0;        // id in [0,effectCount())
  float amount=1.f;    // per-stage amount multiplier
  float timeScale=1.f, timeOffset=0.f, seedOffset=0.f;
  bool useSecondary=false;  // feed the secondary object (bridges)
};
struct EffectRecipe {
  std::string name="Raw";
  int stageCount=0;
  EffectStage stages[6]{};
};

int effectCount();
std::string_view effectName(int id);
bool applyEffect(Mesh3& m, int id, const EffectContext& ctx, const Mesh3* secondary=nullptr);

int recipeCount();
std::string_view recipeName(int id);
EffectRecipe recipe(int id);
EffectRecipe mutateRecipe(uint32_t seed, int depth);
// Chain the recipe's stages; restores m (and returns false) if any stage
// leaves an empty or invalid mesh, so a bad combo never blanks the scene.
bool applyRecipe(Mesh3& m, const EffectRecipe& r, const EffectContext& base,
                 const Mesh3* secondary=nullptr);
