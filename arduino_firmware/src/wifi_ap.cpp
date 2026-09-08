#include "wifi_ap.h"
#include <WiFi.h>

void setupWiFiAP() {
    const char* ssid = "TinyML-AudioClassifier";
    const char* password = "12345678";

    Serial.println("\n[Wi-Fi] Starting Access Point...");
    WiFi.mode(WIFI_AP);
    bool result = WiFi.softAP(ssid, password);
    
    if (result) {
        IPAddress IP = WiFi.softAPIP();
        Serial.println("[Wi-Fi] Access Point Started Successfully!");
        Serial.printf("  SSID:     %s\n", ssid);
        Serial.printf("  Password: %s\n", password);
        Serial.printf("  Web UI:   http://%s/\n\n", IP.toString().c_str());
    } else {
        Serial.println("[Wi-Fi] ERROR: Failed to start Access Point!");
    }
}
