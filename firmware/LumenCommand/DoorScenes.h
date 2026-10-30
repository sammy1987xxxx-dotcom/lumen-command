#pragma once
#include <stdint.h>
#include "DoorLight.h"

// Effekte mit Türzeichen (Claude Opus 5.5, 30.10.2026). Die Tür meldet per UDP offen/zu
// (siehe DoorLight.h); beide Effekte reagieren darauf unabhängig von „Alles AUS“.
//   53 "Ruhe mit Türzeichen" (ohne Musik): sehr dunkles, langsam atmendes Blau-Violett
//      zum Schlafen/Meditieren. Tür auf: weiche goldene Welle von der Tür über den ganzen
//      Schlauch, danach glüht der Türbereich, solange sie offen ist. Tür zu: Glühen klingt aus.
//   54 "ChatGPT Tür + Musik": ChatGPT (36) mit Musik; darüber dieselbe Begrüßungswelle,
//      solange die Tür offen ist starten Beats als Wellen an der Tür. Die Musik geht
//      zusätzlich an die Tür (WifiControl.cpp), deren LED-Streifen dann mitspielt.
// Welle und Türglühen legt Led.ino für alle Effekte gleich darüber (eigener Regler).
// Kein Heap, kein delay; vorzeichenlose Zeitdifferenzen überleben den millis()-Überlauf.
namespace door_scene {

static const uint16_t WAVE_SPEED = 160;      // LEDs pro Sekunde
static const uint16_t WAVE_WIDTH = 18;       // halbe Breite der Welle in LEDs
static const uint32_t GLOW_IN_MS = 800, GLOW_OUT_MS = 3000;
static const uint8_t RIPPLES = 4;

struct Color { uint8_t r, g, b; };

inline uint32_t hash32(uint32_t x) {
  x ^= x >> 16; x *= 0x7feb352dU; x ^= x >> 15; x *= 0x846ca68bU; x ^= x >> 16;
  return x;
}
inline uint8_t add8(uint8_t a, uint16_t b) { return a + b > 255 ? 255 : a + b; }
template<class Pixel> void addColor(Pixel &p, Color c, uint16_t level) {
  p.r = add8(p.r, uint32_t(c.r) * level / 255);
  p.g = add8(p.g, uint32_t(c.g) * level / 255);
  p.b = add8(p.b, uint32_t(c.b) * level / 255);
}
// Dreieck mit weichen Ecken, 0..255; distance in LEDs.
inline uint8_t bump(int32_t distance, uint16_t width) {
  if (distance < 0) distance = -distance;
  if (distance >= width) return 0;
  const uint16_t v = uint32_t(width - distance) * 255 / width;
  return uint32_t(v) * v / 255;
}

// Türzustand aus den UDP-Meldungen, nur im Render-Task benutzen.
class Signal {
 public:
  void open(uint32_t now) {
    if (open_) return;  // Wiederholte Meldung
    open_ = true; known_ = true; openedAt_ = now;
  }
  void close(uint32_t now) {
    if (!open_) return;
    open_ = false; closedAt_ = now;
  }
  bool isOpen() const { return open_; }
  // Strecke der Begrüßungswelle seit dem Öffnen in LEDs; nach 8 s vorbei.
  bool wave(uint32_t now, uint32_t &travelled) const {
    if (!known_ || now - openedAt_ > 8000) return false;
    travelled = (now - openedAt_) * WAVE_SPEED / 1000;
    return true;
  }
  // Etwas zu zeigen: Begrüßungswelle läuft oder die Tür glüht noch.
  bool visible(uint32_t now) const {
    uint32_t travelled = 0;
    return glow(now) > 0 || (wave(now, travelled) && travelled < 1100);
  }
  // Glühen über der Tür: an bei offen, klingt nach dem Schließen aus.
  uint16_t glow(uint32_t now) const {
    if (!known_) return 0;
    if (open_) {
      const uint32_t t = now - openedAt_;
      return t >= GLOW_IN_MS ? 255 : t * 255 / GLOW_IN_MS;
    }
    const uint32_t t = now - closedAt_;
    return t >= GLOW_OUT_MS ? 0 : (GLOW_OUT_MS - t) * 255 / GLOW_OUT_MS;
  }
 private:
  bool open_ = false, known_ = false;
  uint32_t openedAt_ = 0, closedAt_ = 0;
};

class Renderer {
 public:
  void reset(uint32_t now) {
    start_ = now;
    for (uint8_t i = 0; i < RIPPLES; ++i) ripple_[i] = 0;
    rippleUsed_ = 0;
  }

  // 53: Ruhe mit Türzeichen. Läuft mit voller Treiberstufe (Led.ino) und rechnet die
  // Helligkeit selbst: scale = limitLedOutput(Regler), 0..210.
  template<class Pixel>
  void renderCalm(Pixel *pixels, uint16_t count, uint32_t now, uint8_t scale) {
    const uint32_t t = now - start_;
    const uint32_t breath = t % 12000;  // 12 s Atemzug
    const uint32_t tri = breath < 6000 ? breath * 65535 / 6000 : (12000 - breath) * 65535 / 6000;
    const uint32_t soft = tri * tri / 65535;  // 0..65535
    for (uint16_t i = 0; i < count; ++i) {
      // Langsam wandernde, kaum sichtbare Ungleichmäßigkeit statt flacher Fläche.
      const uint32_t drift = bump(int32_t((i * 3 + t / 80) % 240) - 120, 120);
      // Stufe in 1/256: 6..20 wie zuvor, aber feiner. Jede LED rundet an einer eigenen,
      // festen Schwelle; so springen beim Atmen nicht alle LEDs zugleich eine sichtbare Stufe,
      // sondern nacheinander, und das Dimmen wirkt gleitend. Kein zeitliches Flackern.
      const uint32_t level = (6 * 256 + soft * 10 * 256 / 65535 + drift * 4 * 256 / 255) * scale / 210;
      const uint32_t h = hash32(i * 2654435761U + 17);
      pixels[i].r = uint8_t((level * 5 / 10 + (h & 255)) >> 8);
      pixels[i].g = uint8_t((level * 2 / 10 + ((h >> 8) & 255)) >> 8);
      pixels[i].b = uint8_t((level + ((h >> 16) & 255)) >> 8);
    }
  }

  // 54: über ChatGPT (bereits in pixels) die Beat-Wellen ab der Tür.
  template<class Pixel>
  void overlayMusic(Pixel *pixels, uint16_t count, uint32_t now, const Signal &door,
                    bool beat, uint8_t loudness) {
    if (beat && door.isOpen()) {
      ripple_[rippleUsed_ % RIPPLES] = now ? now : 1;  // 0 heißt „frei“; nie in die Zukunft
      ++rippleUsed_;
    }
    const door_light::Range range = door_light::doorRange();  // eingestellte Türlage
    const uint16_t first = range.first < count ? range.first : (count ? count - 1 : 0);
    const uint16_t last = range.last < count ? range.last : (count ? count - 1 : 0);
    const Color mint = {18, 205, 142};
    for (uint8_t r = 0; r < RIPPLES; ++r) {
      if (!ripple_[r]) continue;
      const uint32_t age = now - ripple_[r];
      if (age > 2500) { ripple_[r] = 0; continue; }
      const int32_t reach = int32_t(age * 220 / 1000);
      const uint16_t fade = uint32_t(2500 - age) * (120 + loudness / 2) / 2500;
      for (uint16_t i = 0; i < count; ++i) {
        const int32_t d = i < first ? first - i : (i > last ? i - last : 0);
        const uint8_t v = bump(d - reach, 10);
        if (v) addColor(pixels[i], mint, uint32_t(v) * fade / 255);
      }
    }
  }

  // Goldene Begrüßungswelle von der Tür in beide Richtungen und Glühen über der Tür.
  // Led.ino legt sie auch über alle anderen Effekte (auf einer Kopie des Bildes).
  template<class Pixel>
  static void overlayDoor(Pixel *pixels, uint16_t count, uint32_t now, const Signal &door,
                   uint8_t waveLevel, uint8_t glowLevel) {
    if (!count) return;
    const Color gold = {235, 145, 32};
    const door_light::Range range = door_light::doorRange();
    const uint16_t first = range.first < count ? range.first : count - 1;
    const uint16_t last = range.last < count ? range.last : count - 1;
    uint32_t travelled = 0;
    const bool wave = door.wave(now, travelled);
    // Welle wird zum Ende hin schwächer, damit sie ausläuft statt abzureißen.
    const uint16_t waveFade = wave && travelled < 1100 ? uint32_t(1100 - travelled) * waveLevel / 1100 : 0;
    const uint16_t glow = uint32_t(door.glow(now)) * glowLevel / 255;
    const uint32_t pulse = (now / 8) % 512;
    const uint16_t breathe = 180 + (pulse < 256 ? pulse : 511 - pulse) * 75 / 255;
    for (uint16_t i = 0; i < count; ++i) {
      const bool inDoor = i >= first && i <= last;
      const int32_t d = i < first ? first - i : (i > last ? i - last : 0);
      if (waveFade) {
        const uint8_t v = bump(d - int32_t(travelled), WAVE_WIDTH);
        if (v) addColor(pixels[i], gold, uint32_t(v) * waveFade / 255);
      }
      if (glow) {
        const uint16_t edge = inDoor ? 255 : bump(d, 8);
        if (edge) addColor(pixels[i], gold, uint32_t(glow) * edge / 255 * breathe / 255);
      }
    }
  }

 private:

  uint32_t start_ = 0;
  uint32_t ripple_[RIPPLES] = {};
  uint8_t rippleUsed_ = 0;
};

}  // namespace door_scene
