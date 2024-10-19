#pragma once

#include "Arduino.h"

#define REG_SECS 0
#define REG_MINS 1
#define REG_HOUR 2
#define REG_DATE 3
#define REG_MONT 4
#define REG_WDAY 5
#define REG_YEAR 6
#define REG_WP   7
#define REG_CHRG 8

extern bool usb;

struct mytimeinfo
{
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t date;
    uint8_t month;
    uint8_t year;
    uint8_t wday;
};


class DS1302
{
  public:
    void init(uint8_t p_IO, uint8_t p_CLK, uint8_t p_CE);
    void writeByte(uint8_t addr, uint8_t dat);
    uint8_t readByte(uint8_t addr);
    void enableCharging();
    void setTime(mytimeinfo timestruct);
    mytimeinfo getTime();

  private:
    uint8_t io, clk, ce, del;
    void send(uint8_t dat, bool read_after);
    uint8_t read();
};


