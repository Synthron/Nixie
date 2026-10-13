#pragma once

// Single extensible configuration file stored in SPIFFS (/config.json).
// Holds wifi credentials/mode, display brightness (raw pwm 0-255) and the
// timezone offset. All settings are loaded once during boot and persisted
// whenever a setting is changed from the menu or the wifi fallback.

// Load all settings from /config.json into the global wifi, pwm, time_offset.
// Applies defaults when the file is missing or a key is absent.
void config_load();

// Persist all settings to /config.json.
void config_save();