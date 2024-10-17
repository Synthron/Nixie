#include "myrtc.h"

void debug_xtal_out_dac1() 
{
    SET_PERI_REG_MASK(RTC_IO_XTAL_32K_PAD_REG, RTC_IO_X32N_MUX_SEL | RTC_IO_X32P_MUX_SEL);
    CLEAR_PERI_REG_MASK(RTC_IO_XTAL_32K_PAD_REG, RTC_IO_X32P_RDE | RTC_IO_X32P_RUE | RTC_IO_X32N_RUE | RTC_IO_X32N_RDE);
    CLEAR_PERI_REG_MASK(RTC_IO_XTAL_32K_PAD_REG, RTC_IO_X32N_MUX_SEL | RTC_IO_X32P_MUX_SEL);
    SET_PERI_REG_BITS(RTC_IO_XTAL_32K_PAD_REG, RTC_IO_DAC_XTAL_32K, 1, RTC_IO_DAC_XTAL_32K_S);
    SET_PERI_REG_BITS(RTC_IO_XTAL_32K_PAD_REG, RTC_IO_DRES_XTAL_32K, 3, RTC_IO_DRES_XTAL_32K_S);
    SET_PERI_REG_BITS(RTC_IO_XTAL_32K_PAD_REG, RTC_IO_DBIAS_XTAL_32K, 0, RTC_IO_DBIAS_XTAL_32K_S);
    SET_PERI_REG_MASK(RTC_IO_XTAL_32K_PAD_REG, RTC_IO_XPD_XTAL_32K);
    REG_SET_BIT(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_MUX_SEL_M);
    REG_CLR_BIT(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_RDE_M | RTC_IO_PDAC1_RUE_M);
    REG_SET_FIELD(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_FUN_SEL, 1);
    REG_SET_FIELD(SENS_SAR_DAC_CTRL1_REG, SENS_DEBUG_BIT_SEL, 0);
    const uint8_t sel = 4; /* sel = 4 : 32k XTAL; sel = 5 : internal 150k RC */
    REG_SET_FIELD(RTC_IO_RTC_DEBUG_SEL_REG, RTC_IO_DEBUG_SEL0, sel);
}

static uint32_t calibrate_one(rtc_cal_sel_t cal_clk, const char *name)
{

    const uint32_t cal_count = 1000;
    uint32_t cali_val;
    printf("%s:\n", name);
    for (int i = 0; i < 5; ++i)
    {
        printf("calibrate (%d): ", i);
        cali_val = rtc_clk_cal(cal_clk, cal_count);
        printf("%.3f kHz\n", factor / (float)cali_val);
    }
    return cali_val;
}

float crystal_frequency() 
{
  const uint32_t cal_count = 100;
  uint32_t cali_val;
  cali_val = rtc_clk_cal(RTC_CAL_32K_XTAL, cal_count);
  float freq_32k = factor / (float)cali_val;
  return freq_32k;
}

void setExternalCrystalAsRTCSource()
{
    //rtc_clk_32k_bootstrap(100);
    rtc_clk_32k_enable(true);

    uint32_t cal_32k = CALIBRATE_ONE(RTC_CAL_32K_XTAL);
    debug_xtal_out_dac1();
    float freq_32k = factor / (float)cal_32k;
    float delta = freq_32k - 32.768;
    if (delta < 0) delta = -delta;
    uint32_t startCal=millis();
    while (delta > 0.002 && millis()-startCal<15000) {
      printf("Waiting for 32kHz clock to be stable: %.3f kHz\n", freq_32k);
      cal_32k = CALIBRATE_ONE(RTC_CAL_32K_XTAL);
      freq_32k = factor / (float)cal_32k;
      delta = freq_32k - 32.768;
      if (delta < 0) delta = -delta;
    }
    if(delta < 0.002)
    {
      rtc_clk_slow_freq_set(RTC_SLOW_FREQ_32K_XTAL);
      uint32_t rtc_clk_calibration = REG_READ(RTC_SLOW_CLK_CAL_REG);
      printf("Slow clock calibration: %u\n", rtc_clk_calibration);
      printf("32k calibration: %u\n", cal_32k);
      if ((rtc_clk_calibration > (cal_32k + 5)) || (rtc_clk_calibration < (cal_32k - 5))) {
        printf("Miscalibrated, setting calibration register to 32k calibration.\n");
        REG_WRITE(RTC_SLOW_CLK_CAL_REG, cal_32k);
        rtc_clk_calibration = REG_READ(RTC_SLOW_CLK_CAL_REG);
        if (rtc_clk_calibration != cal_32k) {
          printf("ERROR Calibration write failure.\n");
        }
      }
  
      if (cal_32k == 0)
      {
          printf("32K XTAL OSC has not started up");
      }
      else
      {
          printf("done\n");
      }
  
      if (rtc_clk_32k_enabled())
      {
          printf("OSC Enabled\n");
      }
    }
    else{
      printf("OSC Not Enabled, using Internal 150KHz RC\n");
    }
}