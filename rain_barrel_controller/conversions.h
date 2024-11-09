#include <ArduinoMqttClient.h>

#ifndef CONVERSIONS
#define CONVERSIONS

extern float VOLTS_PER_ANALOG_TICK;  // analog input is 0-1023, mav voltage is 18.0V
extern float PASCALS_PER_IN_WATER;
extern uint MINUTEMILLIS;

#endif