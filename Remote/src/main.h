#ifndef _MAIN_H_
#define _MAIN_H_

#include <Arduino.h>

const uint8_t RADIO_CS_PIN = 9;
const uint8_t RADIO_DO_PIN = 10;
const uint8_t BUTT_PIN = A0;
const uint8_t POTENT_PIN = A1;
const uint8_t BATTERY_PIN = A2;

typedef enum {
    mmOff,
    mmComfort,
    mmNormal,
    mmSport
} MotorMode;


#endif // _MAIN_H_