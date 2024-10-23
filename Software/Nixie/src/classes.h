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
    
    void show_time(mytimeinfo);
    void show_date(mytimeinfo);
    void show_temp(uint8_t temp, bool ch);
    void cycle();
  
  private:
    uint64_t SDAT;
    void serialize(uint8_t, uint8_t, uint8_t);
    uint16_t segmenting(uint8_t);
    void send();
};
