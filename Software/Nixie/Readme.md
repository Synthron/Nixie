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
  - ~~T1~~  (tbd)
  - ~~T2~~  (tbd)
  - ~~LDR~~ (tbd)
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
- [ ] get ADC working
- [ ] get Temperature Readouts
- [x] get different display modes with dots
- [ ] get button interface done

## necessary HW changes

- IO34 + IO35 are **only** INPUT
  - bridge TP5 to SCL
  - bridge TP6 to SDA
- Timing Oszillator MC34063 -> 100pF
- R115 changed to 1k