#include "Audio.h"
#include "Globals.h"

// --- AUDIO-PARAMETER INITIALISIERUNG ---
int audioNoiseFloor = 50;
int audioGain = 1000;
int micSensitivity = 5;
int smoothedLoudness = 0;
bool beatDetected = false;
long averageLoudness = 0;

// Diagnose
int currentPeakToPeak = 0;

AudioDiagFrame audioDiagFrames[AUDIO_DIAG_FRAMES];
volatile uint16_t audioDiagNext = 0;
volatile uint16_t audioDiagCount = 0;
uint16_t audioRawSamples[AUDIO_RAW_SAMPLES];
volatile bool audioRawRequested = false;
volatile bool audioRawReady = false;
volatile unsigned long audioRawDurationUs = 0;

// Rohaufnahme im festen Takt, angefordert vom Webserver-Task. Läuft hier im
// Hauptablauf, damit der ADC nie von zwei Tasks gleichzeitig gelesen wird.
static void captureRawAudio() {
  unsigned long start = micros();
  for (uint16_t i = 0; i < AUDIO_RAW_SAMPLES; i++) {
    while (micros() - start < (unsigned long)i * AUDIO_RAW_INTERVAL_US) {}
    audioRawSamples[i] = analogRead(MIC_PIN_AO);
  }
  audioRawDurationUs = micros() - start;
  audioRawRequested = false;
  audioRawReady = true;
}

void processAudio() {
  if (audioRawRequested && !audioRawReady) captureRawAudio();

  // --- 1. BURST SAMPLING (3ms) ---
  // Messung 30.10.2026: Bei Stille kamen einzelne ADC-Störspitzen (WLAN) von
  // 100-180 Stufen, echtes Rauschen nur ~35. Ungefiltert lagen Stille und laute
  // Musik nur ~1,5-fach auseinander. Daher fester 50-us-Takt, Median aus drei
  // Werten gegen Einzelspitzen und Tiefpass ~1 kHz gegen Reststörungen.
  const uint16_t windowSamples = AUDIO_WINDOW_US / AUDIO_RAW_INTERVAL_US;
  const float lowPassAlpha = 0.27f;   // 1 - exp(-2*pi*1000 Hz*50 us)
  int signalMax = 0;
  int signalMin = 4095;
  int rawMax = 0;
  int rawMin = 4095;
  int previous2 = 0, previous1 = 0;
  float filtered = 0;
  unsigned long startMicros = micros();

  for (uint16_t i = 0; i < windowSamples; i++) {
      while (micros() - startMicros < (unsigned long)i * AUDIO_RAW_INTERVAL_US) {}
      int sample = analogRead(MIC_PIN_AO);
      if (sample > rawMax) rawMax = sample;
      if (sample < rawMin) rawMin = sample;

      if (i == 0) { previous2 = previous1 = sample; filtered = sample; }
      int median = max(min(previous2, previous1), min(max(previous2, previous1), sample));
      previous2 = previous1;
      previous1 = sample;
      filtered += lowPassAlpha * (median - filtered);

      if (i < 2) continue;  // Median braucht drei echte Werte
      int value = (int)(filtered + 0.5f);
      if (value > signalMax) signalMax = value;
      if (value < signalMin) signalMin = value;
  }

  int peakToPeak = signalMax - signalMin;
  if (peakToPeak < 0) peakToPeak = 0;
  int rawPeakToPeak = rawMax - rawMin;

  // Update globale Diagnose-Variable für Webinterface
  currentPeakToPeak = peakToPeak;

  // --- 2. AUTOMATISCHE KALIBRIERUNG (AGC) ---
  // Ein Regler (1-10) verschiebt nur die Untergrenze der Rauschgrenze; die Automatik
  // ist immer an. Stufe 5 = bisherige feste Grenze 30. Abgeleitet aus der Messreihe
  // 30.10.2026 (gefiltert): Stille ~13 (90 % unter ~22), leise Musik ab ~19-38.
  static const uint8_t gates[10] = {60, 52, 44, 37, 30, 27, 24, 22, 20, 18};
  const int gate = gates[constrain(micSensitivity, 1, 10) - 1];
  {
      // A) Rauschfilter (Noise Floor) anpassen
      if (peakToPeak < audioNoiseFloor + 10) {
          if (peakToPeak < audioNoiseFloor) {
              audioNoiseFloor--;
          }
          else if (audioNoiseFloor < 150) {
              if (millis() % 10 == 0) audioNoiseFloor++;
          }
      } else {
           if (audioNoiseFloor > gate && millis() % 50 == 0) audioNoiseFloor--;
      }
      audioNoiseFloor = constrain(audioNoiseFloor, gate, 200);

      // B) Empfindlichkeit (Gain) anpassen
      // FAST ATTACK: Clipping vermeiden, aber Impulse (Klatschen) ignorieren
      if (peakToPeak > audioGain) {
          // Begrenzte Anstiegsgeschwindigkeit (Slew Rate Limiter)
          // Verhindert, dass ein einmaliges Klatschen den Gain sofort maximiert.
          audioGain = min(peakToPeak, audioGain + 50);
      }
      // SLOW DECAY: Empfindlichkeit erhöhen
      else {
          // Schwelle gesenkt von *2 auf *1.3, damit auch leise Musik als Signal erkannt wird
          if (peakToPeak > audioNoiseFloor * 1.3) {
             // Headroom gesenkt von 1.5 auf 1.1 für mehr Helligkeit
             if (audioGain > gate + 20 && audioGain > peakToPeak * 1.1) {
                 audioGain -= 1; // Verlangsamt: Wert bleibt länger stabil
             }
          } else {
             // Bei Stille: Drift zu einem Minimum.
             // Gesenkt auf 100 (war 300), damit wir nicht zu unempfindlich werden
             if (audioGain < gate + 70) audioGain++;
             else if (audioGain > 800) audioGain--;
          }
      }
      // Gain-Bereich erweitert
      audioGain = constrain(audioGain, gate + 20, 4000);
  }

  // --- 3. LAUTSTÄRKE BERECHNEN ---
  int usefulSignal = 0;
  if (peakToPeak > audioNoiseFloor) {
     usefulSignal = peakToPeak;
  }

  // Gain muss über der Rauschgrenze liegen, sonst teilt map() durch null.
  int gainTop = max(audioGain, audioNoiseFloor + 1);
  long volume = map(usefulSignal, audioNoiseFloor, gainTop, 0, 255);
  volume = constrain(volume, 0, 255);

  // Feste Glättung: der frühere Regler stand am Gerät auf 1 (reaktiv), so abgestimmt.
  smoothedLoudness = ((long)smoothedLoudness * 3 + volume) / 4;

  // --- 4. BEAT ERKENNEN ---
  averageLoudness = (averageLoudness * 31 + smoothedLoudness) / 32;

  // Messung 30.10.2026: Ein Schlag blieb mehrere Bilder über der Schwelle und
  // wurde nach 100 ms erneut gezählt (oft genau 114 ms Abstand, 160-240/min).
  // Daher erst nach Abfall unter die Schwelle wieder scharf, Sperrzeit 200 ms.
  static unsigned long lastBeatTime = 0;
  static bool beatArmed = true;
  int currentBeatThreshold = averageLoudness * 1.4;
  if (currentBeatThreshold < 40) currentBeatThreshold = 40;

  bool aboveThreshold = smoothedLoudness > currentBeatThreshold && smoothedLoudness > 50;
  if (!aboveThreshold) beatArmed = true;

  beatDetected = false;
  if (aboveThreshold && beatArmed && millis() - lastBeatTime > 200) {
      beatDetected = true;
      beatArmed = false;
      lastBeatTime = millis();
  }

  // --- 5. MESSDIAGNOSE ---
  AudioDiagFrame &f = audioDiagFrames[audioDiagNext];
  f.ms = millis();
  f.peakToPeak = peakToPeak;
  f.signalMin = signalMin;
  f.signalMax = signalMax;
  f.noiseFloor = audioNoiseFloor;
  f.gain = audioGain;
  f.volume = volume;
  f.smoothed = constrain(smoothedLoudness, 0, 255);
  f.average = constrain(averageLoudness, 0L, 255L);
  f.beat = beatDetected;
  f.digitalPin = digitalRead(MIC_PIN_DO);
  f.rawPeakToPeak = rawPeakToPeak;
  audioDiagNext = (audioDiagNext + 1) % AUDIO_DIAG_FRAMES;
  if (audioDiagCount < AUDIO_DIAG_FRAMES) audioDiagCount = audioDiagCount + 1;
}
