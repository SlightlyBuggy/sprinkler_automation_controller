#include "mqtt_service.h"
#include <cstring>
#include <ArduinoMqttClient.h>
#include <WiFiNINA.h>

MQTTConnection::MQTTConnection(const char* prodBrokerIp, 
                               const char* debugBrokerIp, 
                               int brokerPort,
                               WiFiClient wifiClient,
                               void (*onConnectSideEffects)(),
                               const char* deviceToServerTopic
                               ) : mqttClient(wifiClient)
{
    prodBrokerIp = prodBrokerIp;
    debugBrokerIp = debugBrokerIp;
    brokerPort = brokerPort;

    onConnectSideEffectsPtr = onConnectSideEffects;
};

void MQTTConnection::setProdIpToPrimaryAndDebugIpToSecondary()
{
    strcpy(primaryBrokerIp, prodBrokerIp);
    strcpy(secondaryBrokerIp, debugBrokerIp);
}

bool MQTTConnection::connectToBroker(char* brokerIp)
{
    if(!mqttClient.connected())
    {
        if (mqttClient.connect(brokerIp, brokerPort))
        {
            onConnectSideEffects();

            return true;
        }
        // TODO: figure out how to handle error
        // was previously:
        //       Serial.print("MQTT connection failed! Error code = ");
        // Serial.println(mqttClient.connectError());
        return false;
    }
};

void MQTTConnection::setProdBrokerAsPrimaryAndReconnect()
{
    strcpy(primaryBrokerIp, prodBrokerIp);
    strcpy(secondaryBrokerIp, debugBrokerIp);
    connectToBroker(primaryBrokerIp);
};

void MQTTConnection::setDebugBrokerAsPrimaryAndReconnect()
{
    strcpy(primaryBrokerIp, debugBrokerIp);
    strcpy(secondaryBrokerIp, prodBrokerIp);
    connectToBroker(primaryBrokerIp);
};

void MQTTConnection::connectToPrimaryBrokerOrBackupOnFailure()
{
    if(!connectToBroker(primaryBrokerIp))
    {
        connectToBroker(secondaryBrokerIp);
    }
};

void MQTTConnection::poll() 
{
    mqttClient.poll();
};

void MQTTConnection::subscribeToTopic(const char* serverToDeviceTopic)
{
    mqttClient.subscribe(serverToDeviceTopic);
};

void MQTTConnection::setMessageHandler(void (*messageHandler)(int))
{
    mqttClient.onMessage(messageHandler);
};

void MQTTConnection::sendMessageToTopic(const char* deviceToServerTopic, char* message)
{
    mqttClient.beginMessage(deviceToServerTopic);
    mqttClient.print(message);
    mqttClient.endMessage();
};

void MQTTConnection::onConnectSideEffects()
{
    onConnectSideEffectsPtr();
}