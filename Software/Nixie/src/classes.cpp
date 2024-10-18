#include "classes.h"


/*
*
*   Wifi-Class Function Definitions
*
*/

void Wifi::load_config()
{
  if (SPIFFS.exists("/wifi.json"))
  {
    File file = SPIFFS.open("/wifi.json");
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    if (error)
      Serial.println("Failed to read file");
    else{
      strlcpy(hostname, doc["hostname"], sizeof(hostname));
      strlcpy(ssid, doc["ssid"], sizeof(ssid));
      strlcpy(passwd, doc["password"], sizeof(passwd));
      mode = doc["mode"];
    }
  }
}

void Wifi::save_config()
{
  if (SPIFFS.exists("/wifi.json"))
    SPIFFS.remove("/wifi.json");

  File file = SPIFFS.open("/wifi.json", FILE_WRITE);
  JsonDocument doc;
  doc["hostname"] = hostname;
  doc["ssid"] = ssid;
  doc["password"] = passwd;
  doc["mode"] = mode;

  serializeJson(doc, file);
  file.close();
}

/*************************************************************************************/
/*                  Nixie Clock Class Functions                                      */
/*************************************************************************************/

// Wrapper Function Display Time
void Clock::show_time(mytimeinfo time)
{
  serialize(time.hours, time.minutes, time.seconds);
}

// Wrapper Function Display Date
void Clock::show_date(mytimeinfo time)
{
  mode = d_date;
  serialize(time.date, time.month, time.year);
}

void Clock::cycle()
{
  uint8_t old = mode;
  uint8_t old_pwm = pwm;
  analogWrite(PIN_PWM, 255);
  for(uint8_t i = 0; i < 10; i++)
  {
    serialize(11*i, 11*i, 11*i);
    delay(6000);
  }
  pwm = old_pwm;
  analogWrite(PIN_PWM, pwm);
  mode = old;
}

/* 
 * Serialization function 
 * takes time variables and creates the serial data stream 
 * sends the data stream to the nixies
 * 
 * Dots mode:
 * 0 = default (all on)
 * 4 = Temperature Mode
 */
void Clock::serialize(uint8_t hr, uint8_t min, uint8_t sec)
{
  uint8_t h10, h1, m10, m1, s10, s1, d0, d1, d2, d3;
  uint16_t seg_h10, seg_h1, seg_m10, seg_m1, seg_s10, seg_s1;

  h1 = hr % 10;
  h10 = (hr - h1) / 10;
  m1 = min % 10;
  m10 = (min - m1) / 10;
  s1 = sec % 10;
  s10 = (sec - s1) / 10;

  seg_h10 = segmenting(h10);
  seg_h1  = segmenting(h1);
  seg_m10 = segmenting(m10);
  seg_m1  = segmenting(m1);
  seg_s10 = segmenting(s10);
  seg_s1  = segmenting(s1);

  switch (mode)
  {

    case d_on:
      d0 = 1;
      d1 = 1;
      d2 = 1;
      d3 = 1;
      break;
    case d_off:
      d0 = 0;
      d1 = 0;
      d2 = 0;
      d3 = 0;
      break;
    case d_date:
      d0 = 1;
      d1 = 0;
      d2 = 1;
      d3 = 0;
      break;
    case d_temp:
      d0 = 0;
      d1 = 0;
      d2 = 1;
      d3 = 0;
      break;
  
  }
  // bitstream: MSB -> LSB
  // secs 1s    secs 10s   32 mins 1s    mins 10s   10 hours 1s   hours 10s
  // 0987654321 0987654321 dd 0987654321 0987654321 dd 0987654321 0987654321
  SDAT  = (uint64_t)seg_h10;
  SDAT |= (uint64_t)seg_h1 << 10;
  SDAT |= (uint64_t)d0 << 20;
  SDAT |= (uint64_t)d1 << 21;
  SDAT |= (uint64_t)seg_m10 << 22;
  SDAT |= (uint64_t)seg_m1 << 32;
  SDAT |= (uint64_t)d2 << 42;
  SDAT |= (uint64_t)d3 << 43;
  SDAT |= (uint64_t)seg_s10 << 44;
  SDAT |= (uint64_t)seg_s1 << 54;

  send();
}

uint16_t Clock::segmenting(uint8_t num)
{
  uint16_t temp = 0b0000000000000001;
  if(num == 0)
    return 0b0000001000000000;
  else
  {
    for (uint8_t i = 0; i < num -1; i++)
      temp = temp << 1;

    return temp;
  }
}

void Clock::send()
{
  for (uint8_t i = 0; i < 64; i++)
  {
    digitalWrite(PIN_DAT, SDAT & 0x01);
    digitalWrite(PIN_SCLK, HIGH);
    delayMicroseconds(1);
    digitalWrite(PIN_SCLK, LOW);
    delayMicroseconds(1);
    SDAT = SDAT >> 1;
  }
  digitalWrite(PIN_RCLK, HIGH);
  delayMicroseconds(1);
  digitalWrite(PIN_RCLK, LOW);
}