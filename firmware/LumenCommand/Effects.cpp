#include "Effects.h"
#include "Globals.h"
#include "Audio.h"
#include "ChatGptEffect.h"
#include "EffectStudio.h"
#include "SignatureEffects.h"
#include "QuietEffects.h"
#include "GoldTrail.h"
#include "ClaudeEffect.h"
#include "GoldVariants.h"
#include "HueDrift.h"
#include "OpusEffects.h"
#include "CalmEffects.h"
#include "LedSlots.h"
#include "MusicScenes.h"

int goldSpeed = 80;
int goldPause = 2500;
int goldTail = 350;
int goldRed = 235;
int goldGreen = 165;
int goldBlue = 65;

// --- INITIALISIERUNG DER PARAMETER ---
int rainbowSpeed = 20;
int rainbowGap = 7;
int cylonSpeed = 20;
int cylonFade = 15;
bool cylonColorCycle = false;
int cylonHueSpeed = 2;
int confettiSpeed = 20;
int twinkleSpeed = 100;
int twinkleDensity = 50;
int meteorSpeed = 30;
int meteorFade = 64;
int theaterChaseSpeed = 50;
int fireCooling = 55;
int fireSparking = 120;
int starTrekSpeed = 20;
int starTrekFade = 15;
bool starTrekColorCycle = false;
int starTrekHueSpeed = 2;
int geminiSpeed = 25;
int geminiFade = 40;

int confettiClapSpeed = 20;
int vuMeterSpeed = 30;
int audioPulseFade = 80;
int audioWaveSpeed = 10;

int beatRippleSpeed = 150;
int audioBeatRippleFade = 40;
int audioBeatImplosionFade = 10;
int beatRippleWidth = 2;
bool beatRippleColorCycle = true;

int audioSparkleFade = 80;
int audioSparkleDensity = 50;

int audioStrobeFade = 80;

int fireworksSparkleDensity = 50;

int rocketSpeed = 20;
int rocketExplosionDensity = 80;
int rocketFade = 30;

int sinelonSpeed = 13;
int sinelonFade = 20;
int sinelonHueSpeed = 1;

int vuMeterLeftSpeed = 30;
int beatCometFade = 40;
int loudnessNoiseDensity = 100;
int segmentViewSegments = 10;
int beatFlashFade = 10;
int breathingSpeed = 10;
int plasmaSpeed1 = 100;
int plasmaSpeed2 = 80;
int plasmaScale1 = 10;
int plasmaScale2 = 15;
int colorWipeSpeed = 50;
int chasingDotsNum = 5;
int chasingDotsSpeed = 20;
int policeSpeed = 50;

// --- INTERNE EFFEKT-FUNKTIONEN ---
static effect_studio::Renderer studio;

static void renderStudio(byte id) {
  effect_studio::Settings s;
  s.color={currentColor.r,currentColor.g,currentColor.b};
  switch (id) {
    case 1: s.speed=rainbowSpeed; s.scale=rainbowGap; break;
    case 2: s.speed=cylonSpeed; s.fade=cylonFade; s.colorCycle=cylonColorCycle; s.hueSpeed=cylonHueSpeed; break;
    case 3: s.speed=confettiSpeed; break;
    case 4: s.speed=twinkleSpeed; s.density=twinkleDensity; break;
    case 5: s.speed=meteorSpeed; s.fade=meteorFade; break;
    case 6: s.speed=theaterChaseSpeed; break;
    case 7: s.fade=fireCooling; s.density=fireSparking; break;
    case 8: s.speed=starTrekSpeed; s.fade=starTrekFade; s.colorCycle=starTrekColorCycle; s.hueSpeed=starTrekHueSpeed; break;
    case 9: s.speed=geminiSpeed; s.fade=geminiFade; s.hueSpeed=6; break;
    case 10: s.speed=confettiClapSpeed; break;
    case 11: s.speed=vuMeterSpeed; break;
    case 12: s.fade=audioPulseFade; break;
    case 13: s.speed=audioWaveSpeed; s.scale=rainbowGap; break;
    case 14: case 15:
      s.speed=beatRippleSpeed; s.fade=id==14 ? audioBeatRippleFade : audioBeatImplosionFade;
      s.width=beatRippleWidth; s.colorCycle=beatRippleColorCycle; break;
    case 16: s.fade=audioSparkleFade; s.density=audioSparkleDensity; break;
    case 17: s.fade=audioStrobeFade; break;
    case 18: s.fade=audioBeatRippleFade; s.density=fireworksSparkleDensity; break;
    case 19: s.speed=rocketSpeed; s.density=rocketExplosionDensity; s.fade=rocketFade; break;
    case 20: s.speed=sinelonSpeed; s.fade=sinelonFade; s.hueSpeed=sinelonHueSpeed; break;
    case 21: s.speed=vuMeterLeftSpeed; break;
    case 22: s.fade=beatCometFade; break;
    case 23: s.density=loudnessNoiseDensity; s.hueSpeed=6; break;
    case 24: s.density=segmentViewSegments; break;
    case 25: s.fade=beatFlashFade; break;
    case 26: s.speed=breathingSpeed; break;
    case 27: s.speed=plasmaSpeed1; s.secondSpeed=plasmaSpeed2; s.scale=plasmaScale1; s.secondScale=plasmaScale2; break;
    case 28: s.speed=colorWipeSpeed; break;
    case 29: s.speed=chasingDotsSpeed; s.density=chasingDotsNum; break;
    case 30: s.speed=policeSpeed; break;
    case 35: s.speed=sinelonSpeed; s.fade=sinelonFade; s.hueSpeed=sinelonHueSpeed; break;
    default: break;
  }
  studio.render(leds,numLeds>MAX_LEDS ? MAX_LEDS : numLeds,id,millis(),
                constrain(smoothedLoudness,0,255),constrain(averageLoudness,0L,255L),beatDetected,s);
}

void rainbow() { renderStudio(1); }
void cylon() { renderStudio(2); }
void confetti() { renderStudio(3); }
void twinkle() { renderStudio(4); }
void meteor() { renderStudio(5); }
void theaterChase() { renderStudio(6); }
void fire() { renderStudio(7); }
void starTrek() { renderStudio(8); }
void geminiEffect() { renderStudio(9); }
void audioBeatConfetti() { renderStudio(10); }
void vuMeterCenter() { renderStudio(11); }
void audioPulse() { renderStudio(12); }
void audioWave() { renderStudio(13); }
void audioBeatRipple() { renderStudio(14); }
void audioBeatImplosion() { renderStudio(15); }
void audioSparkle() { renderStudio(16); }
void audioBeatStrobe() { renderStudio(17); }
void audioFireworks() { renderStudio(18); }
void audioRocketFireworks() { renderStudio(19); }
void sinelon() { renderStudio(20); }
void vuMeterLeft() { renderStudio(21); }
void beatComet() { renderStudio(22); }
void loudnessNoise() { renderStudio(23); }
void segmentView() { renderStudio(24); }
void beatFlash() { renderStudio(25); }
void breathing() { renderStudio(26); }
void plasma() { renderStudio(27); }
void colorWipe() { renderStudio(28); }
void chasingDots() { renderStudio(29); }
void police() { renderStudio(30); }
void dualVuMeter() { renderStudio(31); }
void spectrumFiller() { renderStudio(32); }
void rainbowVuCenter() { renderStudio(33); }
void beatShake() { renderStudio(34); }
void sinelonBeatDrop() { renderStudio(35); }

// Türzeichen für 53/54; die Meldungen setzt der Render-Loop in Led.ino.
door_scene::Signal doorSignal;
// Bilderlicht 60-62; Laden/Speichern in Persistence.cpp, Malen und Sichern in WifiControl.cpp.
LedSlot ledSlots[LED_SLOT_COUNT];

// --- DISPATCHER ---
void runCurrentEffect() {
    static byte previousEffect = 255;
    static chatgpt_effect::Choreography chatGpt;
    static signature_effects::QuietGold quietGold;
    static signature_effects::Renderer signatures;
    static quiet_effects::Renderer quietScenes;
    static gold_trail::Renderer goldTrail;
    static claude_effect::Renderer claudeScene;
    static gold_variants::Renderer goldBreath, goldPendulum, goldTrio;
    static hue_drift::Clock chatGptHue;
    static opus_effect::Renderer opusScene;
    static door_scene::Renderer doorScene;
    static calm_effects::Renderer calmScene;
    static music_scenes::Renderer musicScene;
    if (currentEffect != previousEffect) {
      studio.reset(millis());
      if (currentEffect == 44 || currentEffect == 45) claudeScene.reset(millis());
      if (currentEffect == 36 || currentEffect == 37 || currentEffect == 49) chatGpt.reset(millis());
      if (currentEffect == 37) chatGptHue.reset(millis());
      if (currentEffect == 46) goldBreath.reset(millis());
      if (currentEffect == 47) goldPendulum.reset(millis());
      if (currentEffect == 48) goldTrio.reset(millis());
      if (currentEffect == 37 || currentEffect == 49) quietGold.reset(millis());
      if (currentEffect == 43) goldTrail.reset(millis());
      if (currentEffect == 38 || currentEffect == 39) signatures.reset(millis());
      if (currentEffect >= 40 && currentEffect <= 42) quietScenes.reset(millis());
      if (currentEffect >= 50 && currentEffect <= 52) opusScene.reset(millis());
      if (currentEffect == 53 || currentEffect == 54) doorScene.reset(millis());
      if (currentEffect == 54) chatGpt.reset(millis());
      if (currentEffect >= 55 && currentEffect <= 59) calmScene.reset(millis());
      if (currentEffect >= 63 && currentEffect <= 65) musicScene.reset(millis());
    }
    switch(currentEffect) {
      case 1: rainbow(); break;
      case 2: cylon(); break;
      case 3: confetti(); break;
      case 4: twinkle(); break;
      case 5: meteor(); break;
      case 6: theaterChase(); break;
      case 7: fire(); break;
      case 8: starTrek(); break;
      case 9: geminiEffect(); break;
      case 10: audioBeatConfetti(); break;
      case 11: vuMeterCenter(); break;
      case 12: audioPulse(); break;
      case 13: audioWave(); break;
      case 14: audioBeatRipple(); break;
      case 15: audioBeatImplosion(); break;
      case 16: audioSparkle(); break;
      case 17: audioBeatStrobe(); break;
      case 18: audioFireworks(); break;
      case 19: audioRocketFireworks(); break;
      case 20: sinelon(); break;
      case 21: vuMeterLeft(); break;
      case 22: beatComet(); break;
      case 23: loudnessNoise(); break;
      case 24: segmentView(); break;
      case 25: beatFlash(); break;
      case 26: breathing(); break;
      case 27: plasma(); break;
      case 28: colorWipe(); break;
      case 29: chasingDots(); break;
      case 30: police(); break;
      case 31: dualVuMeter(); break;
      case 32: spectrumFiller(); break;
      case 33: rainbowVuCenter(); break;
      case 34: beatShake(); break;
      case 35: sinelonBeatDrop(); break;
      case 36: chatGpt.render(leds, numLeds > MAX_LEDS ? MAX_LEDS : numLeds,
                              millis(), constrain(smoothedLoudness, 0, 255), beatDetected); break;
      case 37: {
        // ChatGPT ohne Musik: gleicher Ablauf, pro 32-s-Durchlauf um 72 Grad weiter im
        // Farbkreis (nach fuenf Durchlaeufen einmal rum, ohne Sprung).
        const uint16_t shown = numLeds > MAX_LEDS ? MAX_LEDS : numLeds;
        chatGpt.render(leds, shown, millis(), 0, quietGold.update(millis()));
        hue_drift::rotate(leds, shown, chatGptHue.angle(millis(), 160000));
        break;
      }
      case 38: signatures.render(leds, numLeds > MAX_LEDS ? MAX_LEDS : numLeds,
                              millis(), false, 0, false); break;
      case 39: signatures.render(leds, numLeds > MAX_LEDS ? MAX_LEDS : numLeds,
                              millis(), true, constrain(smoothedLoudness,0,255), beatDetected); break;
      case 43: goldTrail.render(leds, numLeds > MAX_LEDS ? MAX_LEDS : numLeds, millis(),
          goldSpeed, goldPause, goldTail, goldRed, goldGreen, goldBlue); break;
      case 46: goldBreath.render(leds, numLeds > MAX_LEDS ? MAX_LEDS : numLeds, millis(), 0,
          goldSpeed, goldPause, goldTail, goldRed, goldGreen, goldBlue); break;
      case 47: goldPendulum.render(leds, numLeds > MAX_LEDS ? MAX_LEDS : numLeds, millis(), 1,
          goldSpeed, goldPause, goldTail, goldRed, goldGreen, goldBlue); break;
      case 48: goldTrio.render(leds, numLeds > MAX_LEDS ? MAX_LEDS : numLeds, millis(), 2,
          goldSpeed, goldPause, goldTail, goldRed, goldGreen, goldBlue); break;
      case 49:
        // ChatGPT Original: der bisherige Ablauf von "ohne Musik" mit den festen
        // Ursprungsfarben (ohne Farbwanderung).
        chatGpt.render(leds, numLeds > MAX_LEDS ? MAX_LEDS : numLeds,
                       millis(), 0, quietGold.update(millis())); break;
      case 44: claudeScene.render(leds, numLeds > MAX_LEDS ? MAX_LEDS : numLeds,
                              millis(), true, constrain(smoothedLoudness,0,255), beatDetected); break;
      case 45: claudeScene.render(leds, numLeds > MAX_LEDS ? MAX_LEDS : numLeds,
                              millis(), false, 0, false); break;
      case 50: case 51: case 52:
        opusScene.render(leds, numLeds > MAX_LEDS ? MAX_LEDS : numLeds, millis(),
                         currentEffect - 50, constrain(smoothedLoudness, 0, 255), beatDetected); break;
      case 53: doorScene.renderCalm(leds, numLeds > MAX_LEDS ? MAX_LEDS : numLeds, millis(),
                                    limitLedOutput(brightness)); break;
      case 54: {
        const uint16_t shown = numLeds > MAX_LEDS ? MAX_LEDS : numLeds;
        const uint8_t loud = constrain(smoothedLoudness, 0, 255);
        chatGpt.render(leds, shown, millis(), loud, beatDetected);
        doorScene.overlayMusic(leds, shown, millis(), doorSignal, beatDetected, loud);
        break;
      }
      case 55: case 56: case 57: case 58: case 59:
        calmScene.render(leds, numLeds > MAX_LEDS ? MAX_LEDS : numLeds, millis(),
                         currentEffect - 55, constrain(smoothedLoudness, 0, 255), beatDetected); break;
      case 63: case 64: case 65:
        musicScene.render(leds, numLeds > MAX_LEDS ? MAX_LEDS : numLeds, millis(),
                          currentEffect - 63, constrain(smoothedLoudness, 0, 255), beatDetected); break;
      case 60: case 61: case 62:
        showLedSlot(ledSlots[currentEffect - LED_SLOT_FIRST_EFFECT], leds,
                    numLeds > MAX_LEDS ? MAX_LEDS : numLeds); break;
      case 40: case 41: case 42:
        quietScenes.render(leds, numLeds > MAX_LEDS ? MAX_LEDS : numLeds,
                              millis(), currentEffect-40); break;
      default:
        break;
    }
    previousEffect = currentEffect;
}
