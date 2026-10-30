#pragma once
#include "ChatGptEffect.h"
#include <math.h>

// Drei Effekte von Claude Opus 5.5 (Anthropic), 30.10.2026:
//   mode 0 "Opus Resonanz"   (50, Musik): Jeder Beat schickt eine farbige Wellenfront
//          von der Mitte nach aussen, mit leisem Nachhall dahinter. Fronten laufen
//          ineinander und addieren sich wie Wasserwellen. Bei Stille bleibt nur ein
//          ruhiges, kaum sichtbares Grundglimmen (kein Flackern).
//   mode 1 "Opus Puls"       (51, Musik): Ein Herzschlag (Doppelschlag), der das Tempo
//          der Beats lernt und phasenrichtig weiterschlaegt, auch wenn einzelne Beats
//          fehlen. Farbe nach Tempo: langsam tiefrot, schnell bernstein. Ohne Beats
//          beruhigt er sich auf einen langsamen Ruhepuls.
//   mode 2 "Opus Polarlicht" (52, ohne Musik): Drei Lichtvorhaenge wandern langsam
//          ueber einen Nachthimmel, feine Strahlen ziehen durch sie hindurch, einzelne
//          Sterne funkeln. Mikrofon ohne Einfluss.
// Kein Heap, kein delay, kein Zufall; vorzeichenlose Zeitdifferenzen ueberleben den
// millis()-Ueberlauf. Alle Zeitperioden teilen 720 s, damit die Naht unsichtbar bleibt.
namespace opus_effect {
using chatgpt_effect::Color;
using chatgpt_effect::blend;
using chatgpt_effect::glow;
using chatgpt_effect::wave;

static const uint32_t CYCLE = 720000;

// Vollgesaettigter Farbkreis, 0..4095.
inline Color hue(uint32_t h) {
  h &= 4095;
  const uint16_t scaled = h * 6;
  const uint8_t f = (scaled & 4095) >> 4;
  switch (scaled >> 12) {
    case 0: return {255, f, 0};
    case 1: return {uint8_t(255 - f), 255, 0};
    case 2: return {0, 255, f};
    case 3: return {0, uint8_t(255 - f), 255};
    case 4: return {f, 0, 255};
    default: return {255, 0, uint8_t(255 - f)};
  }
}

// Additive Mischung: Licht addiert sich, Ueberlagerungen werden heller.
struct Light {
  uint16_t r = 0, g = 0, b = 0;
  void add(Color c, uint32_t amount) {
    if (amount > 255) amount = 255;
    r += uint32_t(c.r) * amount / 255;
    g += uint32_t(c.g) * amount / 255;
    b += uint32_t(c.b) * amount / 255;
  }
};

class Renderer {
  struct Front { uint16_t age, hue; uint8_t strength; };
  uint32_t last_ = 0, elapsed_ = 0, energy_ = 0;
  uint16_t entrance_ = 0, level_ = 0, sinceBeat_ = 4000;
  Front fronts_[10];  // 1,8 s Lebensdauer / 0,2 s Mindestabstand der Beats = 9 gleichzeitig
  uint8_t nextFront_ = 0;
  // Puls: gelerntes Tempo, Phase seit dem letzten Herzschlag, Sicherheit 0..4.
  uint16_t period_ = 1500, phase_ = 0, pulseAge_ = 4000, sinceRealBeat_ = 6000;
  uint8_t confidence_ = 0;

  static uint16_t add(uint16_t value, uint32_t delta, uint16_t limit) {
    return delta >= uint32_t(limit - value) ? limit : uint16_t(value + delta);
  }
  static uint8_t out(uint16_t v, uint32_t gain) {
    const uint32_t scaled = uint32_t(v > 255 ? 255 : v) * gain / 255;
    return scaled > 255 ? 255 : scaled;
  }

  void updatePulse(uint32_t delta, bool beat) {
    sinceRealBeat_ = add(sinceRealBeat_, delta, 6000);
    pulseAge_ = add(pulseAge_, delta, 4000);
    if (beat) {
      const uint16_t interval = sinceRealBeat_;
      sinceRealBeat_ = 0;
      if (interval >= 300 && interval <= 1500) {
        period_ = confidence_ ? uint16_t((uint32_t(period_) * 3 + interval) / 4) : interval;
        if (confidence_ < 4) ++confidence_;
      }
      // Phasenregelung: Beat kam frueh -> jetzt schlagen; spaet -> naechsten Schlag
      // um drei Viertel der Abweichung verschieben.
      const int32_t error = phase_ < period_ / 2 ? int32_t(phase_) : int32_t(phase_) - period_;
      if (confidence_ < 2 || error < -60) {
        phase_ = 0; pulseAge_ = 0;
      } else if (error > 0) {
        phase_ = uint16_t(phase_ - error * 3 / 4);
      }
    }
    if (sinceRealBeat_ >= 3000) {
      // Keine Musik mehr: Sicherheit weg, Tempo gleitet zum Ruhepuls (40/min).
      confidence_ = 0;
      const uint32_t step = delta > 1000 ? 1000 : delta;
      period_ = uint16_t(int32_t(period_) + (int32_t(1500) - period_) * int32_t(step) / 4000);
    }
    const uint32_t advanced = uint32_t(phase_) + delta % period_;
    if (advanced >= period_) { phase_ = advanced % period_; pulseAge_ = phase_; }
    else phase_ = advanced;
  }

  // Herzschlag-Huellkurve: schneller Anstieg (70 ms), weiches Ausklingen.
  static uint16_t beatShape(int32_t age) {
    if (age < 0 || age >= 420) return 0;
    return age < 70 ? age * 255 / 70 : (420 - age) * 255 / 350;
  }

public:
  Renderer() { reset(0); }
  void reset(uint32_t now) {
    last_ = now; elapsed_ = energy_ = 0; entrance_ = level_ = 0; sinceBeat_ = 4000;
    for (auto &f : fronts_) { f.age = 4000; f.hue = 0; f.strength = 0; }
    nextFront_ = 0;
    period_ = 1500; phase_ = 0; pulseAge_ = 4000; sinceRealBeat_ = 6000; confidence_ = 0;
  }

  template<class Pixel>
  void render(Pixel *pixels, uint16_t count, uint32_t now, uint8_t mode,
              uint8_t loudness, bool beat) {
    const uint32_t delta = now - last_;
    last_ = now;
    elapsed_ = (elapsed_ + delta % CYCLE) % CYCLE;
    entrance_ = add(entrance_, delta, 1500);
    const uint16_t smoothing = delta > 200 ? 200 : delta;
    if (mode == 2) { loudness = 0; beat = false; }
    level_ = (uint32_t(level_) * (200 - smoothing) + uint32_t(loudness) * smoothing) / 200;
    const uint32_t gain = uint32_t(entrance_) * 255 / 1500;

    if (mode == 0) {
      // Farbe wandert langsam (90 s) und mit der Energie der Musik schneller.
      energy_ = (energy_ + (delta > 1000 ? 1000 : delta) * level_) % 7650000UL;
      const uint16_t baseHue = (elapsed_ * 4096UL / 90000 + uint64_t(energy_) * 4096 / 7650000UL) & 4095;
      sinceBeat_ = add(sinceBeat_, delta, 4000);
      for (auto &f : fronts_) f.age = add(f.age, delta, 4000);
      if (beat && sinceBeat_ >= 150) {
        static const uint16_t offsets[] = {0, 520, 1040};
        Front &f = fronts_[nextFront_];
        f.age = 0;
        f.hue = (baseHue + offsets[nextFront_ % 3]) & 4095;
        f.strength = 140 + level_ * 115 / 255;
        nextFront_ = (nextFront_ + 1) % 10;
        sinceBeat_ = 0;
      }
      const Color deep = blend({2, 1, 6}, hue(baseHue + 2048), 40);
      const uint16_t glimmer = 10 + level_ * 60 / 255;
      for (uint16_t i = 0; i < count; ++i) {
        const uint16_t x = count > 1 ? uint32_t(i) * 4096 / (count - 1) : 2048;
        const int32_t radius = x > 2048 ? x - 2048 : 2048 - x;
        Light l;
        l.add({2, 1, 5}, 255);
        l.add(deep, uint32_t(wave(x * 3UL + elapsed_ * 4096UL / 24000)) * glimmer / 255);
        for (const auto &f : fronts_) {
          if (f.age >= 1800) continue;
          const int32_t front = int32_t(f.age) * 2300 / 1500;
          const uint32_t fade = uint32_t(1800 - f.age) * f.strength / 1800;
          const Color tint = hue(f.hue);
          l.add(tint, uint32_t(glow(radius - front, 220 + f.age / 6)) * fade / 255);
          // Nachhall: schmaler, schwaecherer Ring hinter der Front.
          l.add(blend(tint, {255, 240, 220}, 60), uint32_t(glow(radius - front + 520, 150)) * fade * 90 / 65025);
        }
        pixels[i].r = out(l.r, gain); pixels[i].g = out(l.g, gain); pixels[i].b = out(l.b, gain);
      }
    } else if (mode == 1) {
      updatePulse(delta, beat);
      const uint32_t bpm = 60000UL / period_;
      const uint16_t tempo = bpm <= 50 ? 0 : bpm >= 160 ? 255 : (bpm - 50) * 255 / 110;
      const Color heart = blend({200, 8, 36}, {255, 135, 18}, tempo);
      const Color hot = {255, 222, 185};
      const uint32_t strength = confidence_ ? 150 + level_ * 105 / 255 : 120;
      const int32_t lub = pulseAge_, dub = int32_t(pulseAge_) - int32_t(period_) * 22 / 100;
      const uint32_t lubLevel = beatShape(lub), dubLevel = beatShape(dub) * 150 / 255;
      const int32_t lubFront = lub * 1600 / 420, dubFront = dub * 1200 / 420;
      for (uint16_t i = 0; i < count; ++i) {
        const uint16_t x = count > 1 ? uint32_t(i) * 4096 / (count - 1) : 2048;
        const int32_t radius = x > 2048 ? x - 2048 : 2048 - x;
        Light l;
        l.add({4, 0, 1}, 255);
        // Adern: ein schwaches, langsam ziehendes Muster im Grundton.
        l.add(heart, uint32_t(wave(x * 4UL + elapsed_ * 4096UL / 30000)) * 14 / 255);
        uint32_t beatLight = uint32_t(glow(radius - lubFront, 520)) * lubLevel / 255
                           + uint32_t(glow(radius - dubFront, 400)) * dubLevel / 255;
        beatLight += uint32_t(glow(radius, 260)) * (30 + lubLevel * 120 / 255) / 255;
        beatLight = beatLight * strength / 255;
        l.add(heart, beatLight);
        l.add(hot, beatLight * beatLight / 255 * 150 / 255);
        pixels[i].r = out(l.r, gain); pixels[i].g = out(l.g, gain); pixels[i].b = out(l.b, gain);
      }
    } else {
      static const Color curtain[3] = {{18, 215, 95}, {10, 150, 175}, {125, 40, 205}};
      int32_t centre[3]; uint16_t intensity[3];
      for (uint8_t k = 0; k < 3; ++k) {
        static const uint32_t pathA[3] = {36000, 45000, 60000}, pathB[3] = {24000, 30000, 90000};
        // Gleitkomma statt wave(): dessen 8-Bit-Stufen waeren hier verstaerkt als
        // Ruckeln sichtbar. Nur drei Werte je Bild, nicht je LED.
        const float turn = 6.2831853f * elapsed_;
        centre[k] = 2048 + int32_t(1400.0f * sinf(turn / pathA[k] + k * 2.1f)
                                   + 640.0f * sinf(turn / pathB[k] + k * 1.07f));
        intensity[k] = 80 + uint16_t(87.5f * (1.0f + sinf(turn / pathB[(k + 1) % 3] + k * 3.1f)));
      }
      static const uint16_t twinkle[5] = {6000, 8000, 9000, 10000, 12000};
      for (uint16_t i = 0; i < count; ++i) {
        const uint16_t x = count > 1 ? uint32_t(i) * 4096 / (count - 1) : 2048;
        Light l;
        l.add({1, 2, 7}, 255);
        uint32_t total = 0;
        for (uint8_t k = 0; k < 3; ++k) {
          const uint32_t rays = 120 + wave(x * 19UL + elapsed_ * 4096UL / 16000 + k * 900) * 135 / 255;
          const uint32_t a = uint32_t(glow(int32_t(x) - centre[k], 950)) * intensity[k] / 255 * rays / 255;
          l.add(curtain[k], a);
          total += a;
        }
        // Wo Vorhaenge sich ueberlagern, leuchtet es heller und weisslicher.
        if (total > 255) l.add({150, 255, 200}, (total - 255) / 3);
        // Sterne: feste Stellen (Streuwert der Position), funkeln weich, nie hart.
        const uint32_t hash = uint32_t(uint32_t(i) * 2654435761u) >> 24;
        if (hash % 23 == 0) {
          const uint8_t sparkle = wave(elapsed_ * 4096UL / twinkle[hash % 5] + hash * 37);
          l.add({200, 215, 255}, 4 + uint32_t(sparkle) * sparkle / 255 * 34 / 255);
        }
        pixels[i].r = out(l.r, gain); pixels[i].g = out(l.g, gain); pixels[i].b = out(l.b, gain);
      }
    }
  }
};
} // namespace opus_effect
