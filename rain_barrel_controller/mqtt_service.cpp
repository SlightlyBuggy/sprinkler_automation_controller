#include "mqtt_service.h"
#include <cstring>

MQTTConnection::MQTTConnection(const char* prodBrokerIp, const char* debugBrokerIp, bool setProdAsPrimary)
{
    prodBrokerIp = prodBrokerIp;
    debugBrokerIp = debugBrokerIp;

    if(setProdAsPrimary)
    {
        strcpy(targetIp, prodBrokerIp);
        strcpy(backupIp, debugBrokerIp);
    } else {
        strcpy(targetIp, debugBrokerIp);
        strcpy(backupIp, prodBrokerIp);
    }
};