#include "WifiControl.h"
#include "Globals.h"
#include "Audio.h"
#include "Effects.h"
#include "Persistence.h"
#include "WebSeite.h"
#include "DeviceAccess.h"
#include "LedSlots.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <Update.h>
#include <WiFiUdp.h>

WebServer server(80);
static device_access::Credentials ledAccess;
TaskHandle_t WebServerTask;
// Türlicht: Türen senden „door_light“ auf demselben Port wie ihre Heartbeats.
static WiFiUDP doorLightUdp;
static const uint16_t DOOR_LIGHT_PORT = 4210;
// Effekt 54: Musik an die Tür, die zuletzt „door_light“ gemeldet hat (nur Türen mit Schalter).
static IPAddress musicDoorIp;
static bool musicDoorKnown = false;

// --- WLAN-Daten ---
// Keine Zugangsdaten im Code: Das Gerät nimmt nur das über die Webseite gespeicherte WLAN.
// Ohne gespeichertes WLAN startet sofort der Einrichtungs-Hotspot (Standardzugang).
String ssid = "";
String password = "";

// Webseiten (update_html, index_html): WebSeite.h

// --- HANDLER ---

void handleRoot() {
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/html", "");
    server.sendContent_P(index_html);
    server.sendContent("");
}

void handleStatus() {
  StaticJsonDocument<2048> json;
  if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
    json["activeEffect"] = currentEffect;
    json["brightness"] = brightness;
    json["numLeds"] = numLeds;
    json["doorLight"] = doorLight.phase();
    json["doorLightLevel"] = doorLightLevel;
    json["doorSignalLevel"] = doorSignalLevel;
    json["doorLightEnabled"] = doorLightEnabled;
    json["doorFirst"] = door_light::doorRange().first + 1;  // 1-basiert wie die Webseite
    json["doorLast"] = door_light::doorRange().last + 1;
    json["solidColor"] = (uint32_t)currentColor;
    // Bilderlicht 1-3: gespeicherte LED-Anzahl je Platz, 0 = leer.
    JsonArray slots = json["ledSlots"].to<JsonArray>();
    for (uint8_t s = 0; s < LED_SLOT_COUNT; ++s) slots.add(ledSlots[s].count);
    json["rainbowSpeed"] = rainbowSpeed;
    json["rainbowGap"] = rainbowGap;
    json["cylonSpeed"] = cylonSpeed;
    json["cylonFade"] = cylonFade;
    json["cylonColorCycle"] = cylonColorCycle;
    json["cylonHueSpeed"] = cylonHueSpeed;
    json["confettiSpeed"] = confettiSpeed;
    json["goldSpeed"] = goldSpeed;
    json["goldPause"] = goldPause;
    json["goldTail"] = goldTail;
    json["goldRed"] = goldRed;
    json["goldGreen"] = goldGreen;
    json["goldBlue"] = goldBlue;
    json["starTrekSpeed"] = starTrekSpeed;
    json["starTrekFade"] = starTrekFade;
    json["starTrekColorCycle"] = starTrekColorCycle;
    json["starTrekHueSpeed"] = starTrekHueSpeed;
    json["twinkleSpeed"] = twinkleSpeed;
    json["twinkleDensity"] = twinkleDensity;
    json["meteorSpeed"] = meteorSpeed;
    json["meteorFade"] = meteorFade;
    json["theaterChaseSpeed"] = theaterChaseSpeed;
    json["fireCooling"] = fireCooling;
    json["fireSparking"] = fireSparking;
    json["geminiSpeed"] = geminiSpeed;
    json["geminiFade"] = geminiFade;
    json["micSensitivity"] = micSensitivity;
    json["audioNoiseFloor"] = audioNoiseFloor;
    json["audioGain"] = audioGain;
    json["currentPeakToPeak"] = currentPeakToPeak;
    json["confettiClapSpeed"] = confettiClapSpeed;
    json["vuMeterSpeed"] = vuMeterSpeed;
    json["audioPulseFade"] = audioPulseFade;
    json["audioWaveSpeed"] = audioWaveSpeed;
    json["beatRippleSpeed"] = beatRippleSpeed;
    json["audioBeatRippleFade"] = audioBeatRippleFade;
    json["audioBeatImplosionFade"] = audioBeatImplosionFade;
    json["beatRippleWidth"] = beatRippleWidth;
    json["beatRippleColorCycle"] = beatRippleColorCycle;
    json["audioSparkleFade"] = audioSparkleFade;
    json["audioSparkleDensity"] = audioSparkleDensity;
    json["audioStrobeFade"] = audioStrobeFade;
    json["fireworksSparkleDensity"] = fireworksSparkleDensity;
    json["rocketSpeed"] = rocketSpeed;
    json["rocketExplosionDensity"] = rocketExplosionDensity;
    json["rocketFade"] = rocketFade;
    json["sinelonSpeed"] = sinelonSpeed;
    json["sinelonFade"] = sinelonFade;
    json["sinelonHueSpeed"] = sinelonHueSpeed;
    json["vuMeterLeftSpeed"] = vuMeterLeftSpeed;
    json["beatCometFade"] = beatCometFade;
    json["loudnessNoiseDensity"] = loudnessNoiseDensity;
    json["segmentViewSegments"] = segmentViewSegments;
    json["beatFlashFade"] = beatFlashFade;
    json["breathingSpeed"] = breathingSpeed;
    json["plasmaSpeed1"] = plasmaSpeed1;
    json["plasmaSpeed2"] = plasmaSpeed2;
    json["plasmaScale1"] = plasmaScale1;
    json["plasmaScale2"] = plasmaScale2;
    json["colorWipeSpeed"] = colorWipeSpeed;
    json["chasingDotsNum"] = chasingDotsNum;
    json["chasingDotsSpeed"] = chasingDotsSpeed;
    json["policeSpeed"] = policeSpeed;

    xSemaphoreGive(xMutex);
  } else {
    server.send(500, "text/plain", "Internal Server Error: Mutex timeout");
    return;
  }

  server.setContentLength(measureJson(json));
  server.send(200, "application/json", "");
  serializeJson(json, server.client());
}

void handleEffect() {
  if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
    if (server.hasArg("id")) currentEffect = server.arg("id").toInt();
    ledsPainted = false;
    saveSettings();
    xSemaphoreGive(xMutex);
  }
  server.send(200, "text/plain", "OK");
}

void handleToggleParam() {
  if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
    if (server.hasArg("param")) {
      String param = server.arg("param");
      if (param == "cylonColor") cylonColorCycle = !cylonColorCycle;
      if (param == "starTrekColor") starTrekColorCycle = !starTrekColorCycle;
      if (param == "beatRippleColor") beatRippleColorCycle = !beatRippleColorCycle;
      if (param == "doorLight") doorLightEnabled = !doorLightEnabled;
    }
    saveSettings();
    xSemaphoreGive(xMutex);
  }
  server.send(200, "text/plain", "OK");
}

void handleSetAll() {
  if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
    currentEffect = 0;
    ledsPainted = false;
    if (server.hasArg("color")) {
      String hexColor = server.arg("color");
      long number = strtol(hexColor.c_str(), NULL, 16);
      currentColor = (CRGB)number;
      fill_solid(leds, numLeds, currentColor);
    }
    saveSettings();
    xSemaphoreGive(xMutex);
  }
  server.send(200, "text/plain", "OK");
}

void handleSetSingle() {
  if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
    currentEffect = 0;
    ledsPainted = true;
    if (server.hasArg("color")) {
      long first = -1, last = -1;
      if (server.hasArg("index")) first = last = server.arg("index").toInt();
      // Bereich zum Malen (Bilderlicht): 0-basiert, beide Grenzen eingeschlossen, vertauscht erlaubt.
      if (server.hasArg("from") && server.hasArg("to")) {
        first = server.arg("from").toInt();
        last = server.arg("to").toInt();
        if (first > last) { const long swap = first; first = last; last = swap; }
      }
      const CRGB color = (CRGB)strtol(server.arg("color").c_str(), NULL, 16);
      if (first < 0) first = 0;
      if (last > long(numLeds) - 1) last = long(numLeds) - 1;
      for (long i = first; i <= last; ++i) leds[i] = color;
    }
    saveSettings();
    xSemaphoreGive(xMutex);
  }
  server.send(200, "text/plain", "OK");
}

// Zeigt genau eine LED weiß (Rest aus), um Positionen am Schlauch zu finden.
// Wird bewusst nicht gespeichert: Nach einem Neustart läuft der vorige Effekt.
void handleShowLed() {
  if (!server.hasArg("index")) {
    server.send(400, "text/plain", "index fehlt");
    return;
  }
  const long index = server.arg("index").toInt();
  bool shown = false;
  if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
    if (index >= 0 && index < numLeds) {
      currentEffect = 0;
      ledsPainted = false;
      fill_solid(leds, numLeds, CRGB::Black);
      leds[index] = CRGB::White;
      shown = true;
    }
    xSemaphoreGive(xMutex);
  }
  server.send(shown ? 200 : 400, "text/plain", shown ? "OK" : "index ausserhalb");
}

// Bilderlicht: das aktuelle Bild (gemalte LEDs) auf Platz 1-3 sichern und den Platz einschalten.
void handleSaveSlot() {
  const long slot = server.hasArg("slot") ? server.arg("slot").toInt() : 0;
  if (slot < 1 || slot > LED_SLOT_COUNT) {
    server.send(400, "text/plain", "slot 1-3");
    return;
  }
  LedSlot &target = ledSlots[slot - 1];
  if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
    target.count = numLeds > MAX_LEDS ? MAX_LEDS : numLeds;
    for (uint16_t i = 0; i < target.count; ++i) target.colors[i] = leds[i];
    currentEffect = LED_SLOT_FIRST_EFFECT + slot - 1;
    ledsPainted = false;
    saveSettings();
    xSemaphoreGive(xMutex);
  }
  // Flash-Schreiben außerhalb der Sperre, damit die Ausgabe nicht stockt; nur dieser Task schreibt Plätze.
  const bool stored = storeLedSlot(slot - 1);
  server.send(stored ? 200 : 500, "text/plain", stored ? "OK" : "Speichern fehlgeschlagen");
}

// Gespeicherte Farben eines Platzes als Hex-Folge (6 Zeichen je LED) für das Raster der Webseite.
void handleSlotColors() {
  const long slot = server.hasArg("slot") ? server.arg("slot").toInt() : 0;
  if (slot < 1 || slot > LED_SLOT_COUNT) {
    server.send(400, "text/plain", "slot 1-3");
    return;
  }
  static char text[MAX_LEDS * 6 + 1];
  const LedSlot &source = ledSlots[slot - 1];
  for (uint16_t i = 0; i < source.count; ++i)
    snprintf(text + i * 6, 7, "%02X%02X%02X", source.colors[i].r, source.colors[i].g, source.colors[i].b);
  text[source.count * 6] = 0;
  server.send(200, "text/plain", text);
}

void handleSettings() {
  if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
    if (server.hasArg("brightness")) {
      brightness = constrain(server.arg("brightness").toInt(), 0L, 255L);
      FastLED.setBrightness(limitLedOutput(brightness));
      // Gemalte LEDs (Bilderlicht vor dem Speichern) nicht mit der festen Farbe überschreiben.
      if (currentEffect == 0 && !ledsPainted) {
        fill_solid(leds, numLeds, currentColor);
      }
    }
    if (server.hasArg("doorLightLevel")) {
      doorLightLevel = constrain(server.arg("doorLightLevel").toInt(), 0L, 255L);
    }
    if (server.hasArg("doorSignalLevel")) {
      doorSignalLevel = constrain(server.arg("doorSignalLevel").toInt(), 0L, 255L);
    }
    // Türlage 1-basiert wie die LED-Nummern der Webseite; vertauschte Angaben werden sortiert.
    if (server.hasArg("doorFirst") && server.hasArg("doorLast")) {
      long first = constrain(server.arg("doorFirst").toInt(), 1L, long(MAX_LEDS)) - 1;
      long last = constrain(server.arg("doorLast").toInt(), 1L, long(MAX_LEDS)) - 1;
      if (first > last) { const long swap = first; first = last; last = swap; }
      door_light::doorRange().first = first;
      door_light::doorRange().last = last;
    }
    if (server.hasArg("leds")) {
      numLeds = constrain(server.arg("leds").toInt(), 1L, long(MAX_LEDS));
    }
    saveSettings();
    xSemaphoreGive(xMutex);
  }
  server.send(200, "text/plain", "OK");
}

void handleWifiSettings() {
  if (server.hasArg("ssid") && server.hasArg("password")) {
     String n_ssid = server.arg("ssid");
     String n_pass = server.arg("password");

     if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
         Preferences prefs;
         prefs.begin("wifi-config", false);
         prefs.putString("ssid", n_ssid);
         prefs.putString("password", n_pass);
         prefs.end();
         xSemaphoreGive(xMutex);
     }

     server.send(200, "text/plain", "Gespeichert. Starte neu...");
     delay(1000);
     ESP.restart();
  } else {
     server.send(400, "text/plain", "Fehlende Parameter");
  }
}

void handleParams() {
  if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
    if (server.hasArg("goldSpeed")) { goldSpeed = constrain(server.arg("goldSpeed").toInt(), long(10), long(200)); }
    if (server.hasArg("goldPause")) { goldPause = constrain(server.arg("goldPause").toInt(), long(0), long(15000)); }
    if (server.hasArg("goldTail")) { goldTail = constrain(server.arg("goldTail").toInt(), long(80), long(1200)); }
    if (server.hasArg("goldRed")) { goldRed = constrain(server.arg("goldRed").toInt(), long(0), long(255)); }
    if (server.hasArg("goldGreen")) { goldGreen = constrain(server.arg("goldGreen").toInt(), long(0), long(255)); }
    if (server.hasArg("goldBlue")) { goldBlue = constrain(server.arg("goldBlue").toInt(), long(0), long(255)); }

    if (server.hasArg("micSensitivity")) { micSensitivity = constrain(server.arg("micSensitivity").toInt(), 1L, 10L); }

  if (server.hasArg("rainbowSpeed")) { rainbowSpeed = server.arg("rainbowSpeed").toInt();
}
  if (server.hasArg("rainbowGap")) { rainbowGap = server.arg("rainbowGap").toInt(); }
  if (server.hasArg("cylonSpeed")) { cylonSpeed = server.arg("cylonSpeed").toInt();
}
  if (server.hasArg("cylonFade")) { cylonFade = server.arg("cylonFade").toInt(); }
  if (server.hasArg("cylonHueSpeed")) { cylonHueSpeed = server.arg("cylonHueSpeed").toInt();
}
  if (server.hasArg("confettiSpeed")) { confettiSpeed = server.arg("confettiSpeed").toInt(); }
  if (server.hasArg("starTrekSpeed")) { starTrekSpeed = server.arg("starTrekSpeed").toInt();
}
  if (server.hasArg("starTrekFade")) { starTrekFade = server.arg("starTrekFade").toInt(); }
  if (server.hasArg("starTrekHueSpeed")) { starTrekHueSpeed = server.arg("starTrekHueSpeed").toInt();
}
  if (server.hasArg("twinkleSpeed")) { twinkleSpeed = server.arg("twinkleSpeed").toInt(); }
  if (server.hasArg("twinkleDensity")) { twinkleDensity = server.arg("twinkleDensity").toInt();
}
  if (server.hasArg("meteorSpeed")) { meteorSpeed = server.arg("meteorSpeed").toInt(); }
  if (server.hasArg("meteorFade")) { meteorFade = server.arg("meteorFade").toInt();
}
  if (server.hasArg("theaterChaseSpeed")) { theaterChaseSpeed = server.arg("theaterChaseSpeed").toInt(); }
  if (server.hasArg("fireCooling")) { fireCooling = server.arg("fireCooling").toInt();
}
  if (server.hasArg("fireSparking")) { fireSparking = server.arg("fireSparking").toInt(); }
  if (server.hasArg("geminiSpeed")) { geminiSpeed = server.arg("geminiSpeed").toInt();
}
  if (server.hasArg("geminiFade")) { geminiFade = server.arg("geminiFade").toInt(); }


  if (server.hasArg("confettiClapSpeed")) { confettiClapSpeed = server.arg("confettiClapSpeed").toInt();
}
  if (server.hasArg("vuMeterSpeed")) { vuMeterSpeed = server.arg("vuMeterSpeed").toInt(); }
  if (server.hasArg("audioPulseFade")) { audioPulseFade = server.arg("audioPulseFade").toInt();
}
  if (server.hasArg("audioWaveSpeed")) { audioWaveSpeed = server.arg("audioWaveSpeed").toInt(); }

  if (server.hasArg("beatRippleSpeed")) { beatRippleSpeed = server.arg("beatRippleSpeed").toInt(); }
  if (server.hasArg("audioBeatRippleFade")) { audioBeatRippleFade = server.arg("audioBeatRippleFade").toInt();
}
  if (server.hasArg("audioBeatImplosionFade")) { audioBeatImplosionFade = server.arg("audioBeatImplosionFade").toInt(); }
  if (server.hasArg("beatRippleWidth")) { beatRippleWidth = server.arg("beatRippleWidth").toInt(); }

  if (server.hasArg("audioSparkleFade")) { audioSparkleFade = server.arg("audioSparkleFade").toInt(); }
  if (server.hasArg("audioSparkleDensity")) { audioSparkleDensity = server.arg("audioSparkleDensity").toInt();
}
  if (server.hasArg("audioStrobeFade")) { audioStrobeFade = server.arg("audioStrobeFade").toInt();
}
  if (server.hasArg("fireworksSparkleDensity")) { fireworksSparkleDensity = server.arg("fireworksSparkleDensity").toInt();
}

  if (server.hasArg("rocketSpeed")) { rocketSpeed = server.arg("rocketSpeed").toInt(); }
  if (server.hasArg("rocketExplosionDensity")) { rocketExplosionDensity = server.arg("rocketExplosionDensity").toInt(); }
  if (server.hasArg("rocketFade")) { rocketFade = server.arg("rocketFade").toInt(); }

  if (server.hasArg("sinelonSpeed")) { sinelonSpeed = server.arg("sinelonSpeed").toInt(); }
  if (server.hasArg("sinelonFade")) { sinelonFade = server.arg("sinelonFade").toInt(); }
  if (server.hasArg("sinelonHueSpeed")) { sinelonHueSpeed = server.arg("sinelonHueSpeed").toInt(); }

  if (server.hasArg("vuMeterLeftSpeed")) { vuMeterLeftSpeed = server.arg("vuMeterLeftSpeed").toInt(); }
  if (server.hasArg("beatCometFade")) { beatCometFade = server.arg("beatCometFade").toInt(); }
  if (server.hasArg("loudnessNoiseDensity")) { loudnessNoiseDensity = server.arg("loudnessNoiseDensity").toInt(); }
  if (server.hasArg("segmentViewSegments")) { segmentViewSegments = server.arg("segmentViewSegments").toInt(); }
  if (server.hasArg("beatFlashFade")) { beatFlashFade = server.arg("beatFlashFade").toInt(); }
  if (server.hasArg("breathingSpeed")) { breathingSpeed = server.arg("breathingSpeed").toInt(); }
  if (server.hasArg("plasmaSpeed1")) { plasmaSpeed1 = server.arg("plasmaSpeed1").toInt(); }
  if (server.hasArg("plasmaSpeed2")) { plasmaSpeed2 = server.arg("plasmaSpeed2").toInt(); }
  if (server.hasArg("plasmaScale1")) { plasmaScale1 = server.arg("plasmaScale1").toInt(); }
  if (server.hasArg("plasmaScale2")) { plasmaScale2 = server.arg("plasmaScale2").toInt(); }
  if (server.hasArg("colorWipeSpeed")) { colorWipeSpeed = server.arg("colorWipeSpeed").toInt(); }
  if (server.hasArg("chasingDotsNum")) { chasingDotsNum = server.arg("chasingDotsNum").toInt(); }
  if (server.hasArg("chasingDotsSpeed")) { chasingDotsSpeed = server.arg("chasingDotsSpeed").toInt(); }
  if (server.hasArg("policeSpeed")) { policeSpeed = server.arg("policeSpeed").toInt(); }

    saveSettings();
    xSemaphoreGive(xMutex);
  }
  server.send(200, "text/plain", "OK");
}

// Mikrofon-Messdiagnose als CSV: ?mode=history (Standard, letzte ~10 s je Bild)
// oder ?mode=raw (Rohaufnahme ~100 ms im festen Takt, LEDs stehen dabei kurz still).
void handleAudioDiag() {
  bool raw = server.arg("mode") == "raw";
  if (raw) {
    audioRawReady = false;
    audioRawRequested = true;
    unsigned long waitStart = millis();
    while (!audioRawReady && millis() - waitStart < 2000) delay(5);
    if (!audioRawReady) {
      audioRawRequested = false;
      server.send(503, "text/plain", "Rohaufnahme nicht abgeschlossen");
      return;
    }
  }

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/csv", "");
  String chunk;
  chunk.reserve(2200);
  chunk += "# mode=" + String(raw ? "raw" : "history");
  chunk += " window_us=" + String(AUDIO_WINDOW_US);
  chunk += " sensitivity=" + String(micSensitivity);
  chunk += " effect=" + String(currentEffect);
  chunk += " rssi=" + String(WiFi.RSSI());
  chunk += " uptime_ms=" + String(millis());

  if (raw) {
    chunk += " interval_us=" + String(AUDIO_RAW_INTERVAL_US);
    chunk += " duration_us=" + String(audioRawDurationUs) + "\nsample\n";
    for (uint16_t i = 0; i < AUDIO_RAW_SAMPLES; i++) {
      chunk += String(audioRawSamples[i]) + "\n";
      if (chunk.length() > 2000) { server.sendContent(chunk); chunk = ""; }
    }
  } else {
    uint16_t count = audioDiagCount;
    uint16_t first = (audioDiagNext + AUDIO_DIAG_FRAMES - count) % AUDIO_DIAG_FRAMES;
    chunk += "\nms,peakToPeak,min,max,noiseFloor,gain,volume,smoothed,average,beat,do,rawPeakToPeak\n";
    for (uint16_t n = 0; n < count; n++) {
      AudioDiagFrame f = audioDiagFrames[(first + n) % AUDIO_DIAG_FRAMES];
      char line[80];
      snprintf(line, sizeof(line), "%lu,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
               (unsigned long)f.ms, f.peakToPeak, f.signalMin, f.signalMax, f.noiseFloor, f.gain,
               f.volume, f.smoothed, f.average, f.beat, f.digitalPin, f.rawPeakToPeak);
      chunk += line;
      if (chunk.length() > 2000) { server.sendContent(chunk); chunk = ""; }
    }
  }
  if (chunk.length()) server.sendContent(chunk);
  server.sendContent("");
}

// Eine UDP-Meldung je Aufruf; Heartbeats anderer Geräte auf dem Port werden verworfen.
void pollDoorLight() {
  if (doorLightUdp.parsePacket() <= 0) return;
  char packet[256];
  const int len = doorLightUdp.read(packet, sizeof(packet) - 1);
  if (len <= 0) return;
  packet[len] = 0;
  StaticJsonDocument<256> doc;
  if (deserializeJson(doc, packet)) return;
  // Lebenszeichen einer Tür mit eingeschaltetem Türlicht: Adresse für die Musik merken.
  if (strcmp(doc["type"] | "", "door_announce") == 0 && (doc["door_light"] | false)) {
    musicDoorIp = doorLightUdp.remoteIP();
    musicDoorKnown = true;
    return;
  }
  if (strcmp(doc["type"] | "", "door_light") != 0) return;
  const char *state = doc["state"] | "";
  const uint8_t request = strcmp(state, "open") == 0 ? 1 : (strcmp(state, "closed") == 0 ? 2 : 0);
  if (!request) return;
  musicDoorIp = doorLightUdp.remoteIP();
  musicDoorKnown = true;
  if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
    doorLightRequest = request;
    xSemaphoreGive(xMutex);
  }
  Serial.printf("Türlicht: %s meldet %s\n", (const char *)(doc["name"] | "?"), state);
}

// Effekt 54: etwa 30-mal pro Sekunde Lautstärke und Beat-Zähler direkt an die Tür.
// Der Zähler statt eines Beat-Flags übersteht verlorene Pakete; die Tür erkennt die Änderung.
void sendDoorMusic() {
  static uint32_t lastSent = 0;
  if (currentEffect != 54 || !musicDoorKnown) return;
  const uint32_t now = millis();
  if (now - lastSent < 33) return;
  lastSent = now;
  char packet[64];
  const int len = snprintf(packet, sizeof(packet), "{\"type\":\"door_music\",\"l\":%d,\"b\":%u}",
                           constrain(smoothedLoudness, 0, 255), unsigned(doorMusicBeats));
  doorLightUdp.beginPacket(musicDoorIp, DOOR_LIGHT_PORT);
  doorLightUdp.write(reinterpret_cast<const uint8_t *>(packet), len);
  doorLightUdp.endPacket();
}

void webServerTask(void *pvParameters) {
  Serial.println("Webserver-Task startet auf Kern 0.");
  for (;;) {
    server.handleClient();
    pollDoorLight();
    sendDoorMusic();
    delay(2);
  }
}

void setupNetwork() {
  ledAccess.begin("led-access");
  server.addMiddleware([](WebServer &web, std::function<bool()> next) -> bool {
    web.sendHeader("Cache-Control", "no-store");
    if (!ledAccess.authorize(web)) return false;
    return next();
  });
  server.on("/access", HTTP_GET, []() { ledAccess.page(server); });
  server.on("/access", HTTP_POST, []() { ledAccess.change(server); });
  // --- WLAN Konfiguration laden ---
  Preferences prefs;
  prefs.begin("wifi-config", true);
  String storedSsid = prefs.getString("ssid", "");
  String storedPass = prefs.getString("password", "");
  prefs.end();

  if (storedSsid != "") {
    ssid = storedSsid;
    password = storedPass;
  }

  bool connected = false;
  if (ssid != "") {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    WiFi.setSleep(false);
    Serial.print("Verbinde mit WLAN: ");
    Serial.println(ssid);
  } else {
    Serial.println("Kein WLAN gespeichert.");
  }

  unsigned long startAttempt = millis();

  while (ssid != "" && millis() - startAttempt < 10000) {
    if (WiFi.status() == WL_CONNECTED) {
      connected = true;
      break;
    }
    delay(500);
    Serial.print(".");
  }

  if (!connected) {
    Serial.println("\nKeine Verbindung! Starte AP-Modus...");
    WiFi.disconnect();
    WiFi.mode(WIFI_AP);
    const size_t apLength = strlen(ledAccess.password());
    WiFi.softAP("LED-Controller-Setup", apLength >= 8 && apLength <= 63 ?
        ledAccess.password() : HOMESERVER_DEFAULT_PASSWORD);
    Serial.print("AP gestartet. IP: ");
    Serial.println(WiFi.softAPIP());

    for(int i=0; i<3; i++) {
        fill_solid(leds, numLeds, CRGB::Red);
        FastLED.show();
        delay(500);
        fill_solid(leds, numLeds, CRGB::Black);
        FastLED.show();
        delay(500);
    }
  } else {
    Serial.println("\nVerbunden! IP: ");
    Serial.println(WiFi.localIP());

    for(int i=0; i<3; i++) {
        fill_solid(leds, numLeds, CRGB::Green);
        FastLED.show();
        delay(500);
        fill_solid(leds, numLeds, CRGB::Black);
        FastLED.show();
        delay(500);
    }
  }

  server.on("/", handleRoot);
  server.on("/status.json", handleStatus);
  server.on("/audio-diag", handleAudioDiag);
  server.on("/effect", handleEffect);
  server.on("/toggleparam", handleToggleParam);
  server.on("/setall", handleSetAll);
  server.on("/settings", handleSettings);
  server.on("/params", handleParams);
  server.on("/setled", handleSetSingle);
  server.on("/showled", handleShowLed);
  server.on("/saveslot", handleSaveSlot);
  server.on("/slotcolors", handleSlotColors);
  server.on("/wifi-settings", HTTP_POST, handleWifiSettings);

  server.on("/update", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", update_html);
  });

  server.on("/update-upload", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", "Update ERFOLGREICH! Starte neu...");
    delay(1000);
    ESP.restart();
  }, []() {
    if (!ledAccess.authorize(server)) return; // Upload callback runs before middleware.
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update-Start: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) {
        Serial.printf("Update-Ende: %u bytes\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });

  server.begin();
  doorLightUdp.begin(DOOR_LIGHT_PORT);

  xTaskCreatePinnedToCore(
      webServerTask,
      "WebServerTask",
      10000,
      NULL,
      1,
      &WebServerTask,
      0);
}
