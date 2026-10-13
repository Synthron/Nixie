#pragma once

#include "Arduino.h"
#include "ds1302.h"

// Menu states
enum MenuState {
    MENU_IDLE = 0,
    MENU_SELECT,      // mode selection (1 = Set Time, 2 = Set Date, 3 = Set Brightness, 4 = Set Offset)
    MENU_TIME,        // editing hours/minutes/seconds
    MENU_DATE,        // editing day/month/year
    MENU_BRIGHTNESS,  // editing brightness percentage
    MENU_OFFSET       // editing timezone offset
};

// Sub-field states
enum TimeField { TIME_HOURS = 0, TIME_MINUTES, TIME_SECONDS };
enum DateField { DATE_DAY = 0, DATE_MONTH, DATE_YEAR };

// Button codes (match the input register mapping in main.cpp)
enum MenuButton { BTN_NONE = 0, BTN_MODE = 1, BTN_UP = 2, BTN_DOWN = 3, BTN_ENTER = 4 };

// Brightness and timezone offset settings (persisted in /config.json)
#define BRIGHTNESS_MAX          255

// Timezone offset range in hours (covers all timezones and dateline crossing)
#define OFFSET_MIN              -12
#define OFFSET_MAX              14

// Public API
void menu_handle_button(uint8_t input_no);
bool menu_is_active();
void menu_update_display();
