#pragma once
#include "ChatGptEffect.h"

// Drei kontinuierliche Szenen ohne Audio-/Zufalls-/Wartezustand. Alle
// Positions- und Zeitphasen sind normiert; kein Bildpuffer oder Heap pro Frame.
namespace quiet_effects {
using chatgpt_effect::Color;
using chatgpt_effect::blend;
using chatgpt_effect::glow;
using chatgpt_effect::wave;

// Feine Nachtglut-Helfer: 32-Bit-Phase und weicher Verlauf 0..2^31, damit sehr kleine
// Rohwerte (wenige Stufen) weder in Einzelschritten springen noch durch Rundung
// zwischen zwei Wellen an den Umkehrpunkten zittern.
inline uint32_t phase32(uint32_t elapsed, uint32_t period) {
  return uint32_t((uint64_t(elapsed % period) << 32) / period);
}
inline uint64_t fineWave(uint32_t phase) {
  const uint64_t t = (phase < 0x80000000u ? phase : 0xFFFFFFFFu - phase); // 0..2^31
  return (t * t >> 31) * ((3ULL << 31) - 2 * t) >> 31;                    // Smoothstep 0..2^31
}

// Weicher Uebergang 0..2^31 fuer x in 0..2^31 (Smoothstep).
inline uint64_t smooth31(uint64_t x) {
  return (x * x >> 31) * ((3ULL << 31) - 2 * x) >> 31;
}

class Renderer {
  uint32_t last_ = 0, elapsed_ = 0;
  uint16_t entrance_ = 0;
public:
  void reset(uint32_t now) { last_ = now; elapsed_ = 0; entrance_ = 0; }
  template<class Pixel> void render(Pixel *pixels, uint16_t count, uint32_t now, uint8_t scene) {
    const uint32_t delta = now-last_; last_ = now;
    elapsed_ = (elapsed_ + delta%240000) % 240000;
    const uint16_t fade = scene==1 ? 5000 : 3000;
    entrance_ = entrance_ >= fade || delta >= uint32_t(fade-entrance_) ? fade : entrance_+delta;
    const uint16_t tide = (elapsed_%40000)*4096UL/40000;
    const uint16_t undertow = (elapsed_%60000)*4096UL/60000;
    const uint16_t slow = (elapsed_%120000)*4096UL/120000;
    for (uint16_t i=0;i<count;++i) {
      const uint16_t x = count>1 ? uint32_t(i)*4096/(count-1) : 2048;
      Color c;
      if (scene==0) {
        // Gezeiten: breite, gegenlaeufige Wasserbewegungen statt heller Spitzen.
        const uint8_t a = wave(x*2UL+tide);
        const uint8_t b = wave(x+65536UL-undertow);
        c = blend({0,5,18},{0,45,60},30+uint16_t(a)*120/255);
        c = blend(c,{12,100,96},uint32_t(b)*b*110/65025);
        const uint8_t foam = uint16_t(glow(int32_t(a)-200,55))*wave(x+slow)/255;
        c = blend(c,{85,75,35},uint16_t(foam)*28/255);
      } else if (scene==1) {
        // Nachtglut: absichtlich sehr kleine Rohwerte (Rot 4..14) und kein Blau. Der
        // globale Helligkeitsregler bleibt unberuehrt und kann weiter abdunkeln.
        // Die Helligkeit wird in 2^-31-Stufen berechnet (keine Rundungssprünge an den
        // Wellenumkehrpunkten) und mit einer festen Schwelle je LED gerundet
        // (raeumliches Dithering): kein zeitliches Rauschen, jede LED wechselt nur,
        // wenn das Feld ihre Schwelle durchquert.
        const uint32_t slowPhase = phase32(elapsed_, 120000);
        const uint32_t x32 = count>1 ? uint32_t(uint64_t(i)*0x100000000ULL/(count-1)) : 0x80000000u;
        const uint32_t drift = phase32(elapsed_, 80000);
        uint64_t level = (4ULL<<31) + fineWave(slowPhase)*6 + fineWave(x32+drift)*4;
        level = level*entrance_/fade;
        // Vereinzelt dunkle LEDs (Energie sparen, Schlafzimmer): jede LED hat eine eigene
        // Phase in einem 60-s-Takt, ist knapp ein Zehntel der Zeit ganz aus und blendet
        // dazwischen ueber mehrere Sekunden weich aus/ein. Kein Flackern, kein Zufall.
        const uint32_t own = uint32_t(uint32_t(i)*2246822519u+374761393u);
        const uint32_t ph = phase32(elapsed_, 60000)+own;
        const uint64_t tri = ph < 0x80000000u ? ph : 0xFFFFFFFFu-ph;   // 0..2^31
        const uint64_t lo = 128849019ULL, hi = 300647711ULL;            //  6 % aus, ab 14 % voll
        const uint64_t gate = tri <= lo ? 0 : tri >= hi ? (1ULL<<31)
                                                     : smooth31((tri-lo)*(1ULL<<31)/(hi-lo));
        level = level*(gate>>16)>>15;                                    // ohne 64-Bit-Ueberlauf
        const uint64_t threshold = uint32_t(uint32_t(i)*2654435761u)>>24; // 0..255, fest je LED
        pixels[i].r = uint8_t((level+(threshold<<23))>>31);
        pixels[i].g = uint8_t((level/7+(((threshold+128)&255)<<23))>>31);  // hoechstens 2
        pixels[i].b = 0;
        continue;
      } else {
        // Lichtseide: langsam verflochtene Mint-/Violettbaender mit Goldfaden.
        const uint8_t a = wave(x*3UL+(elapsed_%30000)*4096UL/30000);
        const uint8_t b = wave(x*2UL+65536UL-tide);
        Color silk = blend({68,14,90},{8,95,75},a);
        silk = blend(silk,{135,75,18},uint32_t(b)*b*160/65025);
        c = blend({2,2,12},silk,35+uint16_t(wave(x*5UL+undertow))*150/255);
        c = blend(c,{135,90,32},uint16_t(glow(int32_t(a)-b,40))*40/255);
      }
      pixels[i].r=uint32_t(c.r)*entrance_/fade;
      pixels[i].g=uint32_t(c.g)*entrance_/fade;
      pixels[i].b=uint32_t(c.b)*entrance_/fade;
    }
  }
};
} // namespace quiet_effects
