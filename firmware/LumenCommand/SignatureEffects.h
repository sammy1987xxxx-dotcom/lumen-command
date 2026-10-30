#pragma once
#include "EffectStudio.h"

namespace signature_effects {
using chatgpt_effect::Color;
using chatgpt_effect::blend;
using chatgpt_effect::glow;
using chatgpt_effect::wave;
using effect_studio::add;
using effect_studio::scale;

// Zeitgesteuerte Goldimpulse nur fuer ChatGPT ohne Musik. Bei langen Pausen
// hoechstens ein Ereignis; keine nachgeholte Impulsfolge oder Audioabhaengigkeit.
class QuietGold {
  uint32_t last_ = 0;
  uint16_t remaining_ = 8000;
  uint8_t next_ = 0;
public:
  void reset(uint32_t now) { last_ = now; remaining_ = 8000; next_ = 0; }
  bool update(uint32_t now) {
    const uint32_t delta = now - last_; last_ = now;
    if (delta < remaining_) { remaining_ -= delta; return false; }
    static const uint16_t gaps[] = {11000,9000,8000};
    remaining_ = gaps[next_]; next_ = (next_ + 1) % 3;
    return true;
  }
};

inline uint8_t flowingTrail(int32_t behind, uint16_t tail=680) {
  return glow(behind, behind < 0 ? 210 : tail);
}
inline uint16_t circularDistance(uint16_t a, uint16_t b) {
  const uint16_t d = a > b ? a-b : b-a;
  return d > 2048 ? 4096-d : d;
}

class Renderer {
  uint32_t last_ = 0, elapsed_ = 0;
  uint16_t entrance_ = 0, level_ = 0;
  uint16_t pulses_[6] = {5000,5000,5000,5000,5000,5000};
  uint8_t next_ = 0;
public:
  void reset(uint32_t now) {
    last_ = now; elapsed_ = 0; entrance_ = level_ = 0; next_ = 0;
    for (auto &age:pulses_) age = 5000;
  }
  template<class Pixel> void render(Pixel *pixels, uint16_t count, uint32_t now,
                                     bool prism, uint8_t loudness, bool beat) {
    const uint32_t delta = now-last_; last_ = now;
    elapsed_ = (elapsed_ + delta % 120000) % 120000;
    entrance_ = delta >= uint32_t(600-entrance_) ? 600 : entrance_+delta;
    const uint16_t smooth = delta > 160 ? 160 : delta;
    level_ = (uint32_t(level_)*(160-smooth) + uint32_t(loudness)*smooth)/160;
    for (auto &age:pulses_) age = delta >= uint32_t(5000-age) ? 5000 : age+delta;
    if (prism && beat) { pulses_[next_] = 0; next_ = (next_+1)%6; }
    const uint16_t drift = (elapsed_%24000)*4096UL/24000;
    const uint16_t sky = (elapsed_%12000)*4096UL/12000;
    for (uint16_t i=0;i<count;++i) {
      const uint16_t x = count>1 ? uint32_t(i)*4096/(count-1) : 2048;
      Color c;
      if (!prism) {
        // Sternenfluss: tiefer Nachthimmel, breite Petrol-/Violettschleier,
        // vier gemuetlich ziehende Mint-/Goldkometen und ortsfeste Funkelsterne.
        c = blend({1,3,14}, blend({5,45,65},{34,7,58},wave(x+sky)),
                  55+uint16_t(wave(x*2UL+drift))*80/255);
        for (uint8_t n=0;n<4;++n) {
          const uint16_t head = (uint32_t(drift)*(1+n%2)+n*1024) & 4095;
          int32_t behind = (head+4096UL-x)&4095;
          if (behind>2048) behind-=4096;
          Color tint = blend({12,160,175},{235,150,40},wave(n*1024UL+sky));
          add(c,tint,uint16_t(flowingTrail(behind,540))*150/255);
        }
        for (uint8_t n=0;n<14;++n) {
          const uint16_t seed = effect_studio::hash(n*71UL+921);
          const uint16_t at = seed&4095;
          const uint16_t life = wave((elapsed_%8000)*4096UL/8000+seed);
          const uint8_t light = uint32_t(glow(circularDistance(x&4095,at),36))*life*life/65025;
          add(c,blend({70,125,210},{235,175,65},seed&255),light);
        }
      } else {
        // Prismenpuls: von der Mitte aus farbige Glasbaender, auch bei Stille.
        // Bereits vorhandenes Audio verstaerkt die Szene und setzt Goldringe.
        const uint16_t radius = x>2048 ? x-2048 : 2048-x;
        c = blend({3,1,14},{28,4,42},30+uint16_t(wave(radius*2UL+sky))*60/255);
        for (uint8_t n=0;n<3;++n) {
          const uint16_t step = (elapsed_+n*2000)%6000;
          const int32_t head = -500 + int32_t(step)*3900/6000;
          const uint16_t tintPhase = (elapsed_%30000)*4096UL/30000+n*1365;
          Color tint = blend({45,35,210},{15,190,175},wave(tintPhase));
          tint = blend(tint,{225,55,125},wave(tintPhase+1365)/2);
          const uint8_t strength = uint16_t(flowingTrail(head-radius,650))*(175+level_*55/255)/255;
          add(c,tint,strength);
          add(c,{235,145,32},uint16_t(glow(head-radius,110))*85/255);
        }
        for (const auto age:pulses_) if (age<2200) {
          const uint16_t ring = uint32_t(age)*2800/2200;
          add(c,{235,160,50},uint32_t(glow(int32_t(radius)-ring,260))*(2200-age)/2200);
        }
      }
      c = scale(c,uint32_t(entrance_)*255/600);
      pixels[i].r=c.r; pixels[i].g=c.g; pixels[i].b=c.b;
    }
  }
};
} // namespace signature_effects
