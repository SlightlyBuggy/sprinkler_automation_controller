#include "mqtt_service.h"
#include <cstring>
#include <ArduinoMqttClient.h>
#include <WiFiNINA.h>

MQTTConnection::MQTTConnection(const char* prodBrokerIp, 
                               const char* debugBrokerIp, 
                               int brokerPort,
                               WiFiClient wifiClient) : mqttClient(wifiClient)
{
    prodBrokerIp = prodBrokerIp;
    debugBrokerIp = debugBrokerIp;
    brokerPort = brokerPort;
    
    setProdIpToPrimaryAndDebugIpToSecondary();
};

void MQTTConnection::setProdIpToPrimaryAndDebugIpToSecondary()
{
    strcpy(primaryBrokerIp, prodBrokerIp);
    strcpy(secondaryBrokerIp, debugBrokerIp);
}

void MQTTConnection::connectToPrimaryBroker()
{
    if(!mqttClient.connected())
    {
        // TODO serial print handler
        if (mqttClient.connect(primaryBrokerIp, brokerPort))
        {
            // TODO serial print handler

            // TODO light flashing handler

            // TODO send status handler
            // return true if connected, false if not
        }
    }
};

void MQTTConnection::setProdBrokerAsPrimaryAndReconnect()
{
    strcpy(primaryBrokerIp, prodBrokerIp);
    strcpy(secondaryBrokerIp, prodBrokerIp);
    connectToPrimaryBroker();
};