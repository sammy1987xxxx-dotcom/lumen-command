#ifndef EFFECTS_H
#define EFFECTS_H

#include <Arduino.h>
#include <FastLED.h>

extern int goldSpeed;
extern int goldPause;
extern int goldTail;
extern int goldRed;
extern int goldGreen;
extern int goldBlue;

// --- EFFEKT PARAMETER (Extern) ---
extern int rainbowSpeed;
extern int rainbowGap;
extern int cylonSpeed;
extern int cylonFade;
extern bool cylonColorCycle;
extern int cylonHueSpeed;
extern int confettiSpeed;
extern int twinkleSpeed;
extern int twinkleDensity;
extern int meteorSpeed;
extern int meteorFade;
extern int theaterChaseSpeed;
extern int fireCooling;
extern int fireSparking;
extern int starTrekSpeed;
extern int starTrekFade;
extern bool starTrekColorCycle;
extern int starTrekHueSpeed;
extern int geminiSpeed;
extern int geminiFade;

extern int confettiClapSpeed;
extern int vuMeterSpeed;
extern int audioPulseFade;
extern int audioWaveSpeed;

extern int beatRippleSpeed;
extern int audioBeatRippleFade;
extern int audioBeatImplosionFade;
extern int beatRippleWidth;
extern bool beatRippleColorCycle;

extern int audioSparkleFade;
extern int audioSparkleDensity;
extern int audioStrobeFade;
extern int fireworksSparkleDensity;

extern int rocketSpeed;
extern int rocketExplosionDensity;
extern int rocketFade;

extern int sinelonSpeed;
extern int sinelonFade;
extern int sinelonHueSpeed;

extern int vuMeterLeftSpeed;
extern int beatCometFade;
extern int loudnessNoiseDensity;
extern int segmentViewSegments;
extern int beatFlashFade;
extern int breathingSpeed;
extern int plasmaSpeed1;
extern int plasmaSpeed2;
extern int plasmaScale1;
extern int plasmaScale2;
extern int colorWipeSpeed;
extern int chasingDotsNum;
extern int chasingDotsSpeed;
extern int policeSpeed;

// --- FUNKTIONEN ---
void runCurrentEffect();

#endif
