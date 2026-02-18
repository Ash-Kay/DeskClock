#include <WiFi.h>
#include <MD_Parola.h>
#include <MD_MAX72XX.h>
#include <SPI.h>
#include <time.h>
#include "config.h"
#include "ModuleManager.h"
#include "ClockModule.h"
#include "PomodoroModule.h"
#include "TextModule.h"
#include "BME280Module.h"

// --------------------------------------------------------------------------------
//  CONFIGURATION
// --------------------------------------------------------------------------------
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CLK_PIN   18
#define DATA_PIN  23
#define CS_PIN    5

// --------------------------------------------------------------------------------
//  GLOBALS
// --------------------------------------------------------------------------------
MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
WiFiServer server(80);
ModuleManager moduleManager;

// --------------------------------------------------------------------------------
//  WEB PAGE HTML
// --------------------------------------------------------------------------------
String generateWebPage() {
  String page = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Smart Matrix Clock</title>
  <style>
    body { font-family: 'Segoe UI', sans-serif; text-align: center; background: #1a1a1a; color: #eee; margin:0; padding:20px;}
    div { margin: 15px auto; max-width: 400px; padding: 20px; background: #2d2d2d; border-radius: 12px; }
    button { background-color: #4CAF50; color: white; padding: 12px 20px; border: none; border-radius: 6px; cursor: pointer; width: 100%; margin-top: 8px; font-size: 16px; }
    button.alt { background-color: #2196F3; } 
    button.stop { background-color: #f44336; } 
    button.pomo { background-color: #FF9800; } 
    input, select { width: 90%; padding: 10px; margin: 10px 0; background: #222; color: white; border: 1px solid #444; }
  </style>
  <script>
    function sendRequest(url) { 
      fetch(url).then(response => {
        if(response.ok) {
          console.log('Request sent: ' + url);
        }
      }).catch(err => console.error('Error:', err));
    }
  </script>
</head>
<body>
  <h2>🕰️ Smart Matrix Clock</h2>
  <p>Active Module: <strong>)rawliteral";
  
  page += moduleManager.getActiveModule() ? moduleManager.getActiveModule()->getName() : "None";
  page += R"rawliteral(</strong></p>
  )rawliteral";
  
  page += moduleManager.generateWebControls();
  
  page += R"rawliteral(
</body>
</html>
)rawliteral";
  
  return page;
}

// --------------------------------------------------------------------------------
//  FUNCTIONS
// --------------------------------------------------------------------------------
void handleWiFi() {
  WiFiClient client = server.available();
  if (!client) return;
  
  String req = client.readStringUntil('\r');
  client.flush();

  // Let module manager handle the request
  moduleManager.handleWebRequest(req);

  // Send response
  client.print("HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n");
  client.print(generateWebPage());
  client.stop();
}

void setup() {
  Serial.begin(115200);
  
  // Initialize display
  P.begin();
  P.setIntensity(1);
  P.displayClear();
  
  P.print("WiFi..");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { 
    delay(500); 
    Serial.print(".");
  }
  
  Serial.println("\nWiFi Connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  server.begin();
  
  // Initialize module manager
  moduleManager.setDisplay(&P);
  
  // Add modules
  moduleManager.addModule(new ClockModule());
  moduleManager.addModule(new PomodoroModule());
  moduleManager.addModule(new TextModule());
  moduleManager.addModule(new BME280Module());
  
  P.displayClear();
  Serial.println("System ready!");
}

void loop() {
  handleWiFi();
  moduleManager.update();
}