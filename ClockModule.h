#ifndef CLOCK_MODULE_H
#define CLOCK_MODULE_H

#include "Module.h"
#include <time.h>

class ClockModule : public Module {
private:
    char timeBuffer[10];
    char dateBuffer[20];
    unsigned long lastTimeUpdate = 0;
    unsigned long lastDateScroll = 0;
    bool isPM = false;
    bool use24hr = true;
    float timeZoneOffset = 5.5;
    bool showingDate = false;
    
    const char* ntpServer = "pool.ntp.org";

public:
    ClockModule() {}
    
    void init() override {
        configTime(timeZoneOffset * 3600, 0, ntpServer);
    }
    
    void activate() override {
        active = true;
        showingDate = false;
        P->displayClear();
        lastTimeUpdate = 0; // Force immediate update
    }
    
    void deactivate() override {
        active = false;
    }
    
    void update() override {
        if (!active) return;
        
        if (showingDate) {
            if (P->displayAnimate()) {
                showingDate = false;
                P->displayClear();
                lastDateScroll = millis();
            }
            return;
        }
        
        // Update time display
        if (millis() - lastTimeUpdate > 1000) {
            updateTime();
            lastTimeUpdate = millis();
        }
        
        // Show date every minute
        if (millis() - lastDateScroll > 60000) {
            showDate();
        }
        
        // Animate time display
        if (P->displayAnimate() && !use24hr && isPM) {
            // Show PM indicator
            P->getGraphicObject()->setPoint(0, 31, true);
        }
    }
    
    const char* getName() override {
        return "Clock";
    }
    
    const char* getWebControls() override {
        return R"rawliteral(
        <div style="border-left: 5px solid #2196F3;">
            <h3>🕒 Clock Settings</h3>
            <input type="number" id="tz" value="5.5" step="0.5" placeholder="Timezone">
            <label><input type="radio" name="fmt" value="24" checked> 24H</label>
            <label><input type="radio" name="fmt" value="12"> 12H</label>
            <button class="alt" onclick="sendRequest('/clock/config?tz=' + document.getElementById('tz').value + '&fmt=' + document.querySelector('input[name=\"fmt\"]:checked').value)">Update Settings</button>
            <button onclick="sendRequest('/clock/activate')">Show Clock</button>
        </div>
        )rawliteral";
    }
    
    bool handleWebRequest(String request) override {
        if (request.indexOf("/clock/config") >= 0) {
            if (request.indexOf("tz=") >= 0) {
                timeZoneOffset = request.substring(request.indexOf("tz=")+3).toFloat();
            }
            if (request.indexOf("fmt=") >= 0) {
                use24hr = (request.indexOf("fmt=24") >= 0);
            }
            init(); // Reconfigure time
            return true;
        }
        
        if (request.indexOf("/clock/activate") >= 0) {
            activate();
            return true;
        }
        
        return false;
    }
    
    bool shouldStayActive() override {
        return true; // Clock is default active module
    }

private:
    void updateTime() {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            if (use24hr) {
                strftime(timeBuffer, 10, "%H:%M", &timeinfo);
                isPM = false;
            } else {
                int h = timeinfo.tm_hour;
                isPM = (h >= 12);
                if (h == 0) h = 12;
                else if (h > 12) h -= 12;
                sprintf(timeBuffer, "%d:%02d", h, timeinfo.tm_min);
            }
            
            strftime(dateBuffer, 20, "%a %d", &timeinfo);
            P->displayText(timeBuffer, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
        }
    }
    
    void showDate() {
        showingDate = true;
        P->displayClear();
        P->displayText(dateBuffer, PA_CENTER, 50, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
    }
};

#endif // CLOCK_MODULE_H