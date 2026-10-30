#pragma once
#include "ClaudeEffect.h"

// Farbwanderung fuer einen fertig gerenderten Effekt: dreht alle Farbtoene gleichmaessig
// um die Grauachse (Helligkeit bleibt etwa erhalten). Kein Heap, kein delay, kein Zufall.
// "ChatGPT ohne Musik" (37) verschiebt so pro 32-s-Durchlauf den Farbton um 72 Grad.
namespace hue_drift {
class Clock {
  uint32_t last_ = 0, elapsed_ = 0;
public:
  void reset(uint32_t now) { last_ = now; elapsed_ = 0; }
  // Winkel (Bogenmass) im Kreis von `period` Millisekunden; vorzeichenlose Zeitdifferenz
  // ueberlebt den millis()-Ueberlauf, lange Pausen werden nicht nachgeholt.
  float angle(uint32_t now, uint32_t period) {
    const uint32_t delta = now - last_;
    last_ = now;
    elapsed_ = (elapsed_ + delta % period) % period;
    return elapsed_ * (6.2831853f / period);
  }
};

template<class Pixel> void rotate(Pixel *pixels, uint16_t count, float angle) {
  const claude_effect::HueRotation turn(angle);
  for (uint16_t i = 0; i < count; ++i) {
    const chatgpt_effect::Color c = turn.apply({pixels[i].r, pixels[i].g, pixels[i].b});
    pixels[i].r = c.r; pixels[i].g = c.g; pixels[i].b = c.b;
  }
}
} // namespace hue_drift
