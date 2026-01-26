#ifndef TEXT_MODULE_H
#define TEXT_MODULE_H

#include "Module.h"

class TextModule : public Module {
private:
    char textMessage[255];
    bool scrolling = false;

public:
    TextModule() {
        strcpy(textMessage, "Welcome!");
    }
    
    void init() override {
        // Nothing to initialize
    }
    
    void activate() override {
        active = true;
        scrolling = true;
        P->displayClear();
        P->displayText(textMessage, PA_CENTER, 50, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
    }
    
    void deactivate() override {
        active = false;
        scrolling = false;
    }
    
    void update() override {
        if (!active || !scrolling) return;
        
        if (P->displayAnimate()) {
            // Animation complete, can deactivate or repeat
            scrolling = false;
            active = false; // Auto-deactivate after scroll
        }
    }
    
    const char* getName() override {
        return "TextScroll";
    }
    
    const char* getWebControls() override {
        return R"rawliteral(
        <div style="border-left: 5px solid #9C27B0;">
            <h3>💬 Text Scroll</h3>
            <input type="text" id="textMsg" placeholder="Enter message..." maxlength="250">
            <select id="textSpeed">
                <option value="25">Slow</option>
                <option value="50" selected>Normal</option>
                <option value="75">Fast</option>
                <option value="100">Very Fast</option>
            </select>
            <button onclick="sendRequest('/text/show?msg=' + encodeURIComponent(document.getElementById('textMsg').value) + '&speed=' + document.getElementById('textSpeed').value)">Scroll Text</button>
            <button onclick="sendRequest('/text/show?msg=Hello%20World!&speed=50')">Test Message</button>
        </div>
        )rawliteral";
    }
    
    bool handleWebRequest(String request) override {
        if (request.indexOf("/text/show") >= 0) {
            int start = request.indexOf("msg=") + 4;
            int end = request.indexOf("&", start);
            if (end == -1) end = request.indexOf(" ", start);
            
            String msg = request.substring(start, end);
            msg.replace("%20", " ");
            msg.replace("%21", "!");
            msg.replace("%3F", "?");
            msg.toCharArray(textMessage, 255);
            
            // Get speed if specified
            int speed = 50; // default
            if (request.indexOf("speed=") >= 0) {
                int speedStart = request.indexOf("speed=") + 6;
                int speedEnd = request.indexOf("&", speedStart);
                if (speedEnd == -1) speedEnd = request.indexOf(" ", speedStart);
                speed = request.substring(speedStart, speedEnd).toInt();
            }
            
            displayText(speed);
            return true;
        }
        
        return false;
    }
    
    void setText(const char* text) {
        strncpy(textMessage, text, 254);
        textMessage[254] = '\0';
    }
    
    void displayText(int speed = 50) {
        activate();
        P->displayClear();
        P->displayText(textMessage, PA_CENTER, speed, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
    }
    
    const char* getCurrentText() const {
        return textMessage;
    }
};

#endif // TEXT_MODULE_H