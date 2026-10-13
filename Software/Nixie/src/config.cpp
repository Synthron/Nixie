#include "config.h"
#include "main.h"
#include <ArduinoJson.h>
#include "SPIFFS.h"

static const char* CONFIG_PATH = "/config.json";

void config_load()
{
  // defaults (used when the file is missing or a key is absent)
  wifi.mode = 0;
  strlcpy(wifi.hostname, "Nixie", sizeof(wifi.hostname));
  wifi.ssid[0] = '\0';
  wifi.passwd[0] = '\0';
  nixie.pwm = (50 * BRIGHTNESS_MAX) / 100; // default 50% brightness
  time_offset = 0;

  if (!SPIFFS.exists(CONFIG_PATH)) return;

  File file = SPIFFS.open(CONFIG_PATH, "r");
  if (!file) return;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, file);
  file.close();
  if (err) return;

  JsonObject w = doc["wifi"];
  if (!w.isNull())
  {
    strlcpy(wifi.hostname, w["hostname"] | "Nixie", sizeof(wifi.hostname));
    strlcpy(wifi.ssid, w["ssid"] | "", sizeof(wifi.ssid));
    strlcpy(wifi.passwd, w["passwd"] | "", sizeof(wifi.passwd));
    wifi.mode = w["mode"] | 0;
  }

  int b = doc["brightness"] | (50 * BRIGHTNESS_MAX) / 100;
  if (b < 0) b = 0;
  if (b > BRIGHTNESS_MAX) b = BRIGHTNESS_MAX;
  nixie.pwm = (uint8_t)b;

  int off = doc["offset"] | 0;
  if (off < OFFSET_MIN || off > OFFSET_MAX) off = 0;
  time_offset = (int8_t)off;
}

void config_save()
{
  JsonDocument doc;
  doc["wifi"]["hostname"] = wifi.hostname;
  doc["wifi"]["ssid"] = wifi.ssid;
  doc["wifi"]["passwd"] = wifi.passwd;
  doc["wifi"]["mode"] = wifi.mode;
  doc["brightness"] = nixie.pwm;
  doc["offset"] = time_offset;

  File file = SPIFFS.open(CONFIG_PATH, "w");
  if (!file) return;
  serializeJson(doc, file);
  file.close();
}