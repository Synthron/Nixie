#include "main.h"
#include "menu.h"
#include "config.h"

// Menu state machine state
static MenuState menu_state = MENU_IDLE;
static uint8_t mode_no = 1;                 // currently displayed mode (1, 2, 3)
static TimeField time_field = TIME_HOURS;   // selected field while editing time
static DateField date_field = DATE_DAY;     // selected field while editing date
static mytimeinfo edit_time;                // working copy of time/date being edited
static uint8_t edit_brightness = 50;        // working copy of brightness percentage (0-100)
static int8_t edit_offset = 0;              // working copy of timezone offset in hours

// 2Hz blink state (toggle every 250ms)
static bool blink_on = false;
static uint32_t last_blink = 0;

// Forward declarations
static void enter_selected_mode();
static void save_time_date();
static void save_brightness();
static void save_offset();
static void apply_brightness_now();
static void increment_time_field(int8_t delta);
static void increment_date_field(int8_t delta);
static void cycle_display_mode();
static bool display_mode_enabled(uint8_t mode);

void menu_handle_button(uint8_t input_no)
{
  switch (menu_state)
  {
    case MENU_IDLE:
      // MODE enters the menu and shows mode 1 (Set Time)
      if (input_no == BTN_MODE)
      {
        menu_state = MENU_SELECT;
        mode_no = 1;
      }
      // ENTER cycles the display mode: Time -> Date -> Temperature (if enabled)
      else if (input_no == BTN_ENTER)
      {
        cycle_display_mode();
      }
      break;

    case MENU_SELECT:
      // UP/DOWN select the mode 1 -> 2 -> 3 -> 4
      if (input_no == BTN_UP)
      {
        if (mode_no < 4) mode_no++;
      }
      else if (input_no == BTN_DOWN)
      {
        if (mode_no > 1) mode_no--;
      }
      // MODE exits back to idle
      else if (input_no == BTN_MODE)
      {
        menu_state = MENU_IDLE;
      }
      // ENTER selects the displayed mode
      else if (input_no == BTN_ENTER)
      {
        enter_selected_mode();
      }
      break;

    case MENU_TIME:
      // MODE cycles hours -> minutes -> seconds
      if (input_no == BTN_MODE)
      {
        time_field = (TimeField)((time_field + 1) % 3);
      }
      else if (input_no == BTN_UP)
      {
        increment_time_field(1);
      }
      else if (input_no == BTN_DOWN)
      {
        increment_time_field(-1);
      }
      // ENTER saves and returns to mode selection
      else if (input_no == BTN_ENTER)
      {
        save_time_date();
        menu_state = MENU_SELECT;
      }
      break;

    case MENU_DATE:
      // MODE cycles day -> month -> year
      if (input_no == BTN_MODE)
      {
        date_field = (DateField)((date_field + 1) % 3);
      }
      else if (input_no == BTN_UP)
      {
        increment_date_field(1);
      }
      else if (input_no == BTN_DOWN)
      {
        increment_date_field(-1);
      }
      // ENTER saves and returns to mode selection
      else if (input_no == BTN_ENTER)
      {
        save_time_date();
        menu_state = MENU_SELECT;
      }
      break;

    case MENU_BRIGHTNESS:
      if (input_no == BTN_UP)
      {
        if (edit_brightness < 100) edit_brightness++;
        apply_brightness_now();
      }
      else if (input_no == BTN_DOWN)
      {
        if (edit_brightness > 0) edit_brightness--;
        apply_brightness_now();
      }
      // ENTER saves and returns to mode selection
      else if (input_no == BTN_ENTER)
      {
        save_brightness();
        menu_state = MENU_SELECT;
      }
      break;

    case MENU_OFFSET:
      if (input_no == BTN_UP)
      {
        if (edit_offset < OFFSET_MAX) edit_offset++;
      }
      else if (input_no == BTN_DOWN)
      {
        if (edit_offset > OFFSET_MIN) edit_offset--;
      }
      // ENTER saves and returns to mode selection
      else if (input_no == BTN_ENTER)
      {
        save_offset();
        menu_state = MENU_SELECT;
      }
      break;
  }
}

bool menu_is_active()
{
  return menu_state != MENU_IDLE;
}

void menu_update_display()
{
  // 2Hz blink: toggle every 250ms
  if (millis() - last_blink >= 250)
  {
    last_blink = millis();
    blink_on = !blink_on;
  }

  switch (menu_state)
  {
    case MENU_SELECT:
      // hours/minutes show current time, single-seconds digit shows the mode number
      nixie.show_mode(nix_time, mode_no);
      break;

    case MENU_TIME:
      // show working time, blink the selected field
      nixie.show_time(edit_time, blink_on ? (time_field + 1) : 0);
      break;

    case MENU_DATE:
      // show working date, blink the selected field
      nixie.show_date(edit_time, blink_on ? (date_field + 1) : 0);
      break;

    case MENU_BRIGHTNESS:
      // all six digits show the brightness percentage
      nixie.show_brightness(edit_brightness);
      break;

    case MENU_OFFSET:
      // all three sets show the timezone offset value
      nixie.show_offset(edit_offset);
      break;

    default:
      break;
  }
}

static void enter_selected_mode()
{
  switch (mode_no)
  {
    case 1:
      menu_state = MENU_TIME;
      edit_time = nix_time;
      time_field = TIME_HOURS;
      break;
    case 2:
      menu_state = MENU_DATE;
      edit_time = nix_time;
      date_field = DATE_DAY;
      break;
    case 3:
      menu_state = MENU_BRIGHTNESS;
      edit_brightness = (pwm * 100) / BRIGHTNESS_MAX;
      break;
    case 4:
      menu_state = MENU_OFFSET;
      edit_offset = time_offset;
      break;
  }
}

static void save_time_date()
{
  nix_time = edit_time;
  int_rtc.setTime(nix_time.seconds, nix_time.minutes, nix_time.hours,
                  nix_time.date, nix_time.month, nix_time.year + 2000);
  nix_time.wday = int_rtc.getDayofWeek() + 1;
  // store to external RTC if enabled
  if (rtc)
  {
    DS_RTC.setTime(nix_time);
  }
  USB("Time/Date saved: %2d:%2d:%2d %2d.%2d.%2d\r\n",
      nix_time.hours, nix_time.minutes, nix_time.seconds,
      nix_time.date, nix_time.month, nix_time.year);
}

static void apply_brightness_now()
{
  pwm = (edit_brightness * BRIGHTNESS_MAX) / 100;
  analogWrite(PIN_PWM, pwm);
}

static void save_brightness()
{
  apply_brightness_now();
  config_save();
  USB("Brightness saved: %d%% (pwm %d)\r\n", edit_brightness, pwm);
}

static void save_offset()
{
  time_offset = edit_offset;
  config_save();
  USB("Offset saved: %d h\r\n", time_offset);
}

static bool display_mode_enabled(uint8_t mode)
{
  switch (mode)
  {
    case 0: return true;  // Time
    case 1: return true;  // Date
    case 2: return t1;    // Temperature 1
    case 3: return t2;    // Temperature 2
  }
  return true;
}

static void cycle_display_mode()
{
  uint8_t start = dis_mode;
  do
  {
    dis_mode = (dis_mode + 1) % 4;
  } while (!display_mode_enabled(dis_mode) && dis_mode != start);
}

static void increment_time_field(int8_t delta)
{
  switch (time_field)
  {
    case TIME_HOURS:
      edit_time.hours = (edit_time.hours + delta + 24) % 24;
      break;
    case TIME_MINUTES:
      edit_time.minutes = (edit_time.minutes + delta + 60) % 60;
      break;
    case TIME_SECONDS:
      edit_time.seconds = (edit_time.seconds + delta + 60) % 60;
      break;
  }
}

// number of days in a month, accounting for leap years (year stored as 0-99 = 2000-2099)
static uint8_t days_in_month(uint8_t month, uint8_t year)
{
  static const uint8_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month < 1 || month > 12) return 31;
  uint8_t d = days[month - 1];
  if (month == 2 && (year % 4) == 0) d = 29; // 2000-2099: every 4th year is a leap year
  return d;
}

static void increment_date_field(int8_t delta)
{
  switch (date_field)
  {
    case DATE_DAY:
      edit_time.date = constrain((int)edit_time.date + delta, 1, days_in_month(edit_time.month, edit_time.year));
      break;
    case DATE_MONTH:
      edit_time.month = constrain((int)edit_time.month + delta, 1, 12);
      break;
    case DATE_YEAR:
      edit_time.year = constrain((int)edit_time.year + delta, 0, 99);
      break;
  }
}
