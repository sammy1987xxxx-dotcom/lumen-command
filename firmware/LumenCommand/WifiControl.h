#ifndef WIFICONTROL_H
#define WIFICONTROL_H

#include <Arduino.h>

void setupNetwork();
void webServerTask(void *pvParameters);

#endif
