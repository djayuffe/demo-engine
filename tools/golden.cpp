// Golden-hash harness: prints a bit-exact FNV-1a hash per case so a refactor can be proven
// behaviour-preserving (diff the output of two builds). Not part of the default build.
#include "Effects.hpp"
#include "Geometry.hpp"
#include "Scene.hpp"
#include <cstdint>
#include <cstdio>
#include <cstring>
static uint64_t h(const Mesh3& m){
  uint64_t x=1469598103934665603ull;
  auto mix=[&](const void* p,size_t n){const unsigned char* b=static_cast<const unsigned char*>(p);for(size_t i=0;i<n;++i){x^=b[i];x*=1099511628211ull;}};
  for(const auto& v:m.v){mix(&v.x,4);mix(&v.y,4);mix(&v.z,4);}
  for(const auto& e:m.e){mix(&e.a,4);mix(&e.b,4);}
  return x^(uint64_t(m.v.size())<<40)^(uint64_t(m.e.size())<<16);
}
int main(){
  const Mesh3 bases[3]={geo::torus(24,12),geo::uvSphere(10,20),geo::cube(1.f)};
  for(int id=0;id<effectCount();++id)for(int b=0;b<3;++b)for(uint32_t seed:{1u,77u,9001u}){
    EffectContext c;c.time=3.7f+float(seed%5);c.amount=1.f;c.bass=.6f;c.mid=.4f;c.treble=.3f;c.beat=.5f;c.seed=seed;
    Mesh3 m=bases[b];bool ok=applyEffect(m,id,c,&bases[(b+1)%3]);
    std::printf("effect %d base %d seed %u ok %d %016llx\n",id,b,seed,int(ok),(unsigned long long)h(m));}
  for(int r=0;r<recipeCount();++r){EffectContext c;c.time=5.f;c.bass=.5f;c.mid=.5f;c.treble=.5f;c.beat=.25f;c.seed=11;
    Mesh3 m=bases[0];bool ok=applyRecipe(m,recipe(r),c,&bases[1]);std::printf("recipe %d ok %d %016llx\n",r,int(ok),(unsigned long long)h(m));}
  for(uint32_t s=0;s<120;++s)for(int d=1;d<=3;++d){EffectContext c;c.time=2.f+float(s);c.bass=.5f;c.mid=.4f;c.treble=.6f;c.beat=.3f;c.seed=s;
    EffectRecipe r=mutateRecipe(s,d);Mesh3 m=bases[1];bool ok=applyRecipe(m,r,c,&bases[2]);std::printf("mutate %u %d ok %d %016llx\n",s,d,int(ok),(unsigned long long)h(m));}
  SceneSystem ss;
  for(int id=0;id<ss.count();++id)for(double t:{0.0,3.3,12.9}){const Mesh3& m=ss.mesh(id,t,42);std::printf("scene %d t %g %016llx\n",id,t,(unsigned long long)h(m));}
  return 0;}
