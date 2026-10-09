#include "TextScroller.hpp"
#include <cmath>
#include <vector>
namespace textfont {
// 5x7 font, rows top->bottom, bit 4 (0x10) = leftmost column. Lowercase letters share the
// uppercase shapes (classic demo-scroller look).
static const Glyph G[95] = {
/* */ {{0,0,0,0,0,0,0}},
/*!*/ {{4,4,4,4,4,0,4}},
/*"*/ {{10,10,10,0,0,0,0}},
/*#*/ {{10,10,31,10,31,10,10}},
/*$*/ {{4,15,20,14,5,30,4}},
/*%*/ {{25,25,2,4,8,19,19}},
/*&*/ {{12,18,20,8,21,18,13}},
/*'*/ {{4,4,8,0,0,0,0}},
/*(*/ {{2,4,8,8,8,4,2}},
/*)*/ {{8,4,2,2,2,4,8}},
/*x2A*/ {{0,21,14,31,14,21,0}},
/*+*/ {{0,4,4,31,4,4,0}},
/*,*/ {{0,0,0,0,12,4,8}},
/*-*/ {{0,0,0,31,0,0,0}},
/*.*/ {{0,0,0,0,0,12,12}},
/*x2F*/ {{0,1,2,4,8,16,0}},
/*0*/ {{14,17,19,21,25,17,14}},
/*1*/ {{4,12,4,4,4,4,14}},
/*2*/ {{14,17,1,2,4,8,31}},
/*3*/ {{31,2,4,2,1,17,14}},
/*4*/ {{2,6,10,18,31,2,2}},
/*5*/ {{31,16,30,1,1,17,14}},
/*6*/ {{6,8,16,30,17,17,14}},
/*7*/ {{31,1,2,4,8,8,8}},
/*8*/ {{14,17,17,14,17,17,14}},
/*9*/ {{14,17,17,15,1,2,12}},
/*:*/ {{0,12,12,0,12,12,0}},
/*;*/ {{0,12,12,0,12,4,8}},
/*<*/ {{2,4,8,16,8,4,2}},
/*=*/ {{0,0,31,0,31,0,0}},
/*>*/ {{8,4,2,1,2,4,8}},
/*?*/ {{14,17,1,2,4,0,4}},
/*@*/ {{14,17,23,21,23,16,14}},
/*A*/ {{14,17,17,31,17,17,17}},
/*B*/ {{30,17,17,30,17,17,30}},
/*C*/ {{14,17,16,16,16,17,14}},
/*D*/ {{28,18,17,17,17,18,28}},
/*E*/ {{31,16,16,30,16,16,31}},
/*F*/ {{31,16,16,30,16,16,16}},
/*G*/ {{14,17,16,23,17,17,15}},
/*H*/ {{17,17,17,31,17,17,17}},
/*I*/ {{14,4,4,4,4,4,14}},
/*J*/ {{7,2,2,2,2,18,12}},
/*K*/ {{17,18,20,24,20,18,17}},
/*L*/ {{16,16,16,16,16,16,31}},
/*M*/ {{17,27,21,21,17,17,17}},
/*N*/ {{17,17,25,21,19,17,17}},
/*O*/ {{14,17,17,17,17,17,14}},
/*P*/ {{30,17,17,30,16,16,16}},
/*Q*/ {{14,17,17,17,21,18,13}},
/*R*/ {{30,17,17,30,20,18,17}},
/*S*/ {{15,16,16,14,1,1,30}},
/*T*/ {{31,4,4,4,4,4,4}},
/*U*/ {{17,17,17,17,17,17,14}},
/*V*/ {{17,17,17,17,17,10,4}},
/*W*/ {{17,17,17,21,21,27,17}},
/*X*/ {{17,17,10,4,10,17,17}},
/*Y*/ {{17,17,10,4,4,4,4}},
/*Z*/ {{31,1,2,4,8,16,31}},
/*[*/ {{14,8,8,8,8,8,14}},
/*x5C*/ {{0,16,8,4,2,1,0}},
/*]*/ {{14,2,2,2,2,2,14}},
/*^*/ {{4,10,17,0,0,0,0}},
/*_*/ {{0,0,0,0,0,0,31}},
/*`*/ {{8,4,2,0,0,0,0}},
/*a*/ {{14,17,17,31,17,17,17}},
/*b*/ {{30,17,17,30,17,17,30}},
/*c*/ {{14,17,16,16,16,17,14}},
/*d*/ {{28,18,17,17,17,18,28}},
/*e*/ {{31,16,16,30,16,16,31}},
/*f*/ {{31,16,16,30,16,16,16}},
/*g*/ {{14,17,16,23,17,17,15}},
/*h*/ {{17,17,17,31,17,17,17}},
/*i*/ {{14,4,4,4,4,4,14}},
/*j*/ {{7,2,2,2,2,18,12}},
/*k*/ {{17,18,20,24,20,18,17}},
/*l*/ {{16,16,16,16,16,16,31}},
/*m*/ {{17,27,21,21,17,17,17}},
/*n*/ {{17,17,25,21,19,17,17}},
/*o*/ {{14,17,17,17,17,17,14}},
/*p*/ {{30,17,17,30,16,16,16}},
/*q*/ {{14,17,17,17,21,18,13}},
/*r*/ {{30,17,17,30,20,18,17}},
/*s*/ {{15,16,16,14,1,1,30}},
/*t*/ {{31,4,4,4,4,4,4}},
/*u*/ {{17,17,17,17,17,17,14}},
/*v*/ {{17,17,17,17,17,10,4}},
/*w*/ {{17,17,17,21,21,27,17}},
/*x*/ {{17,17,10,4,10,17,17}},
/*y*/ {{17,17,10,4,4,4,4}},
/*z*/ {{31,1,2,4,8,16,31}},
/*{*/ {{6,4,4,8,4,4,6}},
/*|*/ {{4,4,4,4,4,4,4}},
/*}*/ {{12,4,4,2,4,4,12}},
/*~*/ {{0,0,8,21,2,0,0}},
};
const Glyph* glyphs() { return G; }
int width() { return 5; }
int height() { return 7; }
int glyphCount() { return 95; }
int measure(const std::string& s, int spacing) {
  if (s.empty()) return 0;
  return (int)s.size() * (5 + spacing) - spacing;
}
// Pack all glyphs into 95*7 = 665 bytes, row-major (glyph 0 row 0 first).
// Each byte = one row, MSB = leftmost pixel.
std::vector<uint8_t> pack() {
  std::vector<uint8_t> out;
  out.reserve(95 * 7);
  for (int g = 0; g < 95; ++g)
    for (int r = 0; r < 7; ++r)
      out.push_back(G[g].rows[r]);
  return out;
}
}
float scrollOffset(double seconds, float beatPhase, float windowWidthPx,
                    int textWidthPx, float musicLevel) {
  // The scroll speed is CONSTANT (90 px/s) — deliberately NOT eased by beat or
  // music level. The old code did speed = 90*(0.7+0.3*(1-beatPhase)) +
  // music*40 and returned fmod(seconds*speed, wrap). Because `speed` changes
  // every frame (beatPhase/musicLevel are live), seconds*speed teleported the
  // marquee forward/backward each frame instead of advancing smoothly — a
  // visible stutter/jump. A constant speed makes the offset a continuous,
  // monotonic function of time (fmod of a linear distance). beatPhase and
  // musicLevel are kept in the signature for ABI compatibility but unused.
  (void)beatPhase; (void)musicLevel;
  float speed = 90.f;
  // Use double for the long-running distance/wrap: (float)seconds loses
  // sub-pixel precision after a few minutes, causing scroll jitter. Double
  // keeps the in-cycle remainder stable for hours.
  double wrap = (double)windowWidthPx + (double)textWidthPx;
  if (wrap <= 0.0) return 0.f;
  double dist = seconds * (double)speed;
  // In-cycle remainder [0, wrap): the text scrolls left continuously,
  // and re-enters from the right once per wrap.
  return (float)std::fmod(dist, wrap);
}
