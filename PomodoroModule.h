#ifndef POMODORO_MODULE_H
#define POMODORO_MODULE_H

#include "Module.h"

class PomodoroModule : public Module {
private:
    unsigned long endTime = 0;
    unsigned long duration = 0;
    bool running = false;
    bool isWork = true;
    
    int workMin = 30;
    int breakMin = 5;
    
    unsigned long lastUpdate = 0;
    char timeDisplay[16];

public:
    PomodoroModule() {}
    
    void init() override {}
    
    void activate() override {
        active = true;
        P->displayClear();
        lastUpdate = 0;
    }
    
    void deactivate() override {
        active = false;
    }
    
    void update() override {
        if (!active) return;
        
        if (P->displayAnimate()) {
            // Animation done
        }
        
        if (!running) return;
        
        long remaining = endTime - millis();
        
        if (remaining <= 0) {
            running = false;
            P->displayClear();
            
            if (isWork) {
                P->displayText("BREAK", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
            } else {
                P->displayText("WORK", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
            }
            return;
        }
        
        if (millis() - lastUpdate > 1000) {
            int mins = remaining / 60000;
            int secs = (remaining % 60000) / 1000;
            sprintf(timeDisplay, "%02d:%02d", mins, secs);
            P->displayText(timeDisplay, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
            lastUpdate = millis();
        }
    }
    
    const char* getName() override {
        return "Pomodoro";
    }
    
    const char* getWebControls() override {
        static String html;
        html = "<div style=\"border-left: 5px solid #FF9800;\">";
        html += "<h3>🍅 Pomodoro</h3>";
        html += "<p style=\"margin:5px 0;\"><label>Work: <span id=\"wv\">" + String(workMin) + "</span> min</label><br>";
        html += "<input type=\"range\" id=\"ws\" min=\"5\" max=\"60\" value=\"" + String(workMin) + "\" ";
        html += "oninput=\"document.getElementById('wv').textContent=this.value\" ";
        html += "onchange=\"sendRequest('/pomo/set?w='+this.value)\"></p>";
        html += "<p style=\"margin:5px 0;\"><label>Break: <span id=\"bv\">" + String(breakMin) + "</span> min</label><br>";
        html += "<input type=\"range\" id=\"bs\" min=\"1\" max=\"30\" value=\"" + String(breakMin) + "\" ";
        html += "oninput=\"document.getElementById('bv').textContent=this.value\" ";
        html += "onchange=\"sendRequest('/pomo/set?b='+this.value)\"></p>";
        html += "<button class=\"pomo\" onclick=\"sendRequest('/pomo/work')\">Start Work</button>";
        html += "<button class=\"pomo\" style=\"background:#8BC34A;\" onclick=\"sendRequest('/pomo/break')\">Start Break</button>";
        html += "<button class=\"stop\" onclick=\"sendRequest('/pomo/stop')\">Stop</button>";
        html += "</div>";
        return html.c_str();
    }
    
    bool handleWebRequest(String request) override {
        if (request.indexOf("/pomo/set") >= 0) {
            if (request.indexOf("w=") >= 0) {
                int idx = request.indexOf("w=") + 2;
                int end = request.indexOf("&", idx);
                if (end == -1) end = request.indexOf(" ", idx);
                workMin = request.substring(idx, end).toInt();
                if (workMin < 5) workMin = 5;
                if (workMin > 60) workMin = 60;
            }
            if (request.indexOf("b=") >= 0) {
                int idx = request.indexOf("b=") + 2;
                int end = request.indexOf("&", idx);
                if (end == -1) end = request.indexOf(" ", idx);
                breakMin = request.substring(idx, end).toInt();
                if (breakMin < 1) breakMin = 1;
                if (breakMin > 30) breakMin = 30;
            }
            return true;
        }
        
        if (request.indexOf("/pomo/work") >= 0) {
            isWork = true;
            duration = workMin * 60 * 1000UL;
            endTime = millis() + duration;
            running = true;
            activate();
            return true;
        }
        
        if (request.indexOf("/pomo/break") >= 0) {
            isWork = false;
            duration = breakMin * 60 * 1000UL;
            endTime = millis() + duration;
            running = true;
            activate();
            return true;
        }
        
        if (request.indexOf("/pomo/stop") >= 0) {
            running = false;
            if (active) {
                P->displayClear();
                P->displayText("STOP", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
            }
            return true;
        }
        
        return false;
    }
    
    bool isRunning() const {
        return running;
    }
    
    String getJsonData() override {
        int mins = 0, secs = 0;
        if (running) {
            long remaining = endTime - millis();
            if (remaining > 0) {
                mins = remaining / 60000;
                secs = (remaining % 60000) / 1000;
            }
        }
        String json = "{\"run\":" + String(running ? 1 : 0) + ",\"work\":" + String(isWork ? 1 : 0) +
                      ",\"min\":" + String(mins) + ",\"sec\":" + String(secs) +
                      ",\"wm\":" + String(workMin) + ",\"bm\":" + String(breakMin) + "}";
        return json;
    }
};

#endif // POMODORO_MODULE_H
