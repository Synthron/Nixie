/**
 * 
 * The classes.h and classes.cpp files contain all the 
 * class definitions for the project. 
 * 
*/

#pragma once

#include <Arduino.h>
#include "FS.h"
#include "SPIFFS.h"
#include <ArduinoJson.h>
#include "SPI.h"
#include "defines.h"
#include "ds1302.h"

extern uint8_t pwm;

class Wifi
{
  public:
    char hostname[64];
    char ssid[64];
    char passwd[64];
    uint8_t mode;
    void load_config();
    void save_config();

};

class Clock
{
  public:
    uint8_t mode;
    enum d_mode {d_on, d_off, d_date, d_temp};
    
    void show_time(mytimeinfo, uint8_t blink_field = 0);
    void show_date(mytimeinfo, uint8_t blink_field = 0);
    void show_temp(uint8_t temp, bool ch);
    void show_brightness(uint8_t percent);
    void show_offset(int8_t offset);
    void show_mode(mytimeinfo time, uint8_t mode_no);
    void cycle(uint16_t seconds);
  
  private:
    uint64_t SDAT;
    void serialize(uint8_t, uint8_t, uint8_t, uint8_t blink_field = 0);
    uint16_t segmenting(uint8_t);
    void send();
};
