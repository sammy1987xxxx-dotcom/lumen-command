#pragma once
#include "ChatGptEffect.h"

// Gemeinsame, zeitbasierte Zeichenwerkzeuge; jeder Effekt behält seine eigene Szene.
// Normierte Positionen statt LED-Sprüngen, keine Wartezeiten oder Heap-Bildpuffer.
namespace effect_studio {
using chatgpt_effect::Color;
using chatgpt_effect::blend;
using chatgpt_effect::glow;
using chatgpt_effect::wave;

inline uint16_t bounded(int value, uint16_t low, uint16_t high) {
  return value < low ? low : (value > high ? high : value);
}
inline uint16_t smaller(uint16_t a, uint16_t b) { return a < b ? a : b; }
inline uint16_t larger(uint16_t a, uint16_t b) { return a > b ? a : b; }
inline uint16_t magnitude(int32_t v) { return v < 0 ? -v : v; }
inline uint16_t hash(uint32_t v) {
  v ^= v >> 16; v *= 0x7feb352dUL; v ^= v >> 15; v *= 0x846ca68bUL;
  return (v ^ (v >> 16)) & 65535;
}
inline uint16_t scan(uint32_t phase) {
  const uint16_t p = phase & 4095;
  const uint16_t v = (p < 2048 ? p : 4095-p) * 2;
  return ((uint32_t(v) * v >> 12) * (12288UL - 2*v)) >> 12;
}
inline Color scale(Color c, uint16_t amount) {
  return {uint8_t(uint16_t(c.r)*amount/255), uint8_t(uint16_t(c.g)*amount/255),
          uint8_t(uint16_t(c.b)*amount/255)};
}
inline void add(Color &to, Color c, uint16_t amount=255) {
  to.r = smaller(255, to.r + uint16_t(c.r)*amount/255);
  to.g = smaller(255, to.g + uint16_t(c.g)*amount/255);
  to.b = smaller(255, to.b + uint16_t(c.b)*amount/255);
}
inline Color hue(uint32_t position) {
  // Satte Gegenfarben mit einem kleinen Weißanteil für weichere Farbmischungen.
  const uint16_t six = (position & 4095)*6;
  const uint8_t rise = (six & 4095)*215UL/4096 + 16;
  const uint8_t fall = 247-rise;
  switch (six >> 12) {
    case 0: return {231,rise,16};
    case 1: return {fall,231,16};
    case 2: return {16,231,rise};
    case 3: return {16,fall,231};
    case 4: return {rise,16,231};
    default:return {231,16,fall};
  }
}
inline uint8_t trail(uint16_t x, uint16_t head, uint16_t length, bool forward) {
  const int32_t behind = forward ? int32_t(head)-x : int32_t(x)-head;
  return glow(behind < 0 ? -behind*5 : behind, length);
}
inline uint8_t stars(uint32_t x, uint32_t time, uint16_t density, uint16_t salt=0) {
  const uint32_t grid = (x & 4095)*density;
  const uint16_t seed = hash((grid >> 12)+salt);
  const uint16_t center = 1024+(seed & 2047);
  const uint16_t life = wave(time+(seed >> 2));
  return uint32_t(glow(int32_t(grid & 4095)-center,950))*life*life/65025;
}

struct Settings {
  int speed=20, fade=40, density=50, scale=7, width=2, hueSpeed=1;
  int secondSpeed=80, secondScale=15;
  bool colorCycle=true;
  Color color={255,0,0};
};

class Renderer {
  struct Beat { uint16_t age=60000, origin=2048, color=0; };
  Beat beats_[16];
  uint8_t next_=0;
  uint32_t last_=0, motion_=0, second_=0, hue_=0;
  uint32_t trek_=0, noiseRandom_=0x6d2b79f5UL;
  uint16_t volume_=0, average_=0, entrance_=0, rocket_=60000;
  uint16_t noiseCount_=0, noiseHue_=0, noiseRemainder_=0;

  static uint16_t age(uint16_t value, uint32_t delta) {
    return delta >= uint32_t(60000-value) ? 60000 : value+delta;
  }
  uint32_t noiseRandom() {
    noiseRandom_^=noiseRandom_<<13; noiseRandom_^=noiseRandom_>>17; noiseRandom_^=noiseRandom_<<5;
    return noiseRandom_;
  }
  template<class Pixel>
  void renderNoise(Pixel *pixels, uint16_t count, uint32_t delta, uint8_t loudness,
                   uint16_t density, uint16_t entry) {
    // Wie beim ursprünglichen Rauschen: direkt pegelabhängig neue Zufallspunkte,
    // vorhandene Punkte klingen aus. Keine zweite Pegelglättung nach processAudio().
    for (uint16_t i=noiseCount_;i<count;++i) pixels[i].r=pixels[i].g=pixels[i].b=0;
    noiseCount_=count;
    if (!count) { noiseRemainder_=0; return; }
    if (delta>1000) {
      for (uint16_t i=0;i<count;++i) pixels[i].r=pixels[i].g=pixels[i].b=0;
    }
    const uint16_t elapsed=noiseRemainder_+(delta>250 ? 250 : delta);
    noiseRemainder_=elapsed%16;
    const uint16_t sparks=uint16_t(loudness)*density/255;
    for (uint16_t frame=0;frame<elapsed/16;++frame) {
      for (uint16_t i=0;i<count;++i) {
        pixels[i].r=uint16_t(pixels[i].r)*235/255;
        pixels[i].g=uint16_t(pixels[i].g)*235/255;
        pixels[i].b=uint16_t(pixels[i].b)*235/255;
      }
      const Color color=scale(hue(noiseHue_),entry);
      for (uint16_t spark=0;spark<sparks;++spark) {
        auto &pixel=pixels[noiseRandom()%count];
        pixel.r=color.r; pixel.g=color.g; pixel.b=color.b;
      }
      noiseHue_=(noiseHue_+16)&4095;
    }
  }
public:
  void reset(uint32_t now) {
    last_=now; motion_=second_=hue_=0; next_=0;
    volume_=average_=entrance_=0; rocket_=60000;
    trek_=0; noiseRandom_=0x6d2b79f5UL; noiseCount_=noiseHue_=noiseRemainder_=0;
    for (auto &b:beats_) b=Beat{};
  }

  template<class Pixel>
  void render(Pixel *pixels, uint16_t count, uint8_t id, uint32_t now,
              uint8_t loudness, uint8_t average, bool beat, const Settings &s) {
    const uint32_t delta=now-last_; last_=now;
    const uint16_t speed=bounded(s.speed,1,255), fade=bounded(s.fade,1,255);
    const uint16_t density=bounded(s.density,1,255), waves=bounded(s.scale,1,50);
    const uint16_t width=bounded(s.width,1,40), hueSpeed=bounded(s.hueSpeed,0,600);
    const uint16_t speed2=bounded(s.secondSpeed,1,255), waves2=bounded(s.secondScale,1,50);
    const uint32_t rate=id==27 ? 4096/speed : 20+speed*5;
    motion_=(motion_+uint64_t(delta)*rate) & 0xffffffUL;
    second_=(second_+uint64_t(delta)*(4096/speed2)) & 0xffffffUL;
    hue_=(hue_+uint64_t(delta)*hueSpeed*48) & 0xffffffUL;
    const uint32_t step=delta > 250 ? 250 : delta;
    const uint16_t response=(id==11 || id==21) ? 280-speed : (id==12 ? 20+(255-fade)*3 : 160);
    const uint32_t smoothing=step > response ? response : step;
    volume_=(uint32_t(volume_)*(response-smoothing)+(uint32_t(loudness)<<8)*smoothing)/response;
    average_=(uint32_t(average_)*(response-smoothing)+(uint32_t(average)<<8)*smoothing)/response;
    entrance_=smaller(600,age(entrance_,delta));
    rocket_=age(rocket_,delta);
    for (auto &b:beats_) b.age=age(b.age,delta);
    if (beat) {
      Beat &b=beats_[next_]; b.age=0; b.origin=hash(hue_+next_*991UL)&4095;
      b.color=(hue_>>10)+next_*733; next_=(next_+1)%16;
      if (rocket_>=6000) rocket_=0;
    }
    const uint16_t level=volume_>>8, mean=average_>>8;
    const uint16_t p=motion_>>8, p2=second_>>8, h=hue_>>10;
    const uint16_t tail=120+(255-fade)*4;
    const uint16_t trekHalo=larger(40,tail/5), trekLeg=2048+tail+trekHalo;
    if (id==8) trek_=(trek_+uint64_t(delta)*rate)%(uint32_t(trekLeg)*512);
    const uint16_t trekDistance=trek_>>8;
    const bool trekForward=trekDistance<trekLeg;
    const int32_t trekHead=trekForward ? int32_t(trekDistance)-trekHalo
        : int32_t(2048+trekHalo)-(trekDistance-trekLeg);
    if (id==23) {
      renderNoise(pixels,count,delta,loudness,density,entrance_*255UL/600);
      return;
    }
    const uint16_t head=scan(p), radial=scan(p)/2;
    const bool forward=(p & 4095)<2048;
    const Color mint={18,205,142}, violet={145,28,200}, cyan={12,125,210};
    const Color gold={235,145,32}, blue={10,45,230}, red={230,12,3};
    const Color base=s.color;
    const uint16_t newest=beats_[(next_+15)%16].age;
    for (uint16_t i=0;i<count;++i) {
      const uint16_t x=count>1 ? (i<count/2 ? uint32_t(i)*4096/(count-1)
                                  : 4096-uint32_t(count-1-i)*4096/(count-1)) : 2048;
      const uint16_t radius=magnitude(int32_t(x)-2048);
      Color c={0,0,0};
      switch (id) {
        case 1: { // Gegenläufige Regenbogenbänder statt ganzzahliger Farbsprünge.
          c=blend(hue(x*waves+p),hue(x*waves+65536UL-p),wave(x*2UL+p));
          c=scale(c,100+wave(x*3UL+65536UL-p)*155/255);
          break;
        }
        case 2: { // Cylon: roter Kern und langer, richtungsabhängiger Schweif.
          // Beim Wenden bleibt der Schweif der letzten Bahn an der Kante stehen und klingt
          // dort aus, während der Kopf in die Gegenrichtung startet (wie das klassische
          // Ausblenden pro Schritt). Ohne diesen Ausklang verschwände er schlagartig.
          const uint16_t phase=p & 4095;
          const uint16_t since=forward ? phase : phase-2048;   // Phasenschritte seit der Umkehr
          const uint16_t runout=tail/2+1;
          uint8_t amount=trail(x,head,tail,forward);
          if (since<runout) {
            const uint32_t left=runout-since;
            const uint8_t edgeTail=forward ? glow(int32_t(x),tail) : glow(4095-int32_t(x),tail);
            const uint8_t afterglow=uint32_t(edgeTail)*left*left/(uint32_t(runout)*runout);
            if (afterglow>amount) amount=afterglow;
          }
          const Color color=s.colorCycle ? hue(h) : red;
          add(c,color,amount);
          add(c,{240,95,25},glow(int32_t(x)-head,45));
          break;
        }
        case 3:
          c=scale(hue(hash(x/64)+h),stars(x+p/4,p,70)); break;
        case 4:
          c=scale(blend(base,gold,wave(x+h)),stars(x,p,larger(4,density/2),79)); break;
        case 5:
          add(c,cyan,trail(x,p & 4095,tail,true));
          add(c,gold,trail(x,4095-(p & 4095),tail,false)); break;
        case 6: {
          const uint8_t ribbon=wave(x*9UL+65536UL-p);
          c=scale(blend(base,violet,wave(x*3UL+p)),uint16_t(ribbon)*ribbon/255);
          break;
        }
        case 7: { // Glut und zwei Turbulenzgrößen, ohne hektisches Einzelpixelrauschen.
          const uint16_t heat=(wave(x*7UL+p)+wave(x*13UL+p/2)+wave(x*3UL+65536UL-p))/3;
          c=scale(blend(red,gold,heat),smaller(255,40+heat*(300-fade/2)/255));
          add(c,{235,75,4},stars(x+p/4,p/2,larger(8,density/3),31)/3);
          break;
        }
        case 8: { // Star Trek: bekannte eisblaue, gespiegelte Spuren mit weicher Bewegung.
          const Color color=s.colorCycle ? hue(h) : Color{105,175,235};
          const uint16_t inward=2048-radius;
          // Der Kopf läuft über Mitte/Rand hinaus; erst der letzte Schweifrest
          // verlässt den sichtbaren Bereich, dann beginnt die nächste Richtung.
          const int32_t behind=trekForward ? trekHead-inward : int32_t(inward)-trekHead;
          add(c,color,glow(behind<0 ? -behind*5 : behind,tail));
          add(c,scale(color,180),glow(int32_t(inward)-trekHead,40));
          break;
        }
        case 9: { // Gemini: zwei exakt gegenläufige Spuren in Komplementärfarben.
          const uint16_t inward=2048-radius;
          const Color color=hue(h+(x<2048 ? 0 : 2048));
          add(c,color,trail(inward,radial,tail,forward));
          break;
        }
        case 11: case 21: case 33: {
          const uint16_t position=id==21 ? x : radius*2;
          const int32_t edge=int32_t(level)*4096/255-position;
          const uint8_t amount=level ? bounded(edge+64,0,128)*255/128 : 0;
          const Color color=id==33 ? hue(position+h) : blend(mint,red,position*255UL/4096);
          c=scale(color,amount);
          if (level) add(c,gold,glow(edge,60)/2);
          break;
        }
        case 12:
          c=scale(blend(mint,violet,wave(x*2UL+p)),level*(140+wave(x*2UL+65536UL-p)*115/255)/255);
          break;
        case 13:
          c=scale(blend(cyan,violet,wave(x*waves+p)),
                  (30+wave(x*4UL+65536UL-p)*225/255)*level/255); break;
        case 24: {
          const uint16_t segments=bounded(s.density,2,40);
          const uint32_t cell=uint32_t(x)*segments;
          const uint16_t fill=bounded(int32_t(level)*segments-int32_t(cell>>12)*255,0,255);
          const uint16_t gap=smaller(255,smaller(cell & 4095,4095-(cell & 4095))/2);
          c=scale(hue((cell>>12)*4096UL/segments+h),fill*gap/255); break;
        }
        case 26:
          c=scale(blend(mint,violet,wave(x+p/4)),25+wave(p)*205/255); break;
        case 27: {
          const uint16_t a=wave(x*waves+ p), b=wave(x*waves2+65536UL-p2);
          c=scale(blend(hue(h+x/3),hue(h+2048+x/2),a),40+(a+b)*180/510);
          break;
        }
        case 28: {
          const uint16_t cycle=p>>12;
          const int32_t edge=uint32_t(p & 4095)*4416/4096;
          c=blend(hue(cycle*768UL),hue((cycle+1)*768UL),bounded(edge-x,0,320)*255/320);
          break;
        }
        case 29: {
          const uint16_t dots=bounded(s.density,1,20);
          const uint32_t lane=x*dots+65536UL-p;
          c=scale(hue(x*2UL+h),glow(int32_t(lane & 4095)-2048,600));
          break;
        }
        case 30:
          c=scale(x<2048 ? red : blue,wave(p*4UL+(x<2048 ? 0 : 2048)));
          add(c,x<2048 ? red : blue,glow(int32_t(x)-head,240)/2); break;
        case 31: {
          const uint16_t avg=mean*4096UL/255, peak=level*4096UL/255;
          if (x<avg) c=scale(blue,160);
          else if (x<peak) c=scale(red,180);
          if (level) add(c,gold,glow(int32_t(x)-peak,64));
          break;
        }
        case 32:
          c=scale(blend(hue(level*12UL+x/2),hue(level*12UL+2048+x/2),wave(x*2UL+p)),level);
          break;
        case 20: case 35:
          add(c,hue(h),trail(x,head,tail,forward));
          add(c,hue(h+2048),trail(x,4095-head,tail/2,!forward)/3);
          if (id==35 && newest<1000) add(c,gold,glow(int32_t(x)-head,180)*(1000-newest)/1000);
          break;
        case 17: case 25: {
          const uint16_t life=200+(255-fade)*4;
          if (newest<life) {
            const uint16_t envelope=uint32_t(life-newest)*255/life;
            c=scale(blend(gold,violet,wave(x*2UL+h)),envelope);
            if (id==25) c=blend(c,scale(hue(h+x/3),35),255-envelope);
          } else if (id==25) c=scale(hue(h+x/3),35);
          break;
        }
        case 19: {
          const uint16_t launch=3000-speed*8;
          if (rocket_<launch) {
            const uint16_t pos=uint32_t(rocket_)*2048/launch;
            add(c,mint,trail(x,pos,tail,true));
            add(c,violet,trail(x,4096-pos,tail,false));
          } else if (rocket_<launch+2200) {
            const uint16_t since=rocket_-launch;
            add(c,gold,glow(int32_t(radius)-since*2,250)*(2200-since)/2200);
            add(c,hue(hash(x/32)+h),stars(x,p,density,19)*(2200-since)/2200);
          }
          break;
        }
        default: break;
      }
      // Die Beatliste nimmt Impulse unabhängig von Effektgeschwindigkeit/Bildwechsel auf.
      // Mehrere Wellen bleiben gleichzeitig sichtbar; nur kleine Metadaten, keine LED-Kopie.
      if (id==10 || id==14 || id==15 || id==16 || id==18 || id==22 || id==34) {
        for (const auto &b:beats_) {
          const uint16_t life=600+(255-fade)*12;
          if (b.age>=life) continue;
          const uint16_t envelope=uint32_t(life-b.age)*255/life;
          const Color color=s.colorCycle ? hue(b.color) : base;
          const int32_t front=uint32_t(b.age)*(200+speed*8)/1000;
          if (id==14 || id==15 || id==18) {
            const int32_t position=id==15 ? 2048-front : front;
            uint16_t ring=glow(int32_t(radius)-position,40+width*16);
            if (id!=18) {
              const int32_t behind=id==15 ? int32_t(radius)-position : position-radius;
              ring=larger(ring,glow(behind<0 ? -behind*4 : behind,tail/3+width*12));
            }
            add(c,id==18 ? gold : color,ring*envelope/255);
            if (id==18) add(c,hue(b.color+x),stars(x,p,density,b.color)*envelope/510);
          } else if (id==22) {
            const bool right=(b.color & 1)==0;
            const int32_t position=right ? front : 4096-front;
            if (position>=0 && position<=4096) add(c,color,trail(x,position,tail,right)*envelope/255);
          } else if (id==34) {
            add(c,color,glow(magnitude(int32_t(x)-b.origin)-front,500)*envelope/255);
          } else {
            const uint32_t drifting=x+(id==10 ? front/3 : 0);
            add(c,id==16 ? blend(color,gold,100) : color,
                stars(drifting,p,density,b.color)*envelope/255);
          }
        }
      }
      const uint16_t entry=entrance_*255UL/600;
      pixels[i].r=uint16_t(c.r)*entry/255;
      pixels[i].g=uint16_t(c.g)*entry/255;
      pixels[i].b=uint16_t(c.b)*entry/255;
    }
  }
};
} // namespace effect_studio
