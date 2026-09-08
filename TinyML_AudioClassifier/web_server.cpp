#include "web_server.h"
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "index_html.h"
#include "class_labels.h"
#include "audio_processor.h"
#include "ml_inference.h"

static AsyncWebServer server(80);
static int16_t* audio_buffer = nullptr;
static int8_t mel_buffer[N_FRAMES * N_MELS];
static size_t total_received_bytes = 0;
static unsigned long upload_start_time = 0;
static unsigned long upload_duration_ms = 0;

void setupWebServer() {
    // Allocate audio buffer dynamically (PSRAM if available, or internal heap)
    if (audio_buffer == nullptr) {
        if (psramFound()) {
            audio_buffer = (int16_t*)ps_malloc(MAX_AUDIO_SAMPLES * sizeof(int16_t));
        }
        if (audio_buffer == nullptr) {
            audio_buffer = (int16_t*)malloc(MAX_AUDIO_SAMPLES * sizeof(int16_t));
        }
        if (audio_buffer != nullptr) {
            memset(audio_buffer, 0, MAX_AUDIO_SAMPLES * sizeof(int16_t));
        } else {
            Serial.println("ERROR: Failed to allocate audio buffer!");
        }
    }

    // GET / - Serve HTML UI
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", (const uint8_t*)INDEX_HTML, INDEX_HTML_LEN);
        request->send(response);
    });

    // GET /health - System health check
    server.on("/health", HTTP_GET, [](AsyncWebServerRequest *request){
        String json = "{\"free_heap\":" + String(ESP.getFreeHeap()) + 
                      ",\"uptime_ms\":" + String(millis()) + 
                      ",\"model_status\":\"ready\"}";
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
    });
    
    // POST /classify - Receive audio and classify
    server.on("/classify", HTTP_POST, [](AsyncWebServerRequest *request){
        // 2. Compute mel spectrogram
        if (audio_buffer != nullptr) {
            computeMelSpectrogram(audio_buffer, MAX_AUDIO_SAMPLES, mel_buffer);
        } else {
            memset(mel_buffer, 0, sizeof(mel_buffer));
        }
        
        // 3. Run inference
        float output_probs[NUM_CLASSES];
        int predicted_class = 0;
        float confidence = 0.0;
        
        runInference(mel_buffer, output_probs, &predicted_class, &confidence);
        
        unsigned long total_device_ms = millis() - upload_start_time;
        if (total_device_ms < (upload_duration_ms + last_preprocessing_time + last_inference_time)) {
            total_device_ms = upload_duration_ms + last_preprocessing_time + last_inference_time + 1;
        }

        float conf_pct = confidence * 100.0f;
        if (conf_pct < 0.0f) conf_pct = 0.0f;
        if (conf_pct > 100.0f) conf_pct = 100.0f;

        float upload_s = (float)upload_duration_ms / 1000.0f;
        float mel_s = (float)last_preprocessing_time / 1000.0f;
        float inf_s = (float)last_inference_time / 1000.0f;
        float parse_s = 0.001f;
        float resample_s = 0.000f;
        float total_device_s = (float)total_device_ms / 1000.0f;

        // 4. Return JSON
        String json = "{";
        json += "\"class\":\"" + String(CLASS_LABELS[predicted_class]) + "\",";
        json += "\"top_class\":\"" + String(CLASS_LABELS[predicted_class]) + "\",";
        json += "\"category\":\"" + String(CLASS_CATEGORIES[predicted_class]) + "\",";
        json += "\"confidence\":" + String(conf_pct, 2) + ",";
        json += "\"inference_time_ms\":" + String(last_inference_time) + ",";
        json += "\"preprocessing_time_ms\":" + String(last_preprocessing_time) + ",";
        json += "\"inference_ms\":" + String(last_inference_time) + ",";
        json += "\"prep_ms\":" + String(last_preprocessing_time) + ",";
        json += "\"timing\":{";
        json += "\"upload_time_s\":" + String(upload_s, 3) + ",";
        json += "\"wav_parse_time_s\":" + String(parse_s, 3) + ",";
        json += "\"resample_time_s\":" + String(resample_s, 3) + ",";
        json += "\"mel_spectrogram_time_s\":" + String(mel_s, 3) + ",";
        json += "\"inference_time_s\":" + String(inf_s, 3) + ",";
        json += "\"total_device_time_s\":" + String(total_device_s, 3);
        json += "},";
        json += "\"predictions\":[";
        
        // Sort indices for top-5 predictions
        int indices[NUM_CLASSES];
        for(int i = 0; i < NUM_CLASSES; i++) indices[i] = i;
        for(int i = 0; i < NUM_CLASSES - 1; i++) {
            for(int j = i + 1; j < NUM_CLASSES; j++) {
                if(output_probs[indices[j]] > output_probs[indices[i]]) {
                    int tmp = indices[i];
                    indices[i] = indices[j];
                    indices[j] = tmp;
                }
            }
        }
        for(int i = 0; i < NUM_CLASSES; i++) {
            int idx = indices[i];
            float p_conf = output_probs[idx] * 100.0f;
            if (p_conf < 0.0f) p_conf = 0.0f;
            if (p_conf > 100.0f) p_conf = 100.0f;
            json += "{\"class\":\"" + String(CLASS_LABELS[idx]) + "\",\"category\":\"" + String(CLASS_CATEGORIES[idx]) + "\",\"conf\":" + String(p_conf, 2) + "}";
            if (i < NUM_CLASSES - 1) json += ",";
        }
        json += "]}";
        
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
        
        // Reset for next request
        total_received_bytes = 0;
    }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
        size_t max_bytes = MAX_AUDIO_SAMPLES * sizeof(int16_t); // 160,000 bytes (80,000 samples)

        // 1. Receive body into buffer
        if (index == 0) {
            upload_start_time = millis();
            total_received_bytes = 0;
            if (audio_buffer != nullptr) {
                memset(audio_buffer, 0, max_bytes);
            }
        }
        
        if (audio_buffer == nullptr) return;
        
        size_t center_start = 0;
        
        // If we receive more than 5 seconds, capture the center 5 seconds
        if (total > max_bytes) {
            center_start = (total - max_bytes) / 2;
            // Ensure 2-byte alignment for int16_t samples
            if (center_start % 2 != 0) {
                center_start--;
            }
        }
        
        size_t chunk_start = index;
        size_t chunk_end = index + len;
        size_t target_start = center_start;
        size_t target_end = center_start + max_bytes;
        
        // Check if current chunk overlaps with the target center window
        if (chunk_end > target_start && chunk_start < target_end) {
            size_t copy_start = max(chunk_start, target_start);
            size_t copy_end = min(chunk_end, target_end);
            size_t copy_len = copy_end - copy_start;
            
            size_t offset_in_buffer = copy_start - target_start;
            size_t offset_in_data = copy_start - chunk_start;
            
            memcpy((uint8_t*)audio_buffer + offset_in_buffer, data + offset_in_data, copy_len);
        }
        
        total_received_bytes += len;
        if (index + len >= total) {
            upload_duration_ms = millis() - upload_start_time;
        }
    });

    // Handle Preflight CORS
    server.on("/classify", HTTP_OPTIONS, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse* response = request->beginResponse(204);
        response->addHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
        response->addHeader("Access-Control-Allow-Headers", "Content-Type");
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
    });

    server.begin();
    Serial.println("Web server started");
}
