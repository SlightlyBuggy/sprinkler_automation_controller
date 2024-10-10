#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <ArduinoLowPower.h>

#include <ArduinoMqttClient.h>
#include <WiFiNINA.h>
#include "arduino_secrets.h"


char ssid[] = SECRET_SSID;  // your network SSID (name)
char pass[] = SECRET_PASS;  // your network password (use for WPA, or use as key for WEP)

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

const char brokerDebug[] = "192.168.0.170";
const char brokerProd[] = "192.168.0.232";
char brokerCurrent[14] = {};
char brokerSecondary[14] = {};

const int brokerPort = 1883;
int wiFiStatus = WL_IDLE_STATUS;

// device to server topics
const char topicDeviceStatus[] = "device_status";

// server-to-device command
const char topicCommand[] = "command";

// individual server-to-device commands 
const char commandStatus[] = "status";
const char commandSprinkleStart[] = "sprinkle_start";
const char commandSprinkleOn[] = "sprinkle_on";
const char commandSprinkleOff[] = "sprinkle_off";
const char commandSleep[] = "sleep_now";
const char commandSwitchToDebug[] = "switch_broker_debug";
const char commandSwitchToProd[] = "switch_broker_prod";
const char commandPowerOff[] = "power_off";

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

// constants
float VOLTS_PER_ANALOG_TICK = 0.0187874;  // analog input is 0-1023, mav voltage is 18.0V
float PASCALS_PER_IN_WATER = 248.84;
uint MINUTEMILLIS = 60 * 1000;

// debug
const bool debug_pressure = false;
const bool debug_relay = false;
const bool debug_batt_voltage = false;
bool relay_on = false;
int thisDeviceId;

// TODO: bigtime refactoring
// put all constants somewhere
// put mqtt stuff somewhere else
// hw commands go somewhere else

void setup() {

  // prioritize connecting to the debug broker initially
  strcpy(brokerCurrent, brokerDebug);
  strcpy(brokerSecondary, brokerProd);

  // set built in LED pin to output mode
  pinMode(LED_BUILTIN, OUTPUT);

  // set the ID pins
  pinMode(ID_PIN_1, INPUT_PULLUP);
  pinMode(ID_PIN_2, INPUT_PULLUP);
  pinMode(ID_PIN_3, INPUT_PULLUP);
  pinMode(ID_PIN_4, INPUT_PULLUP);

    // set the power pin to off initially
  pinMode(POWER_CONTROL, OUTPUT);
  digitalWrite(POWER_CONTROL, LOW);

  // wait just to make sure this isn't a power transient, which seems to happen when it suddenly becomes dark
  delay(5000);

  // ground the power control pin, which engages an external MOSFET
  digitalWrite(POWER_CONTROL, HIGH);

  // set the pump relay control pin
  pinMode(PUMP_RELAY_CONTROL, OUTPUT);

  //Initialize serial and wait for port to open:
  Serial.begin(9600);

  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);

  reconnectIfNeeded();

  setDeviceId();

  delay(1000);
}

void loop() {
  // call poll() regularly to allow the library to send MQTT keep alive which
  // avoids being disconnected by the broker
  mqttClient.poll();

  reconnectIfNeeded();

  // debug data printed to serial monitor
  if (debug_pressure) {
    float pressureTicks = getPressureAdcTicks();
    Serial.print("Pressure ticks: ");
    Serial.print(pressureTicks);
  }

  if (debug_relay) {
    if (relay_on) {
      relay_on = false;
      digitalWrite(PUMP_RELAY_CONTROL, LOW);
      // Serial.println("Relay is OFF");
    } else {
      relay_on = true;
      digitalWrite(PUMP_RELAY_CONTROL, HIGH);
      // Serial.println("Relay is ON");
    }
  }

  if (debug_batt_voltage) {
    float voltage = getBusVoltage();
    Serial.print("Bus voltage: ");
    Serial.println(voltage);
  }

  delay(1000);
}

// connect to wifi
void connectWiFiIfNeeded() {

  // attempt to connect to Wifi network:
  wiFiStatus = WiFi.status();

  if (wiFiStatus != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
      // failed, retry
      Serial.print(".");
      delay(5000);
    }

    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);

    Serial.println("You're connected to the network");
    Serial.println();
  }

  return;
}

// connect to broker
// TODO: have device attempt to prod server first, then debug
// TODO: have way to tell device to switch networks
void connectToMqttBrokerIfNeeded(const char brokerIp[], const bool force = false) {

  if (!mqttClient.connected() || force) {
    Serial.print("Attempting to connect to the MQTT broker: ");
    Serial.println(brokerIp);

    if (mqttClient.connect(brokerIp, brokerPort)) {
      Serial.println("You're connected to the MQTT broker!");
      Serial.println();

      // ensure we come back to this broker on accidental disconnect
      strcpy(brokerCurrent, brokerIp);

      // set the secondary broker appropriately
      // TODO: refactor this.  in fact, refactor this entire goddamn file 
      if(strcmp(brokerIp, brokerDebug) == 0)
      {
        strcpy(brokerSecondary, brokerProd);
      } else {
        strcpy(brokerSecondary, brokerDebug);
      }

      // subscribe to command topic
      mqttClient.subscribe(topicCommand);
      mqttClient.onMessage(onMqttMessage);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(500);
      digitalWrite(LED_BUILTIN, LOW);
      delay(500);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(500);
      digitalWrite(LED_BUILTIN, LOW);
      delay(500);

      // send status when connected 
      sendStatusMessage();
    } else {
      Serial.print("MQTT connection failed! Error code = ");
      Serial.println(mqttClient.connectError());
    }
  }
}

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
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.begin(9600);
  delay(1000);
  Serial.println("Waking up");
  reconnectIfNeeded();
  loop();
}

// reconnect to WiFi and mqtt broker
void reconnectIfNeeded() {

  connectWiFiIfNeeded();

  connectToMqttBrokerIfNeeded(brokerCurrent);

  connectToMqttBrokerIfNeeded(brokerSecondary);
}

// force connection to debug server
void switchToDebugBroker() {
  strcpy(brokerCurrent, brokerDebug);
  strcpy(brokerSecondary, brokerProd);
  connectToMqttBrokerIfNeeded(brokerCurrent, true);
}

// force connection to prod server
void switchToProdBroker() {
  strcpy(brokerCurrent, brokerProd);
  strcpy(brokerSecondary, brokerDebug);
  connectToMqttBrokerIfNeeded(brokerProd, true);
}

// relay control
void turnOnWaterPumpRelay() {
  digitalWrite(PUMP_RELAY_CONTROL, HIGH);
  Serial.println("Water pump relay on");
}

void turnOffWaterPumpRelay() {
  digitalWrite(PUMP_RELAY_CONTROL, LOW);
  Serial.println("Water pump relay off");
}

float getBusVoltage() {
  int raw_val = getBusVoltageAdcTicks();
  return raw_val * VOLTS_PER_ANALOG_TICK;
}

int getPressureAdcTicks() {
    // raw_val is between 0 and 1023
    // TODO: make a new function that takes multiple samples and computes average
  int raw_val = analogRead(PRESSURE_SENSOR_INPUT);
  return raw_val;
}

int getBusVoltageAdcTicks() {
  // raw_val is between 0 and 1023
  int raw_val = analogRead(BUS_VOLTAGE_INPUT);
  return raw_val;
}

// TODO: rework so we can send various status messages
void sendStatusMessage()
{
  float pressureTicks = getPressureAdcTicks();
  float busVoltage = getBusVoltage();
  int rawVoltageTickets = getBusVoltageAdcTicks();


  StaticJsonDocument<256> responseJson;
  StaticJsonDocument<256> statusJson;

  JsonObject responseObject = responseJson.to<JsonObject>();
  JsonObject statusObject = statusJson.to<JsonObject>();


  responseObject["device_id"] = thisDeviceId;
  statusObject["voltage"] = busVoltage;
  statusObject["pressure_ticks"] = pressureTicks;
  statusObject["voltage_ticks"] = rawVoltageTickets;
  responseObject["status"] = statusObject;

  char responseChars[128];
  serializeJson(responseObject, responseChars);

  mqttClient.beginMessage(topicDeviceStatus);
  mqttClient.print(responseChars);
  mqttClient.endMessage();
}

// handle various mqtt messages
void onMqttMessage(int messageSize) {
  Serial.print("Received a message with topic: ");
  Serial.println(mqttClient.messageTopic());
  Serial.print("', length ");
  Serial.print(messageSize);
  Serial.println(" bytes: ");

  Serial.print("Received json: ");
  StaticJsonDocument<256> doc;
  deserializeJson(doc, mqttClient);
  serializeJson(doc, Serial);
  Serial.println();

  const int msg_device_id = int(doc["device_id"]);

  // debug
  Serial.print("Received a command with device_id: ");
  Serial.println(msg_device_id);
  Serial.print("My device id is: ");
  Serial.println(thisDeviceId);

  // only proceed if there is a device id match
  if (msg_device_id == thisDeviceId) {

    // grab the actual command
    const char* command = doc["command"];

    // handle status command
    if (strcmp(command, commandStatus) == 0) {
      sendStatusMessage();
    }

    // handle sprinkle command (wtih time length)
    if (strcmp(command, commandSprinkleStart) == 0) {
      // Serial.println("Received sprinkle start command");
      JsonVariant watering_length_seconds = doc["body"]["watering_length_seconds"];
      JsonVariant watering_length_minutes = doc["body"]["watering_length_minutes"];
      JsonVariant watering_wait_minutes = doc["body"]["watering_wait_minutes"];
      JsonVariant watering_repetitions = doc["body"]["watering_repetitions"];

      if (!watering_length_seconds.isNull()) {
        Serial.print("Watering for ");
        Serial.print(uint(watering_length_seconds));
        Serial.println(" seconds");
        spinkleForSeconds(uint(watering_length_seconds));
      } else if (!watering_length_minutes.isNull()) {

        // if the correct params are there for multiple cycles, do that
        if(!watering_wait_minutes.isNull() && !watering_repetitions.isNull())
        {
          sprinkleMinutesRepeatedly(uint(watering_length_minutes), uint(watering_wait_minutes), uint(watering_repetitions));
        }
        Serial.print("Watering for ");
        Serial.print(uint(watering_length_minutes));
        Serial.println(" minutes");
        spinkleForMinutes(uint(watering_length_minutes));
      }
    }

    // handle sprinkle on/off command
    if (strcmp(command, commandSprinkleOn) == 0) {
      Serial.println("Received spinkle on command");
      turnOnWaterPumpRelay();
    }

    if (strcmp(command, commandSprinkleOff) == 0) {
      Serial.println("Received spinkle off command");
      turnOffWaterPumpRelay();
    }

    // sleep
    if (strcmp(command, commandSleep) == 0) {
      uint sleep_length_minutes = uint(doc["body"]["sleep_length_minutes"]);
      sleepForMinutes(sleep_length_minutes);
    }

    // broker switch commands
    if (strcmp(command, commandSwitchToDebug) == 0) {
      switchToDebugBroker();
    }

    if (strcmp(command, commandSwitchToProd) == 0) {
      switchToProdBroker();
    }

    // handle power off command
    if (strcmp(command, commandPowerOff) == 0) {
      powerOffDevice();
    }
  }

  Serial.println();
  Serial.println();
}

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

void spinkleForSeconds(uint seconds) {

  // second's worth of milliseconds
  uint secondMillis = 1000;
  turnOnWaterPumpRelay();

  for (int i = 0; i < seconds; i++) {
    delay(secondMillis);
  }

  turnOffWaterPumpRelay();
}

void spinkleForMinutes(uint minutes) {

  turnOnWaterPumpRelay();
  for (int i = 0; i < minutes; i++) {
    delay(MINUTEMILLIS);
  }
  turnOffWaterPumpRelay();
}

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
}

void powerOffDevice()
{
  // this will de-energize an external N-channel MOSFET, which un-grounds the Arduino board
  Serial.print("Attempting to power off");
  digitalWrite(POWER_CONTROL, LOW);
}