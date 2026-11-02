#pragma once
#include "Globals.h"

// Bilderlicht: drei Plaetze fuer von Hand gemalte LEDs (z. B. ueber Bildern), aufrufbar
// als Effekte 60-62. Die Farben liegen im RAM; Persistence.cpp schreibt einen Platz nur beim
// Speichern als Blob in den NVS-Namensraum "led-slots" (nie im Effekttakt). Nur der
// Webserver-Task veraendert Plaetze, und zwar unter xMutex.
const uint8_t LED_SLOT_COUNT = 3;
const uint8_t LED_SLOT_FIRST_EFFECT = 60;

struct LedSlot {
  uint16_t count;          // gespeicherte LEDs; 0 = leer
  CRGB colors[MAX_LEDS];
};
extern LedSlot ledSlots[LED_SLOT_COUNT];

// Zeigt einen Platz: gespeicherte Farben, alle uebrigen LEDs aus.
template<class Pixel> void showLedSlot(const LedSlot &slot, Pixel *pixels, uint16_t count) {
  for (uint16_t i = 0; i < count; ++i) {
    if (i < slot.count) pixels[i] = slot.colors[i];
    else pixels[i].r = pixels[i].g = pixels[i].b = 0;
  }
}
