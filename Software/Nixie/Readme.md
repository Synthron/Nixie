# Nixie Clock Software

## How to use this software

In `src` folder:
copy file `network.h.template` and input your wifi settings. 
Compile and upload to ESP32

## Features

- Config via DIP switch:
  - Enable USB debug messages
  - Enable external RTC DS1302
  - ~~t1/t2/LDR~~
  - Enable Anode Dimming
  - ~~enable DCF77~~
  - Enable Wifi & NTP services

## To-Do

- [ ] display date on tubes every xx minutes
- [x] refactor nixie class to use full data struct
- [ ] get ADC working
- [ ] get Temperature Readouts
- [x] get different display modes with dots
- [ ] get button interface done

## necessary HW changes

- IO34 + IO35 are **only** INPUT
    - bridge TP5 to SCL
    - bridge TP6 to SDA