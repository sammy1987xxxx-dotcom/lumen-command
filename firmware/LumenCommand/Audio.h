#ifndef AUDIO_H
#define AUDIO_H

#include <Arduino.h>

// --- AUDIO VARIABLEN (Extern) ---
extern int audioNoiseFloor;   // Automatik: aktuelle Rauschgrenze
extern int audioGain;         // Automatik: aktueller Vollausschlag
extern int micSensitivity;    // 1 = nur laute Musik ... 10 = sehr empfindlich
extern int smoothedLoudness;
extern bool beatDetected;
extern long averageLoudness;

// --- DIAGNOSE VARIABLEN ---
extern int currentPeakToPeak;

// --- MESSDIAGNOSE (/audio-diag) ---
// Nur Aufzeichnung, ändert die Auswertung nicht.
struct AudioDiagFrame {
  uint32_t ms;
  uint16_t peakToPeak, signalMin, signalMax, noiseFloor, gain;
  uint8_t volume, smoothed, average, beat, digitalPin;
  uint16_t rawPeakToPeak;  // ungefiltert, zum Vergleich
};
const uint16_t AUDIO_DIAG_FRAMES = 600;        // ~10 s bei 60 Bildern/s
extern AudioDiagFrame audioDiagFrames[AUDIO_DIAG_FRAMES];
extern volatile uint16_t audioDiagNext;
extern volatile uint16_t audioDiagCount;

const uint16_t AUDIO_RAW_SAMPLES = 2048;       // ~102 ms bei 50 us Abstand
const uint16_t AUDIO_RAW_INTERVAL_US = 50;
extern uint16_t audioRawSamples[AUDIO_RAW_SAMPLES];
extern volatile bool audioRawRequested;
extern volatile bool audioRawReady;
extern volatile unsigned long audioRawDurationUs;
const unsigned long AUDIO_WINDOW_US = 3000;

// --- FUNKTIONEN ---
void processAudio();

#endif
