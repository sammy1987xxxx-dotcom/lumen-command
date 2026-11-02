#pragma once
#include "OpusEffects.h"
#include <math.h>

// Fuenf Ruhe- und Schlafeffekte von Claude Opus 5.5 (Anthropic), 02.11.2026:
//   mode 0 "Atemfluss"    (55, ohne Musik): Atembegleiter mit 6 Atemzuegen pro Minute.
//          Einatmen 4 s: Licht waechst von der Mitte nach aussen (Lavendel -> Tuerkis),
//          1 s halten mit warmem Kern, Ausatmen 5 s: es zieht sich wieder zusammen.
//   mode 1 "Klangschale"  (56, Musik): Anschlaege und anschwellende Klaenge schicken sehr
//          langsame, lange nachklingende Ringe von der Mitte nach aussen. Der Grundton
//          folgt traege der Lautstaerke. In Stille schlaegt die Schale alle 16 s leise an.
//   mode 2 "Mondsee"      (57, Musik): Dunkles Wasser mit dem Spiegelbild des Mondes, das
//          leise glitzert. Musik (Pegel ueber 1,5 s geglaettet, keine Beats) macht das
//          Spiegelbild breiter und waermer, das Glitzern heller und die Wellen schneller.
//   mode 3 "Einschlafen"  (58, ohne Musik): Abendrot, das in 30 Minuten ueber Rosenrot zu
//          Glut wird, dabei langsamer atmet und ganz ausgeht. Kein Blauanteil.
//   mode 4 "Gluehwuermchen" (59, ohne Musik): Wenige gelbgruene Lichtpunkte glimmen an
//          wechselnden Stellen weich auf und verloeschen; dazwischen ist es ganz dunkel.
// Kein Heap, kein delay, kein Zufall (Streuwerte aus Position/Zeit); vorzeichenlose
// Zeitdifferenzen ueberleben den millis()-Ueberlauf. Wellenperioden teilen 720 s.
// 58 und 59 runden fein mit fester Schwelle je LED (kein zeitliches Dithering, siehe Led.ino).
namespace calm_effects {
using chatgpt_effect::Color;
using chatgpt_effect::blend;
using chatgpt_effect::glow;
using chatgpt_effect::wave;
using opus_effect::Light;

static const uint32_t CYCLE = 720000;
static const uint32_t SLEEP_MS = 1800000;  // Einschlafen: 30 min bis ganz dunkel

inline uint32_t scatter(uint32_t v) {
  v ^= v >> 16; v *= 0x7feb352dU; v ^= v >> 15; v *= 0x846ca68bU; v ^= v >> 16;
  return v;
}
inline float ease(float t) { return t <= 0 ? 0 : t >= 1 ? 1 : t * t * (3.0f - 2.0f * t); }
inline uint8_t out(uint16_t v, uint32_t gain) {
  const uint32_t scaled = uint32_t(v > 255 ? 255 : v) * gain / 255;
  return scaled > 255 ? 255 : scaled;
}

class Renderer {
  struct Ring { uint16_t age; uint8_t strength, tint; };
  uint32_t last_ = 0, elapsed_ = 0, clock_ = 0, asleep_ = 0, breath_ = 0, water_ = 0;
  uint32_t slow_ = 0, fast_ = 0;  // Pegel * 256
  uint16_t entrance_ = 0, sinceRing_ = 0;
  Ring rings_[6];  // 7 s Lebensdauer / 2,5 s Mindestabstand = hoechstens 3 gleichzeitig
  uint8_t nextRing_ = 0;

  static uint16_t add(uint16_t value, uint32_t delta, uint16_t limit) {
    return delta >= uint32_t(limit - value) ? limit : uint16_t(value + delta);
  }
  // Traege Glaettung ohne Gleitkomma: Zeitkonstante tau in ms.
  static void follow(uint32_t &level, uint8_t target, uint32_t delta, uint32_t tau) {
    const int32_t goal = int32_t(target) << 8;
    const uint32_t step = delta > tau ? tau : delta;
    level = uint32_t(int32_t(level) + (goal - int32_t(level)) * int32_t(step) / int32_t(tau));
  }
  void strike(uint8_t strength) {
    Ring &r = rings_[nextRing_];
    r.age = 0; r.strength = strength; r.tint = nextRing_ % 4;
    nextRing_ = (nextRing_ + 1) % 6;
    sinceRing_ = 0;
  }

public:
  Renderer() { reset(0); }
  void reset(uint32_t now) {
    last_ = now; elapsed_ = clock_ = asleep_ = breath_ = water_ = slow_ = fast_ = 0;
    entrance_ = 0; sinceRing_ = 14000; nextRing_ = 0;  // erster leiser Anschlag nach 2 s
    for (auto &r : rings_) { r.age = 7000; r.strength = 0; r.tint = 0; }
  }

  template<class Pixel>
  void render(Pixel *pixels, uint16_t count, uint32_t now, uint8_t mode,
              uint8_t loudness, bool beat) {
    const uint32_t delta = now - last_;
    last_ = now;
    elapsed_ = (elapsed_ + delta % CYCLE) % CYCLE;
    clock_ += delta;
    asleep_ = delta >= SLEEP_MS - asleep_ ? SLEEP_MS : asleep_ + delta;
    entrance_ = add(entrance_, delta, 3000);
    const uint32_t gain = uint32_t(entrance_) * 255 / 3000;
    if (mode != 1 && mode != 2) { loudness = 0; beat = false; }
    follow(slow_, loudness, delta, mode == 2 ? 1500 : 2500);
    follow(fast_, loudness, delta, 250);

    if (mode == 0) {
      // Atemkurve: 4 s ein, 1 s halten, 5 s aus (10 s, 6 Atemzuege/min).
      const uint32_t t = elapsed_ % 10000;
      const float r = t < 4000 ? ease(t / 4000.0f) : t < 5000 ? 1.0f : 1.0f - ease((t - 5000) / 5000.0f);
      const int32_t reach = int32_t((0.08f + 0.92f * r) * 2048);
      const Color breath = blend({70, 40, 140}, {20, 160, 150}, uint16_t(r * 255));
      const uint32_t warm = uint32_t(r * r * r * 110);
      for (uint16_t i = 0; i < count; ++i) {
        const uint16_t x = count > 1 ? uint32_t(i) * 4096 / (count - 1) : 2048;
        const int32_t d = x > 2048 ? x - 2048 : 2048 - x;
        Light l;
        l.add({1, 1, 5}, 255);
        const uint32_t inside = d <= reach ? 255 : glow(d - reach, 420);
        const uint32_t life = 185 + wave(x * 3UL + elapsed_ * 4096UL / 20000) * 70 / 255;
        l.add(breath, inside * life / 255);
        l.add({210, 140, 60}, uint32_t(glow(d, uint16_t(reach / 2 + 1))) * warm / 255);
        pixels[i].r = out(l.r, gain); pixels[i].g = out(l.g, gain); pixels[i].b = out(l.b, gain);
      }
    } else if (mode == 1) {
      sinceRing_ = add(sinceRing_, delta, 60000);
      for (auto &r : rings_) r.age = add(r.age, delta, 7000);
      // Anschlag: erkannter Beat oder ein Klang, der deutlich ueber den traegen Pegel
      // anschwillt; mindestens 2,5 s Abstand, damit es nie hektisch wird.
      const bool swell = fast_ > slow_ + slow_ / 3 + (20u << 8);
      if (sinceRing_ >= 2500 && (beat || swell))
        strike(130 + (fast_ >> 8) * 125 / 255);
      else if (sinceRing_ >= 16000)
        strike(90);
      static const Color tints[4] = {{235, 150, 70}, {170, 90, 210}, {60, 170, 180}, {230, 110, 120}};
      const uint32_t calm = 22 + wave(elapsed_ * 4096UL / 16000) * 16 / 255 + (slow_ >> 8) * 70 / 255;
      for (uint16_t i = 0; i < count; ++i) {
        const uint16_t x = count > 1 ? uint32_t(i) * 4096 / (count - 1) : 2048;
        const int32_t d = x > 2048 ? x - 2048 : 2048 - x;
        Light l;
        l.add({3, 2, 10}, 255);
        l.add({40, 22, 95}, calm * (200 + wave(x * 2UL + elapsed_ * 4096UL / 45000) * 55 / 255) / 255);
        for (const auto &r : rings_) {
          if (r.age >= 7000) continue;
          // Ausschwingen wie eine Schale: schnell los, immer langsamer, lange leise.
          const uint32_t inv = 1024 - uint32_t(r.age) * 1024 / 7000;
          const int32_t front = 2048 - int32_t(2048 * inv * inv >> 20);
          // Weicher Anschlag (0,6 s), damit es nie blitzt.
          const uint32_t attack = r.age < 600 ? r.age * 255u / 600 : 255;
          const uint32_t fade = (r.strength * inv * inv >> 20) * attack / 255;
          l.add(tints[r.tint], uint32_t(glow(d - front, 380 + r.age / 16)) * fade / 255);
          if (r.age < 1500)
            l.add({255, 225, 190}, uint32_t(glow(d, 300)) * fade * (1500 - r.age) / 1500 / 3 / 255);
        }
        pixels[i].r = out(l.r, gain); pixels[i].g = out(l.g, gain); pixels[i].b = out(l.b, gain);
      }
    } else if (mode == 2) {
      // Wurzelartige Kurve: auch leise Musik ist schon zu sehen, laute nicht grell.
      const uint32_t level = slow_ >> 8;
      const uint32_t swell = level * (510 - level) / 255;
      // Wellenzeit laeuft mit Musik bis 2,5-mal so schnell; stetig aufsummiert, kein Sprung.
      water_ = (water_ + (delta > 1000 ? 1000 : delta) * (256 + swell * 384 / 255) / 256) % CYCLE;
      const float turn = 6.2831853f * elapsed_;
      const int32_t moon = 2048 + int32_t(320.0f * sinf(turn / 180000));
      const uint16_t width = 480 + swell * 600 / 255;
      const uint32_t sparkle = 90 + swell * 290 / 255;
      const Color silver = blend({165, 180, 215}, {235, 175, 95}, swell * 150 / 255);
      const Color core = blend({220, 225, 235}, {255, 200, 120}, swell * 150 / 255);
      for (uint16_t i = 0; i < count; ++i) {
        const uint16_t x = count > 1 ? uint32_t(i) * 4096 / (count - 1) : 2048;
        const uint8_t a = wave(x * 2UL + water_ * 4096UL / 40000);
        const uint8_t b = wave(x * 5UL + 65536UL - water_ * 4096UL / 24000);
        Color water = blend({0, 2, 9}, {3, 20, 45}, 40 + uint16_t(a) * 120 / 255);
        water = blend(water, {6, 40, 60}, uint32_t(b) * b * 80 / 65025);
        Light l;
        l.add(water, 255);
        const uint32_t band = glow(int32_t(x) - moon, width);
        const uint32_t s = uint32_t(wave(x * 29UL + water_ * 4096UL / 8000))
                         * wave(x * 17UL + 65536UL - water_ * 4096UL / 12000) / 255;
        const uint32_t highlight = s * s / 255;
        l.add(silver, band * (25 + swell * 25 / 255 + highlight * sparkle / 255) / 255);
        l.add(core, uint32_t(glow(int32_t(x) - moon, 160)) * (30 + swell * 30 / 255) / 255);
        pixels[i].r = out(l.r, gain); pixels[i].g = out(l.g, gain); pixels[i].b = out(l.b, gain);
      }
    } else if (mode == 3) {
      if (asleep_ >= SLEEP_MS) {
        for (uint16_t i = 0; i < count; ++i) pixels[i].r = pixels[i].g = pixels[i].b = 0;
        return;
      }
      const float p = float(asleep_) / SLEEP_MS;
      const float dim = (1.0f - p) * (1.0f - p);  // wirkt fuers Auge gleichmaessig
      const float h = p < 0.5f ? p * 2 : (p - 0.5f) * 2;
      const float from[3] = {p < 0.5f ? 255.0f : 220.0f, p < 0.5f ? 120.0f : 50.0f, 0.0f};
      const float to[3] = {p < 0.5f ? 220.0f : 150.0f, p < 0.5f ? 50.0f : 15.0f, 0.0f};
      const uint32_t c[3] = {uint32_t(from[0] + (to[0] - from[0]) * h), uint32_t(from[1] + (to[1] - from[1]) * h),
                             uint32_t(from[2] + (to[2] - from[2]) * h)};
      // Atem verlangsamt sich von 7,5 auf gut 4 Zuege pro Minute (Phase stetig, kein Sprung).
      const uint32_t period = 8000 + uint32_t(6000 * p);
      breath_ = (breath_ + uint32_t(uint64_t(delta > 1000 ? 1000 : delta) * 65536 / period)) & 0xFFFF;
      const float b = 0.82f + 0.09f * (1.0f - cosf(6.2831853f * breath_ / 65536));
      const uint32_t frame = uint32_t(dim * b * (180.0f / 255) * 65536 * entrance_ / 3000);
      for (uint16_t i = 0; i < count; ++i) {
        const uint16_t x = count > 1 ? uint32_t(i) * 4096 / (count - 1) : 2048;
        const uint32_t s = 150 + wave(x * 2UL + elapsed_ * 4096UL / 60000) * 70 / 255
                         + wave(x * 3UL + 65536UL - elapsed_ * 4096UL / 90000) * 35 / 255;
        const uint32_t level = frame * s / 255;  // 0..65536
        const uint32_t threshold = (uint32_t(i) * 2654435761u) >> 24;  // fest je LED
        pixels[i].r = uint8_t((c[0] * level / 256 + threshold) >> 8);
        pixels[i].g = uint8_t((c[1] * level / 256 + threshold) >> 8);
        pixels[i].b = uint8_t((c[2] * level / 256 + threshold) >> 8);
      }
    } else {
      if (!count) return;
      static const uint16_t periods[5] = {7000, 9000, 11000, 13000, 17000};
      uint8_t flies = 2 + count / 30;
      if (flies > 20) flies = 20;
      int32_t at[20]; uint16_t shine[20];
      for (uint8_t k = 0; k < flies; ++k) {
        const uint32_t id = scatter(k * 7919u + 17);
        const uint32_t period = periods[id % 5];
        const uint32_t local = clock_ + id % period;
        const uint32_t t = local % period, round = local / period;
        // Aufglimmen 1,5 s, 0,8 s leuchten, 2,2 s verloeschen, danach Pause.
        shine[k] = t < 1500 ? uint16_t(ease(t / 1500.0f) * 255) : t < 2300 ? 255
                 : t < 4500 ? uint16_t((1.0f - ease((t - 2300) / 2200.0f)) * 255) : 0;
        const uint32_t spot = scatter(id ^ (round * 104729u));
        const int32_t drift = int32_t(t < 4500 ? t : 4500) * 512 / 4500;  // bis 2 LEDs
        at[k] = int32_t(spot % (uint32_t(count) * 256)) + ((spot >> 31) ? drift : -drift);
      }
      for (uint16_t i = 0; i < count; ++i) {
        uint32_t light = 0;
        for (uint8_t k = 0; k < flies; ++k)
          if (shine[k]) light += uint32_t(glow(int32_t(i) * 256 - at[k], 520)) * shine[k] / 255;
        if (light > 255) light = 255;
        const Color c = blend({40, 90, 0}, {190, 210, 40}, light);
        const uint32_t level = light * 115 * gain / 255;  // Spitze 115/255 der Farbe, 1/256-Stufen
        const uint32_t threshold = (uint32_t(i) * 2654435761u) >> 24;
        pixels[i].r = uint8_t((c.r * level / 255 + threshold) >> 8);
        pixels[i].g = uint8_t((c.g * level / 255 + threshold) >> 8);
        pixels[i].b = uint8_t((c.b * level / 255 + threshold) >> 8);
      }
    }
  }
};
} // namespace calm_effects
