#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <ArduinoLowPower.h>

#include <ArduinoMqttClient.h>
#include <WiFiNINA.h>

#include "MqttConnection.h"
#include "hardware_config.h"
#include "network_config.h"
#include "conversions.h"
#include "hardware_util.h"
#include "wifi_util.h"
#include "command_util.h"

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

MQTTConnection* mqttConnection;

// debug stuff
const bool debug_pressure = false;
const bool debug_relay = false;
const bool debug_batt_voltage = false;

// run vars to be altered by program
bool relay_on = false;

void setup() {
  // TODO: continuing to build this out and use it
  // buildMqttConnection(brokerProd, brokerDebug, brokerPort, 
  //                                     wifiClient, &onConnectToMqttBroker);
  mqttConnection = new MQTTConnection(brokerProd, brokerDebug, brokerPort, 
                                      wifiClient, &onConnectToMqttBroker);

  setPinsInitialState();

  // wait just to make sure this isn't a power transient, which seems to happen when it suddenly becomes dark
  delay(5000);

  Serial.begin(9600);

  ledSingleFlashHalfSecond();

  reconnectIfNeeded();

  setDeviceId();

  delay(1000);
}

void loop() {
  mqttConnection->poll();

  reconnectIfNeeded();

  delay(1000);
}

void setPinsInitialState()
{
  // set built in LED pin to output mode
  pinMode(LED_BUILTIN, OUTPUT);

  // set the ID pins
  pinMode(ID_PIN_1, INPUT_PULLUP);
  pinMode(ID_PIN_2, INPUT_PULLUP);
  pinMode(ID_PIN_3, INPUT_PULLUP);
  pinMode(ID_PIN_4, INPUT_PULLUP);

  // set the pump relay control pin
  pinMode(PUMP_RELAY_CONTROL, OUTPUT);

  // set the power pin to off initially
  pinMode(POWER_CONTROL, OUTPUT);
  digitalWrite(POWER_CONTROL, LOW);
}

void handleDebugCommands()
{
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
}

void onConnectToMqttBroker() {
  Serial.println("Device connected to the MQTT broker!");
  Serial.println();

  ledDoubleFlashHalfSeconds();

  sendMqttStatusMessage();
}

void reconnectIfNeeded() {

  connectWiFiIfNeeded();

  mqttConnection->connectToPrimaryBrokerOrBackupOnFailure();
}

void sendMqttStatusMessage()
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

  mqttConnection->sendDeviceMessageToServer(responseChars);
}

void onMqttMessageReceipt(int messageSize) {
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
      sendMqttStatusMessage();
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
      sleepForMinutesThenWakeUp(sleep_length_minutes, &wakeUpDevice);
    }

    // broker switch commands
    if (strcmp(command, commandSwitchToDebug) == 0) {
      mqttConnection->setDebugBrokerAsPrimaryAndReconnect();
    }

    if (strcmp(command, commandSwitchToProd) == 0) {
      mqttConnection->setProdBrokerAsPrimaryAndReconnect();
    }

    // handle power off command
    if (strcmp(command, commandPowerOff) == 0) {
      powerOffDevice();
    }
  }

  Serial.println();
  Serial.println();
}

void wakeUpDevice() {
  Serial.begin(9600);
  delay(1000);
  Serial.println("Waking up");
  reconnectIfNeeded();
  loop();
}
