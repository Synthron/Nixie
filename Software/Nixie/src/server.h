#pragma once

#include <Arduino.h>
#include "FS.h"
#include "SPIFFS.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include "time.h"

// OTA
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include "classes.h"
#include "defines.h"
#include "network.h"

void ota_start();
void wifi_start(Wifi wifi);
