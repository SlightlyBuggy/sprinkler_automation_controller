#ifndef MQTT_SERVICE
#define MQTT_SERVICE

// individual server-to-device commands 
const char commandStatus[] = "status";
const char commandSprinkleStart[] = "sprinkle_start";
const char commandSprinkleOn[] = "sprinkle_on";
const char commandSprinkleOff[] = "sprinkle_off";
const char commandSleep[] = "sleep_now";
const char commandSwitchToDebug[] = "switch_broker_debug";
const char commandSwitchToProd[] = "switch_broker_prod";
const char commandPowerOff[] = "power_off";

// device to server topics
const char topicDeviceStatus[] = "device_status";

// server-to-device command
const char topicCommand[] = "command";

#endif

