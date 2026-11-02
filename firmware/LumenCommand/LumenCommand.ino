// =================================================================================
// Lumen Command – WLAN-LED-Steuerung für ESP32 und WS2812B
// Copyright (C) Thomas Brants (Sammy)
//
// Freie Software unter der GNU General Public License v3 mit Zusatz nach Abschnitt 7(b):
// Der Urheberhinweis „Thomas Brants (Sammy)“ bleibt in den Quelldateien und im Bereich
// „Entwicklung & Mitwirkung“ der Weboberfläche erhalten (siehe LICENSE, LIZENZ-ZUSATZ.md).
// Ohne jede Gewährleistung. Verkabelung und LED-Typ: Block „HIER ANPASSEN“ in Globals.h.
// =================================================================================

#include "Globals.h"
#include "Audio.h"
#include "Effects.h"
#include "WifiControl.h"
#include "Persistence.h"

// --- GLOBALE VARIABLEN DEFINITION ---
CRGB leds[MAX_LEDS];
uint16_t numLeds = 144;
uint8_t brightness = 96;
CRGB currentColor = CRGB::Red;
byte currentEffect = 0;
byte gHue = 0;
bool ledsPainted = false;
uint8_t doorLightRequest = 0;
door_light::Renderer doorLight;
uint8_t doorLightLevel = door_light::NIGHT_LEVEL;
uint8_t doorSignalLevel = 200;
bool doorLightEnabled = true;
volatile uint8_t doorMusicBeats = 0;

SemaphoreHandle_t xMutex;

void setup() {
  xMutex = xSemaphoreCreateMutex();

  Serial.begin(115200);
  
  loadSettings();
  
  pinMode(MIC_PIN_DO, INPUT);
  
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, numLeds);
  FastLED.setBrightness(limitLedOutput(brightness));
  
  fill_solid(leds, numLeds, CRGB::Black);
  FastLED.show();

  setupNetwork();
}

void loop() {
  const unsigned long frameInterval = 1000 / 60;
  unsigned long startLoopTime = millis();

  processAudio();
  if (beatDetected) doorMusicBeats = doorMusicBeats + 1;

  if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
    // Türlicht nur bei „Alles AUS“ (feste Farbe Schwarz); jede andere Wahl beendet es sofort.
    const uint32_t now = millis();
    const bool allOff = currentEffect == 0 && !ledsPainted && !(currentColor.r | currentColor.g | currentColor.b);
    // Türlicht abgeschaltet: neue Meldungen verwerfen, Laufendes ruhig ausgehen lassen.
    if (!doorLightEnabled) {
      doorLightRequest = 0;
      doorLight.close(now);
      doorSignal.close(now);
    }
    if (doorLightRequest == 1 && allOff) doorLight.open(now);
    if (doorLightRequest == 2) doorLight.close(now);
    if (doorLightRequest == 1) doorSignal.open(now);
    if (doorLightRequest == 2) doorSignal.close(now);
    doorLightRequest = 0;
    if (doorLight.active() && !allOff) doorLight.cancel();
    runCurrentEffect();
    if (doorLight.active()) {
      doorLight.render(leds, numLeds, now, doorLightLevel);
      if (!doorLight.active()) fill_solid(leds, numLeds, CRGB::Black);
    }
    // Türzeichen über jedem Effekt (außer „Alles AUS“ mit eigenem Türlicht). Nur auf einer Kopie
    // fürs Anzeigen: Effekte mit Nachleuchten rechnen mit ihrem eigenen Bild weiter, sonst würde
    // sich das Gold Bild für Bild aufsummieren.
    static CRGB effectFrame[MAX_LEDS];
    const uint16_t shown = numLeds > MAX_LEDS ? MAX_LEDS : numLeds;
    const bool doorOverlay = !allOff && doorSignal.visible(now);
    if (doorOverlay) {
      for (uint16_t i = 0; i < shown; ++i) effectFrame[i] = leds[i];
      // Eigener Regler fürs Türzeichen: den Effekt hier selbst auf die Hauptstufe bringen (so hell
      // wie sonst, 53 rechnet sie schon selbst), das Zeichen mit seiner Stufe darüberlegen; der
      // Treiber läuft dann auf voller Stufe. So bleibt das Zeichen hell, auch wenn der Hauptregler
      // fast ganz unten steht.
      if (currentEffect != 53) {
        const uint8_t effectScale = uint32_t(limitLedOutput(brightness)) * 255 / LED_OUTPUT_MAX;
        for (uint16_t i = 0; i < shown; ++i) leds[i].nscale8_video(effectScale);
      }
      door_scene::Renderer::overlayDoor(leds, shown, now, doorSignal, doorSignalLevel,
                                        uint32_t(doorSignalLevel) * 70 / 200);
    }
    // Türlicht, Türzeichen und Ruhe (53) rechnen ihre Helligkeit selbst und fein abgestuft; der
    // Treiber läuft dafür auf voller Stufe, die Ausgangsgrenze bleibt. Danach gilt wieder der Regler.
    static bool ownLevel = false;
    const bool wantOwnLevel = doorLight.active() || currentEffect == 53 || doorOverlay;
    if (wantOwnLevel) FastLED.setBrightness(limitLedOutput(255));
    else if (ownLevel) FastLED.setBrightness(limitLedOutput(brightness));
    ownLevel = wantOwnLevel;
    // Nachtglut (41), Ruhe (53), Einschlafen (58) und Gluehwuermchen (59) runden selbst fest je
    // LED; das zeitliche FastLED-Dithering wuerde bei so kleinen Werten sichtbar zappeln.
    const bool ownDither = currentEffect == 41 || currentEffect == 53 || currentEffect == 58 || currentEffect == 59;
    FastLED.setDither(ownDither ? DISABLE_DITHER : BINARY_DITHER);
    // LED-Anzahl im Betrieb geaendert: Die Ausgabe kannte bisher nur den Startwert.
    // Beim Verkleinern zuerst die ueberzaehligen LEDs schwarz senden, sonst behalten
    // sie ihre letzte Farbe.
    static uint16_t outputCount = numLeds;
    if (numLeds != outputCount) {
      if (numLeds < outputCount) fill_solid(leds + numLeds, outputCount - numLeds, CRGB::Black);
      FastLED.show();
      FastLED[0].setLeds(leds, numLeds);
      outputCount = numLeds;
    }
    FastLED.show();
    if (doorOverlay) for (uint16_t i = 0; i < shown; ++i) leds[i] = effectFrame[i];
    xSemaphoreGive(xMutex);
  }

  unsigned long loopDuration = millis() - startLoopTime;
  if (loopDuration < frameInterval) {
    vTaskDelay((frameInterval - loopDuration) / portTICK_PERIOD_MS);
  } 

  handlePersistence();
}
