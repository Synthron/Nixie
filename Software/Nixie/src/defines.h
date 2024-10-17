#pragma once

#define FORMAT_SPIFFS_IF_FAILED true

// Pin Definitions
#define PIN_INT 4
#define PIN_A2 5
#define PIN_DAT 12
#define PIN_RCLK 13
#define PIN_SCLK 14
#define PIN_SCLR 15
#define PIN_PWM 16
#define PIN_TP0 17
#define PIN_TP1 18
#define PIN_TP2 19
#define PIN_TP3 21
#define PIN_DCFSIG 22
#define PIN_DCFEN 23
#define PIN_RTCCLK 25
#define PIN_RTCCE1 26
#define PIN_RTCIO 27
#define PIN_SDA 34
#define PIN_SCL 35

//I2C Addresses
#define ADDR_CONFIG 0x20
#define ADDR_INPUT  0x21
#define ADDR_OUTPUT 0x22
#define ADDR_ADC    0x68