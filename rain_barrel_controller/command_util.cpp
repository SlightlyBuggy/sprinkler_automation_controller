#include "conversions.h"
#include "hardware_config.h"
#include "hardware_util.h"

void spinkleForSeconds(uint seconds) {

  uint secondMillis = 1000;
  turnOnWaterPumpRelay();

  for (int i = 0; i < seconds; i++) {
    delay(secondMillis);
  }

  turnOffWaterPumpRelay();
};

void spinkleForMinutes(uint minutes) {

  turnOnWaterPumpRelay();
  for (int i = 0; i < minutes; i++) {
    delay(MINUTEMILLIS);
  }
  turnOffWaterPumpRelay();
};

void sprinkleMinutesRepeatedly(uint sprinkleMinutes, uint waitMinutes, uint repetitions)
{
  Serial.print("Sprinkler cycle starting.  ");
  for(int i = 0; i < repetitions; i++)
  {
    Serial.print("Cycle ");
    Serial.print(i + 1);
    Serial.print(" out of ");
    Serial.println(repetitions);

    // run the sprinkle cycle
    Serial.print("Sprinkling for ");
    Serial.print(sprinkleMinutes);
    Serial.println(" minutes");
    spinkleForMinutes(sprinkleMinutes);

    // now delay if this isn't the last cycle
    if(i != repetitions - 1)
    {
      Serial.print("Now waiting for ");
      Serial.print(waitMinutes);
      Serial.println(" minutes");
      for (int j = 0; j < waitMinutes; j++) {
        delay(MINUTEMILLIS);
      }
    }
  }
};

void powerOffDevice()
{
  // this will de-energize an external N-channel MOSFET, which un-grounds the Arduino board
  Serial.print("Attempting to power off");
  digitalWrite(POWER_CONTROL, LOW);
};

void keepDevicePowerOn()
{
  digitalWrite(POWER_CONTROL, HIGH);
};