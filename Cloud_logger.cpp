#include "Cloud_logger.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define BUFFER_SIZE 64
#define UPLOAD_INTERVAL_MS 30000 // alle 30 sekunden
#define RETRY_TIMEOUT_MS 25000   // 25 sekunden retry-phase
#define RETRY_DELAY_MS 2000      // 2 sekunden zwischen versuchen

static CloudSample buffer[BUFFER_SIZE];
static volatile uint16_t head = 0;
static volatile uint16_t tail = 0;
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
static StaticJsonDocument<6144> doc;
static char payload[6144]; // same size as StaticJsonDocument

static bool bufferEmpty() {
    return head == tail;
}

static const char* scriptUrl = "YOUR_WEB_APP_URL";
static const char* secretKey = "YOUR_SECRET";
static TaskHandle_t googleTaskHandle = nullptr;

void CloudLogger::addSample(const CloudSample& s) {
    if (s.timeStamp < 1700000000) { //  < 1700000000 --> datum ist vor 2023 (z.b. 1970)
        return;  // NTP not synced yet --> no data will be sampled
    }
    portENTER_CRITICAL(&mux);
    uint16_t next = (head + 1) % BUFFER_SIZE;
    if (next != tail) {
        buffer[head] = s;
        head = next;
    }
    portEXIT_CRITICAL(&mux);
}

// Hilfsfunktion: Buffer komplett leeren
static void clearBuffer() {
    portENTER_CRITICAL(&mux);
    tail = head;  // Alle Daten verwerfen
    portEXIT_CRITICAL(&mux);
}

// Hilfsfunktion: Samples in temporäres Array kopieren (für Retry)
static int copyBufferToTemp(CloudSample* tempBuffer, int maxSamples) {
    int count = 0;
    portENTER_CRITICAL(&mux);
    uint16_t tempTail = tail;
    while (tempTail != head && count < maxSamples) {
        tempBuffer[count++] = buffer[tempTail];
        tempTail = (tempTail + 1) % BUFFER_SIZE;
    }
    portEXIT_CRITICAL(&mux);
    return count;
}

// Hilfsfunktion: JSON erstellen und Upload durchführen
static bool performUpload(WiFiClientSecure& client, HTTPClient& https, CloudSample* samples, int sampleCount) {
    if (sampleCount == 0) return true;
    doc.clear();
    doc["key"] = secretKey;
    JsonArray arr = doc.createNestedArray("samples");
    
    for (int i = 0; i < sampleCount; i++) {
        JsonArray row = arr.createNestedArray();
        char timeString[20];
        struct tm timeinfo;
        localtime_r(&samples[i].timeStamp, &timeinfo);
        strftime(timeString, sizeof(timeString), "%d.%m.%Y,%H:%M:%S", &timeinfo);
        row.add(timeString);
        row.add(samples[i].powerSaldo);
        row.add(samples[i].dcVoltage);
        row.add(samples[i].invTemp);
        row.add(samples[i].yield_day);
    }
    
    size_t len = serializeJson(doc, payload);
    https.begin(client, scriptUrl);
    https.addHeader("Content-Type", "application/json");
    int code = https.POST((uint8_t*)payload, len);
    https.end();
    return (code == 200);
}

static void googleTask(void* parameter) {
    WiFiClientSecure client;
    HTTPClient https;
    client.setInsecure();
    
    uint32_t lastUpload = millis();
    bool inRetryMode = false;
    uint32_t retryStartTime = 0;
    CloudSample tempBuffer[BUFFER_SIZE];
    int tempSampleCount = 0;
    
    while (true) {
        uint32_t currentMillis = millis();
        
        // Upload-Zeitpunkt erreicht oder wir sind im Retry-Modus
        if (inRetryMode || (currentMillis - lastUpload >= UPLOAD_INTERVAL_MS)) {
            
            if (!inRetryMode) {
                // Neuer Upload-Zyklus beginnt
                lastUpload = currentMillis;
                
                if (WiFi.status() == WL_CONNECTED && !bufferEmpty()) {
                    // In Retry-Modus wechseln und Daten sichern
                    inRetryMode = true;
                    retryStartTime = currentMillis;
                    tempSampleCount = copyBufferToTemp(tempBuffer, BUFFER_SIZE);
                    Serial.printf("Upload-Versuch gestartet mit %d Samples\n", tempSampleCount);
                } else {
                    // Kein WiFi oder keine Daten
                    vTaskDelay(pdMS_TO_TICKS(100));
                    continue;
                }
            }
            
            // Wir sind im Retry-Modus
            if (inRetryMode) {
                // Timeout-Prüfung
                if (currentMillis - retryStartTime > RETRY_TIMEOUT_MS) {
                    Serial.println("Upload timeout (25s) - Daten werden verworfen");
                    clearBuffer();
                    inRetryMode = false;
                    tempSampleCount = 0;
                    lastUpload = currentMillis;  // Timer zurücksetzen
                    vTaskDelay(pdMS_TO_TICKS(100));
                    continue;
                }
                
                // WiFi-Status prüfen
                if (WiFi.status() != WL_CONNECTED) {
                    Serial.println("Warte auf WiFi...");
                    vTaskDelay(pdMS_TO_TICKS(RETRY_DELAY_MS));
                    continue;
                }
                
                // Upload versuchen
                Serial.println("Upload-Versuch...");
                if (performUpload(client, https, tempBuffer, tempSampleCount)) {
                    // Erfolg!
                    Serial.println("Upload erfolgreich!");
                    
                    // Buffer leeren (die gesendeten Daten entfernen)
                    portENTER_CRITICAL(&mux);
                    for (int i = 0; i < tempSampleCount; i++) {
                        if (tail != head) {
                            tail = (tail + 1) % BUFFER_SIZE;
                        }
                    }
                    portEXIT_CRITICAL(&mux);
                    
                    // Retry-Modus beenden
                    inRetryMode = false;
                    tempSampleCount = 0;
                    lastUpload = currentMillis;
                } else {
                    // Fehler - warten und erneut versuchen
                    Serial.println("Upload fehlgeschlagen, erneuter Versuch in 2s...");
                    vTaskDelay(pdMS_TO_TICKS(RETRY_DELAY_MS));
                }
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void CloudLogger::begin() {
    xTaskCreatePinnedToCore(
        googleTask,
        "GoogleTask",
        9216,
        nullptr,
        1,
        &googleTaskHandle,
        1   // Core 1 (APP_CPU)
    );
}