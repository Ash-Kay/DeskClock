#include <WiFi.h>
#include <MD_Parola.h>
#include <MD_MAX72XX.h>
#include <SPI.h>
#include <time.h>
#include "config.h"

// --------------------------------------------------------------------------------
//  CONFIGURATION
// --------------------------------------------------------------------------------
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CLK_PIN   18
#define DATA_PIN  23
#define CS_PIN    5

// TIME SETTINGS
const char* ntpServer = "pool.ntp.org";
float timeZoneOffset = 5.5; 
bool use24hr = true;

// --------------------------------------------------------------------------------
//  TINY FONT (3x5 pixels)
// --------------------------------------------------------------------------------
// This fits INSIDE the border (Rows 1-5)
// Format: 3 bytes per digit (columns)
const uint8_t TINY_FONT[][3] = {
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

// --------------------------------------------------------------------------------
//  GLOBALS
// --------------------------------------------------------------------------------
MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
WiFiServer server(80);

enum DisplayMode { MODE_CLOCK, MODE_TEXT, MODE_POMODORO, MODE_DATE_CYCLE };
DisplayMode currentMode = MODE_CLOCK;

char timeBuffer[10];
char dateBuffer[20];
char textMessage[255] = "Welcome";

// Timing
unsigned long lastTimeUpdate = 0;
unsigned long lastDateScroll = 0;
unsigned long pomodoroEndTime = 0;
unsigned long pomodoroDuration = 25 * 60 * 1000;
bool pomodoroRunning = false;
bool isPM = false; // Track PM status

// --------------------------------------------------------------------------------
//  WEB PAGE HTML
// --------------------------------------------------------------------------------
const char WebPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Smart Matrix Clock</title>
  <style>
    body { font-family: 'Segoe UI', sans-serif; text-align: center; background: #1a1a1a; color: #eee; margin:0; padding:20px;}
    div { margin: 15px auto; max-width: 400px; padding: 20px; background: #2d2d2d; border-radius: 12px; }
    button { background-color: #4CAF50; color: white; padding: 12px 20px; border: none; border-radius: 6px; cursor: pointer; width: 100%; margin-top: 8px; font-size: 16px; }
    button.alt { background-color: #2196F3; } 
    button.stop { background-color: #f44336; } 
    button.pomo { background-color: #FF9800; } 
    input { width: 90%; padding: 10px; margin: 10px 0; background: #222; color: white; border: 1px solid #444; }
  </style>
  <script>
    function sendReq(url) { fetch(url); }
    function sendConfig() {
      let tz = document.getElementById("tz").value;
      let fmt = document.querySelector('input[name="fmt"]:checked').value;
      sendReq("/config?tz=" + tz + "&fmt=" + fmt);
    }
    function sendText() { sendReq("/setText?msg=" + encodeURIComponent(document.getElementById("msg").value)); }
    function startPomo(mins) { sendReq("/pomo?min=" + mins); }
    function stopPomo() { sendReq("/pomo?action=stop"); }
  </script>
</head>
<body>
  <h2>Matrix Control</h2>
  
  <div style="border-left: 5px solid #FF9800;">
    <h3>🍅 Pomodoro Timer</h3>
    <button class="pomo" onclick="startPomo(25)">Start Focus (25m)</button>
    <button class="pomo" onclick="startPomo(5)" style="background:#8BC34A;">Short Break (5m)</button>
    <button class="stop" onclick="stopPomo()">Stop</button>
  </div>

  <div style="border-left: 5px solid #2196F3;">
    <h3>🕒 Clock Settings</h3>
    <input type="number" id="tz" value="5.5" step="0.5">
    <label><input type="radio" name="fmt" value="24" checked> 24H</label>
    <label><input type="radio" name="fmt" value="12"> 12H</label>
    <button class="alt" onclick="sendConfig()">Update Settings</button>
  </div>

  <div style="border-left: 5px solid #9C27B0;">
    <h3>💬 Custom Text</h3>
    <input type="text" id="msg" placeholder="Message...">
    <button onclick="sendText()">Scroll</button>
  </div>
</body>
</html>
)rawliteral";

// --------------------------------------------------------------------------------
//  FUNCTIONS
// --------------------------------------------------------------------------------

void setupTime() { configTime(timeZoneOffset * 3600, 0, ntpServer); }

// --- CUSTOM DRAWING FUNCTIONS ---

// Draw a single tiny digit at x,y
void drawDigit(int num, int x, int y) {
  MD_MAX72XX *mx = P.getGraphicObject();
  int max_x = (MAX_DEVICES * 8) - 1; // 31
  
  for (int i=0; i<3; i++) {
    uint8_t col = TINY_FONT[num][i];
    for(int bit=0; bit<5; bit++) {
      if (col & (1 << (7-bit))) { 
         int targetY = y + bit - 1;
         int targetX = x + i;
         
         // APPLY GLOBAL 180 ROTATION (Invert X and Y)
         mx->setPoint(7 - targetY, max_x - targetX, true); 
      }
    }
  }
}

// Draw the tiny time (MM:SS) centered
void drawTinyTime(int mins, int secs) {
  MD_MAX72XX *mx = P.getGraphicObject();
  mx->clear(); // Clear background
  
  int startX = 8; 
  int max_x = (MAX_DEVICES * 8) - 1; // 31

  // Draw Digits (The drawDigit function handles the flip internally now)
  drawDigit(mins / 10, startX, 2);
  drawDigit(mins % 10, startX + 4, 2);
  
  // Draw Colon (Manual Flip needed here)
  // Original: (3, startX+7) and (5, startX+7)
  // Flipped:  (7-3, 31-(startX+7)) -> (4, ...)
  mx->setPoint(7 - 3, max_x - (startX + 8), true); 
  mx->setPoint(7 - 5, max_x - (startX + 8), true);
  
  drawDigit(secs / 10, startX + 10, 2);
  drawDigit(secs % 10, startX + 14, 2);
}

// Helper to draw the border progress bar
void drawBorder(float percent) {
  int w = MAX_DEVICES * 8; 
  int h = 8;
  int totalPerimeter = (2 * w) + (2 * (h - 2)); 
  int ledsLit = (int)(percent * totalPerimeter);
  
  int count = 0;
  MD_MAX72XX *mx = P.getGraphicObject();
  
  // NOTE: We swapped 0 and 7 for the Y coordinates to flip the border
  
  // TOP ROW (Physically appearing at bottom previously)
  for (int x = 0; x < w; x++) { 
    if (count < ledsLit) mx->setPoint(7, x, true); // Changed 0 to 7
    count++;
  }
  // RIGHT COL
  for (int y = 1; y < h - 1; y++) { 
    if (count < ledsLit) mx->setPoint(7 - y, w - 1, true); // Inverted Y
    count++;
  }
  // BOTTOM ROW
  for (int x = w - 1; x >= 0; x--) { 
    if (count < ledsLit) mx->setPoint(0, x, true); // Changed 7 to 0
    count++;
  }
  // LEFT COL
  for (int y = h - 2; y >= 1; y--) { 
    if (count < ledsLit) mx->setPoint(7 - y, 0, true); // Inverted Y
    count++;
  }
}

void handleWiFi() {
  WiFiClient client = server.available();
  if (!client) return;
  String req = client.readStringUntil('\r');
  client.flush();

  if (req.indexOf("GET /pomo") >= 0) {
    if (req.indexOf("action=stop") >= 0) {
      pomodoroRunning = false;
      currentMode = MODE_CLOCK;
      P.displayClear();
    } 
    else if (req.indexOf("min=") >= 0) {
      int mStart = req.indexOf("min=") + 4;
      int minutes = req.substring(mStart, req.indexOf(" ", mStart)).toInt();
      pomodoroDuration = minutes * 60 * 1000;
      pomodoroEndTime = millis() + pomodoroDuration;
      pomodoroRunning = true;
      currentMode = MODE_POMODORO;
    }
  }
  else if (req.indexOf("GET /config") >= 0) {
    if (req.indexOf("tz=") >= 0) timeZoneOffset = req.substring(req.indexOf("tz=")+3).toFloat();
    if (req.indexOf("fmt=") >= 0) use24hr = (req.indexOf("fmt=24") >= 0);
    setupTime();
    currentMode = MODE_CLOCK; 
    P.displayClear();
  }
  else if (req.indexOf("GET /setText") >= 0) {
    int start = req.indexOf("msg=") + 4;
    String msg = req.substring(start, req.indexOf(" ", start));
    msg.replace("%20", " ");
    msg.toCharArray(textMessage, 255);
    currentMode = MODE_TEXT;
    P.displayClear();
    P.displayText(textMessage, PA_CENTER, 50, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
  }

  client.print("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
  client.print(WebPage);
  client.stop();
}

void setup() {
  Serial.begin(115200);
  P.begin();
  P.setIntensity(1);
  P.displayClear();
  
  P.print("WiFi..");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
  
  server.begin();
  setupTime();
  P.displayClear();
  currentMode = MODE_CLOCK;
}

void loop() {
  handleWiFi(); 

  // --- MODE: POMODORO ---
  if (currentMode == MODE_POMODORO) {
    if (!pomodoroRunning) return;
    long remaining = pomodoroEndTime - millis();
    
    if (remaining <= 0) {
      pomodoroRunning = false;
      currentMode = MODE_TEXT;
      strcpy(textMessage, "DONE");
      P.displayClear();
      P.displayText(textMessage, PA_CENTER, 100, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
      return;
    }

    // Refresh display every 100ms so it doesn't flicker too much
    static unsigned long lastDraw = 0;
    if (millis() - lastDraw > 100) {
      int mins = remaining / 60000;
      int secs = (remaining % 60000) / 1000;
      float progress = (float)remaining / (float)pomodoroDuration;
      
      // We do NOT use P.displayText here. We draw pixels manually.
      // This is "Graphics Mode"
      drawTinyTime(mins, secs);
      drawBorder(progress);
      
      lastDraw = millis();
    }
  }

  // --- MODE: CLOCK ---
  else if (currentMode == MODE_CLOCK) {
    if (millis() - lastTimeUpdate > 1000) {
      struct tm timeinfo;
      if (getLocalTime(&timeinfo)) {
        
        // FORMAT TIME (Always short format "12:30")
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
        
        // Display Text
        P.displayText(timeBuffer, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
        if(P.displayAnimate()) {
           // If we are in 12hr mode and it is PM, manually light a dot
           // after the text is rendered
           if (!use24hr && isPM) {
              // Light up top-right pixel (Module 3, Col 7, Row 0)
              // Adjust X coordinate based on your specific layout (approx 31)
              P.getGraphicObject()->setPoint(0, 31, true); 
           }
        }
      }
      lastTimeUpdate = millis();

      // Scroll Date
      if (millis() - lastDateScroll > 60000) {
        currentMode = MODE_DATE_CYCLE;
        P.displayClear();
        P.displayText(dateBuffer, PA_CENTER, 50, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        lastDateScroll = millis();
      }
    }
  }

  // --- MODE: TEXT / DATE SCROLL ---
  else if (currentMode == MODE_TEXT || currentMode == MODE_DATE_CYCLE) {
    if (P.displayAnimate()) {
      if (currentMode == MODE_DATE_CYCLE) {
        currentMode = MODE_CLOCK; 
        P.displayClear();
      } else {
        P.displayReset(); 
      }
    }
  }
}