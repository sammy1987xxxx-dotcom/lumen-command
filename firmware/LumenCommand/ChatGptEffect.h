#pragma once
#include <stdint.h>

// A one-dimensional choreography: gathering, weaving, insight, aurora.
// No allocation, delay, random state, or dependency on the physical strip shape.
namespace chatgpt_effect {
struct Color { uint8_t r, g, b; };
inline uint8_t mix(uint8_t a, uint8_t b, uint16_t amount) {
  return (uint32_t(a) * (255 - amount) + uint32_t(b) * amount) / 255;
}
inline Color blend(Color a, Color b, uint16_t amount) {
  return {mix(a.r,b.r,amount), mix(a.g,b.g,amount), mix(a.b,b.b,amount)};
}
inline uint8_t wave(uint32_t position) {
  uint16_t p = position & 4095;
  uint16_t v = (p < 2048 ? p : 4095 - p) >> 3;
  // Smoothstep rounds the corners of the triangle without floating point.
  return uint32_t(v) * v * (765 - 2 * v) / 65025;
}
inline uint8_t glow(int32_t distance, uint16_t width) {
  if (distance < 0) distance = -distance;
  if (distance >= width) return 0;
  const uint16_t v = uint32_t(width - distance) * 255 / width;
  return uint32_t(v) * v / 255;
}
inline Color scene(uint8_t stage, uint16_t x, uint32_t time) {
  const Color dark = {1,3,9}, mint = {18,205,142};
  const Color violet = {145,28,200}, cyan = {12,125,210}, gold = {235,145,32};
  const uint16_t radius = x > 2048 ? x - 2048 : 2048 - x;
  switch (stage) {
    case 0: {
      // Three pairs of ideas travel from the ends towards the middle.
      const uint16_t head = (time % 8000) * 2048 / 8000;
      uint8_t light = 0;
      for (uint8_t n=0; n<3; ++n) {
        const uint16_t position = (head + n * 683) % 2048;
        const uint8_t v = uint16_t(glow(int32_t(2048 - radius) - position, 320))
                          * wave(position * 2UL) / 255;
        if (v > light) light = v;
      }
      return blend(dark, x < 2048 ? mint : violet, light);
    }
    case 1: {
      const uint8_t a = wave(x * 6UL + time * 2);
      const uint8_t b = wave(x * 6UL + 65536UL - time * 2);
      Color braid = blend(violet, mint, a);
      return blend(dark, braid, 40 + uint16_t(a > b ? a : b) * 215 / 255);
    }
    case 2: {
      const uint8_t ring = wave(radius * 6UL + 131072UL - time * 3);
      return blend(blend(dark, cyan, 35), gold, uint16_t(ring) * ring / 255);
    }
    default: {
      const uint8_t curtain = wave(x * 3UL + time / 2);
      const Color sky = blend(violet, mint, wave(x * 2UL + 65536UL - time));
      return blend(dark, sky, 30 + uint16_t(curtain) * 180 / 255);
    }
  }
}

class Choreography {
  uint32_t last_ = 0, elapsed_ = 0;
  uint16_t beatAge_ = 2000, level_ = 0, entrance_ = 0;
public:
  void reset(uint32_t now) {
    last_ = now; elapsed_ = 0; beatAge_ = 2000; level_ = 0; entrance_ = 0;
  }
  template<class Pixel>
  void render(Pixel *pixels, uint16_t count, uint32_t now, uint8_t loudness, bool beat) {
    const uint32_t delta = now - last_; // Unsigned subtraction survives millis() rollover.
    last_ = now;
    elapsed_ = (elapsed_ + delta % 32000) % 32000;
    entrance_ = delta >= uint32_t(600 - entrance_) ? 600 : entrance_ + delta;
    beatAge_ = delta >= uint32_t(2000 - beatAge_) ? 2000 : beatAge_ + delta;
    if (beat) beatAge_ = 0;
    const uint16_t smoothing = delta > 250 ? 250 : delta;
    level_ = (uint32_t(level_) * (250 - smoothing) + uint32_t(loudness) * smoothing) / 250;
    const uint8_t stage = elapsed_ / 8000;
    const uint16_t local = elapsed_ % 8000;
    const uint16_t crossfade = local > 6800 ? uint32_t(local - 6800) * 255 / 1200 : 0;
    for (uint16_t i=0; i<count; ++i) {
      const uint16_t x = count > 1 ? uint32_t(i) * 4096 / (count - 1) : 2048;
      Color c = scene(stage, x, elapsed_);
      if (crossfade) c = blend(c, scene((stage + 1) % 4, x, elapsed_), crossfade);
      // Audio adds a soft expanding golden ring, never a full-strip white flash.
      if (beatAge_ < 1800) {
        const int32_t radius = x > 2048 ? x - 2048 : 2048 - x;
        const uint16_t ring = glow(radius - int32_t(beatAge_) * 2600 / 1800, 350);
        c = blend(c, {235,165,65}, uint32_t(ring) * (1800 - beatAge_) / 1800);
      }
      const uint16_t gain = (180 + level_ * 75 / 255) * uint32_t(entrance_) / 600;
      pixels[i].r = uint16_t(c.r) * gain / 255;
      pixels[i].g = uint16_t(c.g) * gain / 255;
      pixels[i].b = uint16_t(c.b) * gain / 255;
    }
  }
};
} // namespace chatgpt_effect
