# Code Refacture — Working ToDo List

> Only this file is edited as the working ToDo list. The actual code is NOT changed.
> Status legend: `[ ]` = open ToDo, `[x]` = resolved/verified, `[~]` = intentional / no action.

---

## Critical Bugs

1. **[x] network.h missing** — RESOLVED. A `src/network.h` file already exists, so the build is not blocked. No action.

2. **[x] wifi_start passes the Wifi object by value** — RESOLVED. Signature changed to `void wifi_start(Wifi& wifi)` in both `src/server.cpp` and `src/server.h`, so `load_config()`/`save_config()` now mutate the global `wifi` object.

3. **[x] btn_irq is not volatile** — FIXED (please verify). `btn_irq` is set inside the ISR `input_isr()` and read in `loop()`, so it must be `volatile bool btn_irq;`. Confirm the declaration in `src/main.cpp` is now `volatile`.

4. **[x] T2 temperature uses the T1 offset** — FIXED (please verify). In `src/main.cpp`, `dis_mode == 3` should call `get_temp(read_adc(), 1)` (channel 1 → `T2_Offset`), not channel `0`. Confirm the argument is now `1`.

5. **[x] Timezone offset is applied twice** — RESOLVED. `time_offset` is now the single source of truth: `gmtOffset_sec` set to `0` in `src/main.cpp` so NTP stores true UTC into `int_rtc`, and `update_time()` applies `time_offset*3600` exactly once. The `+1` magic in `ntp_setup()` was also removed (grouped with Potential Bug #5).

---

## Potential Bugs / Fragile Code

1. **[ ] Wifi::load_config() doesn't verify SPIFFS.open() succeeded** — OPEN (ToDo).
   - **Answer to your question:** `SPIFFS.exists()` returning true does *not* guarantee `SPIFFS.open()` succeeds (e.g. corrupt file, or open-for-write failing due to space). Also the `File` object is not checked before `deserializeJson`. It's defensive programming — cheap to add a `if (!file)` guard.

2. **[ ] EEPROM.begin() called twice** — OPEN (ToDo). `menu_load_brightness()` and `menu_load_offset()` each call `EEPROM.begin(EEPROM_SIZE)`, and `EEPROM.end()` is never called. **Action:** call `EEPROM.begin()` once during the boot sequence (in `setup()`), then just `read()`/`write()`/`commit()`.

3. **[~] cycle() blocks the loop for up to 60 s** — INTENTIONAL. The blocking loop is for nixie filament health (cathode poisoning prevention). No action.

4. **[ ] wifi_start uses hardcoded `_ssid`/`_pass` macros** — OPEN (ToDo).
   - **Answer to your question:** Yes, this is related to Critical Bug #2 — both stem from the wifi config handling. The credentials come from the `network.h` template, and the AP fallback branch is commented out, so a failed STA connect leaves the device with no network. **Action:** decide on credential source + re-enable/implement AP fallback.

5. **[x] ntp_setup() sets `nix_time.seconds = timeinfo.tm_sec+1`** — RESOLVED. The magic `+1` was removed in `src/main.cpp`; the offset is now applied once via `time_offset` (grouped with Critical Bug #5).

6. **[ ] get_temp() negative wrap** — OPEN (ToDo). `(uint8_t)(temp + 0.5)` wraps a negative temperature to a large value. **Action:** clamp/handle negative temps.

7. **[ ] increment_date_field() date clamping** — OPEN (ToDo). Day is clamped to 1–31 without validating against the month (Feb 31 allowed). **Action:** validate day against month/year.

8. **[ ] show_temp() bitmasks are fragile** — OPEN (ToDo). Correct but magic literals; a bit shift in `serialize()` silently breaks them. **Action:** consider named constants or a comment tying them to `serialize()` bit layout.

---

## Style / Best Practices

1. **[~] Magic numbers in serialize()/show_offset()** — INTENTIONAL. Only used for bit-shifting the segments, with an explanatory comment above; `show_offset()` follows the same pattern. No action.

2. **[ ] `#define` for constants instead of `constexpr`/`enum`** — OPEN (ToDo). Separate from #1 (that's about bit positions; this is about constant declarations in `defines.h`/`menu.h`). **Action:** migrate to `constexpr`/`enum` where appropriate.

3. **[ ] `if(usb)` guards** — OPEN (ToDo). **Answer to your question — yes, there is a better way:** wrap debug output in a single macro or a small `Debug` helper, e.g. `#define DBG(...) do { if (usb) Serial.printf(__VA_ARGS__); } while(0)`, or a `Serial` wrapper class. This removes the scattered `if(usb)` noise.

4. **[ ] Serial used without the usb guard** — OPEN (ToDo). `src/myrtc.cpp` and `src/classes.cpp` print unconditionally. **Action:** apply the same debug guard as #3.

5. **[ ] Global mutable state is pervasive** — OPEN (ToDo). `nix_time`, `pwm`, `dis_mode`, flags. **Action:** consider encapsulating in the existing classes.

6. **[ ] Web_Tasks / web server** — OPEN (ToDo). `Web_Tasks` only calls `ArduinoOTA.handle()`; the web server is commented out (previously malfunctioning). **Action:** decide whether to re-enable the AsyncWebServer or rename the task/comment.

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
2. Fix `wifi_start` to take `Wifi&` by reference (Critical #2).
3. ~~Mark `btn_irq` as volatile~~ — DONE (verify).
4. ~~Fix the T2 channel argument in `dis_mode == 3`~~ — DONE (verify).
5. Resolve the double timezone-offset design (Critical #5 + Potential #5).
6. Begin EEPROM once during boot (Potential #2).
7. Remove dead code and replace `#define` constants with `constexpr`/`enum`.