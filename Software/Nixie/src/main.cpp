#include "main.h"

Clock nixie;

// Class Objects
Wifi wifi;

TaskHandle_t WebTasks;
struct tm timeinfo;

//DS1302 Classes
DS1302 DS_RTC;
mytimeinfo nix_time;


// variables

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

bool ntp;
bool dcf;
bool ldr;
bool t1;
bool t2;
bool dimm;
bool rtc;
bool usb;

bool trigger = 0;

uint8_t k = 0;

hw_timer_t *Timer_Channel1 = NULL;

void setup()
{
  read_Straps();

  if(usb) Serial.begin(115200);
  if(usb) Serial.println("Booting...");
  
  //File System Init
  if(usb) Serial.println("Setup File Systems");

  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
  {
    if(usb) Serial.println("SPIFFS Mount Failed");
    ESP.restart();
  }
  if(usb) Serial.println("SPIFFS mounted");

  //Setup IO pins
  pinConfig();

  //Dimming Enable
  if(dimm)
  {
    analogWriteFrequency(100);
    analogWrite(PIN_PWM, 64);
  }

  //RTC Enable and Initial Read
  //If RTC empty, clock starts at 00:00:00
  //RTC can be set via NTP, DCF or manually
  if(rtc)
  {
    DS_RTC.init(PIN_RTCIO, PIN_RTCCLK, PIN_RTCCE1);
    DS_RTC.enableCharging();
  }

  rtc_setup();

  if(ntp)
  {
    wifi_setup();
    ntp_setup();
  }

  //setup Timers
  //Channel 1: 500ms timer
  Timer_Channel1 = timerBegin(0, 80, true);
  timerAlarmWrite(Timer_Channel1, 3000, true);
  timerAttachInterrupt(Timer_Channel1, &timer1, true);
  timerAlarmEnable(Timer_Channel1);

  if(usb) Serial.println("Booting complete");
}

void loop()
{


  if(trigger)
  {
    nixie.mode = 0;
    nixie.tick();
    nixie.serialize(nixie.hours, nixie.minutes, nixie.seconds);
    delay(1000);
    trigger = 0;
  }

}

void write_Registers(uint64_t data)
{
  for (uint8_t i = 0; i < 64; i++)
  {
    digitalWrite(PIN_DAT, data & 0x01);
    digitalWrite(PIN_SCLK, HIGH);
    delayMicroseconds(1);
    digitalWrite(PIN_SCLK, LOW);
    delayMicroseconds(1);
    data = data >> 1;
  }
  digitalWrite(PIN_RCLK, HIGH);
  delayMicroseconds(1);
  digitalWrite(PIN_RCLK, LOW);
  delayMicroseconds(1);
}

// Run OTA and Web Service on different core than main loop
void Web_Tasks(void *pvParameters)
{
  for (;;)
  {
    ArduinoOTA.handle();
    vTaskDelay(1);
  }
}


// Setup Functions

//Bootstrap
void read_Straps()
{
    //IO Expander
  Wire.begin(PIN_TP1, PIN_TP0);
  
  //Read bootstraps
  Wire.requestFrom(ADDR_CONFIG, 1);
  uint8_t temp_strap = ~Wire.read();
  Wire.endTransmission();

  ntp  =  temp_strap       & 0x01;
  dcf  = (temp_strap >> 1) & 0x01;
  dimm = (temp_strap >> 2) & 0x01;
  t1   = (temp_strap >> 3) & 0x01;
  t2   = (temp_strap >> 4) & 0x01;
  ldr  = (temp_strap >> 5) & 0x01;
  rtc  = (temp_strap >> 6) & 0x01;
  usb  = (temp_strap >> 7) & 0x01;
}
void pinConfig()
{
  if(usb) Serial.println("IO Config");
  //74HC595 pins 
  pinMode(PIN_SCLR, OUTPUT);
  pinMode(PIN_SCLK, OUTPUT);
  pinMode(PIN_RCLK, OUTPUT);
  pinMode(PIN_DAT, OUTPUT);

  digitalWrite(PIN_SCLR, LOW);
  digitalWrite(PIN_DAT, LOW);
  digitalWrite(PIN_SCLK, LOW);
  digitalWrite(PIN_RCLK, LOW);
  digitalWrite(PIN_SCLR, HIGH);

  //DS1302 Pins
  pinMode(PIN_RTCCE1, OUTPUT);
  pinMode(PIN_RTCCLK, OUTPUT);
  pinMode(PIN_RTCIO, OUTPUT);

  digitalWrite(PIN_RTCCE1, LOW);
  digitalWrite(PIN_RTCCLK, LOW);
  digitalWrite(PIN_RTCIO,  LOW);

  //PWM Pin
  pinMode(PIN_PWM, OUTPUT);
  digitalWrite(PIN_PWM, HIGH);

}

void ntp_setup()
{
  if(usb) Serial.println("Update Time Data");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  if (!getLocalTime(&timeinfo))
  {
    if(usb) Serial.println("Failed to obtain time, setting 01-01-2000 00:00:00");
    timeinfo.tm_hour = 0;
    timeinfo.tm_min = 0;
    timeinfo.tm_sec = 0;
    timeinfo.tm_mday = 1;
    timeinfo.tm_mon = 1;
    timeinfo.tm_year = 2000;
  }
  if(usb) Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  timeinfo.tm_year += 1900;

  //update time info of nixie class
  nix_time.hours = timeinfo.tm_hour;
  nix_time.minutes = timeinfo.tm_min;
  nix_time.seconds = timeinfo.tm_sec;
  nix_time.date = timeinfo.tm_mday;
  nix_time.month = timeinfo.tm_mon;
  nix_time.year = timeinfo.tm_year % 100;
  nix_time.wday = timeinfo.tm_wday;

  DS_RTC.setTime(nix_time);

  nixie.hours = nix_time.hours;
  nixie.minutes = nix_time.minutes;
  nixie.seconds = nix_time.seconds;
}

void rtc_setup()
{
  if(usb) Serial.println("Internal RTC Setup");
  rtc_clk_32k_enable(true);
  delay(1000);
  //rtc_clk_32k_bootstrap(10);

  uint32_t cal_val = rtc_clk_cal(RTC_CAL_32K_XTAL, 1000);

  if (cal_val == 0)
  {
    if(usb) Serial.println("Kalibrierung ist fehlgeschlagen.");
  }
  else
  {
    if(usb) Serial.print("Kalibrierungswert: ");
    if(usb) Serial.println(cal_val);
  }

  rtc_clk_slow_freq_set(RTC_SLOW_FREQ_32K_XTAL);
}

void wifi_setup()
{
  if(usb) Serial.println("Setup WiFi");

  wifi_start(wifi);

  if(usb) Serial.print("IP address: ");
  if(usb) Serial.println(WiFi.localIP());

  ota_start();
  if(usb) Serial.println("OTA Start");

  delay(500);

  // create special task for OTA and Webserver on seperate core
  xTaskCreatePinnedToCore(
      Web_Tasks, /* Task function. */
      "Web_OTA", /* name of task. */
      10000,     /* Stack size of task */
      NULL,      /* parameter of the task */
      1,         /* priority of the task */
      &WebTasks, /* Task handle to keep track of created task */
      0);        /* pin task to core 0 */
  delay(500);

}
// ISR FUnctions
void IRAM_ATTR timer1()
{
  trigger = 1;
}