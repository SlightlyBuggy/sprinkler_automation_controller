#ifndef HARDWARE_UTIL
#define HARDWARE_UTIL

extern int thisDeviceId;

void ledSingleFlashHalfSecond();
void ledDoubleFlashHalfSeconds();
void ledSingleFlashOneSecond();
void sleepForMinutesThenWakeUp(uint minutesToSleep, void (*wakeUpDevice)());
void sleepForMinutes (uint minutesToSleep);
void turnOnWaterPumpRelay();
void turnOffWaterPumpRelay();
float getBusVoltage();
int getPressureAdcTicks();
int getBusVoltageAdcTicks();
void setDeviceId();

#endif