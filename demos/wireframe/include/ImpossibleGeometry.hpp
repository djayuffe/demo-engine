#pragma once
#include "Geometry.hpp"
#include <cstdint>
// "Impossible" wire objects (scenes 59..66), adapted from the 100-effect design notes into what a
// CPU-built, GL 4.1 wireframe engine can honestly show. Every function is pure (same arguments ->
// same mesh), finite, duplicate/self-edge free, and bounded in edges, so each is safe on a worker thread.
namespace geo {
// #86 Noncommutative camera: the Heisenberg group. Vertex (i,j) is reached by X^i then Y^j on the upper
// sheet and Y^j then X^i on the lower one; the vertical gap is the commutator, zero only on the axes.
Mesh3 noncommutativeLattice(unsigned n=17,float t=0.f);
// #90 Spatial holonomy: a tangent vector parallel-transported round latitude loops of a sphere comes
// back rotated by 2*pi*(1-cos(colatitude)) with no rotation applied. `t` sweeps the transport.
Mesh3 holonomySphere(float t=0.f);
// #55 Motion-only sculpture: dashes whose velocities are those of a hidden rotating trefoil tube, among
// incoherent decoys. Any single frame is noise; the form is only readable in motion.
Mesh3 motionOnlySculpture(float t=0.f,unsigned dots=4200);
// #61 Atomic collision universe: a million-thread-style scatter into a shared integer field through a
// designed nonlinear address function (CPU model of atomicAdd on an SSBO); dense cells are drawn.
Mesh3 atomicCollisionField(float t=0.f,unsigned grid=26,unsigned threads=90000);
// #77 Relational gravity: attraction by similarity in a drifting feature space, not by distance.
Mesh3 relationalGravity(float t=0.f,unsigned agents=200,unsigned steps=70);
// #37 Data-collision sculpture: ONE small word buffer decoded three incompatible ways plus a graph
// reading; the edges between the interpretations are the artwork.
Mesh3 dataCollisionSculpture(float t=0.f,unsigned words=720);
// #48 Self-refuting object: r = F(r) with F built to destabilise its own fixed point (r=1 is a solution
// but repels); a bounded solver is stopped mid-flight, so the sphere never settles.
Mesh3 selfRefutingObject(float t=0.f,unsigned iterations=9);
// #8 Contradictory silhouette: the visual hull of three incompatible silhouettes (disk, cathedral, gear
// star) seen along three axes, carved on a voxel grid and drawn as its exposed lattice faces.
Mesh3 contradictorySilhouette(float t=0.f,unsigned grid=30);
}
