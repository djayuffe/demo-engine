#pragma once
#include <cstdint>
#include <string>
#include <vector>
// 5x7 bitmap font for ASCII 32..126 (95 glyphs). Each glyph = 7 bytes (rows),
// MSB = leftmost pixel. Zero new dependencies.
struct Glyph { uint8_t rows[7]; };
namespace textfont {
const Glyph* glyphs();          // 95 entries, index = char - 32
int width();                    // 5
int height();                   // 7
int glyphCount();               // 95
std::vector<uint8_t> pack();    // 95*7 = 665 bytes, row-major
// Measure the string in pixels at scale 1 (add spacing between chars).
int measure(const std::string& s, int spacing = 1);
}
// Compute the marquee scroll offset for a given time and window width.
// Returns pixels the text has moved left from x=0. Speed eases on the beat.
float scrollOffset(double seconds, float beatPhase, float windowWidthPx,
                   int textWidthPx, float musicLevel = 0.f);
