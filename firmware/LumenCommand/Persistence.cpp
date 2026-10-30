#include "Persistence.h"
#include "Globals.h"
#include "Audio.h"
#include "Effects.h"
#include <Preferences.h>

Preferences preferences;

// Status für entprelltes Speichern
static volatile bool settingsChanged = false;
static volatile unsigned long lastSettingsChangeTime = 0;
const unsigned long DEBOUNCE_DELAY_MS = 10000; // 10 Sekunden

struct SettingsCache {
    int32_t goldSpeed;
    int32_t goldPause;
    int32_t goldTail;
    int32_t goldRed;
    int32_t goldGreen;
    int32_t goldBlue;

    uint32_t numLeds;
    uint8_t brightness;
    uint8_t currentEffect;
    uint32_t currentColor;

    int32_t micSensitivity;
    uint8_t doorLightLevel;
    uint8_t doorSignalLevel;
    uint32_t doorFirst;
    uint32_t doorLast;
    bool doorLightEnabled;

    int32_t rainbowSpeed;
    int32_t rainbowGap;
    int32_t cylonSpeed;
    int32_t cylonFade;
    bool cylonColorCycle;
    int32_t cylonHueSpeed;
    int32_t confettiSpeed;
    int32_t twinkleSpeed;
    int32_t twinkleDensity;
    int32_t meteorSpeed;
    int32_t meteorFade;
    int32_t theaterChaseSpeed;
    int32_t fireCooling;
    int32_t fireSparking;
    int32_t starTrekSpeed;
    int32_t starTrekFade;
    bool starTrekColorCycle;
    int32_t starTrekHueSpeed;
    int32_t geminiSpeed;
    int32_t geminiFade;

    int32_t confettiClapSpeed;
    int32_t vuMeterSpeed;
    int32_t audioPulseFade;
    int32_t audioWaveSpeed;

    int32_t beatRippleSpeed;
    int32_t audioBeatRippleFade;
    int32_t audioBeatImplosionFade;
    int32_t beatRippleWidth;
    bool beatRippleColorCycle;

    int32_t audioSparkleFade;
    int32_t audioSparkleDensity;
    int32_t audioStrobeFade;
    int32_t fireworksSparkleDensity;

    int32_t rocketSpeed;
    int32_t rocketExplosionDensity;
    int32_t rocketFade;

    int32_t sinelonSpeed;
    int32_t sinelonFade;
    int32_t sinelonHueSpeed;

    int32_t vuMeterLeftSpeed;
    int32_t beatCometFade;
    int32_t loudnessNoiseDensity;
    int32_t segmentViewSegments;
    int32_t beatFlashFade;
    int32_t breathingSpeed;
    int32_t plasmaSpeed1;
    int32_t plasmaSpeed2;
    int32_t plasmaScale1;
    int32_t plasmaScale2;
    int32_t colorWipeSpeed;
    int32_t chasingDotsNum;
    int32_t chasingDotsSpeed;
    int32_t policeSpeed;
};

static SettingsCache lastSaved;

// Helper-Funktionen, um nur bei Änderung zu schreiben (gegen Shadow-Cache)
static void updateUInt(const char* key, uint32_t value, uint32_t& cached) {
  if (cached != value) {
      preferences.putUInt(key, value);
      cached = value;
  }
}

static void updateUChar(const char* key, uint8_t value, uint8_t& cached) {
  if (cached != value) {
      preferences.putUChar(key, value);
      cached = value;
  }
}

static void updateULong(const char* key, uint32_t value, uint32_t& cached) {
    if (cached != value) {
        preferences.putULong(key, value);
        cached = value;
    }
}

static void updateBool(const char* key, bool value, bool& cached) {
  if (cached != value) {
      preferences.putBool(key, value);
      cached = value;
  }
}

static void updateInt(const char* key, int32_t value, int32_t& cached) {
  if (cached != value) {
      preferences.putInt(key, value);
      cached = value;
  }
}


void saveSettingsInternal() {
  if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
    preferences.begin("led-config", false);
    updateInt("goldSpeed", goldSpeed, lastSaved.goldSpeed);
    updateInt("goldPause", goldPause, lastSaved.goldPause);
    updateInt("goldTail", goldTail, lastSaved.goldTail);
    updateInt("goldRed", goldRed, lastSaved.goldRed);
    updateInt("goldGreen", goldGreen, lastSaved.goldGreen);
    updateInt("goldBlue", goldBlue, lastSaved.goldBlue);


    updateUInt("numLeds", numLeds, lastSaved.numLeds);
    updateUChar("brightness", brightness, lastSaved.brightness);
    updateUChar("currentEffect", currentEffect, lastSaved.currentEffect);
    updateULong("currentColor", (uint32_t)currentColor, lastSaved.currentColor);

    updateInt("micSensitivity", micSensitivity, lastSaved.micSensitivity);
    updateUChar("doorLightLevel", doorLightLevel, lastSaved.doorLightLevel);
    updateUChar("doorSignalLvl", doorSignalLevel, lastSaved.doorSignalLevel);
    updateUInt("doorFirst", door_light::doorRange().first, lastSaved.doorFirst);
    updateUInt("doorLast", door_light::doorRange().last, lastSaved.doorLast);
    updateBool("doorLightOn", doorLightEnabled, lastSaved.doorLightEnabled);

    updateInt("rainbowSpeed", rainbowSpeed, lastSaved.rainbowSpeed);
    updateInt("rainbowGap", rainbowGap, lastSaved.rainbowGap);
    updateInt("cylonSpeed", cylonSpeed, lastSaved.cylonSpeed);
    updateInt("cylonFade", cylonFade, lastSaved.cylonFade);
    updateBool("cylonColorCycle", cylonColorCycle, lastSaved.cylonColorCycle);
    updateInt("cylonHueSpeed", cylonHueSpeed, lastSaved.cylonHueSpeed);
    updateInt("confettiSpeed", confettiSpeed, lastSaved.confettiSpeed);
    updateInt("twinkleSpeed", twinkleSpeed, lastSaved.twinkleSpeed);
    updateInt("twinkleDensity", twinkleDensity, lastSaved.twinkleDensity);
    updateInt("meteorSpeed", meteorSpeed, lastSaved.meteorSpeed);
    updateInt("meteorFade", meteorFade, lastSaved.meteorFade);
    updateInt("theaterChaseSpd", theaterChaseSpeed, lastSaved.theaterChaseSpeed);
    updateInt("fireCooling", fireCooling, lastSaved.fireCooling);
    updateInt("fireSparking", fireSparking, lastSaved.fireSparking);
    updateInt("starTrekSpeed", starTrekSpeed, lastSaved.starTrekSpeed);
    updateInt("starTrekFade", starTrekFade, lastSaved.starTrekFade);
    updateBool("starTrekColor", starTrekColorCycle, lastSaved.starTrekColorCycle);
    updateInt("starTrekHueSpd", starTrekHueSpeed, lastSaved.starTrekHueSpeed);
    updateInt("geminiSpeed", geminiSpeed, lastSaved.geminiSpeed);
    updateInt("geminiFade", geminiFade, lastSaved.geminiFade);

    updateInt("confettiClapSpd", confettiClapSpeed, lastSaved.confettiClapSpeed);
    updateInt("vuMeterSpeed", vuMeterSpeed, lastSaved.vuMeterSpeed);
    updateInt("audioPulseFade", audioPulseFade, lastSaved.audioPulseFade);
    updateInt("audioWaveSpeed", audioWaveSpeed, lastSaved.audioWaveSpeed);

    updateInt("beatRippleSpeed", beatRippleSpeed, lastSaved.beatRippleSpeed);
    updateInt("audioBeatRipple", audioBeatRippleFade, lastSaved.audioBeatRippleFade);
    updateInt("audioBeatImpl", audioBeatImplosionFade, lastSaved.audioBeatImplosionFade);
    updateInt("beatRippleWidth", beatRippleWidth, lastSaved.beatRippleWidth);
    updateBool("beatRippleColor", beatRippleColorCycle, lastSaved.beatRippleColorCycle);

    updateInt("audioSparkleFade", audioSparkleFade, lastSaved.audioSparkleFade);
    updateInt("audioSparkleDens", audioSparkleDensity, lastSaved.audioSparkleDensity);
    updateInt("audioStrobeFade", audioStrobeFade, lastSaved.audioStrobeFade);
    updateInt("fireworksSparkle", fireworksSparkleDensity, lastSaved.fireworksSparkleDensity);

    updateInt("rocketSpeed", rocketSpeed, lastSaved.rocketSpeed);
    updateInt("rocketExplDens", rocketExplosionDensity, lastSaved.rocketExplosionDensity);
    updateInt("rocketFade", rocketFade, lastSaved.rocketFade);

    updateInt("sinelonSpeed", sinelonSpeed, lastSaved.sinelonSpeed);
    updateInt("sinelonFade", sinelonFade, lastSaved.sinelonFade);
    updateInt("sinelonHueSpeed", sinelonHueSpeed, lastSaved.sinelonHueSpeed);

    updateInt("vuMeterLeftSpd", vuMeterLeftSpeed, lastSaved.vuMeterLeftSpeed);
    updateInt("beatCometFade", beatCometFade, lastSaved.beatCometFade);
    updateInt("loudnessNoiseDen", loudnessNoiseDensity, lastSaved.loudnessNoiseDensity);
    updateInt("segmentViewSegments", segmentViewSegments, lastSaved.segmentViewSegments);
    updateInt("beatFlashFade", beatFlashFade, lastSaved.beatFlashFade);
    updateInt("breathingSpeed", breathingSpeed, lastSaved.breathingSpeed);
    updateInt("plasmaSpeed1", plasmaSpeed1, lastSaved.plasmaSpeed1);
    updateInt("plasmaSpeed2", plasmaSpeed2, lastSaved.plasmaSpeed2);
    updateInt("plasmaScale1", plasmaScale1, lastSaved.plasmaScale1);
    updateInt("plasmaScale2", plasmaScale2, lastSaved.plasmaScale2);
    updateInt("colorWipeSpeed", colorWipeSpeed, lastSaved.colorWipeSpeed);
    updateInt("chasingDotsNum", chasingDotsNum, lastSaved.chasingDotsNum);
    updateInt("chasingDotsSpd", chasingDotsSpeed, lastSaved.chasingDotsSpeed);
    updateInt("policeSpeed", policeSpeed, lastSaved.policeSpeed);

    preferences.end();
    Serial.println("Einstellungen dauerhaft gespeichert (Cached Check).");
    xSemaphoreGive(xMutex);
  }
}

void saveSettings() {
  settingsChanged = true;
  lastSettingsChangeTime = millis();
}

void handlePersistence() {
  if (settingsChanged) {
    if (millis() - lastSettingsChangeTime > DEBOUNCE_DELAY_MS) {
      settingsChanged = false; // Reset first to avoid race condition
      saveSettingsInternal();
    }
  }
}

void loadSettings() {
  preferences.begin("led-config", false);
  goldSpeed = constrain(preferences.getInt("goldSpeed", goldSpeed), 10, 200);
  lastSaved.goldSpeed = goldSpeed;
  goldPause = constrain(preferences.getInt("goldPause", goldPause), 0, 15000);
  lastSaved.goldPause = goldPause;
  goldTail = constrain(preferences.getInt("goldTail", goldTail), 80, 1200);
  lastSaved.goldTail = goldTail;
  goldRed = constrain(preferences.getInt("goldRed", goldRed), 0, 255);
  lastSaved.goldRed = goldRed;
  goldGreen = constrain(preferences.getInt("goldGreen", goldGreen), 0, 255);
  lastSaved.goldGreen = goldGreen;
  goldBlue = constrain(preferences.getInt("goldBlue", goldBlue), 0, 255);
  lastSaved.goldBlue = goldBlue;


  const uint32_t storedLeds = preferences.getUInt("numLeds", numLeds);
  numLeds = constrain(storedLeds, uint32_t(1), uint32_t(MAX_LEDS));
  lastSaved.numLeds = numLeds;

  brightness = preferences.getUChar("brightness", brightness);
  lastSaved.brightness = brightness;

  currentEffect = preferences.getUChar("currentEffect", currentEffect);
  lastSaved.currentEffect = currentEffect;

  uint32_t colorTmp = preferences.getULong("currentColor", (uint32_t)currentColor);
  currentColor = (CRGB)colorTmp;
  lastSaved.currentColor = colorTmp;





  // Frühere Audio-Schlüssel (micAutoGain, userNoiseFloor, userAudioGain,
  // audioSmoothing, beatThreshold) werden nicht mehr gelesen; ein Regler genügt.
  const int32_t storedSensitivity = preferences.getInt("micSensitivity", micSensitivity);
  micSensitivity = constrain(storedSensitivity, int32_t(1), int32_t(10));
  lastSaved.micSensitivity = micSensitivity;

  doorLightLevel = preferences.getUChar("doorLightLevel", doorLightLevel);
  lastSaved.doorLightLevel = doorLightLevel;
  doorSignalLevel = preferences.getUChar("doorSignalLvl", doorSignalLevel);
  lastSaved.doorSignalLevel = doorSignalLevel;
  door_light::Range &door = door_light::doorRange();
  door.first = constrain(preferences.getUInt("doorFirst", door.first), uint32_t(0), uint32_t(MAX_LEDS - 1));
  door.last = constrain(preferences.getUInt("doorLast", door.last), uint32_t(door.first), uint32_t(MAX_LEDS - 1));
  lastSaved.doorFirst = door.first;
  lastSaved.doorLast = door.last;
  doorLightEnabled = preferences.getBool("doorLightOn", doorLightEnabled);
  lastSaved.doorLightEnabled = doorLightEnabled;

  rainbowSpeed = preferences.getInt("rainbowSpeed", rainbowSpeed);
  lastSaved.rainbowSpeed = rainbowSpeed;

  rainbowGap = preferences.getInt("rainbowGap", rainbowGap);
  lastSaved.rainbowGap = rainbowGap;

  cylonSpeed = preferences.getInt("cylonSpeed", cylonSpeed);
  lastSaved.cylonSpeed = cylonSpeed;

  cylonFade = preferences.getInt("cylonFade", cylonFade);
  lastSaved.cylonFade = cylonFade;

  cylonColorCycle = preferences.getBool("cylonColorCycle", cylonColorCycle);
  lastSaved.cylonColorCycle = cylonColorCycle;

  cylonHueSpeed = preferences.getInt("cylonHueSpeed", cylonHueSpeed);
  lastSaved.cylonHueSpeed = cylonHueSpeed;

  confettiSpeed = preferences.getInt("confettiSpeed", confettiSpeed);
  lastSaved.confettiSpeed = confettiSpeed;

  twinkleSpeed = preferences.getInt("twinkleSpeed", twinkleSpeed);
  lastSaved.twinkleSpeed = twinkleSpeed;

  twinkleDensity = preferences.getInt("twinkleDensity", twinkleDensity);
  lastSaved.twinkleDensity = twinkleDensity;

  meteorSpeed = preferences.getInt("meteorSpeed", meteorSpeed);
  lastSaved.meteorSpeed = meteorSpeed;

  meteorFade = preferences.getInt("meteorFade", meteorFade);
  lastSaved.meteorFade = meteorFade;

  theaterChaseSpeed = preferences.getInt("theaterChaseSpd", theaterChaseSpeed);
  lastSaved.theaterChaseSpeed = theaterChaseSpeed;

  fireCooling = preferences.getInt("fireCooling", fireCooling);
  lastSaved.fireCooling = fireCooling;

  fireSparking = preferences.getInt("fireSparking", fireSparking);
  lastSaved.fireSparking = fireSparking;

  starTrekSpeed = preferences.getInt("starTrekSpeed", starTrekSpeed);
  lastSaved.starTrekSpeed = starTrekSpeed;

  starTrekFade = preferences.getInt("starTrekFade", starTrekFade);
  lastSaved.starTrekFade = starTrekFade;

  starTrekColorCycle = preferences.getBool("starTrekColor", starTrekColorCycle);
  lastSaved.starTrekColorCycle = starTrekColorCycle;

  starTrekHueSpeed = preferences.getInt("starTrekHueSpd", starTrekHueSpeed);
  lastSaved.starTrekHueSpeed = starTrekHueSpeed;

  geminiSpeed = preferences.getInt("geminiSpeed", geminiSpeed);
  lastSaved.geminiSpeed = geminiSpeed;

  geminiFade = preferences.getInt("geminiFade", geminiFade);
  lastSaved.geminiFade = geminiFade;

  confettiClapSpeed = preferences.getInt("confettiClapSpd", confettiClapSpeed);
  lastSaved.confettiClapSpeed = confettiClapSpeed;

  vuMeterSpeed = preferences.getInt("vuMeterSpeed", vuMeterSpeed);
  lastSaved.vuMeterSpeed = vuMeterSpeed;

  audioPulseFade = preferences.getInt("audioPulseFade", audioPulseFade);
  lastSaved.audioPulseFade = audioPulseFade;

  audioWaveSpeed = preferences.getInt("audioWaveSpeed", audioWaveSpeed);
  lastSaved.audioWaveSpeed = audioWaveSpeed;

  beatRippleSpeed = preferences.getInt("beatRippleSpeed", beatRippleSpeed);
  lastSaved.beatRippleSpeed = beatRippleSpeed;

  audioBeatRippleFade = preferences.getInt("audioBeatRipple", audioBeatRippleFade);
  lastSaved.audioBeatRippleFade = audioBeatRippleFade;

  audioBeatImplosionFade = preferences.getInt("audioBeatImpl", audioBeatImplosionFade);
  lastSaved.audioBeatImplosionFade = audioBeatImplosionFade;

  beatRippleWidth = preferences.getInt("beatRippleWidth", beatRippleWidth);
  lastSaved.beatRippleWidth = beatRippleWidth;

  beatRippleColorCycle = preferences.getBool("beatRippleColor", beatRippleColorCycle);
  lastSaved.beatRippleColorCycle = beatRippleColorCycle;

  audioSparkleFade = preferences.getInt("audioSparkleFade", audioSparkleFade);
  lastSaved.audioSparkleFade = audioSparkleFade;

  audioSparkleDensity = preferences.getInt("audioSparkleDens", audioSparkleDensity);
  lastSaved.audioSparkleDensity = audioSparkleDensity;

  audioStrobeFade = preferences.getInt("audioStrobeFade", audioStrobeFade);
  lastSaved.audioStrobeFade = audioStrobeFade;

  fireworksSparkleDensity = preferences.getInt("fireworksSparkle", fireworksSparkleDensity);
  lastSaved.fireworksSparkleDensity = fireworksSparkleDensity;

  rocketSpeed = preferences.getInt("rocketSpeed", rocketSpeed);
  lastSaved.rocketSpeed = rocketSpeed;

  rocketExplosionDensity = preferences.getInt("rocketExplDens", rocketExplosionDensity);
  lastSaved.rocketExplosionDensity = rocketExplosionDensity;

  rocketFade = preferences.getInt("rocketFade", rocketFade);
  lastSaved.rocketFade = rocketFade;

  sinelonSpeed = preferences.getInt("sinelonSpeed", sinelonSpeed);
  lastSaved.sinelonSpeed = sinelonSpeed;

  sinelonFade = preferences.getInt("sinelonFade", sinelonFade);
  lastSaved.sinelonFade = sinelonFade;

  sinelonHueSpeed = preferences.getInt("sinelonHueSpeed", sinelonHueSpeed);
  lastSaved.sinelonHueSpeed = sinelonHueSpeed;

  vuMeterLeftSpeed = preferences.getInt("vuMeterLeftSpd", vuMeterLeftSpeed);
  lastSaved.vuMeterLeftSpeed = vuMeterLeftSpeed;

  beatCometFade = preferences.getInt("beatCometFade", beatCometFade);
  lastSaved.beatCometFade = beatCometFade;

  loudnessNoiseDensity = preferences.getInt("loudnessNoiseDen", loudnessNoiseDensity);
  lastSaved.loudnessNoiseDensity = loudnessNoiseDensity;

  segmentViewSegments = preferences.getInt("segmentViewSegs", segmentViewSegments);
  lastSaved.segmentViewSegments = segmentViewSegments;

  beatFlashFade = preferences.getInt("beatFlashFade", beatFlashFade);
  lastSaved.beatFlashFade = beatFlashFade;

  breathingSpeed = preferences.getInt("breathingSpeed", breathingSpeed);
  lastSaved.breathingSpeed = breathingSpeed;

  plasmaSpeed1 = preferences.getInt("plasmaSpeed1", plasmaSpeed1);
  lastSaved.plasmaSpeed1 = plasmaSpeed1;

  plasmaSpeed2 = preferences.getInt("plasmaSpeed2", plasmaSpeed2);
  lastSaved.plasmaSpeed2 = plasmaSpeed2;

  plasmaScale1 = preferences.getInt("plasmaScale1", plasmaScale1);
  lastSaved.plasmaScale1 = plasmaScale1;

  plasmaScale2 = preferences.getInt("plasmaScale2", plasmaScale2);
  lastSaved.plasmaScale2 = plasmaScale2;

  colorWipeSpeed = preferences.getInt("colorWipeSpeed", colorWipeSpeed);
  lastSaved.colorWipeSpeed = colorWipeSpeed;

  chasingDotsNum = preferences.getInt("chasingDotsNum", chasingDotsNum);
  lastSaved.chasingDotsNum = chasingDotsNum;

  chasingDotsSpeed = preferences.getInt("chasingDotsSpd", chasingDotsSpeed);
  lastSaved.chasingDotsSpeed = chasingDotsSpeed;

  policeSpeed = preferences.getInt("policeSpeed", policeSpeed);
  lastSaved.policeSpeed = policeSpeed;

  preferences.end();
  Serial.println("Einstellungen geladen (Cached Init).");
}
