#include <WiFiNINA.h>
#include "arduino_secrets.h"
#include "hardware_util.h"

char ssid[] = SECRET_SSID;  
char pass[] = SECRET_PASS;
int wiFiStatus = WL_IDLE_STATUS;

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

    ledSingleFlashOneSecond();

    Serial.println("You're connected to the network");
    Serial.println();
  }

  return;
};