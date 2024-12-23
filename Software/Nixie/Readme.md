# Nixie Clock Software

## How to use this software

In `src` folder:  
copy file `network.h.template`, rename it to `network.h` and input your wifi settings.  
Compile and upload to ESP32

## Features

Configuration:

- Config via DIP switch:
  - Enable USB debug messages
  - Enable external RTC DS1302
  - T1
  - T2
  - LDR
  - Enable Anode Dimming
  - ~~enable DCF77~~ (tbd)
  - Enable Wifi & NTP services

Feature Overview:

- internal RTC as timekeeping
- prioritized synchronization
  - NTP sync if enabled
  - external RTC sync if enabled and NTP not available
- adjustable brightness via button interface
- different display modes via button interface
  - Time
  - Date
  - Temperature Sensor 1
  - Temperature Sensor 2
- USB debug messages if enabled

## To-Do

- [x] ~~display date on tubes every xx minutes~~
  - display date as different mode
- [x] refactor nixie class to use full data struct
- [x] get ADC working
- [x] get Temperature Readouts
- [x] get different display modes with dots
- [ ] get button interface done
- [x] fix error when reading external RTC
- [ ] fix cathode cycling to correct duration

## necessary HW changes

- IO34 + IO35 are **only** INPUT
  - bridge TP5 to SCL
  - bridge TP6 to SDA
- Timing Oszillator MC34063 -> 100pF
- R115 changed to 1k

If Channel 2 of the ADC should be used as LDR input:

- change R113 to 1k

for ADC to work:

- bridge R111 & R113 with 0 Ohm respectively
- 100 Ohm Parallel C112
- 100 Ohm (PT100) / 1kOhm (LDR) parallel C113
