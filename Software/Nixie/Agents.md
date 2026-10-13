# Agents.md

This file is a guide for AI coding agents (and human contributors) working on the **Nixie Clock** project. It defines the project's goals, architecture, conventions, and how an agent should assist the user when writing or modifying code.

---

## 1. Project Overview

A **Nixie Clock** built around **Z5600M** Nixie tubes and an **ESP32** microcontroller.

- **Hardware**: Custom KiCad PCB designs (`Hardware/PCB/`) and 3D-printed covers (`Hardware/`).
- **Firmware**: PlatformIO + Arduino framework for the ESP32 (`Software/Nixie/`).

The clock displays time, date, and temperature on the Nixie tubes, keeps accurate time via multiple redundant sources, and is configurable through hardware DIP switches and a button interface.

---

## 2. Project Goals

1. **Accurate timekeeping** with prioritized synchronization:
   - NTP (network) sync when enabled.
   - External DS1302 RTC sync when NTP is unavailable.
   - Internal ESP32 RTC as the primary running clock.
2. **Reliable Nixie display** driven through 74HC595 shift registers, with support for time, date, and temperature modes.
3. **Configurable behavior** via DIP switches (bootstrap straps) and a button-driven menu.
4. **Tube longevity** through automatic cathode cycling to prevent cathode poisoning.
5. **User-friendly operation**:
   - Adjustable brightness (PWM dimming, optionally LDR-based auto-dimming).
   - Multiple display modes (Time, Date, Temp 1, Temp 2).
   - USB debug output when enabled.
6. **Maintainable, well-structured firmware** that follows the existing architecture and conventions.

---

## 3. Repository Layout

```
Nixie/
├── README.md                     # Top-level project readme
├── Agents.md                     # This file
├── Hardware/                     # Mechanical + PCB design
│   ├── *.3mf / *.FCStd           # 3D-printed covers & spacers
│   └── PCB/                      # KiCad schematics, layouts, BOM, 3D models
└── Software/
    └── Nixie/                    # PlatformIO ESP32 firmware
        ├── platformio.ini        # Build/upload config, dependencies
        ├── Readme.md             # Firmware-specific docs & to-do list
        ├── lib/ds1302/           # DS1302 external RTC driver
        └── src/
            ├── main.cpp          # setup/loop, ADC, timezone offset, dimming, ISRs
            ├── main.h            # Includes + extern declarations
            ├── defines.h         # Pin definitions, I2C addresses, offsets
            ├── classes.h/.cpp    # Wifi + Clock (Nixie display) classes
            ├── menu.h/.cpp       # Button-driven menu state machine (MODE/UP/DOWN/ENTER)
            ├── server.h/.cpp     # WiFi startup + OTA
            ├── myrtc.h/.cpp      # External 32kHz crystal RTC source setup
            └── network.h.template # WiFi credentials template (copy to network.h)
```

---

## 4. Architecture & Key Concepts

### 4.1 Timekeeping
- **Internal RTC** (`ESP32Time int_rtc`) is the running clock; `update_time()` copies it into the `mytimeinfo nix_time` struct each loop.
- **External DS1302 RTC** (`DS_RTC`) provides battery-backed time; synced from NTP and read back periodically.
- **NTP** syncs from `pool.ntp.org` at boot and daily at 00:00:00.
- A user-configurable **timezone offset** (`int8_t time_offset`, in hours, range -12 .. +14) is applied to the displayed time, replacing the old DST switch. `update_time()` applies it via epoch arithmetic (`int_rtc.getEpoch()` + offset, then `gmtime_r()`), so crossing the dateline (negative times or past 24h) rolls the date/month/year over correctly. The offset is stored in EEPROM and settable via menu mode 4.

### 4.2 Nixie Display (`Clock` class)
- Tubes are driven by **74HC595 shift registers** via a 64-bit serial bitstream.
- `Clock::serialize()` builds the 64-bit `SDAT` from hours/minutes/seconds (or date/temp) and dot-separator bits.
- `Clock::serialize()` accepts a `blink_field` argument (1=hours, 2=minutes, 3=seconds, 4=seconds-tens-only, 5=hours+minutes) to blank fields for blinking or single-digit indicators.
- `Clock::show_mode()` shows only the mode number on the single-seconds digit, with hours/minutes blanked (used by the menu).
- `Clock::show_brightness()` displays the value on all three tube sets (00-99); `Clock::show_offset()` does the same for the timezone offset (dots indicate a negative offset).
- `Clock::send()` shifts the data out on `PIN_DAT`/`PIN_SCLK` and latches with `PIN_RCLK`.
- `Clock::cycle()` runs cathode cycling to prevent poisoning.

### 4.3 Configuration (DIP switches / bootstrap straps)
- `read_Straps()` reads an IO expander (`ADDR_CONFIG`) to set feature flags:
  `ntp`, `dcf`, `dimm`, `t1`, `t2`, `ldr`, `rtc`, `usb`.

### 4.4 Button Interface (Menu)
- Four buttons (**MODE**, **UP**, **DOWN**, **ENTER**) are read via an IO expander input register (`ADDR_INPUT`), triggered by an interrupt (`input_isr`).
- `handle_menu()` (in `main.cpp`) debounces and decodes the pressed button, then routes it to the menu state machine in `menu.cpp`.
- The menu state machine (`menu.h`) has states: idle, mode select, set time, set date, set brightness, set offset.
- **Outside the menu (idle):**
  - **MODE** enters the menu.
  - **ENTER** cycles the display mode: Time → Date → Temperature 1 (if `t1`) → Temperature 2 (if `t2`) → Time.
- **In mode select:**
  - **UP/DOWN** select the mode (1=Set Time, 2=Set Date, 3=Set Brightness, 4=Set Offset).
  - **MODE** exits back to idle.
  - **ENTER** enters the selected mode.
- **In a sub-mode** (time/date/offset): **MODE** cycles the sub-field (hours/minutes/seconds or day/month/year); **UP/DOWN** adjust the value; **ENTER** saves and returns to mode select.
- The mode number is shown on the single-seconds Nixie digit while hours/minutes are blanked; the selected field blinks at ~2Hz while the others stay static.
- Brightness is **applied live** while adjusting (PWM updated on each press) and shown on all three tube sets as a 00-99 value; on completion it is stored to EEPROM. Time/date are stored to the internal RTC and the external DS1302 RTC (if enabled); the timezone offset is stored to EEPROM.

### 4.5 Sensors & Dimming
- **ADC** (`ADDR_ADC`, 16-bit) reads PT100 temperature sensors (channels 0/1) and an optional LDR.
- `get_temp()` converts ADC readings to temperature with per-channel offsets.
- `dimming()` maps the LDR reading to a PWM brightness value.

### 4.6 Networking & OTA
- `wifi_start()` connects to a saved network or falls back to the credentials in `network.h`.
- `ota_start()` enables ArduinoOTA; OTA runs on a separate core via `Web_Tasks`.

---

## 5. Conventions & Guidelines

- **Language**: C++ (Arduino framework), C++11/14 style.
- **Headers**: Use `#pragma once`.
- **Global state**: Shared globals are declared in `main.cpp` and exposed via `extern` in `main.h` (e.g., `nixie`, `nix_time`, `int_rtc`, `pwm`).
- **Feature flags**: Gate optional behavior behind the bootstrap flags (`usb`, `ntp`, `rtc`, `t1`, `t2`, `ldr`, `dimm`). Debug prints should be wrapped in `if(usb)`.
- **Pin definitions**: All pins and I2C addresses live in `defines.h` — never hardcode magic numbers in logic.
- **Time struct**: Use the `mytimeinfo` struct (defined in `lib/ds1302/ds1302.h`) for all time data.
- **ISRs**: Keep interrupt handlers minimal; set a flag and handle work in `loop()`.
- **Build**: PlatformIO. Dependencies are declared in `platformio.ini` (`ArduinoJson`, `ESP32Time`).

---

## 6. How the Agent Should Assist

The agent's role is to help the user **write, refactor, debug, and extend** the firmware while respecting the existing architecture and the user's intent. When working on this project, the agent should:

### 6.1 Understand Before Changing
- Read the relevant source files (`main.cpp`, `classes.cpp`, `server.cpp`, `myrtc.cpp`, `ds1302.cpp`) before proposing changes.
- Trace how data flows (e.g., time → `nix_time` → `Clock::serialize` → shift registers) to avoid breaking existing behavior.
- Check `defines.h` for pin/address definitions and `platformio.ini` for dependencies before adding new hardware features.

### 6.2 Follow the User's Guidance
- The user drives the direction. Confirm the intended behavior and scope before implementing non-trivial features.
- Respect the existing feature-flag pattern: new optional features should be gated behind a bootstrap flag where appropriate.
- Preserve the current display modes and menu behavior unless the user explicitly asks to change them.

### 6.3 Write Clean, Consistent Code
- Match the existing style (naming, `#pragma once`, `extern` globals, `if(usb)` debug guards).
- Keep hardware-specific values in `defines.h`.
- Prefer small, focused functions; keep ISRs minimal.
- Add or update comments where the logic is non-obvious (e.g., the 64-bit bitstream layout in `serialize()`).

### 6.4 Help Debug & Verify
- When investigating issues (e.g., the open to-do items), reason about the hardware interaction and suggest targeted fixes.
- Use the USB debug output (`if(usb)` prints) to trace behavior.
- Be mindful of ESP32 constraints: pin capabilities (e.g., IO34/IO35 are input-only), timing, and the two-core task model.

### 6.5 Respect the To-Do List
- The firmware `Readme.md` contains an active to-do list. Known open items include:
  - Fix **cathode cycling** to the correct duration.
  - DCF77 support (currently disabled / tbd).
- The **button interface** (MODE/UP/DOWN/ENTER menu) is implemented in `menu.cpp`/`menu.h`; extend it there rather than in `main.cpp`.
- When the user asks to work on these, treat them as the priority and coordinate with the user on the approach.

### 6.6 Ask When Needed
- If a request is ambiguous or could break existing behavior, ask the user for clarification rather than guessing.
- If a change affects hardware (pins, wiring, PCB), flag the implications and reference the relevant `defines.h` / `Readme.md` notes.

---

## 7. Build & Run

```bash
# From Software/Nixie/
# 1. Create network.h from the template and fill in WiFi credentials
cp src/network.h.template src/network.h

# 2. Build
pio run

# 3. Upload to the ESP32
pio run -t upload

# 4. Monitor serial output
pio device monitor
```

> **Note**: `network.h` is git-ignored and must be created locally from `network.h.template`.

---

## 8. Important Hardware Notes

- **IO34 + IO35 are input-only** — bridge TP5→SCL and TP6→SDA for I2C.
- Timing oscillator MC34063 uses a 100pF cap; R115 changed to 1k.
- For LDR on ADC channel 2, change R113 to 1k.
- For ADC to work: bridge R111 & R113 with 0Ω, add 100Ω parallel to C112, and 100Ω (PT100) / 1kΩ (LDR) parallel to C113.

These constraints must be respected when writing firmware that touches I2C or ADC pins.