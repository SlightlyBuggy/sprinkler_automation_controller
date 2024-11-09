#include <ArduinoMqttClient.h>

#ifndef HARDWARE_CONFIG
#define HARDWARE_CONFIG

// pin assignments
const int PUMP_RELAY_CONTROL = 7;
const int POWER_CONTROL = 1;
const byte BUS_VOLTAGE_INPUT = A0;
const byte PRESSURE_SENSOR_INPUT = A1;

// each device ID pin is a place in a binary value
const int ID_PIN_1 = 2;  // 0001
const int ID_PIN_2 = 3;  // 0010
const int ID_PIN_3 = 4;  // 0100
const int ID_PIN_4 = 5;  // 1000

#endif