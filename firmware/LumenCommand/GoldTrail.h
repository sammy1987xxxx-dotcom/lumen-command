#pragma once
#include "ChatGptEffect.h"
namespace gold_trail {
inline int bound(int n,int lo,int hi) { return n<lo?lo:n>hi?hi:n; }
// One symmetric impulse, then a separate dark pause. No audio or allocation.
class Renderer {
  uint32_t last_=0, phase_=0;
public:
  void reset(uint32_t now) { last_=now; phase_=0; }
  template<class Pixel> void render(Pixel *pixels,uint16_t count,uint32_t now,
      int speed,int pause,int tail,int red,int green,int blue) {
    speed=bound(speed,10,200); pause=bound(pause,0,15000); tail=bound(tail,80,1200);
    const uint32_t duration=144000UL/speed;
    const uint32_t cycle=duration+pause;
    const uint32_t delta=now-last_;last_=now;
    phase_=(phase_%cycle+delta%cycle)%cycle;
    const int32_t head=int32_t(phase_)*(2048+tail)/duration;
    for(uint16_t i=0;i<count;++i) {
      const int32_t offset=int32_t(i)*2-int32_t(count-1);
      const int32_t radius=count>1?uint32_t(offset<0?-offset:offset)*2048/(count-1):0;
      const int32_t behind=head-radius;
      const uint8_t light=phase_>=duration?0:chatgpt_effect::glow(behind,behind<0?110:tail);
      pixels[i].r=uint16_t(bound(red,0,255))*light/255;
      pixels[i].g=uint16_t(bound(green,0,255))*light/255;
      pixels[i].b=uint16_t(bound(blue,0,255))*light/255;
    }
  }
};
}
