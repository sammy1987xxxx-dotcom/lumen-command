#pragma once
#include "ChatGptEffect.h"
#include <math.h>

// "Claude" (mit Musik) und "Claude ohne Musik": eine Szene als Bild fuer Denken
// und Antworten. Tiefes Dunkel atmet nach aussen, Lehm-/Daemmerfaeden weben sich
// zusammen, ein Schreibcursor zieht ein warmes Spurband von links nach rechts
// und gelegentlich blueht ein Einfall als gesegmenteter Strahlenring aus der Mitte.
// Ohne Audio setzt ein Zeitplan die Einfaelle (hoechstens einer je Aufruf, nie
// nachgeholt); mit Audio loest ein erkannter Beat ihn aus und die Lautstaerke
// hebt Faeden und Gesamthelligkeit. Kein Heap, kein delay, kein Zufall.
// Nur "Claude ohne Musik" dreht zusaetzlich die Farbtoene langsam durch den ganzen
// Farbkreis (Rot, Gelb, Gruen, Tuerkis, Blau, Violett ... in 240 s, nie alle zugleich),
// mit satteren Farben und versetzten Farbtoenen fuer Glut, Spurband und Daemmerton.
namespace claude_effect {
using chatgpt_effect::Color;
using chatgpt_effect::blend;
using chatgpt_effect::glow;
using chatgpt_effect::wave;

// Farbtondrehung um die Grauachse (Helligkeit bleibt etwa erhalten). Die Matrix wird
// einmal je Bild berechnet und nur auf die wenigen Palettenfarben angewendet.
struct HueRotation {
  float m[9];
  explicit HueRotation(float angle) {
    const float c = cosf(angle), s = sinf(angle);
    m[0] = 0.213f + c * 0.787f - s * 0.213f; m[1] = 0.715f - c * 0.715f - s * 0.715f; m[2] = 0.072f - c * 0.072f + s * 0.928f;
    m[3] = 0.213f - c * 0.213f + s * 0.143f; m[4] = 0.715f + c * 0.285f + s * 0.140f; m[5] = 0.072f - c * 0.072f - s * 0.283f;
    m[6] = 0.213f - c * 0.213f - s * 0.787f; m[7] = 0.715f - c * 0.715f + s * 0.715f; m[8] = 0.072f + c * 0.928f + s * 0.072f;
  }
  static uint8_t clamp(float v) { return v <= 0.0f ? 0 : v >= 255.0f ? 255 : uint8_t(v + 0.5f); }
  // Sattere Farbe bei gleicher Helligkeit: vom Grauwert weg um den Faktor k.
  static Color saturate(Color in, float k) {
    const float y = 0.299f * in.r + 0.587f * in.g + 0.114f * in.b;
    return {clamp(y + (in.r - y) * k), clamp(y + (in.g - y) * k), clamp(y + (in.b - y) * k)};
  }
  Color apply(Color in) const {
    return {clamp(m[0] * in.r + m[1] * in.g + m[2] * in.b),
            clamp(m[3] * in.r + m[4] * in.g + m[5] * in.b),
            clamp(m[6] * in.r + m[7] * in.g + m[8] * in.b)};
  }
};

inline void lay(Color &c, Color tint, uint32_t amount) {
  c = blend(c, tint, amount > 255 ? 255 : amount);
}

class Renderer {
  // Alle Zeitphasen teilen 40 s, damit die Naht beim Umlauf unsichtbar bleibt.
  uint32_t last_ = 0, elapsed_ = 0, hue_ = 0; // hue_: 240-s-Farbkreis (ohne Musik)
  // Mehrere Einfaelle gleichzeitig: Beats kommen alle 0,5 s, ein Ring lebt 2,6 s.
  // Mit nur einem Platz brach jeder neue Beat den laufenden Ring hart ab.
  static const uint8_t IDEAS = 6;
  uint16_t entrance_ = 0, level_ = 0, ideaAge_[IDEAS] = {4000, 4000, 4000, 4000, 4000, 4000}, nextIdea_ = 2500;
  uint8_t newestIdea_ = 0;
  uint8_t gap_ = 0;
public:
  void reset(uint32_t now) {
    last_ = now; elapsed_ = 0; hue_ = 0; entrance_ = level_ = 0;
    for (auto &age : ideaAge_) age = 4000;
    nextIdea_ = 2500; gap_ = 0; newestIdea_ = 0;
  }
  template<class Pixel>
  void render(Pixel *pixels, uint16_t count, uint32_t now, bool audio,
              uint8_t loudness, bool beat) {
    const uint32_t delta = now - last_; // Vorzeichenlos: ueberlebt den millis()-Ueberlauf.
    last_ = now;
    elapsed_ = (elapsed_ + delta % 40000) % 40000;
    hue_ = (hue_ + delta % 240000) % 240000; // 240 = 6 x 40 s: Naht bleibt unsichtbar
    entrance_ = delta >= uint32_t(800 - entrance_) ? 800 : entrance_ + delta;
    for (auto &age : ideaAge_) age = delta >= uint32_t(4000 - age) ? 4000 : age + delta;
    const uint16_t smoothing = delta > 200 ? 200 : delta;
    level_ = (uint32_t(level_) * (200 - smoothing) + uint32_t(audio ? loudness : 0) * smoothing) / 200;
    bool idea = false;
    if (audio) {
      idea = beat && ideaAge_[newestIdea_] >= 500;
    } else if (delta >= nextIdea_) {
      static const uint16_t gaps[] = {10000, 13000, 9000, 12000};
      nextIdea_ = gaps[gap_]; gap_ = (gap_ + 1) & 3;
      idea = true;
    } else {
      nextIdea_ = uint16_t(nextIdea_ - delta);
    }
    if (idea) {
      // Den aeltesten Platz neu belegen; bei mind. 0,5 s Abstand ist er laengst ausgeklungen.
      newestIdea_ = (newestIdea_ + 1) % IDEAS;
      ideaAge_[newestIdea_] = 0;
    }

    const Color dark = {3,1,5};
    Color cream = {250,232,205}, ember = {58,22,14}, clay = {217,119,87};
    Color sand = {235,170,120}, dusk = {26,40,105};
    if (!audio) {
      // Ohne Musik wandern die Farbtoene langsam durch den Regenbogen und sind deutlich
      // satter als das Lehm-Original. Glut/Faeden, Spurband und Daemmerton laufen mit
      // versetztem Farbton, so entstehen im Bild mehrere Farben statt einer. Dunkler
      // Grund bleibt; Creme (Kern, Cursor) bekommt nur einen Hauch der Farbe.
      const float angle = hue_ * (6.2831853f / 240000.0f);
      const HueRotation turn(angle), turnSand(angle + 0.8727f), turnDusk(angle - 0.6981f);
      ember = turn.apply(HueRotation::saturate(ember, 1.8f));
      clay = turn.apply(HueRotation::saturate(clay, 1.7f));
      sand = turnSand.apply(HueRotation::saturate(sand, 1.8f));
      dusk = turnDusk.apply(HueRotation::saturate(dusk, 1.5f));
      cream = blend(cream, sand, 60);
    }
    const uint32_t breathing = elapsed_ * 4096UL / 8000;
    const uint16_t think = wave(elapsed_ * 4096UL / 20000);
    const int32_t head = int32_t(elapsed_ % 20000) * 4700 / 20000 - 300;
    // Der Cursor blendet an beiden Enden seines 20-s-Wegs weich ein/aus.
    const uint32_t sweep = elapsed_ % 20000;
    const uint32_t cursor = (sweep < 20000 - sweep ? sweep : 20000 - sweep) >= 1500
                            ? 255 : (sweep < 20000 - sweep ? sweep : 20000 - sweep) * 255 / 1500;
    const uint16_t blink =150 + wave(elapsed_ * 4096UL / 1000) * 105 / 255; // 1-s-Takt
    const uint32_t gain = (205UL + level_ * 50UL / 255) * entrance_ / 800;
    for (uint16_t i=0; i<count; ++i) {
      const uint16_t x = count > 1 ? uint32_t(i) * 4096 / (count - 1) : 2048;
      const uint16_t radius = x > 2048 ? x - 2048 : 2048 - x;
      // Atem: ruhige Lehmglut, die sich von der Mitte nach aussen schiebt.
      Color c = blend(dark, ember, 30 + uint16_t(wave(radius * 2UL + 65536UL - breathing)) * 110 / 255);
      // Gedankenfaeden: zwei gegenlaeufige Wellen, hell nur wo sie sich treffen.
      const uint8_t a = wave(x * 5UL + elapsed_ * 4096UL / 10000);
      const uint8_t b = wave(x * 5UL + 65536UL - elapsed_ * 4096UL / 8000);
      const uint32_t weave = uint32_t(a) * b / 255;
      lay(c, dusk, uint32_t(b) * b * 45 / 65025);
      lay(c, blend(clay, sand, a), weave * weave / 255 * (50 + think * 130 / 255 + level_ * 75 / 255) / 255);
      // Schreibcursor mit warmer Spur dahinter; vor dem Cursor bleibt es dunkel.
      const int32_t behind = head - int32_t(x);
      lay(c, sand, uint32_t(glow(behind < 0 ? behind * 5 : behind, 1000)) * 70 / 255 * cursor / 255);
      lay(c, cream, uint32_t(glow(behind, 80)) * blink / 255 * 220 / 255 * cursor / 255);
      // Einfall: heller Kern und ein gesegmenteter Strahlenring, der ausklingt.
      for (const uint16_t age : ideaAge_) {
        if (age < 2600) {
          const int32_t ring = int32_t(radius) - int32_t(age * 2150UL / 2600);
          const uint32_t segments = 100 + wave(x * 8UL) * 155UL / 255;
          const uint32_t rise = age < 300 ? age * 255UL / 300 : 255;
          lay(c, blend(clay, cream, 140),
              uint32_t(glow(ring, 320)) * segments / 255 * (2600 - age) / 2600 * rise / 255);
          // Kurzer Anstieg statt Blitz: der Kern waechst in 250 ms und klingt dann aus.
          if (age < 1400) {
            const uint32_t core = age < 250 ? age * 255UL / 250 : (1400 - age) * 255UL / 1150;
            lay(c, cream, uint32_t(glow(radius, 300)) * core / 255 * 230 / 255);
          }
        }
      }
      pixels[i].r = uint32_t(c.r) * gain / 255;
      pixels[i].g = uint32_t(c.g) * gain / 255;
      pixels[i].b = uint32_t(c.b) * gain / 255;
    }
  }
};
} // namespace claude_effect
