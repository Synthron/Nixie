#pragma once

#include <Arduino.h>
#include "defines.h"
#include "classes.h"
#include "server.h"
#include "ds1302.h"
#include "myrtc.h"

#include "FS.h"
#include "SPIFFS.h"
#include <ArduinoJson.h>
#include "Wire.h"
#include <WiFi.h>
#include "time.h"
#include <soc/rtc.h>
#include <driver/rtc_io.h>
#include <ESP32Time.h>

// OTA
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
void Web_Tasks(void *pvParameters);
extern Wifi wifi;

extern TaskHandle_t WebTasks;
extern struct tm timeinfo;

extern const char* ntpServer;
extern const long  gmtOffset_sec;
extern int   daylightOffset_sec;

void write_Registers(uint64_t data);
void handle_menu();
void update_time();
void pinConfig();
void ntp_setup();
void rtc_setup();
void wifi_setup();
void read_Straps();
bool check_DST();
void set_DST(bool start_DST);
void input_isr();
void IRAM_ATTR timer1();

uint64_t run = 1;

extern Clock nixie;
extern mytimeinfo nix_time;
extern ESP32Time int_rtc;

/*
extern bool ntp;
extern bool dcf;
extern bool ldr;
extern bool t1;
extern bool t2;
extern bool dimm;
extern bool rtc;
extern bool usb;
*/