#include "hardware_config.h"
#include "conversions.h"
#include "wifi_util.h"
#include <ArduinoMqttClient.h>
#include <ArduinoLowPower.h>
#include "hardware_util.h"

int thisDeviceId;

void ledSingleFlashHalfSecond() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
};

void ledDoubleFlashHalfSeconds() {
  ledSingleFlashHalfSecond();
  ledSingleFlashHalfSecond();
};

void ledSingleFlashOneSecond() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
};

void sleepForMinutes(uint minutesToSleep) {
  Serial.print("Going to sleep for ");
  Serial.print(minutesToSleep);
  Serial.println(" minutes");
  WiFi.end();
  Serial.end();
  // sleep for one minute at a time
  for (int i = 0; i < minutesToSleep; i++) {
    LowPower.deepSleep(60000);
  }
};

void sleepForMinutesThenWakeUp(uint minutesToSleep, void (*wakeUpDevice)())
{
    sleepForMinutes(minutesToSleep);
    wakeUpDevice();
}

void turnOnWaterPumpRelay() {
  digitalWrite(PUMP_RELAY_CONTROL, HIGH);
  Serial.println("Water pump relay on");
};

void turnOffWaterPumpRelay() {
  digitalWrite(PUMP_RELAY_CONTROL, LOW);
  Serial.println("Water pump relay off");
};

int getBusVoltageAdcTicks() {
  // raw_val is between 0 and 1023
  int raw_val = analogRead(BUS_VOLTAGE_INPUT);
  return raw_val;
};

float getBusVoltage() {
  int raw_val = getBusVoltageAdcTicks();
  return raw_val * VOLTS_PER_ANALOG_TICK;
};

int getPressureAdcTicks() {
    // raw_val is between 0 and 1023
    // TODO: make a new function that takes multiple samples and computes average
  int raw_val = analogRead(PRESSURE_SENSOR_INPUT);
  return raw_val;
};

// use the pin strapping to determine device id
void setDeviceId() {
  // get raw pin vals
  int pin1Val = digitalRead(ID_PIN_1);
  int pin2Val = digitalRead(ID_PIN_2);
  int pin3Val = digitalRead(ID_PIN_3);
  int pin4Val = digitalRead(ID_PIN_4);

  // bitwise XOR to flip the bits - grounded (0) indicates that digit is populated
  int id1 = pin1Val ^ 1;
  int id2 = pin2Val ^ 1;
  int id3 = pin3Val ^ 1;
  int id4 = pin4Val ^ 1;

  // sum them in the right positions to get the device id
  thisDeviceId = 4 * id4 + 3 * id3 + 2 * id2 + id1;
}