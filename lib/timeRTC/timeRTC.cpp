
#include "rtc.h"

#include "timeRTC.h"
#include <WiFi.h>
#include <time.h>

const char* ssid = "PLAY_Swiatlowod_C894";
const char* password = "MFEtYRc3";

void setupRTC() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(1000);
    }
    Serial.println("\nConnected to WiFi");
    configTime(0, 0, "pool.ntp.org");
  }
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}

