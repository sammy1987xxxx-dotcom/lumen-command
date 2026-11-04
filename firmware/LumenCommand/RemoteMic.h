#ifndef REMOTE_MIC_H
#define REMOTE_MIC_H

#include <stdint.h>
#include <stddef.h>

// WLAN-Mikrofon (Würfel AtomMartin ab 0.5.0): Der Würfel schickt etwa 60-mal pro Sekunde
// nur einen Pegel per UDP, kein Audio. Paket 8 Byte: "HMIC", Version 1, Folgezähler,
// Spitze-Spitze 0..4095 (Little Endian) im Wertebereich des eigenen ADC.
// Gegenstück: Sprachassistant/AtomMartin/LedMicRules.h.
namespace remote_mic {
const uint16_t PORT = 4212;
const size_t PACKET_SIZE = 8;
const uint16_t MAX_LEVEL = 4095;
// Ohne frisches Paket nach 250 ms gilt wieder das eingebaute Mikrofon.
const uint32_t STALE_MS = 250;

inline bool parse(const uint8_t *p, size_t len, uint16_t &level) {
  if (len != PACKET_SIZE || p[0] != 'H' || p[1] != 'M' || p[2] != 'I' || p[3] != 'C' || p[4] != 1) return false;
  level = uint16_t(p[6] | (uint16_t(p[7]) << 8));
  return level <= MAX_LEVEL;
}

// Schreiben im Webserver-Task (Kern 0), Lesen in processAudio() (Kern 1). Ein gemischtes
// Paar aus altem Zeitpunkt und neuem Pegel ist harmlos.
struct Receiver {
  volatile uint16_t level = 0;
  volatile uint32_t at = 0;
  volatile bool seen = false;
  void accept(uint16_t value, uint32_t now) { level = value; at = now; seen = true; }
  bool fresh(uint32_t now) const { return seen && uint32_t(now - at) <= STALE_MS; }
};
}

#endif
