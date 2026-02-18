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
#include "AirQualityModule.h"

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
    *{box-sizing:border-box;margin:0;padding:0}
    body{font-family:'Segoe UI',sans-serif;background:#121212;color:#eee;padding:16px;max-width:460px;margin:0 auto}
    h2{text-align:center;margin-bottom:4px}
    .bar{display:flex;align-items:center;justify-content:center;gap:8px;padding:8px;margin-bottom:12px;font-size:13px;opacity:0.8}
    .dot{width:9px;height:9px;border-radius:50%;background:#f44336;transition:background 0.3s}
    .dot.on{background:#4CAF50}
    .grid{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-bottom:14px}
    .card{background:#1e1e1e;border-radius:12px;padding:14px;text-align:center;border:1px solid #2a2a2a}
    .card h4{font-size:11px;text-transform:uppercase;letter-spacing:1px;opacity:0.5;margin-bottom:6px}
    .big{font-size:26px;font-weight:700;margin:4px 0;transition:color 0.3s}
    .sub{font-size:11px;opacity:0.55;margin-top:4px}
    .ctrl{margin:10px 0;padding:16px;background:#1e1e1e;border-radius:12px;border:1px solid #2a2a2a}
    button{background:#4CAF50;color:#fff;padding:11px 16px;border:none;border-radius:6px;cursor:pointer;width:100%;margin-top:7px;font-size:15px}
    button.alt{background:#2196F3}
    button.stop{background:#f44336}
    button.pomo{background:#FF9800}
    input,select{width:100%;padding:9px;margin:6px 0;background:#262626;color:#fff;border:1px solid #444;border-radius:6px}
    label{font-size:14px}
  </style>
</head>
<body>
  <h2>&#128368; Smart Matrix Clock</h2>
  <div class="bar">
    <div class="dot" id="dot"></div>
    <span>Active: <strong id="active">--</strong></span>
  </div>

  <div class="grid">
    <div class="card">
      <h4>&#128338; Clock</h4>
      <div class="big" id="c-time">--:--</div>
      <div class="sub" id="c-date">---</div>
    </div>
    <div class="card">
      <h4>&#127777; Weather</h4>
      <div class="big" id="w-temp">--</div>
      <div class="sub"><span id="w-hum">--</span>% &middot; <span id="w-pres">--</span> hPa</div>
    </div>
    <div class="card">
      <h4>&#127788; Air Quality</h4>
      <div class="big" id="a-label">--</div>
      <div class="sub">PM2.5: <span id="a-pm25">--</span> &middot; PM10: <span id="a-pm10">--</span></div>
      <div class="sub" id="a-status"></div>
    </div>
    <div class="card">
      <h4>&#127813; Pomodoro</h4>
      <div class="big" id="p-time">--:--</div>
      <div class="sub" id="p-status">Stopped</div>
    </div>
  </div>

)rawliteral";
  
  page += moduleManager.generateWebControls();
  
  page += R"rawliteral(
  <script>
    function sendRequest(url){
      fetch(url).then(function(r){if(r.ok)console.log('OK:'+url)}).catch(function(e){console.error(e)});
    }
    function aqiInfo(v){
      if(v<=12)return['Good','#4CAF50'];
      if(v<=35)return['Moderate','#FFEB3B'];
      if(v<=55)return['Unhealthy*','#FF9800'];
      if(v<=150)return['Unhealthy','#f44336'];
      if(v<=250)return['V.Unhealthy','#9C27B0'];
      return['Hazardous','#880E4F'];
    }
    function pad(n){return String(n).padStart(2,'0')}
    function $(id){return document.getElementById(id)}

    function refresh(){
      fetch('/api/data').then(function(r){return r.json()}).then(function(d){
        $('dot').classList.add('on');
        $('active').textContent=d.active||'--';

        if(d.Clock){
          $('c-time').textContent=d.Clock.time||'--:--';
          $('c-date').textContent=d.Clock.date||'---';
        }

        if(d.Weather){
          if(d.Weather.ok){
            $('w-temp').textContent=d.Weather.t.toFixed(1)+'\u00B0'+d.Weather.u;
            $('w-hum').textContent=d.Weather.h.toFixed(0);
            $('w-pres').textContent=d.Weather.p.toFixed(0);
          }else{
            $('w-temp').textContent='N/A';
          }
        }

        if(d.AirQuality){
          var a=d.AirQuality;
          if(!a.on){
            $('a-label').textContent='Sleep';$('a-label').style.color='';
            $('a-status').textContent='Sensor sleeping';
          }else if(!a.warm){
            $('a-label').textContent='Warmup';$('a-label').style.color='#FF9800';
            $('a-status').textContent=a.sec+'s remaining';
            if(a.pm25>0){$('a-pm25').textContent=a.pm25;$('a-pm10').textContent=a.pm10;}
          }else{
            var q=aqiInfo(a.pm25);
            $('a-label').textContent=q[0];$('a-label').style.color=q[1];
            $('a-pm25').textContent=a.pm25;$('a-pm10').textContent=a.pm10;
            $('a-status').textContent='PM1.0: '+a.pm1;
          }
        }

        if(d.Pomodoro){
          var p=d.Pomodoro;
          if(p.run){
            $('p-time').textContent=pad(p.min)+':'+pad(p.sec);
            $('p-status').textContent=p.work?'Working':'Break';
            $('p-status').style.color=p.work?'#FF9800':'#8BC34A';
          }else{
            $('p-time').textContent='--:--';
            $('p-status').textContent='Stopped';$('p-status').style.color='';
          }
        }
      }).catch(function(){$('dot').classList.remove('on')});
    }

    setInterval(refresh,2000);
    refresh();
  </script>
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

  if (req.indexOf("/api/data") >= 0) {
    String json = moduleManager.generateJsonData();
    client.print("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nCache-Control: no-cache\r\n\r\n");
    client.print(json);
    client.stop();
    return;
  }

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
  moduleManager.addModule(new AirQualityModule());
  
  P.displayClear();
  Serial.println("System ready!");
}

void loop() {
  handleWiFi();
  moduleManager.update();
}