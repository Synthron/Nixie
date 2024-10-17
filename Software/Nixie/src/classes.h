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
    uint64_t SDAT;
    uint8_t hours, minutes, seconds;
    uint8_t mode;
    uint8_t pwm;

    void serialize(uint8_t, uint8_t, uint8_t);
    uint16_t segmenting(uint8_t);
    void tick();
    void cycle();
    void send();
};
