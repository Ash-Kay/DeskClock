#ifndef BME280_MODULE_H
#define BME280_MODULE_H

#include "Module.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define BME_SDA 21
#define BME_SCL 22

class BME280Module : public Module {
private:
    Adafruit_BME280 bme;
    bool sensorReady = false;

    float temperature = 0;
    float humidity = 0;
    float pressure = 0;

    unsigned long lastRead = 0;
    unsigned long lastCycle = 0;
    const unsigned long READ_INTERVAL = 2000;
    const unsigned long CYCLE_INTERVAL = 4000;

    enum DisplayMode { TEMP, HUMIDITY, PRESSURE };
    DisplayMode currentMode = TEMP;

    char displayBuffer[32];
    bool useCelsius = true;

    void scanI2C() {
        Serial.println("Scanning I2C bus...");
        for (uint8_t addr = 1; addr < 127; addr++) {
            Wire.beginTransmission(addr);
            if (Wire.endTransmission() == 0) {
                Serial.print("  Device found at 0x");
                if (addr < 16) Serial.print("0");
                Serial.println(addr, HEX);
            }
        }
    }

public:
    BME280Module() {}

    void init() override {
        Wire.begin(BME_SDA, BME_SCL);
        delay(100);

        scanI2C();

        sensorReady = bme.begin(0x76, &Wire);
        if (!sensorReady) {
            Serial.println("BME280 not at 0x76, trying 0x77...");
            sensorReady = bme.begin(0x77, &Wire);
        }

        if (sensorReady) {
            bme.setSampling(
                Adafruit_BME280::MODE_NORMAL,
                Adafruit_BME280::SAMPLING_X2,
                Adafruit_BME280::SAMPLING_X16,
                Adafruit_BME280::SAMPLING_X1,
                Adafruit_BME280::FILTER_X16,
                Adafruit_BME280::STANDBY_MS_500
            );
            Serial.println("BME280 initialized successfully");
        } else {
            Serial.println("BME280 not found on either address. Check wiring:");
            Serial.println("  SDA -> GPIO 21, SCL -> GPIO 22, VCC -> 3.3V");
        }
    }

    void activate() override {
        active = true;
        currentMode = TEMP;
        lastRead = 0;
        lastCycle = millis();
        P->displayClear();
    }

    void deactivate() override {
        active = false;
    }

    void update() override {
        if (!active || !sensorReady) return;

        if (millis() - lastRead > READ_INTERVAL) {
            readSensor();
            lastRead = millis();
        }

        if (millis() - lastCycle > CYCLE_INTERVAL) {
            currentMode = static_cast<DisplayMode>((currentMode + 1) % 3);
            lastCycle = millis();
        }

        formatDisplay();
        P->displayText(displayBuffer, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
        P->displayAnimate();
    }

    const char* getName() override {
        return "Weather";
    }

    const char* getWebControls() override {
        static String html;
        html = "<div style=\"border-left: 5px solid #00BCD4;\">";
        html += "<h3>🌡️ Weather Station</h3>";

        if (!sensorReady) {
            html += "<p style=\"color:#f44336;\">Sensor not detected. Check wiring.</p>";
        } else {
            html += "<p>Temp: <strong>" + String(temperature, 1);
            html += useCelsius ? " °C" : " °F";
            html += "</strong></p>";
            html += "<p>Humidity: <strong>" + String(humidity, 1) + " %</strong></p>";
            html += "<p>Pressure: <strong>" + String(pressure, 1) + " hPa</strong></p>";
        }

        html += "<label><input type=\"radio\" name=\"unit\" value=\"C\"";
        if (useCelsius) html += " checked";
        html += "> °C</label> ";
        html += "<label><input type=\"radio\" name=\"unit\" value=\"F\"";
        if (!useCelsius) html += " checked";
        html += "> °F</label>";
        html += "<button class=\"alt\" onclick=\"sendRequest('/weather/unit?u=' + document.querySelector('input[name=\\\"unit\\\"]:checked').value)\">Set Unit</button>";
        html += "<button onclick=\"sendRequest('/weather/activate')\">Show Weather</button>";
        html += "</div>";
        return html.c_str();
    }

    bool handleWebRequest(String request) override {
        if (request.indexOf("/weather/unit") >= 0) {
            useCelsius = (request.indexOf("u=C") >= 0);
            return true;
        }

        if (request.indexOf("/weather/activate") >= 0) {
            activate();
            return true;
        }

        return false;
    }

private:
    void readSensor() {
        temperature = bme.readTemperature();
        humidity = bme.readHumidity();
        pressure = bme.readPressure() / 100.0F;
    }

    void formatDisplay() {
        switch (currentMode) {
            case TEMP: {
                float t = useCelsius ? temperature : (temperature * 9.0 / 5.0 + 32.0);
                sprintf(displayBuffer, "%.0f%s", t, useCelsius ? "C" : "F");
                break;
            }
            case HUMIDITY:
                sprintf(displayBuffer, "%.0f%%", humidity);
                break;
            case PRESSURE:
                sprintf(displayBuffer, "%.0fhP", pressure);
                break;
        }
    }
};

#endif // BME280_MODULE_H
