#pragma once
#include "OpusEffects.h"
#include <math.h>

// Drei Musik-Effekte von Claude Opus 5.5 (Anthropic), 02.11.2026. Alle drei leuchten nur mit
// Musik: Bei Stille gehen sie in wenigen Sekunden ganz aus (Ausgabe exakt 0, kein Grundglimmen).
//   mode 0 "Nachtbluete"     (63, ruhig): Eine Bluete oeffnet sich mit dem Klang aus der Mitte
//          (Gold innen, Rose, Flieder aussen), Bluetenringe ziehen langsam nach aussen; ein Beat
//          laesst sie kurz weich aufbluehen.
//   mode 1 "Klangpolarlicht" (64, ruhig): Drei Lichtvorhaenge wie beim Polarlicht, die nur mit
//          Musik leuchten; jeder folgt der Lautstaerke verschieden traege, mit Musik ziehen sie
//          schneller. Beats lassen die Strahlen kurz heller schimmern.
//   mode 2 "Lichttanz"       (65, lebhaft): Fuenf farbige Lichtkugeln, die jeder Beat anstoesst;
//          sie prallen an den Enden ab, leuchten mit der Lautstaerke, blitzen beim Anstoss kurz
//          weiss auf, und ihre Farben wandern mit der Musik langsam durch den Farbkreis.
// Kein Heap, kein delay, kein Zufall; vorzeichenlose Zeitdifferenzen ueberleben den
// millis()-Ueberlauf. Gerauschspitzen in der Stille bleiben unter der Schwelle (dunkel).
namespace music_scenes {
using chatgpt_effect::Color;
using chatgpt_effect::blend;
using chatgpt_effect::glow;
using chatgpt_effect::wave;
using opus_effect::Light;

static const uint32_t CYCLE = 720000;
static const uint8_t BALLS = 5;

inline uint8_t out(uint16_t v, uint32_t gain) {
  const uint32_t scaled = uint32_t(v > 255 ? 255 : v) * gain / 255;
  return scaled > 255 ? 255 : scaled;
}

class Renderer {
  uint32_t last_ = 0, flow_ = 0;
  uint32_t fast_ = 0, soft_ = 0, mid_ = 0, slow_ = 0;  // Pegel * 256, verschieden traege
  uint16_t bloom_ = 0, rise_ = 0;           // Beat-Aufbluehen 0..255, verbleibende Anstiegszeit (ms)
  uint8_t beats_ = 0;
  float pos_[BALLS], vel_[BALLS];
  uint16_t flare_[BALLS];

  // Pegel folgt mit eigener Anstiegs-/Abklingzeit (ms).
  static void follow(uint32_t &level, uint8_t target, uint32_t delta, uint32_t rise, uint32_t fall) {
    const int32_t goal = int32_t(target) << 8;
    const uint32_t tau = goal > int32_t(level) ? rise : fall;
    const uint32_t step = delta > tau ? tau : delta;
    level = uint32_t(int32_t(level) + (goal - int32_t(level)) * int32_t(step) / int32_t(tau));
  }
  // Schwelle gegen Rauschspitzen; darunter exakt 0.
  static uint32_t gated(uint32_t level, uint32_t gate) {
    const uint32_t v = level >> 8;
    return v > gate ? (v - gate) * 255 / (255 - gate) : 0;
  }

public:
  Renderer() { reset(0); }
  void reset(uint32_t now) {
    last_ = now; flow_ = fast_ = soft_ = mid_ = slow_ = 0;
    bloom_ = 0; rise_ = 0; beats_ = 0;
    for (uint8_t k = 0; k < BALLS; ++k) {
      pos_[k] = (k + 0.5f) / BALLS; vel_[k] = 0; flare_[k] = 0;
    }
  }

  template<class Pixel>
  void render(Pixel *pixels, uint16_t count, uint32_t now, uint8_t mode,
              uint8_t loudness, bool beat) {
    const uint32_t delta = now - last_;
    last_ = now;
    const uint32_t step = delta > 200 ? 200 : delta;  // nach langer Pause nicht springen
    follow(fast_, loudness, delta, 50, 400);
    follow(soft_, loudness, delta, 150, 700);
    follow(mid_, loudness, delta, 300, 1200);
    follow(slow_, loudness, delta, 800, 2500);
    // Beat: 0,25 s weich ansteigen, dann 0,8 s abklingen; ein neuer Beat setzt nur den Anstieg
    // fort, damit es nie springt.
    if (beat) { rise_ = 250; ++beats_; }
    if (rise_) {
      const uint32_t up = uint32_t(step) * 255 / 250;
      bloom_ = bloom_ + up > 255 ? 255 : bloom_ + up;
      rise_ = rise_ > step ? rise_ - step : 0;
    } else {
      const uint32_t down = uint32_t(step) * 255 / 800;
      bloom_ = bloom_ > down ? bloom_ - down : 0;
    }

    if (mode == 0) {
      const uint32_t open = gated(slow_, 8);
      const uint32_t petal = open * (510 - open) / 255;  // auch leise Musik oeffnet sichtbar
      // Bluetenringe ziehen mit der Musik nach aussen (stetig aufsummiert, kein Sprung).
      flow_ = (flow_ + step * (40 + petal * 200 / 255) / 40) % CYCLE;
      const uint32_t lift = 255 + uint32_t(bloom_) * open / 255 * 40 / 255;  // Beat: bis +16 %
      const int32_t reach = int32_t(petal * 1950 / 255 + uint32_t(bloom_) * open / 255 * 150 / 255);
      for (uint16_t i = 0; i < count; ++i) {
        const uint16_t x = count > 1 ? uint32_t(i) * 4096 / (count - 1) : 2048;
        const int32_t d = x > 2048 ? x - 2048 : 2048 - x;
        uint32_t shape = d <= reach ? 255 : glow(d - reach, 380);
        if (!open) shape = 0;
        const uint32_t rel = reach > 0 ? (d < reach ? uint32_t(d) * 255 / reach : 255) : 255;
        const Color c = rel < 128 ? blend({255, 170, 60}, {225, 60, 115}, rel * 2)
                                  : blend({225, 60, 115}, {125, 50, 205}, (rel - 128) * 2);
        const uint32_t rings = 150 + wave(uint32_t(d) * 6 + 65536UL - flow_ * 4096UL / 6000) * 105 / 255;
        Light l;
        l.add(c, shape * rings / 255 * petal / 255 * lift / 255);
        // Kern: warmes Licht, das mit dem Beat aufbluet.
        l.add({255, 215, 150}, uint32_t(glow(d, 260)) * open / 255 * (60 + bloom_ / 3) / 255);
        pixels[i].r = out(l.r, 255); pixels[i].g = out(l.g, 255); pixels[i].b = out(l.b, 255);
      }
    } else if (mode == 1) {
      static const Color curtain[3] = {{30, 220, 120}, {20, 140, 200}, {150, 50, 220}};
      const uint32_t lit[3] = {gated(soft_, 12), gated(mid_, 8), gated(slow_, 8)};
      const uint32_t drive = gated(mid_, 8);
      flow_ = (flow_ + step * (60 + drive * 200 / 255) / 60) % CYCLE;
      const uint32_t shimmer = bloom_;  // Beat: Strahlen kurz heller
      int32_t centre[3];
      for (uint8_t k = 0; k < 3; ++k) {
        static const uint32_t pathA[3] = {36000, 45000, 60000}, pathB[3] = {24000, 30000, 90000};
        const float turn = 6.2831853f * flow_;
        centre[k] = 2048 + int32_t(1400.0f * sinf(turn / pathA[k] + k * 2.1f)
                                   + 640.0f * sinf(turn / pathB[k] + k * 1.07f));
      }
      for (uint16_t i = 0; i < count; ++i) {
        const uint16_t x = count > 1 ? uint32_t(i) * 4096 / (count - 1) : 2048;
        Light l;
        uint32_t total = 0;
        for (uint8_t k = 0; k < 3; ++k) {
          if (!lit[k]) continue;
          const uint32_t rays = 120 + wave(x * 19UL + flow_ * 4096UL / 16000 + k * 900) * (135 + shimmer / 3) / 255;
          const uint32_t a = uint32_t(glow(int32_t(x) - centre[k], 1000)) * lit[k] / 255 * rays / 255;
          l.add(curtain[k], a);
          total += a;
        }
        if (total > 255) l.add({150, 255, 200}, (total - 255) / 3);
        pixels[i].r = out(l.r, 255); pixels[i].g = out(l.g, 255); pixels[i].b = out(l.b, 255);
      }
    } else {
      const uint32_t glowLevel = gated(fast_, 16);
      const float dt = step / 1000.0f;
      if (glowLevel) flow_ = (flow_ + step) % CYCLE;  // Farben wandern nur mit Musik (40 s je Runde)
      if (beat) {
        // Jeder Beat stoesst zwei Kugeln in ihrer Laufrichtung an (abwechselnd alle fuenf).
        const float push = 0.12f + 0.18f * (fast_ >> 8) / 255.0f;  // Streifenlaengen pro Sekunde
        for (uint8_t j = 0; j < 2; ++j) {
          const uint8_t k = (beats_ * 2 + j) % BALLS;
          vel_[k] += vel_[k] < 0 || (vel_[k] == 0 && (k & 1)) ? -push : push;
          flare_[k] = 255;
        }
      }
      for (uint8_t k = 0; k < BALLS; ++k) {
        // Musik haelt sie in Bewegung, Reibung bremst; an den Enden prallen sie ab.
        const float drift = 0.04f * gated(mid_, 8) / 255.0f;
        if (vel_[k] >= 0 && vel_[k] < drift) vel_[k] = drift;
        if (vel_[k] < 0 && vel_[k] > -drift) vel_[k] = -drift;
        vel_[k] *= 1.0f - 1.2f * dt;
        if (vel_[k] > 0.6f) vel_[k] = 0.6f;
        if (vel_[k] < -0.6f) vel_[k] = -0.6f;
        pos_[k] += vel_[k] * dt;
        if (pos_[k] < 0) { pos_[k] = -pos_[k]; vel_[k] = -vel_[k]; }
        if (pos_[k] > 1) { pos_[k] = 2 - pos_[k]; vel_[k] = -vel_[k]; }
        flare_[k] = flare_[k] > step * 255 / 300 ? flare_[k] - step * 255 / 300 : 0;
      }
      const uint16_t span = count > 1 ? count - 1 : 1;
      // Breite: etwa 1/40 des Streifens, mindestens 1,5 LEDs; schnelle Kugeln ziehen breiter.
      const float base = count / 40.0f > 1.5f ? count / 40.0f : 1.5f;
      for (uint16_t i = 0; i < count; ++i) {
        Light l;
        if (glowLevel) {
          for (uint8_t k = 0; k < BALLS; ++k) {
            const float width = base * (1.0f + 2.0f * fabsf(vel_[k]));
            const float dist = fabsf(i - pos_[k] * span) / width;
            if (dist >= 1) continue;
            const uint32_t v = uint32_t((1 - dist) * (1 - dist) * 255);
            const Color tint = opus_effect::hue(k * 819 + flow_ * 4096UL / 40000);
            l.add(tint, v * glowLevel / 255);
            l.add({255, 255, 255}, v * flare_[k] / 255 * glowLevel / 255 / 2);
          }
        }
        pixels[i].r = out(l.r, 255); pixels[i].g = out(l.g, 255); pixels[i].b = out(l.b, 255);
      }
    }
  }
};
} // namespace music_scenes
