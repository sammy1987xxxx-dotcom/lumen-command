#ifndef GLOBALS_H
#define GLOBALS_H
#include "LedOutputLimit.h"

#include <Arduino.h>
#include <FastLED.h>
#include <FreeRTOS.h>

// =================================================================================
// HIER ANPASSEN: Verkabelung und LED-Typ (nur beim Bauen festgelegt).
// Alles andere (WLAN, LED-Anzahl, Türbereich, Helligkeiten) stellt man auf der Webseite ein.
// =================================================================================
#ifndef LED_PIN
#define LED_PIN     2        // Datenleitung (DIN) des LED-Streifens an diesen GPIO
#endif
#ifndef LED_TYPE
#define LED_TYPE    WS2812B  // LED-Chip, z. B. WS2812B, SK6812, WS2811 (FastLED-Name)
#endif
#ifndef COLOR_ORDER
#define COLOR_ORDER GRB      // Falls Rot und Grün vertauscht sind: RGB versuchen
#endif
#ifndef MAX_LEDS
#define MAX_LEDS    600      // Obergrenze; die tatsächliche Anzahl kommt von der Webseite
#endif

// Mikrofonmodul KY-037 (am Gerät verwendet) für die Musik-Effekte; ohne Mikrofon
// einfach weglassen, die übrigen Effekte laufen trotzdem.
#ifndef MIC_PIN_DO
#define MIC_PIN_DO 16        // Digitalausgang (DO) des Mikrofonmoduls
#endif
#ifndef MIC_PIN_AO
#define MIC_PIN_AO 35        // Analogausgang (AO/OUT); nur ADC1-Pins (32–39) gehen mit WLAN
#endif

// --- GLOBALE VARIABLEN (Extern) ---
extern CRGB leds[MAX_LEDS];
extern uint16_t numLeds;
extern uint8_t brightness;
extern CRGB currentColor;
extern byte currentEffect;
extern byte gHue;

extern SemaphoreHandle_t xMutex;

// Türlicht: Meldung der Tür (1 = offen, 2 = zu), nur unter xMutex lesen/schreiben.
#include "DoorLight.h"
extern uint8_t doorLightRequest;
extern door_light::Renderer doorLight;
// Eigene Helligkeiten (0..255): Türlicht bei „Alles AUS“ und Türzeichen über laufenden Effekten.
extern uint8_t doorLightLevel;
extern uint8_t doorSignalLevel;
// Türlicht-Schalter der LED-Seite: aus = Türmeldungen werden ignoriert.
extern bool doorLightEnabled;
// Türzeichen für 53/54 (nur im Render-Loop) und Beat-Zähler für die Musik an die Tür.
#include "DoorScenes.h"
extern door_scene::Signal doorSignal;
extern volatile uint8_t doorMusicBeats;

#endif
