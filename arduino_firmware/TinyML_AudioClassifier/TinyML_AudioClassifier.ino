#include <TensorFlowLite_ESP32.h>
#include "wifi_ap.h"
#include "web_server.h" 
#include "ml_inference.h"
#include "audio_processor.h"

void setup() {
    Serial.begin(115200);

    // Wait up to 3 seconds for USB Serial Monitor to connect (needed for ESP32-S3 USB CDC)
    unsigned long start = millis();
    while (!Serial && (millis() - start < 3000)) {
        delay(10);
    }
    delay(500);

    Serial.println("\n\n========================================");
    Serial.println("       TinyML Audio Classifier          ");
    Serial.println("========================================");
    Serial.printf("Chip Model: %s (Rev %d)\n", ESP.getChipModel(), ESP.getChipRevision());
    Serial.printf("CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("Initial Free Heap: %d bytes\n", ESP.getFreeHeap());
    if (psramFound()) {
        Serial.printf("PSRAM Total: %d bytes, Free: %d bytes\n", ESP.getPsramSize(), ESP.getFreePsram());
    } else {
        Serial.println("PSRAM: Not Found / Disabled");
    }
    
    setupWiFiAP();
    setupAudioProcessor();
    
    if (!setupModel()) {
        Serial.println("ERROR: Failed to setup ML model!");
        while(1) delay(1000);
    }
    
    setupWebServer();
    
    Serial.printf("Free heap after setup: %d bytes\n", ESP.getFreeHeap());
    Serial.println("========================================");
    Serial.println("Ready! Connect to Wi-Fi AP & open browser");
    Serial.println("========================================\n");
}

void loop() {
    delay(10);
}
