#version 410 core
in vec2 uv;
out vec4 FragColor;
uniform float uTime;
uniform float uBeatPhase;
uniform vec2 uResolution;
uniform float uMusicLevel;
uniform float uScrollOffset;
uniform float uTextWidth;
uniform float uOpacity;
// Static card: font scale, columns, rows, live beat intensity. Zero scale = marquee.
uniform vec4 uPanel;
// 95 glyphs (ASCII 32..126), 5x7 each, packed row-major into a 665-pixel
// texture (a 1x665 GL_R8 GL_TEXTURE_2D — Apple's Metal-based OpenGL driver
// does not support 1D textures, so a 1-row 2D texture is used instead).
// Each pixel = one row, MSB = leftmost column.
uniform sampler2D uFont;
// Per-character ASCII codes: a 1xN GL_R8 strip, N = number of characters in
// the current string. uCodes[i] = the ASCII code of character i (0-based).
// This is what maps a marquee POSITION to the correct glyph — the position
// alone (floor(fi)) is NOT the character code.
uniform sampler2D uCodes;
uniform int uCount;

// Sample one pixel of glyph g at (x 0..4, y 0..6).
float fontPix(float g, float x, float y){
  float idx = g*7.0 + y;
  float bit = 4.0 - x;                 // MSB = leftmost column
  // 1x665 texture: u = (idx+0.5)/665, v = 0.5 (the single row).
  // The GL_R8 read-back is the stored byte NORMALIZED to [0,1] (b -> b/255),
  // so scale it back to 0..255 and round before bit extraction. (The old code
  // divided the normalized float by 2^bit directly — since the float is < 1 and
  // 2^bit >= 1, floor() collapsed every bit except (sometimes) bit 0, corrupting
  // ~all glyph pixels. Same class of bug as the uCodes *255 fix.)
  float v = round(texture(uFont, vec2((idx + 0.5)/665.0, 0.5)).r * 255.0);
  // Extract bit `bit` of byte v: (v >> bit) & 1  ==  floor(v/2^bit) mod 2.
  // (GLSL built-in is `mod`, not C's `fmod` — Apple's stricter compiler rejects fmod.)
  return float((int(v) >> int(bit)) & 1);   // integer ops: pow(2,bit) is inexact on some GL drivers and floor() then drops pixels
}

void main(){
  vec2 px = uv * uResolution;
  if (uPanel.x > .5) {
    float s = uPanel.x;
    vec2 p = vec2(px.x, uResolution.y - px.y) - vec2(24);
    vec2 size = vec2(uPanel.y * 6.0 * s + 24.0, uPanel.z * 11.0 * s + 24.0);
    vec2 q = abs(p - size * .5) - (size * .5 - 9.0);
    float d = length(max(q, vec2(0))) + min(max(q.x, q.y), 0.0) - 9.0;
    float cover = 1.0 - smoothstep(-.5, 1.0, d);
    float edge = cover * smoothstep(-2.0, -.5, d);
    vec3 tint = mix(vec3(.20,.70,.78), vec3(.65,.38,.85), clamp(p.y / size.y,0,1));
    vec3 color = vec3(.018,.026,.044) + edge * tint * .35;
    float alpha = cover * .90;
    vec2 t = p - vec2(12);
    int col = int(floor(t.x / (6.0*s)));
    int row = int(floor(t.y / (11.0*s)));
    vec2 glyphXY = floor(mod(t, vec2(6.0,11.0)*s) / s);
    int ci = row * int(uPanel.y) + col;
    if (t.x >= 0 && t.y >= 0 && col < int(uPanel.y) && row < int(uPanel.z) &&
        ci < uCount && glyphXY.x < 5 && glyphXY.y < 7) {
      int code = int(round(texture(uCodes,vec2((float(ci)+.5)/2048.0,.5)).r*255.0));
      float ink = code >= 32 && code <= 126 ? fontPix(float(code-32),glyphXY.x,glyphXY.y) : 0.0;
      color = mix(color, row == 0 ? vec3(.48,.94,1) : vec3(.84,.89,.95), ink);
      alpha = max(alpha,ink);
    }
    // Slim activity rail below the text; only the rail reacts to beats.
    float rail = (1.0-smoothstep(1.0,2.0,abs(p.y-size.y+6.0))) *
                 step(12.0,p.x)*step(p.x,size.x-12.0);
    color += rail * tint * (.20 + .35*uPanel.w);
    FragColor = vec4(color * alpha, alpha) * uOpacity;
    return;
  }
  float scale = 6.0;                    // screen px per font px (larger, more readable)
  float glyphW = 5.0 * scale;           // 30 px
  float glyphH = 7.0 * scale;           // 42 px
  float spacing = 1.0 * scale;          // 6 px
  float cellW = glyphW + spacing;       // 36 px
  // Bottom-anchored band (like a marquee/news ticker), 24 px above the bottom.
  float margin = 24.0;
  float bandY0 = uResolution.y - glyphH - margin;
  float y = uResolution.y - px.y;       // top-down (GL uv.y is bottom-up): row 0 of a glyph is its top
  float a = 0.0;
  if(y >= bandY0 && y < bandY0 + glyphH){
    // Scroll left; wrap so the text re-enters from the right. uScrollOffset is
    // a positive in-cycle distance, so SUBTRACT it (text moves left over time).
    float period = uResolution.x + uTextWidth;
    float wrapped = mod(px.x - uScrollOffset, period);
    if(wrapped < uTextWidth){
      float fi = floor(wrapped / cellW);
      float fx = mod(wrapped, cellW);
      float gx = floor(fx / scale);
      float gy = floor((y - bandY0) / scale);
      if(gx >= 0.0 && gx < 5.0 && gy >= 0.0 && gy < 7.0){
        int ci = int(floor(fi));
        if(ci >= 0 && ci < uCount){
          // Sample the code strip for this character's ASCII code. GL_R8
          // normalizes the stored byte to [0,1] (b -> b/255), so the read-back
          // float must be SCALED BACK to 0..255 and rounded to recover the byte.
          // (The old `int(r + 0.5)` collapsed every printable char — all < 0.5 —
          // to code 0 -> glyph -32 -> blank marquee.)
          float u = (float(ci) + 0.5) / 2048.0;   // strip texture is allocated at a fixed 2048 texels, not uCount
          int code = int(texture(uCodes, vec2(u, 0.5)).r * 255.0 + 0.5);
          float glyph = float(code) - 32.0;   // char - 32 -> 0..94
          if(glyph >= 0.0 && glyph < 95.0){
            a = fontPix(glyph, gx, gy);
          }
        }
      }
    }
  }
  // Beat pulse: sharp flash on the downbeat (uBeatPhase=0), decays to a soft
  // floor. musicLevel adds a sustained glow. Scaled so col stays <= ~0.98 in
  // the 8-bit default framebuffer (no white clipping on strong beats).
  float beatFlash = pow(1.0 - clamp(uBeatPhase, 0.0, 1.0), 3.0);   // 1 at downbeat, ->0
  float music = clamp(uMusicLevel, 0.0, 1.0);
  float pulse = 0.74 + 0.16 * beatFlash + 0.08 * music;            // max ~0.98
  // Color: cyan-white base, shifts to hot pink with music + a beat flash.
  vec3 base = vec3(0.45, 0.90, 1.00);
  vec3 hot  = vec3(1.00, 0.55, 0.95);
  vec3 flash = vec3(1.00, 1.00, 1.00);
  vec3 col = mix(base, hot, music * 0.55 + beatFlash * 0.25);
  col = mix(col, flash, beatFlash * 0.45);
  col *= pulse;
  // Soft vertical fade at the band edges (8 px).
  float edgeFade = smoothstep(0.0, 8.0, y - bandY0) * smoothstep(0.0, 8.0, (bandY0 + glyphH) - y);
  // Subtle left/right screen-edge fade so text doesn't hard-clip at the borders.
  float xFade = smoothstep(0.0, 40.0, px.x) * smoothstep(0.0, 40.0, uResolution.x - px.x);
  float alpha = a * uOpacity * edgeFade * xFade;
  // PREMULTIPLIED output (rgb *= alpha) — the C++ blend is (ONE, ONE_MINUS_SRC_ALPHA).
  FragColor = vec4(col * alpha, alpha);
}
