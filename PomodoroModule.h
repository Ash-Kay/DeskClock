#ifndef POMODORO_MODULE_H
#define POMODORO_MODULE_H

#include "Module.h"

class PomodoroModule : public Module {
private:
    unsigned long pomodoroEndTime = 0;
    unsigned long pomodoroDuration = 25 * 60 * 1000;
    bool pomodoroRunning = false;
    unsigned long lastDraw = 0;
    
    // Custom tiny font for compact display
    const uint8_t TINY_FONT[10][3] = {
        {0xF8, 0x88, 0xF8}, // 0
        {0x00, 0xF8, 0x00}, // 1
        {0xE8, 0xA8, 0xB8}, // 2
        {0x88, 0xA8, 0xF8}, // 3
        {0x38, 0x20, 0xF8}, // 4
        {0xB8, 0xA8, 0xE8}, // 5
        {0xF8, 0xA8, 0xE8}, // 6
        {0x08, 0x08, 0xF8}, // 7
        {0xF8, 0xA8, 0xF8}, // 8
        {0xB8, 0xA8, 0xF8}  // 9
    };

public:
    PomodoroModule() {}
    
    void init() override {
        // Nothing to initialize
    }
    
    void activate() override {
        active = true;
        P->displayClear();
    }
    
    void deactivate() override {
        active = false;
        pomodoroRunning = false;
    }
    
    void update() override {
        if (!active || !pomodoroRunning) return;
        
        long remaining = pomodoroEndTime - millis();
        
        if (remaining <= 0) {
            pomodoroRunning = false;
            active = false; // Auto-deactivate when done
            // Switch to completion message
            P->displayClear();
            P->displayText("DONE!", PA_CENTER, 100, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
            return;
        }

        // Refresh display every 100ms
        if (millis() - lastDraw > 100) {
            int mins = remaining / 60000;
            int secs = (remaining % 60000) / 1000;
            float progress = (float)remaining / (float)pomodoroDuration;
            
            drawTinyTime(mins, secs);
            drawBorder(progress);
            
            lastDraw = millis();
        }
    }
    
    const char* getName() override {
        return "Pomodoro";
    }
    
    const char* getWebControls() override {
        return R"rawliteral(
        <div style="border-left: 5px solid #FF9800;">
            <h3>🍅 Pomodoro Timer</h3>
            <button class="pomo" onclick="sendRequest('/pomodoro/start?min=25')">Focus (25m)</button>
            <button class="pomo" onclick="sendRequest('/pomodoro/start?min=5')" style="background:#8BC34A;">Break (5m)</button>
            <button class="pomo" onclick="sendRequest('/pomodoro/start?min=15')" style="background:#9C27B0;">Custom (15m)</button>
            <button class="stop" onclick="sendRequest('/pomodoro/stop')">Stop</button>
        </div>
        )rawliteral";
    }
    
    bool handleWebRequest(String request) override {
        if (request.indexOf("/pomodoro/start") >= 0) {
            if (request.indexOf("min=") >= 0) {
                int mStart = request.indexOf("min=") + 4;
                int minutes = request.substring(mStart, request.indexOf("&", mStart) == -1 ? 
                    request.indexOf(" ", mStart) : request.indexOf("&", mStart)).toInt();
                startTimer(minutes);
                return true;
            }
        }
        
        if (request.indexOf("/pomodoro/stop") >= 0) {
            stopTimer();
            return true;
        }
        
        return false;
    }
    
    bool isRunning() const {
        return pomodoroRunning;
    }

private:
    void startTimer(int minutes) {
        pomodoroDuration = minutes * 60 * 1000UL;
        pomodoroEndTime = millis() + pomodoroDuration;
        pomodoroRunning = true;
        activate();
    }
    
    void stopTimer() {
        pomodoroRunning = false;
        active = false;
    }
    
    void drawDigit(int num, int x, int y) {
        MD_MAX72XX *mx = P->getGraphicObject();
        int max_x = (4 * 8) - 1; // Assuming 4 modules
        
        for (int i = 0; i < 3; i++) {
            uint8_t col = TINY_FONT[num][i];
            for (int bit = 0; bit < 5; bit++) {
                if (col & (1 << (7 - bit))) {
                    int targetY = y + bit - 1;
                    int targetX = x + i;
                    mx->setPoint(7 - targetY, max_x - targetX, true);
                }
            }
        }
    }

    void drawTinyTime(int mins, int secs) {
        MD_MAX72XX *mx = P->getGraphicObject();
        mx->clear();
        
        int startX = 8;
        int max_x = (4 * 8) - 1;

        drawDigit(mins / 10, startX, 2);
        drawDigit(mins % 10, startX + 4, 2);
        
        // Colon
        mx->setPoint(7 - 3, max_x - (startX + 8), true);
        mx->setPoint(7 - 5, max_x - (startX + 8), true);
        
        drawDigit(secs / 10, startX + 10, 2);
        drawDigit(secs % 10, startX + 14, 2);
    }

    void drawBorder(float percent) {
        int w = 4 * 8;
        int h = 8;
        int totalPerimeter = (2 * w) + (2 * (h - 2));
        int ledsLit = (int)(percent * totalPerimeter);
        
        int count = 0;
        MD_MAX72XX *mx = P->getGraphicObject();
        
        // Top row
        for (int x = 0; x < w; x++) {
            if (count < ledsLit) mx->setPoint(7, x, true);
            count++;
        }
        // Right column
        for (int y = 1; y < h - 1; y++) {
            if (count < ledsLit) mx->setPoint(7 - y, w - 1, true);
            count++;
        }
        // Bottom row
        for (int x = w - 1; x >= 0; x--) {
            if (count < ledsLit) mx->setPoint(0, x, true);
            count++;
        }
        // Left column
        for (int y = h - 2; y >= 1; y--) {
            if (count < ledsLit) mx->setPoint(7 - y, 0, true);
            count++;
        }
    }
};

#endif // POMODORO_MODULE_H