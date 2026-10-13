# Code Refacture — Working ToDo List

> Only this file is edited as the working ToDo list. The actual code is NOT changed.
> Status legend: `[ ]` = open ToDo, `[x]` = resolved/verified, `[~]` = intentional / no action.

---

## Critical Bugs

1. **[x] network.h missing** — OBSOLETE. `network.h` is no longer used: WiFi credentials are configured at runtime via the AP-mode web manager and stored in `/config.json`. The `#include "network.h"` was removed from `src/server.h`.

2. **[x] wifi_start passes the Wifi object by value** — RESOLVED. Signature changed to `void wifi_start(Wifi& wifi)` in both `src/server.cpp` and `src/server.h`, so `load_config()`/`save_config()` now mutate the global `wifi` object.

3. **[x] btn_irq is not volatile** — FIXED (please verify). `btn_irq` is set inside the ISR `input_isr()` and read in `loop()`, so it must be `volatile bool btn_irq;`. Confirm the declaration in `src/main.cpp` is now `volatile`.

4. **[x] T2 temperature uses the T1 offset** — FIXED (please verify). In `src/main.cpp`, `dis_mode == 3` should call `get_temp(read_adc(), 1)` (channel 1 → `T2_Offset`), not channel `0`. Confirm the argument is now `1`.

5. **[x] Timezone offset is applied twice** — RESOLVED. `time_offset` is now the single source of truth: `gmtOffset_sec` set to `0` in `src/main.cpp` so NTP stores true UTC into `int_rtc`, and `update_time()` applies `time_offset*3600` exactly once. The `+1` magic in `ntp_setup()` was also removed (grouped with Potential Bug #5).

---

## Potential Bugs / Fragile Code

1. **[x] Wifi::load_config() doesn't verify SPIFFS.open() succeeded** — RESOLVED. `Wifi::load_config()`/`save_config()` were removed; all file I/O now lives in the config module (`src/config.cpp`), where `config_load()`/`config_save()` guard the `File` object with `if (!file) return;` before (de)serializing.

2. **[x] EEPROM.begin() called twice** — RESOLVED (superseded by SPIFFS config migration). Brightness and timezone offset are now persisted in a single extensible `/config.json` alongside wifi, managed by the new config module (`src/config.h`/`src/config.cpp`). `config_load()` runs once in `setup()`; `config_save()` is called from the menu savers and the wifi fallback. All EEPROM code and the `EEPROM_ADDR_*`/`EEPROM_SIZE` defines were removed.

3. **[~] cycle() blocks the loop for up to 60 s** — INTENTIONAL. The blocking loop is for nixie filament health (cathode poisoning prevention). No action.

4. **[x] wifi_start uses hardcoded `_ssid`/`_pass` macros** — RESOLVED. `wifi_start()` no longer uses `network.h`; on a failed STA connect it starts an AP (`Nixie-<mac>`, password `Nixie1234`) with a web-based WiFi manager to enter new credentials, which are saved to `/config.json` and the device reboots.

5. **[x] ntp_setup() sets `nix_time.seconds = timeinfo.tm_sec+1`** — RESOLVED. The magic `+1` was removed in `src/main.cpp`; the offset is now applied once via `time_offset` (grouped with Critical Bug #5).

6. **[x] get_temp() negative wrap** — RESOLVED. `get_temp()` in `src/main.cpp` now applies the channel offset in `float`, clamps the result to the displayable range `0..99`, then casts to `uint8_t` — no more negative wrap-around.

7. **[x] increment_date_field() date clamping** — RESOLVED. Added a `days_in_month()` helper in `src/menu.cpp` (with leap-year handling for years 2000–2099); the day field is now clamped to the actual number of days in the selected month/year.

8. **[ ] show_temp() bitmasks are fragile** — OPEN (ToDo). Correct but magic literals; a bit shift in `serialize()` silently breaks them. **Action:** consider named constants or a comment tying them to `serialize()` bit layout.

---

## Style / Best Practices

1. **[~] Magic numbers in serialize()/show_offset()** — INTENTIONAL. Only used for bit-shifting the segments, with an explanatory comment above; `show_offset()` follows the same pattern. No action.

2. **[ ] `#define` for constants instead of `constexpr`/`enum`** — OPEN (ToDo). Separate from #1 (that's about bit positions; this is about constant declarations in `defines.h`/`menu.h`). **Action:** migrate to `constexpr`/`enum` where appropriate.

3. **[ ] `if(usb)` guards** — OPEN (ToDo). **Answer to your question — yes, there is a better way:** wrap debug output in a single macro or a small `Debug` helper, e.g. `#define DBG(...) do { if (usb) Serial.printf(__VA_ARGS__); } while(0)`, or a `Serial` wrapper class. This removes the scattered `if(usb)` noise.

4. **[ ] Serial used without the usb guard** — OPEN (ToDo). `src/myrtc.cpp` and `src/classes.cpp` print unconditionally. **Action:** apply the same debug guard as #3.

5. **[ ] Global mutable state is pervasive** — OPEN (ToDo). `nix_time`, `pwm`, `dis_mode`, flags. **Action:** consider encapsulating in the existing classes.

6. **[x] Web_Tasks / web server** — RESOLVED. `Web_Tasks` now also services the WiFi-manager web server (`web_server_handle()`). The web server uses the built-in ESP32 `WebServer` (not AsyncWebServer) and runs only in AP mode.

---

## Dead Code (all ToDo — remove)

- [ ] `write_Registers()` — never called; duplicates `Clock::send()`.
- [ ] `timer1()` ISR and `ms500_cycle` — timer is commented out.
- [ ] `k`, `boot`, `dcf` flags — read but never functionally used.
- [ ] Unused defines: `PIN_A2`, `PIN_TP2`, `PIN_TP3`, `PIN_DCFSIG`, `PIN_DCFEN`, `PIN_SDA`, `PIN_SCL`, `ADDR_OUTPUT`.
- [ ] `src/myrtc.cpp`: `debug_xtal_out_dac1()`, `crystal_frequency()`, `calibrate_one()`; `src/myrtc.h`: `uS_TO_S_FACTOR`, `TIME_TO_SLEEP`, `CALIBRATE_ONE` — all unused.
- [ ] Large commented-out blocks in `src/main.cpp` and `src/server.cpp`.

---

## Recommended Priority Order

1. ~~Create `src/network.h` from the template~~ — DONE (file already exists).
2. ~~Fix `wifi_start` to take `Wifi&` by reference (Critical #2).~~ - DONE.
3. ~~Mark `btn_irq` as volatile~~ — DONE.
4. ~~Fix the T2 channel argument in `dis_mode == 3`~~ — DONE.
5. ~~Resolve the double timezone-offset design (Critical #5 + Potential #5).~~
6. ~~Begin EEPROM once during boot (Potential #2).~~ — DONE (migrated to a single SPIFFS `/config.json`).
7. Remove dead code and replace `#define` constants with `constexpr`/`enum`.