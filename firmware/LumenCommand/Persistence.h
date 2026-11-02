#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include <Arduino.h>

// Triggered eine verzögerte Speicherung (Entprellung)
void saveSettings();

// Muss im main loop() aufgerufen werden, um die Speicherung auszuführen
void handlePersistence();

// Lädt die Einstellungen beim Start
void loadSettings();

// Bilderlicht: Plätze beim Start laden bzw. einen Platz (0..2) dauerhaft sichern.
void loadLedSlots();
bool storeLedSlot(uint8_t slot);

#endif
