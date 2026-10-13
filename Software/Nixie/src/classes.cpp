#include "classes.h"

// 64-bit serial bitstream layout (MSB -> LSB), used by serialize() and
// show_temp(). Each digit segment field is SEG_BITS wide; the two dot bits
// (d0..d3) sit between the digit fields. Keeping these as named constants
// means a shift change in serialize() is reflected everywhere automatically.
//   seg_h10 (0-9) | seg_h1 (10-19) | d0(20) d1(21) |
//   seg_m10 (22-31) | seg_m1 (32-41) | d2(42) d3(43) |
//   seg_s10 (44-53) | seg_s1 (54-63)
constexpr uint8_t H10_SHIFT = 0;
constexpr uint8_t H1_SHIFT  = 10;
constexpr uint8_t D0_SHIFT  = 20;
constexpr uint8_t D1_SHIFT  = 21;
constexpr uint8_t M10_SHIFT = 22;
constexpr uint8_t M1_SHIFT  = 32;
constexpr uint8_t D2_SHIFT  = 42;
constexpr uint8_t D3_SHIFT  = 43;
constexpr uint8_t S10_SHIFT = 44;
constexpr uint8_t S1_SHIFT  = 54;
constexpr uint8_t SEG_BITS  = 10;

/*************************************************************************************/
/*                  Nixie Clock Class Functions                                      */
/*************************************************************************************/

// Wrapper Function Display Time
void Clock::show_time(mytimeinfo time, uint8_t blink_field)
{
  serialize(time.hours, time.minutes, time.seconds, blink_field);

  send();
}

// Wrapper Function Display Date
void Clock::show_date(mytimeinfo time, uint8_t blink_field)
{
  mode = d_date;
  serialize(time.date, time.month, time.year, blink_field);

  send();
}

// Display the brightness value on all three sets (hours, minutes, seconds),
// each showing the same 00-99 value.
void Clock::show_brightness(uint8_t percent)
{
  mode = d_off;
  uint8_t v = (percent > 99) ? 99 : percent;
  serialize(v, v, v, 0);

  send();
}

// Display the timezone offset value on all three sets (hours, minutes, seconds).
// Negative offsets light the dots to indicate the minus sign.
void Clock::show_offset(int8_t offset)
{
  mode = d_off;
  uint8_t v = (offset < 0) ? (uint8_t)(-offset) : (uint8_t)offset;
  if (v > 99) v = 99;
  serialize(v, v, v, 0);
  if (offset < 0)
  {
    SDAT |= (uint64_t)1 << 20;
    SDAT |= (uint64_t)1 << 21;
    SDAT |= (uint64_t)1 << 42;
    SDAT |= (uint64_t)1 << 43;
  }
  send();
}

// Display the menu mode number on the single-seconds digit (1, 2, 3, ...)
// while hours/minutes are blanked.
void Clock::show_mode(mytimeinfo time, uint8_t mode_no)
{
  mode = d_on;
  // blink_field 5 blanks hours and minutes so only the mode number
  // appears on the single-seconds display.
  serialize(time.hours, time.minutes, mode_no, 5);

  send();
}

void Clock::show_temp(uint8_t temp, bool ch)
{
  mode = d_off;
  serialize((uint8_t)ch+1, temp/100, temp % 100);

  // Build the keep-mask from the named bit positions so it stays in sync with
  // serialize(). The channel number (hours ones) and the two-digit temp
  // (seconds) are always shown; the hundreds digit (minutes ones) is only
  // shown for temps >= 100. All other fields are blanked.
  uint64_t keep = 0;
  keep |= ((uint64_t)0x3FF) << S1_SHIFT;   // temp ones digit
  keep |= ((uint64_t)0x3FF) << S10_SHIFT;  // temp tens digit
  keep |= ((uint64_t)1) << D3_SHIFT;
  keep |= ((uint64_t)1) << D2_SHIFT;
  keep |= ((uint64_t)1) << D1_SHIFT;
  keep |= ((uint64_t)1) << D0_SHIFT;
  keep |= ((uint64_t)0x3FF) << H1_SHIFT;   // channel ones digit
  if (temp > 99)
    keep |= ((uint64_t)0x3FF) << M1_SHIFT; // hundreds digit
  SDAT &= keep;

  send();
}

void Clock::cycle(uint16_t seconds)
{
  uint8_t old = mode;
  uint8_t old_pwm = pwm;
  analogWrite(PIN_PWM, 150);
  for(uint8_t i = 0; i < 10; i++)
  {
    serialize(11*i, 11*i, 11*i);
    send();
    delay(seconds * 1000);
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
void Clock::serialize(uint8_t hr, uint8_t min, uint8_t sec, uint8_t blink_field)
{
  uint8_t h10, h1, m10, m1, s10, s1, d0, d1, d2, d3;
  uint16_t seg_h10, seg_h1, seg_m10, seg_m1, seg_s10, seg_s1;

  h1  = hr % 10;
  h10 = hr / 10;
  m1  = min % 10;
  m10 = min / 10;
  s1  = sec % 10;
  s10 = sec / 10;

  seg_h10 = segmenting(h10);
  seg_h1  = segmenting(h1);
  seg_m10 = segmenting(m10);
  seg_m1  = segmenting(m1);
  seg_s10 = segmenting(s10);
  seg_s1  = segmenting(s1);

  // blank the blinking field (1 = hours, 2 = minutes, 3 = seconds)
  if (blink_field == 1)
  {
    seg_h10 = 0;
    seg_h1  = 0;
  }
  else if (blink_field == 2)
  {
    seg_m10 = 0;
    seg_m1  = 0;
  }
  else if (blink_field == 3)
  {
    seg_s10 = 0;
    seg_s1  = 0;
  }
  // blank only the seconds tens digit (single-digit mode indicator)
  else if (blink_field == 4)
  {
    seg_s10 = 0;
  }
  // blank hours and minutes (mode selection: only show the mode number)
  else if (blink_field == 5)
  {
    seg_h10 = 0;
    seg_h1  = 0;
    seg_m10 = 0;
    seg_m1  = 0;
  }

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
  // bitstream: MSB -> LSB, see the H*_SHIFT/M*_SHIFT/S*_SHIFT constants above.
  // Each digit field is SEG_BITS wide; the dot bits (d0..d3) sit between them.
  SDAT  = (uint64_t)seg_h10 << H10_SHIFT;
  SDAT |= (uint64_t)seg_h1 << H1_SHIFT;
  SDAT |= (uint64_t)d0 << D0_SHIFT;
  SDAT |= (uint64_t)d1 << D1_SHIFT;
  SDAT |= (uint64_t)seg_m10 << M10_SHIFT;
  SDAT |= (uint64_t)seg_m1 << M1_SHIFT;
  SDAT |= (uint64_t)d2 << D2_SHIFT;
  SDAT |= (uint64_t)d3 << D3_SHIFT;
  SDAT |= (uint64_t)seg_s10 << S10_SHIFT;
  SDAT |= (uint64_t)seg_s1 << S1_SHIFT;

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