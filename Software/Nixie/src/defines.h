#pragma once

// Compile-time constants (constexpr instead of #define so they are typed and
// scoped; each translation unit gets its own copy, which is fine for these).
constexpr bool FORMAT_SPIFFS_IF_FAILED = true;

// Pin Definitions
constexpr uint8_t PIN_INT   = 4;
constexpr uint8_t PIN_A2    = 5;
constexpr uint8_t PIN_DAT   = 12;
constexpr uint8_t PIN_RCLK  = 13;
constexpr uint8_t PIN_SCLK  = 14;
constexpr uint8_t PIN_SCLR  = 15;
constexpr uint8_t PIN_PWM   = 16;
constexpr uint8_t PIN_TP0   = 17;
constexpr uint8_t PIN_TP1   = 18;
constexpr uint8_t PIN_TP2   = 19;
constexpr uint8_t PIN_TP3   = 21;
constexpr uint8_t PIN_DCFSIG = 22;
constexpr uint8_t PIN_DCFEN = 23;
constexpr uint8_t PIN_RTCCLK = 25;
constexpr uint8_t PIN_RTCCE1 = 26;
constexpr uint8_t PIN_RTCIO = 27;
constexpr uint8_t PIN_SDA   = 34;
constexpr uint8_t PIN_SCL   = 35;

// I2C Addresses (int to match the Wire API's requestFrom/beginTransmission
// signatures and avoid overload ambiguity with int size arguments)
constexpr int ADDR_CONFIG = 0x20;
constexpr int ADDR_INPUT  = 0x21;
constexpr int ADDR_OUTPUT = 0x22;
constexpr int ADDR_ADC    = 0x68; // 0x01101000

constexpr int8_t T1_Offset = 5;
constexpr int8_t T2_Offset = 5;

// Debug output guard: only print to the USB serial when the USB strap is
// enabled (usb == true). Replaces the scattered `if(usb)` guards.
//   USB(...)   -> Serial.printf(...)
//   USBLN(...) -> Serial.println(...)
//   USBPR(...) -> Serial.print(...)
#define USB(...)   do { if (usb) Serial.printf(__VA_ARGS__); } while(0)
#define USBLN(...) do { if (usb) Serial.println(__VA_ARGS__); } while(0)
#define USBPR(...) do { if (usb) Serial.print(__VA_ARGS__); } while(0)