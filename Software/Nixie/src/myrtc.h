#pragma once

#include "Arduino.h"
#include "USBCDC.h"
#include "soc/rtc.h"

#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define CALIBRATE_ONE(cali_clk) calibrate_one(cali_clk, #cali_clk)

#include "soc/rtc_io_reg.h"
#include "soc/sens_reg.h"
#include "rom/rtc.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

const float factor = (1 << 19) * 1000.0f;

void debug_xtal_out_dac1();
static uint32_t calibrate_one(rtc_cal_sel_t cal_clk, const char *name);
void setExternalCrystalAsRTCSource();
