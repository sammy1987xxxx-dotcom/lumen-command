#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include <Arduino.h>

// Triggered eine verzögerte Speicherung (Entprellung)
void saveSettings();

// Muss im main loop() aufgerufen werden, um die Speicherung auszuführen
void handlePersistence();

// Lädt die Einstellungen beim Start
void loadSettings();

#endif
