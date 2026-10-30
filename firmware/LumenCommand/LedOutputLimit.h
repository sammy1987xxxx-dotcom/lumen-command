#ifndef LED_OUTPUT_LIMIT_H
#define LED_OUTPUT_LIMIT_H

// Ausgang begrenzen; die gespeicherte/API-Skala 0..255 bleibt kompatibel.
constexpr int LED_OUTPUT_MAX = 210;
constexpr unsigned char limitLedOutput(int requested) {
  return requested < 0 ? 0 : (requested > LED_OUTPUT_MAX ? LED_OUTPUT_MAX : requested);
}

#endif
