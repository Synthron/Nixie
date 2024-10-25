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
int daylightOffset_sec = 0;
bool dst_last;

//bootstrap flags
bool ntp=1;
bool dcf=0;
bool ldr=0;
bool t1=0;
bool t2=0;
bool dimm=1;
bool rtc=0;
bool usb=1;

bool dim_once;
bool half = false;

bool boot;

//isr flags
uint8_t ms500_cycle = 0;
bool btn_irq = 0;

//menu variables
uint8_t dis_mode, menu_mode = 0; 

//misc
uint8_t k = 0, pwm = 64;
uint8_t T1_Temp, T2_Temp;
float ldr_log;

hw_timer_t *Timer_Channel1 = NULL;

void setup()
{
  boot = 1;
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
    analogWrite(PIN_PWM, 0);
  }
  else
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
    if(ntp) 
    {
      DS_RTC.setTime(nix_time);
      if (usb) Serial.printf("Saving internallay\r\nTime: %2d:%2d:%2d\r\nDate: %2d.%2d.%2d\r\n",nix_time.hours, nix_time.minutes, nix_time.seconds, nix_time.date, nix_time.month, nix_time.year);
    }
    else nix_time = DS_RTC.getTime();
  }

  rtc_setup();



  //setup Timers
  //Channel 1: 500ms timer
/*
  Timer_Channel1 = timerBegin(0, 80, true);
  timerAlarmWrite(Timer_Channel1, 500000, true);
  timerAttachInterrupt(Timer_Channel1, &timer1, true);
  timerAlarmEnable(Timer_Channel1);
*/

  //setup Button Input Interrupts
  pinMode(PIN_INT, INPUT_PULLUP);
  attachInterrupt(PIN_INT, &input_isr, GPIO_INTR_NEGEDGE);

  if (usb) Serial.println("Booting complete");

  //Serial.printf("Current Time\r\nTime: %2d:%2d:%2d\r\nDate: %2d.%2d.%2d\r\nDay: %d\r\n",nix_time.hours, nix_time.minutes, nix_time.seconds, nix_time.date, nix_time.month, nix_time.year, nix_time.wday);

  dst_last = check_DST();
  set_DST(dst_last);
  //Serial.printf("After DST Check\r\nTime: %2d:%2d:%2d\r\nDate: %2d.%2d.%2d\r\nDay: %d\r\n",nix_time.hours, nix_time.minutes, nix_time.seconds, nix_time.date, nix_time.month, nix_time.year, nix_time.wday);


  boot = 0;

  if(t1) start_adc(0, 0);
}

void loop()
{
  update_time();

  if(btn_irq)
  {
    handle_menu();
  }

// Half Second Cycle
  if ((int_rtc.getMillis()<500) && !half)
  {
    nixie.mode = nixie.d_on;
    if (t1)T1_Temp = get_temp(read_adc(), 0);
    if (ldr || t2) start_adc(1, 0);
    dim_once = 0;
    half = 1;
  }
  if((int_rtc.getMillis()>500) && half)
  {
    nixie.mode = nixie.d_off;
    if (ldr) ldr_log = log10(read_adc());
    else if(t2) T2_Temp = get_temp(read_adc(), 1);
    if (t1) start_adc(0, 0);
    half = 0;
  }
// Display Mode
  if(dis_mode == 0)
  {
    if((int_rtc.getMillis()>500) && (nixie.mode == nixie.d_on))
    {
      nixie.mode = nixie.d_off;
    }
    else if ((int_rtc.getMillis()<500) && (nixie.mode == nixie.d_off))
    {
      nixie.mode = nixie.d_on;
    }
    nixie.show_time(nix_time);
  }
  else if(dis_mode == 1)
  {
    nixie.mode = nixie.d_date;
    nixie.show_date(nix_time);

  }
  else if (dis_mode == 2)
  {
    start_adc(0, 0);
    if(int_rtc.getMillis() > 900)
      T1_Temp = get_temp(read_adc(), 0);
    nixie.show_temp(T1_Temp, 0);
  }
  else if (dis_mode == 3)
  {
    start_adc(1, 0);
    if(int_rtc.getMillis() > 900)
      T2_Temp = get_temp(read_adc(), 0);
    nixie.show_temp(T2_Temp, 1);
  }

  //if enabled, fetch the network time daily at 00:00:00 and sync every hour with external RTC
  if(ntp)
  {
    if(nix_time.hours == 0 && nix_time.minutes == 0 && nix_time.seconds == 0)
    {
      ntp_setup();
      int_rtc.setTime(nix_time.seconds, nix_time.minutes, nix_time.hours, nix_time.date, nix_time.month+1, nix_time.year + 2000);
      if(rtc) 
      {
        DS_RTC.setTime(nix_time);
      }
    }
  }
  if (rtc)
  {
    if(nix_time.minutes == 0 && nix_time.seconds == 0)
    {
      nix_time = DS_RTC.getTime();
      int_rtc.setTime(nix_time.seconds, nix_time.minutes, nix_time.hours, nix_time.date, nix_time.month+1, nix_time.year + 2000);
      if (usb) Serial.printf("syncing time from external rtc\r\nTime: %2d:%2d:%2d\r\nDate: %2d.%2d.%2d\r\n",nix_time.hours, nix_time.minutes, nix_time.seconds, nix_time.date, nix_time.month, nix_time.year);
    }
  }

  //every hour check for daylight savings time
  if(nix_time.minutes == 0 && nix_time.seconds == 0)
  {
    if (check_DST() != dst_last)
    {
      dst_last = check_DST();
      set_DST(dst_last);
    }
  }


  //cycle segments every four hours at the hours
  if(nix_time.hours % 4 == 0 && nix_time.minutes == 0)
    nixie.cycle();

  if(dimm && (int_rtc.getMillis() > 900) && !dim_once) 
  {
    dimming();
    dim_once = true;
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
    else if (input_no == 2 && pwm < 248 && !dimm)
    {
      pwm += 8;
      analogWrite(PIN_PWM, pwm);
    }
    //decrease brightness via PWM
    else if (input_no == 3 && pwm > 0 && !dimm)
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
  nix_time.hours    = int_rtc.getHour(true);
  nix_time.minutes  = int_rtc.getMinute();
  nix_time.seconds  = int_rtc.getSecond();
  nix_time.date     = int_rtc.getDay();
  nix_time.month    = int_rtc.getMonth();
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
    nix_time.seconds = timeinfo.tm_sec+1;
    nix_time.date = timeinfo.tm_mday;
    nix_time.month = timeinfo.tm_mon+1;
    nix_time.year = timeinfo.tm_year % 100;
    nix_time.wday = timeinfo.tm_wday;
  }
}

void rtc_setup()
{
  setExternalCrystalAsRTCSource();

  if (usb) Serial.printf("storing internally\r\nTime: %2d:%2d:%2d\r\nDate: %2d.%2d.%2d\r\n",nix_time.hours, nix_time.minutes, nix_time.seconds, nix_time.date, nix_time.month, nix_time.year);

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

bool check_DST()
{
  bool was_dst;
  bool is_DST;

  //detect previous Daylight Savings Time Status
  if(daylightOffset_sec == 3600)
  {
    was_dst = 1;
  }
  else 
  {
    was_dst = 0;
  }

  if(usb) Serial.printf("[debug] was_DST: %x\r\n", was_dst);

  // in winter months no savings time
  if(nix_time.month < 3 || nix_time.month > 10 || (nix_time.month == 3 && ((31 - nix_time.date) >= (7 - nix_time.wday))) || (nix_time.month == 10 && ((31 - nix_time.date) <= (7 - nix_time.wday))))
  {
    is_DST = 0;
  }
  // in summer months savings time
  else if (nix_time.month > 3 && nix_time.month < 10 || (nix_time.month == 10 && ((31 - nix_time.date) >= (7 - nix_time.wday))) || (nix_time.month == 3 && ((31 - nix_time.date) <= (7 - nix_time.wday))))
  {
    is_DST = 1;
  }

  /**
   * March and october have 31 days
   * change occurs on last sunday of the month
   * weekdays are saved as 0..6 starting with 0 = sunday
   * therefore: 
   * if (<current week day> == <sunday> && <31 days> - <current date> < <7>)
   *    last sunday of the month
   */
  if ((nix_time.wday == 0) && ((31 - nix_time.date) < 7))
  {
    //if march and 02:00 or later = DST
    if(nix_time.month == 3)
    {
      if (nix_time.hours >= 2)
      {
        is_DST = 1;
      }
      else
      {
        is_DST = 0;
      }
    }
    //if october and 03:00 or later = no DST
    else if (nix_time.month == 10)
    {
      if(nix_time.hours >= 2 && !was_dst)
      {
        is_DST = 0;
      }
      else if(was_dst)
      {
        is_DST = 1;
      }
    }
  }

  if(usb) Serial.printf("[debug] is_DST: %x\r\n", is_DST);
  return is_DST;

}

void set_DST(bool start_DST)
{
  uint8_t rtc_dst = DS_RTC.readByte(REG_RAM1);
  if(usb) Serial.printf("[debug] Read from RTC Register: %x\r\n", rtc_dst);
  //change time if Daylight Savings Time has changed
  if(start_DST && (!rtc_dst || (boot && ntp)))
  {
    if(usb) Serial.println("[debug] Set Summer Time");
    daylightOffset_sec = 3600;
    DS_RTC.writeByte(REG_RAM1, 1);
    nix_time.hours++;
    int_rtc.setTime(nix_time.seconds, nix_time.minutes, nix_time.hours, nix_time.date, nix_time.month+1, nix_time.year + 2000);
    DS_RTC.setTime(nix_time);
  }
  else if (!start_DST && (rtc_dst || (boot && ntp)))
  {
    if(usb) Serial.println("[debug] Set Winter Time");
    daylightOffset_sec = 0;
    DS_RTC.writeByte(REG_RAM1, 0);
    if (!(boot && ntp)) nix_time.hours--;
    int_rtc.setTime(nix_time.seconds, nix_time.minutes, nix_time.hours, nix_time.date, nix_time.month+1, nix_time.year + 2000);
    DS_RTC.setTime(nix_time);
  }
}

// ADC Functions
void start_adc(bool channel, bool continuous)
{
  //0b S C1 C0 O/C S1 S0 G1 G0
  //start conversion in OneShot Mode
  uint8_t config_reg = 0x80;
  //select channel
  config_reg |= (uint8_t)channel << 5;

  //set sample mode
  if(continuous)
    config_reg |= 0x10;
  
  //set sample rate and resolution 15SPS 16bit
  config_reg |= 0x08;

  Wire.beginTransmission(ADDR_ADC);
  Wire.write(config_reg);
  Wire.endTransmission();
}

uint16_t read_adc()
{
  uint8_t d_high, d_low, conf;
  Wire.requestFrom(ADDR_ADC, 3);
  d_high = Wire.read();
  d_low = Wire.read();
  conf = Wire.read();
  Wire.endTransmission();

  return ((uint16_t)d_high << 8) + d_low;
}

uint8_t get_temp(uint16_t adc, bool channel)
{
  uint8_t t_out;
  float a = 0.0039083;
  float rt;
  float ref = 2.048;
  float fac = ref / 32768;
  float volt = adc * fac;

  rt = (3.3 - volt) / (volt / 100);

  float temp = ((rt / 100)-1)/a;
  t_out = (uint8_t)(temp + 0.5);
  if(channel == 0)
    return t_out - T1_Offset;
  else
    return t_out - T2_Offset;
}

void dimming()
{
  uint8_t pwm_fac;

  if(ldr_log > 4.0)
    ldr_log = 4.0;

  pwm_fac = (uint8_t)(20.0 * ldr_log);

  if(pwm_fac < 16)
    pwm_fac = 16;

  analogWrite(PIN_PWM, pwm_fac);

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