#pragma once
#include "ChatGptEffect.h"

// Zwei ruhige Goldschweif-Varianten mit mehr Bewegung, immer nur wenige schmale
// Balken auf dunklem Schlauch. Dieselben Regler wie der Goldschweif (Tempo, Pause,
// Schweiflaenge, Farbe). Kein Audio, kein Heap, kein delay, kein Zufall.
//   mode 0 "Goldatem":   zwei Balken laufen von der Mitte nach aussen, danach zwei von
//                        den Enden zur Mitte, dann Pause (Aus- und Einatmen).
//   mode 1 "Goldpendel": zwei Balken starten an gegenueberliegenden Enden, laufen
//                        aneinander vorbei bis zum anderen Ende. Der zweite startet
//                        etwas spaeter, sodass sie sich abwechselnd rechts und links
//                        der Mitte begegnen, dann Pause. Ab und zu loest sich vom Balken
//                        ein kleiner, schwaecherer Funke nach links und rechts (kurzer
//                        Ausflug, dann aus); Zeitpunkte aus einem festen Hash, kein Zufall.
//   mode 2 "Goldtrio":   drei Balken starten nacheinander an einem Ende und ziehen zum
//                        anderen, im naechsten Gang in die Gegenrichtung, dann Pause.
namespace gold_variants {
inline int bound(int n,int lo,int hi) { return n<lo?lo:n>hi?hi:n; }

class Renderer {
  uint32_t last_=0, phase_=0, round_=0;   // round_ zaehlt volle Durchlaeufe (Funkenvariation)

  static uint32_t mix(uint32_t v) {
    v^=v>>16; v*=0x7feb352dUL; v^=v>>15; v*=0x846ca68bUL;
    return v^(v>>16);
  }
  // Kleine Funken eines Balkens: drei Gelegenheiten je Gang, jeweils ein Funke nach links
  // und einer nach rechts von der Balkenspitze, der in einem Viertel der Bahnzeit
  // abklingt. `tt` = Laufzeit des Balkens, `coordinate` = Position des Pixels in der
  // Bewegungsrichtung des Balkens.
  static uint8_t sparks(int64_t tt,int32_t coordinate,uint32_t travel,int32_t span,
                        int front,uint32_t seed) {
    static const int32_t base[3]={220,460,700};           // Promille der Bahnzeit
    const uint32_t life=travel/4;
    uint8_t best=0;
    for(int k=0;k<3;++k) {
      const int32_t jitter=int32_t(mix(seed*3+uint32_t(k))%141)-70;   // +-7 %
      const int64_t start=int64_t(travel)*(base[k]+jitter)/1000;
      const int64_t age=tt-start;
      if(age<0||age>=int64_t(life)) continue;
      const int32_t origin=int32_t(start*span/int64_t(travel))-front;
      const int32_t reach=int32_t(age*span*4/(int64_t(travel)*10));   // 0,4 x Balkentempo
      const uint32_t fade=uint32_t(int64_t(life)-age)*255/life;
      const uint32_t alpha=fade*fade/255*110/255;                      // hoechstens ca. 43 %
      const uint8_t l=chatgpt_effect::glow(coordinate-(origin-reach),60);
      const uint8_t r=chatgpt_effect::glow(coordinate-(origin+reach),60);
      const uint8_t s=uint8_t(uint32_t(l>r?l:r)*alpha/255);
      if(s>best) best=s;
    }
    return best;
  }

  // Leuchtstaerke eines Balkens: scharfe Spitze vorn, laengerer Schweif hinten.
  static uint8_t comet(int32_t behind,int front,int back) {
    return chatgpt_effect::glow(behind,behind<0?front:back);
  }
public:
  void reset(uint32_t now) { last_=now; phase_=0; }
  template<class Pixel> void render(Pixel *pixels,uint16_t count,uint32_t now,int mode,
      int speed,int pause,int tail,int red,int green,int blue) {
    speed=bound(speed,10,200); pause=bound(pause,0,15000); tail=bound(tail,80,1200);
    red=bound(red,0,255); green=bound(green,0,255); blue=bound(blue,0,255);
    // Eine Bahn von der Mitte zum Rand entspricht dem Goldschweif (144000/speed ms);
    // das Pendel durchquert den ganzen Streifen, also doppelt so lang bei gleichem Tempo.
    const uint32_t lane=144000UL/speed;
    const uint32_t travel=mode==0?lane:lane*2;
    const uint32_t delay=mode==0?0:travel/4;       // Pendel: Versatz; Trio: Abstand der Balken
    const uint32_t slot=mode==2?travel+delay*2:travel+delay;
    const uint32_t cycle=slot*2+pause;
    const uint32_t delta=now-last_;last_=now;
    const uint32_t advanced=phase_%cycle+delta%cycle;
    round_+=delta/cycle+(advanced>=cycle?1:0);
    phase_=advanced%cycle;
    const bool first=phase_<slot, second=!first&&phase_<slot*2;
    const uint32_t t=first?phase_:second?phase_-slot:0;
    // Pendel: Laufzeiten beider Balken (negativ = noch nicht gestartet). Im ersten Gang
    // startet A zuerst, im zweiten B: die Begegnung wechselt von rechts zu links der Mitte.
    const int64_t tA=first?int64_t(t):int64_t(t)-int64_t(delay);
    const int64_t tB=first?int64_t(t)-int64_t(delay):int64_t(t);
    const int front=110, back=tail, span=4096+front+back;   // gleiche Balkenform wie der Goldschweif
    for(uint16_t i=0;i<count;++i) {
      uint8_t light=0;
      if(first||second) {
        if(mode==0) {
          const int32_t offset=int32_t(i)*2-int32_t(count-1);
          const int32_t radius=count>1?int32_t(uint32_t(offset<0?-offset:offset)*2048/(count-1)):0;
          // Ausatmen: Spitze wandert von der Mitte nach aussen (Radius waechst);
          // Einatmen: Spitze wandert vom Rand zur Mitte (Abstand zum Rand waechst).
          const int32_t head=int32_t(int64_t(t)*(2048+tail)/int64_t(travel));
          const int32_t behind=head-(first?radius:2048-radius);
          light=comet(behind,110,tail);
        } else if(mode==2) {
          // Drei gleich geformte Balken im Abstand von `delay`; erster Gang von links
          // nach rechts, zweiter Gang von rechts nach links.
          const int32_t x=count>1?int32_t(uint64_t(i)*4096/(count-1)):2048;
          const int32_t coordinate=first?x:4096-x;
          for(int k=0;k<3;++k) {
            const int64_t tk=int64_t(t)-int64_t(delay)*k;
            if(tk<0) continue;
            const int32_t head=int32_t(tk*span/int64_t(travel))-front;
            const uint8_t one=comet(head-coordinate,front,back);
            if(one>light) light=one;
          }
        } else {
          const int32_t x=count>1?int32_t(uint64_t(i)*4096/(count-1)):2048;
          const int32_t xm=4096-x;
          uint8_t a=0,b=0;
          if(tA>=0) {
            const int32_t head=int32_t(tA*span/int64_t(travel))-front;
            a=comet(head-x,front,back);          // A: immer links -> rechts
          }
          if(tB>=0) {
            const int32_t head=int32_t(tB*span/int64_t(travel))-front;
            b=comet(head-xm,front,back);         // B: immer rechts -> links
          }
          light=a>b?a:b;
          // Funken: Rolle (zuerst/spaeter startender Balken) und Durchlauf bestimmen die Zeitpunkte,
          // dadurch bleibt der zweite Gang das Spiegelbild des ersten.
          const uint32_t early=mix(round_*3+1), late=mix(round_*3+2);
          const uint8_t sa=sparks(tA,x,travel,span,front,first?early:late);
          const uint8_t sb=sparks(tB,xm,travel,span,front,first?late:early);
          if(sa>light) light=sa;
          if(sb>light) light=sb;
        }
      }
      pixels[i].r=uint16_t(red)*light/255;
      pixels[i].g=uint16_t(green)*light/255;
      pixels[i].b=uint16_t(blue)*light/255;
    }
  }
};
}
