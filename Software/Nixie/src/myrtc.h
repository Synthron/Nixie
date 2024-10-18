#pragma once

#include "Arduino.h"
#include "soc/rtc.h"


#include "soc/rtc_io_reg.h"
#include "soc/sens_reg.h"
#include "rom/rtc.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  5        /* Time ESP32 will go to sleep (in seconds) */

#define CALIBRATE_ONE(cali_clk) calibrate_one(cali_clk, #cali_clk)

const float factor = (1 << 19) * 1000.0f;

extern bool usb;

void debug_xtal_out_dac1();
static uint32_t calibrate_one(rtc_cal_sel_t cal_clk, const char *name);
void setExternalCrystalAsRTCSource();
