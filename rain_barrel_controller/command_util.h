#include <ArduinoMqttClient.h>

#ifndef COMMAND_UTIL
#define COMMAND_UTIL

void spinkleForSeconds(uint seconds);
void spinkleForMinutes(uint minutes);
void sprinkleMinutesRepeatedly(uint sprinkleMinutes, uint waitMinutes, uint repetitions);
void powerOffDevice();
void keepDevicePowerOn();

#endif