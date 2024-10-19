#include "main.h"

Clock nixie;

// Class Objects
Wifi wifi;

TaskHandle_t WebTasks;
struct tm timeinfo;

//DS1302 Classes
DS1302 DS_RTC;
mytimeinfo nix_time;

ESP32Time int_rtc;
// variables

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

//bootstrap flags
bool ntp;
bool dcf;
bool ldr;
bool t1;
bool t2;
bool dimm;
bool rtc;
bool usb;

//isr flags
uint8_t ms500_cycle = 0;
bool btn_irq = 0;

//menu variables
uint8_t dis_mode, menu_mode = 0; 

//misc
uint8_t k = 0, pwm = 64;

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
    analogWrite(PIN_PWM, pwm);
  }

  if(ntp)
  {
    wifi_setup();
    ntp_setup();
  }

  //RTC Enable and Initial Read
  //If RTC empty, clock starts at 00:00:00
  //RTC can be set via NTP, DCF or manually
  if(rtc)
  {
    DS_RTC.init(PIN_RTCIO, PIN_RTCCLK, PIN_RTCCE1);
    DS_RTC.enableCharging();
    if(ntp) DS_RTC.setTime(nix_time);
    else nix_time = DS_RTC.getTime();
  }

  rtc_setup();



  //setup Timers
  //Channel 1: 500ms timer
  Timer_Channel1 = timerBegin(0, 80, true);
  timerAlarmWrite(Timer_Channel1, 500200, true);
  timerAttachInterrupt(Timer_Channel1, &timer1, true);
  timerAlarmEnable(Timer_Channel1);

  //setup Button Input Interrupts
  pinMode(PIN_INT, INPUT_PULLUP);
  attachInterrupt(PIN_INT, &input_isr, GPIO_INTR_NEGEDGE);

  if(usb) Serial.println("Booting complete");
}

void loop()
{
  update_time();

  if(btn_irq)
  {
    handle_menu();
  }

  if(dis_mode == 0)
  {
    if(int_rtc.getMillis()>500)
      nixie.mode = nixie.d_off;
    else
      nixie.mode = nixie.d_on;

    nixie.show_time(nix_time);
  }
  else if(dis_mode == 1)
  {
    nixie.mode = nixie.d_date;
    nixie.show_date(nix_time);
  }

  //if enabled, fetch the network time daily at 00:00:00 and sync every hour with external RTC
  if(ntp)
  {
    if(nix_time.hours == 0 && nix_time.minutes == 0 && nix_time.seconds == 0)
    {
      ntp_setup();
      int_rtc.setTime(nix_time.seconds, nix_time.minutes, nix_time.hours, nix_time.date, nix_time.month+1, nix_time.year + 2000);
      if(rtc) DS_RTC.setTime(nix_time);
    }
  }
  if (rtc)
  {
    if(nix_time.minutes == 0 && nix_time.seconds == 0)
    {
      nix_time = DS_RTC.getTime();
      int_rtc.setTime(nix_time.seconds, nix_time.minutes, nix_time.hours, nix_time.date, nix_time.month, nix_time.year + 2000);
      if (usb) Serial.printf("syncing time from external rtc\r\nTime: %2d:%2d:%2d\r\nDate: %2d.%2d.%2d",nix_time.hours, nix_time.minutes, nix_time.seconds, nix_time.date, nix_time.month, nix_time.year);
    }
  }

  //cycle segments every four hours at the hours
  if(nix_time.hours % 4 == 0 && nix_time.minutes == 0)
    nixie.cycle();
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


void handle_menu()
{
  uint8_t input_no = 0;
  //debounce
  delay(10);
  //read input register, clear !INT signal
  Wire.requestFrom(ADDR_INPUT, 1);
  uint8_t reg_input = ~Wire.read();
  Wire.endTransmission();

  //check if and which button is pressed
  if(reg_input)
  {
    //check if source was first button (MODE)
    if(reg_input & 0x01)
    {
      input_no = 1;
    }
    //check if source was second button (UP)
    else if (reg_input & 0x02)
    {
      input_no = 2;
    }
    //check if source was third button (DOWN)
    else if (reg_input & 0x04)
    {
      input_no = 3;
    }
    //check if source was fourth button (ENTER)
    else if (reg_input & 0x08)
    {
      input_no = 4;
    }
  }

  if(menu_mode == 0)
  {
    //cycle between time, date, T1*, T2*
    //* if enabled
    if (input_no == 1)
    {
      dis_mode++;
      if(!t1 && dis_mode == 2)
        dis_mode++;
      if(!t2 && dis_mode == 3)
        dis_mode = 0;

      input_no = 0;
      
    }
    //increase brightness via PWM
    else if (input_no == 2 && pwm < 248)
    {
      pwm += 8;
      analogWrite(PIN_PWM, pwm);
    }
    //decrease brightness via PWM
    else if (input_no == 3 && pwm > 0)
    {
      pwm -= 8;
      analogWrite(PIN_PWM, pwm);
    }
    //
    else if (input_no == 4)
    {
      
    }
    
  }


  //reset flag
  btn_irq = 0;
}

void update_time()
{
  nix_time.hours    = int_rtc.getHour(true)-1;
  nix_time.minutes  = int_rtc.getMinute();
  nix_time.seconds  = int_rtc.getSecond();
  nix_time.date     = int_rtc.getDay();
  nix_time.month    = int_rtc.getMonth()+1;
  nix_time.year     = int_rtc.getYear() % 100;
  nix_time.wday     = int_rtc.getDayofWeek();
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
  delayMicroseconds(1);
  digitalWrite(PIN_RCLK, HIGH);
  delayMicroseconds(1);
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
    ntp = 0;
  }
  else
  {
    if(usb) Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    if(usb) Serial.println(timeinfo.tm_mon, DEC);

    //update time info of nixie class
    nix_time.hours = timeinfo.tm_hour;
    nix_time.minutes = timeinfo.tm_min;
    nix_time.seconds = timeinfo.tm_sec;
    nix_time.date = timeinfo.tm_mday;
    nix_time.month = timeinfo.tm_mon;
    nix_time.year = timeinfo.tm_year % 100;
    nix_time.wday = timeinfo.tm_wday;
  }
}

void rtc_setup()
{
  setExternalCrystalAsRTCSource();

  int_rtc.setTime(nix_time.seconds, nix_time.minutes, nix_time.hours, nix_time.date, nix_time.month+1, nix_time.year + 2000);
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
  //ms500_cycle++;
}

void IRAM_ATTR input_isr()
{
  btn_irq = 1;
}