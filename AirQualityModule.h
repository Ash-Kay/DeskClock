#ifndef AIR_QUALITY_MODULE_H
#define AIR_QUALITY_MODULE_H

#include "Module.h"
#include <HardwareSerial.h>

#define PMS_RX 16
#define PMS_TX 17
#define PMS_SET 4

#define PMS_FRAME_LEN 32
#define PMS_BAUD 9600
#define PMS_WARMUP_MS 30000
#define PMS_READ_INTERVAL 2500
#define PMS_DISPLAY_CYCLE 3000
#define PMS_AUTO_SLEEP_MS 120000

class AirQualityModule : public Module {
private:
    HardwareSerial pmsSerial = HardwareSerial(2);
    bool sensorReady = false;
    bool warmedUp = false;
    unsigned long activatedAt = 0;
    unsigned long lastRead = 0;
    unsigned long lastCycle = 0;

    uint16_t pm1_0 = 0;
    uint16_t pm2_5 = 0;
    uint16_t pm10 = 0;

    uint16_t last_pm1_0 = 0;
    uint16_t last_pm2_5 = 0;
    uint16_t last_pm10 = 0;
    bool hasLastValues = false;

    enum DisplayMode { AQI_PM25, AQI_PM10, AQI_PM1, AQI_WARMUP };
    DisplayMode currentMode = AQI_PM25;
    int modeCount = 3;

    char displayBuffer[32];
    char warmupBuffer[16];
    uint8_t frameBuffer[PMS_FRAME_LEN];

public:
    AirQualityModule() {}

    void init() override {
        pinMode(PMS_SET, OUTPUT);
        digitalWrite(PMS_SET, LOW);
        pmsSerial.begin(PMS_BAUD, SERIAL_8N1, PMS_RX, PMS_TX);
        Serial.println("PMSA003 initialized (sleeping)");
    }

    void activate() override {
        active = true;
        warmedUp = false;
        currentMode = AQI_PM25;
        activatedAt = millis();
        lastRead = 0;
        lastCycle = millis();

        wakeUpSensor();
        P->displayClear();
        P->displayText("AQI..", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
        P->displayAnimate();
        Serial.println("PMSA003 waking up, 30s warmup...");
    }

    void deactivate() override {
        active = false;
        warmedUp = false;

        last_pm1_0 = pm1_0;
        last_pm2_5 = pm2_5;
        last_pm10 = pm10;
        if (pm1_0 || pm2_5 || pm10) hasLastValues = true;

        sleepSensor();
        Serial.println("PMSA003 sleeping");
    }

    void update() override {
        if (!active) return;

        if (!warmedUp) {
            unsigned long elapsed = millis() - activatedAt;
            if (elapsed < PMS_WARMUP_MS) {
                if (hasLastValues) {
                    pm1_0 = last_pm1_0;
                    pm2_5 = last_pm2_5;
                    pm10 = last_pm10;
                    modeCount = 4;
                } else {
                    int secs = (PMS_WARMUP_MS - elapsed) / 1000;
                    sprintf(warmupBuffer, "W %ds", secs);
                    P->displayText(warmupBuffer, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
                    P->displayAnimate();
                    drainSerial();
                    return;
                }
            } else {
                warmedUp = true;
                modeCount = 3;
                if (currentMode == AQI_WARMUP) currentMode = AQI_PM25;
                drainSerial();
                Serial.println("PMSA003 ready, showing fresh values");
            }
        }

        if (warmedUp && millis() - activatedAt > PMS_AUTO_SLEEP_MS) {
            Serial.println("PMSA003 auto-sleep after 2min");
            deactivate();
            return;
        }

        if (millis() - lastRead > PMS_READ_INTERVAL) {
            readSensor();
            lastRead = millis();
        }

        if (millis() - lastCycle > PMS_DISPLAY_CYCLE) {
            currentMode = static_cast<DisplayMode>((currentMode + 1) % modeCount);
            lastCycle = millis();
        }

        formatDisplay();
        P->displayText(displayBuffer, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
        P->displayAnimate();
    }

    const char* getName() override {
        return "AirQuality";
    }

    const char* getWebControls() override {
        static String html;
        html = "<div style=\"border-left: 5px solid #8BC34A;\">";
        html += "<h3>🌬️ Air Quality (PMSA003)</h3>";

        if (!active) {
            html += "<p>Sensor is sleeping to extend lifespan.</p>";
        } else if (!warmedUp) {
            html += "<p>Warming up... please wait 30s.</p>";
        } else {
            String aqiLabel = getAQILabel(pm2_5);
            String aqiColor = getAQIColor(pm2_5);
            html += "<p style=\"color:" + aqiColor + "; font-size:1.2em;\"><strong>" + aqiLabel + "</strong></p>";
            html += "<p>PM1.0: <strong>" + String(pm1_0) + " µg/m³</strong></p>";
            html += "<p>PM2.5: <strong>" + String(pm2_5) + " µg/m³</strong></p>";
            html += "<p>PM10: <strong>" + String(pm10) + " µg/m³</strong></p>";
        }

        html += "<button onclick=\"sendRequest('/aqi/activate')\">Wake &amp; Show</button>";
        html += "<button class=\"stop\" onclick=\"sendRequest('/aqi/sleep')\">Sleep Sensor</button>";
        html += "</div>";
        return html.c_str();
    }

    bool handleWebRequest(String request) override {
        if (request.indexOf("/aqi/activate") >= 0) {
            activate();
            return true;
        }

        if (request.indexOf("/aqi/sleep") >= 0) {
            deactivate();
            return true;
        }

        return false;
    }
    
    String getJsonData() override {
        int secs = 0;
        if (active && !warmedUp) {
            secs = (PMS_WARMUP_MS - (millis() - activatedAt)) / 1000;
            if (secs < 0) secs = 0;
        }
        String json = "{\"pm1\":" + String(pm1_0) + ",\"pm25\":" + String(pm2_5) +
                      ",\"pm10\":" + String(pm10) + ",\"on\":" + String(active ? 1 : 0) +
                      ",\"warm\":" + String(warmedUp ? 1 : 0) + ",\"sec\":" + String(secs) + "}";
        return json;
    }

private:
    void wakeUpSensor() {
        digitalWrite(PMS_SET, HIGH);
        sensorReady = true;
        Serial.printf("[PMS] SET pin %d -> HIGH (read back: %d)\n", PMS_SET, digitalRead(PMS_SET));
    }

    void sleepSensor() {
        digitalWrite(PMS_SET, LOW);
        sensorReady = false;
    }

    void drainSerial() {
        while (pmsSerial.available()) {
            pmsSerial.read();
        }
    }

    bool readSensor() {
        int avail = pmsSerial.available();
        if (avail == 0) {
            Serial.println("[PMS] No serial data available");
            return false;
        }
        Serial.printf("[PMS] %d bytes in buffer\n", avail);

        while (pmsSerial.available() > 0) {
            uint8_t b = pmsSerial.read();
            if (b != 0x42) continue;

            unsigned long timeout = millis() + 500;
            while (!pmsSerial.available() && millis() < timeout) {
                delay(1);
            }
            if (!pmsSerial.available()) {
                Serial.println("[PMS] Timeout waiting for 2nd byte");
                return false;
            }

            b = pmsSerial.peek();
            if (b != 0x4D) continue;
            pmsSerial.read();

            frameBuffer[0] = 0x42;
            frameBuffer[1] = 0x4D;

            pmsSerial.setTimeout(1000);
            int bytesRead = pmsSerial.readBytes(&frameBuffer[2], PMS_FRAME_LEN - 2);
            if (bytesRead != PMS_FRAME_LEN - 2) {
                Serial.printf("[PMS] Incomplete frame: got %d of %d bytes\n", bytesRead, PMS_FRAME_LEN - 2);
                continue;
            }

            uint16_t frameLen = (frameBuffer[2] << 8) | frameBuffer[3];
            Serial.printf("[PMS] Frame length field: %d\n", frameLen);

            if (!validateChecksum()) {
                Serial.println("[PMS] Checksum failed");
                continue;
            }

            pm1_0 = (frameBuffer[10] << 8) | frameBuffer[11];
            pm2_5 = (frameBuffer[12] << 8) | frameBuffer[13];
            pm10  = (frameBuffer[14] << 8) | frameBuffer[15];

            Serial.printf("[PMS] OK -> PM1.0=%d PM2.5=%d PM10=%d\n", pm1_0, pm2_5, pm10);
            return true;
        }
        return false;
    }

    bool validateChecksum() {
        uint16_t sum = 0;
        for (int i = 0; i < PMS_FRAME_LEN - 2; i++) {
            sum += frameBuffer[i];
        }
        uint16_t check = (frameBuffer[PMS_FRAME_LEN - 2] << 8) | frameBuffer[PMS_FRAME_LEN - 1];
        Serial.printf("[PMS] Checksum calc=%d frame=%d\n", sum, check);
        return sum == check;
    }

    void formatDisplay() {
        switch (currentMode) {
            case AQI_PM25:
                sprintf(displayBuffer, "2.5:%d", pm2_5);
                break;
            case AQI_PM10:
                sprintf(displayBuffer, "10:%d", pm10);
                break;
            case AQI_PM1:
                sprintf(displayBuffer, "1:%d", pm1_0);
                break;
            case AQI_WARMUP: {
                int secs = (PMS_WARMUP_MS - (millis() - activatedAt)) / 1000;
                if (secs < 0) secs = 0;
                sprintf(displayBuffer, "W%ds", secs);
                break;
            }
        }
    }

    String getAQILabel(uint16_t pm25) {
        if (pm25 <= 12) return "Good";
        if (pm25 <= 35) return "Moderate";
        if (pm25 <= 55) return "Unhealthy (Sensitive)";
        if (pm25 <= 150) return "Unhealthy";
        if (pm25 <= 250) return "Very Unhealthy";
        return "Hazardous";
    }

    String getAQIColor(uint16_t pm25) {
        if (pm25 <= 12) return "#4CAF50";
        if (pm25 <= 35) return "#FFEB3B";
        if (pm25 <= 55) return "#FF9800";
        if (pm25 <= 150) return "#f44336";
        if (pm25 <= 250) return "#9C27B0";
        return "#880E4F";
    }
};

#endif // AIR_QUALITY_MODULE_H
