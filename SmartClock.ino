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
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1">
<title>SmartClock</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
:root{--bg:#0d1117;--s1:#161b22;--s2:#1c2128;--bd:#30363d;--t1:#e6edf3;--t2:#8b949e;--t3:#484f58;--acc:#58a6ff;--grn:#3fb950;--red:#f85149;--org:#d29922;--pur:#bc8cff}
body{font-family:-apple-system,'Segoe UI',Roboto,sans-serif;background:var(--bg);color:var(--t1);min-height:100vh}
.wrap{max-width:520px;margin:0 auto;padding:16px 14px 32px}
header{text-align:center;padding:16px 0 12px}
header h1{font-size:20px;font-weight:600;letter-spacing:-.3px}
header .st{display:inline-flex;align-items:center;gap:6px;font-size:12px;color:var(--t2);margin-top:6px}
header .pip{width:7px;height:7px;border-radius:50%;background:var(--red);transition:.3s}
header .pip.on{background:var(--grn);box-shadow:0 0 6px var(--grn)}
.tabs{display:flex;gap:4px;border-bottom:1px solid var(--bd);margin-bottom:14px;overflow-x:auto;-webkit-overflow-scrolling:touch}
.tab{padding:9px 14px;font-size:13px;font-weight:500;color:var(--t2);cursor:pointer;border-bottom:2px solid transparent;white-space:nowrap;transition:.15s}
.tab.on{color:var(--acc);border-color:var(--acc)}
.pane{display:none}
.pane.on{display:block}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-bottom:16px}
@media(max-width:360px){.grid{grid-template-columns:1fr}}
.card{background:var(--s1);border:1px solid var(--bd);border-radius:10px;padding:14px;text-align:center;transition:border-color .2s}
.card:hover{border-color:var(--t3)}
.card .lbl{font-size:10px;text-transform:uppercase;letter-spacing:1.2px;color:var(--t3);margin-bottom:4px}
.val{font-size:28px;font-weight:700;line-height:1.2;transition:color .3s}
.meta{font-size:11px;color:var(--t2);margin-top:5px}
.sec{background:var(--s1);border:1px solid var(--bd);border-radius:10px;padding:16px;margin-bottom:12px}
.sec h3{font-size:13px;font-weight:600;margin-bottom:10px;display:flex;align-items:center;gap:6px}
.row{display:flex;gap:8px;align-items:center;margin-bottom:8px;flex-wrap:wrap}
.row label{font-size:13px;color:var(--t2)}
input[type=number],input[type=text],select{background:var(--s2);border:1px solid var(--bd);color:var(--t1);padding:8px 10px;border-radius:6px;font-size:14px;width:100%;outline:none;transition:border .15s}
input:focus,select:focus{border-color:var(--acc)}
input[type=range]{width:100%;accent-color:var(--acc);margin:4px 0}
.rg{display:inline-flex;border:1px solid var(--bd);border-radius:6px;overflow:hidden}
.rg label{padding:7px 14px;font-size:13px;cursor:pointer;color:var(--t2);transition:.15s}
.rg input{display:none}
.rg input:checked+span{background:var(--acc);color:#fff}
.rg label span{display:block;padding:7px 14px}
.btn{display:inline-flex;align-items:center;justify-content:center;width:100%;padding:10px 16px;border:none;border-radius:6px;font-size:14px;font-weight:500;cursor:pointer;transition:.15s;color:#fff;margin-top:6px}
.btn:active{transform:scale(.98)}
.b-pri{background:var(--acc)}
.b-grn{background:var(--grn)}
.b-red{background:var(--red)}
.b-org{background:var(--org);color:#000}
.b-pur{background:var(--pur);color:#000}
.b-out{background:transparent;border:1px solid var(--bd);color:var(--t2)}
.badge{display:inline-block;padding:2px 8px;border-radius:10px;font-size:11px;font-weight:600}
.sl{display:flex;align-items:center;gap:8px;font-size:13px;color:var(--t2)}
.sl .sv{min-width:32px;text-align:right;font-weight:600;color:var(--t1)}
.aqi-bar{height:4px;border-radius:2px;margin-top:8px;background:linear-gradient(90deg,#3fb950 0%,#d29922 25%,#f85149 50%,#bc8cff 75%,#880e4f 100%)}
.aqi-pip{width:8px;height:8px;border-radius:50%;background:#fff;margin-top:-6px;transition:margin-left .5s;box-shadow:0 0 4px rgba(0,0,0,.5)}
.modules{display:flex;gap:6px;flex-wrap:wrap;margin-bottom:12px}
.mbtn{padding:7px 14px;border-radius:16px;font-size:12px;font-weight:500;border:1px solid var(--bd);background:var(--s2);color:var(--t2);cursor:pointer;transition:.15s}
.mbtn.on{background:var(--acc);border-color:var(--acc);color:#fff}
</style>
</head>
<body>
<div class="wrap">
<header>
 <h1>&#9201; SmartClock</h1>
 <div class="st"><div class="pip" id="pip"></div><span id="stxt">Connecting...</span></div>
</header>

<div class="modules" id="mods">
 <button class="mbtn on" onclick="sw('Clock')">Clock</button>
 <button class="mbtn" onclick="sw('Weather')">Weather</button>
 <button class="mbtn" onclick="sw('AirQuality')">Air Quality</button>
 <button class="mbtn" onclick="sw('Pomodoro')">Pomodoro</button>
 <button class="mbtn" onclick="sw('TextScroll')">Text</button>
</div>

<div class="tabs">
 <div class="tab on" onclick="tab(this,0)">Dashboard</div>
 <div class="tab" onclick="tab(this,1)">Controls</div>
</div>

<div class="pane on" id="p0">
 <div class="grid">
  <div class="card">
   <div class="lbl">&#128338; Time</div>
   <div class="val" id="c-t">--:--</div>
   <div class="meta" id="c-d">---</div>
  </div>
  <div class="card">
   <div class="lbl">&#127777; Temperature</div>
   <div class="val" id="w-t">--</div>
   <div class="meta"><span id="w-h">--</span>% &#183; <span id="w-p">--</span> hPa</div>
  </div>
 </div>
 <div class="grid">
  <div class="card">
   <div class="lbl">&#127788; Air Quality</div>
   <div class="val" id="a-v">--</div>
   <div class="meta">PM2.5: <span id="a-25">--</span> &#183; PM10: <span id="a-10">--</span></div>
   <div class="aqi-bar"></div>
   <div class="aqi-pip" id="a-pip"></div>
   <div class="meta" id="a-st"></div>
  </div>
  <div class="card">
   <div class="lbl">&#127813; Pomodoro</div>
   <div class="val" id="p-t">--:--</div>
   <div class="meta" id="p-s">Stopped</div>
  </div>
 </div>
</div>

<div class="pane" id="p1">

 <div class="sec">
  <h3>&#128338; Clock</h3>
  <div class="row">
   <input type="number" id="tz" value="5.5" step="0.5" placeholder="Timezone offset" style="flex:1">
  </div>
  <div class="row">
   <div class="rg">
    <label><input type="radio" name="cf" value="12" checked><span>12H</span></label>
    <label><input type="radio" name="cf" value="24"><span>24H</span></label>
   </div>
  </div>
  <button class="btn b-pri" onclick="sendRequest('/clock/config?tz='+$('tz').value+'&fmt='+document.querySelector('input[name=cf]:checked').value)">Apply</button>
 </div>

 <div class="sec">
  <h3>&#127777; Weather</h3>
  <div class="row">
   <div class="rg">
    <label><input type="radio" name="wu" value="C" checked><span>&#176;C</span></label>
    <label><input type="radio" name="wu" value="F"><span>&#176;F</span></label>
   </div>
  </div>
  <button class="btn b-pri" onclick="sendRequest('/weather/unit?u='+document.querySelector('input[name=wu]:checked').value)">Set Unit</button>
 </div>

 <div class="sec">
  <h3>&#127788; Air Quality</h3>
  <button class="btn b-grn" onclick="sendRequest('/aqi/activate')">&#9654; Wake Sensor</button>
  <button class="btn b-red" onclick="sendRequest('/aqi/sleep')">&#9724; Sleep Sensor</button>
 </div>

 <div class="sec">
  <h3>&#127813; Pomodoro</h3>
  <div class="sl">
   <span>Work</span>
   <input type="range" id="pw" min="5" max="60" value="30" oninput="$('pwv').textContent=this.value">
   <span class="sv" id="pwv">30</span><span>min</span>
  </div>
  <div class="sl">
   <span>Break</span>
   <input type="range" id="pb" min="1" max="30" value="5" oninput="$('pbv').textContent=this.value">
   <span class="sv" id="pbv">5</span><span>min</span>
  </div>
  <div class="row" style="margin-top:6px">
   <button class="btn b-org" style="flex:1" onclick="sendRequest('/pomo/set?w='+$('pw').value);sendRequest('/pomo/work')">Work</button>
   <button class="btn b-grn" style="flex:1" onclick="sendRequest('/pomo/set?b='+$('pb').value);sendRequest('/pomo/break')">Break</button>
  </div>
  <button class="btn b-red" onclick="sendRequest('/pomo/stop')">Stop</button>
 </div>

 <div class="sec">
  <h3>&#128172; Scroll Text</h3>
  <input type="text" id="tm" placeholder="Enter message..." maxlength="250">
  <div class="row">
   <select id="ts" style="flex:1">
    <option value="25">Slow</option>
    <option value="50" selected>Normal</option>
    <option value="75">Fast</option>
    <option value="100">Very Fast</option>
   </select>
  </div>
  <button class="btn b-pur" onclick="sendRequest('/text/show?msg='+encodeURIComponent($('tm').value)+'&speed='+$('ts').value)">Scroll</button>
 </div>

</div>
</div>

<script>
function $(id){return document.getElementById(id)}
function pad(n){return String(n).padStart(2,'0')}

function sendRequest(u){
  fetch(u).then(function(r){if(!r.ok)console.error(u)}).catch(function(e){console.error(e)});
}

function sw(m){
  sendRequest('/module/'+m);
}

function tab(el,i){
  var ts=document.querySelectorAll('.tab');
  var ps=document.querySelectorAll('.pane');
  for(var j=0;j<ts.length;j++){ts[j].classList.remove('on');ps[j].classList.remove('on')}
  el.classList.add('on');ps[i].classList.add('on');
}

function aqiInfo(v){
  if(v<=12)return['Good','#3fb950',0];
  if(v<=35)return['Moderate','#d29922',25];
  if(v<=55)return['Sensitive','#f0883e',45];
  if(v<=150)return['Unhealthy','#f85149',60];
  if(v<=250)return['V.Unhealthy','#bc8cff',80];
  return['Hazardous','#880e4f',95];
}

function refresh(){
  fetch('/api/data').then(function(r){return r.json()}).then(function(d){
    $('pip').classList.add('on');
    $('stxt').textContent=d.active||'--';

    var btns=document.querySelectorAll('.mbtn');
    for(var i=0;i<btns.length;i++){
      btns[i].classList.toggle('on',btns[i].textContent.trim()===d.active);
    }

    if(d.Clock){
      $('c-t').textContent=d.Clock.time||'--:--';
      $('c-d').textContent=d.Clock.date||'---';
    }

    if(d.Weather){
      if(d.Weather.ok){
        $('w-t').textContent=d.Weather.t.toFixed(1)+'\u00B0'+d.Weather.u;
        $('w-h').textContent=d.Weather.h.toFixed(0);
        $('w-p').textContent=d.Weather.p.toFixed(0);
      }else{$('w-t').textContent='N/A';}
    }

    if(d.AirQuality){
      var a=d.AirQuality;
      if(!a.on){
        $('a-v').textContent='Sleep';$('a-v').style.color='var(--t3)';
        $('a-st').textContent='Sleeping to extend lifespan';
      }else if(!a.warm){
        $('a-v').textContent='Warmup';$('a-v').style.color='var(--org)';
        $('a-st').textContent=a.sec+'s remaining';
        if(a.pm25>0){$('a-25').textContent=a.pm25;$('a-10').textContent=a.pm10;}
      }else{
        var q=aqiInfo(a.pm25);
        $('a-v').textContent=q[0];$('a-v').style.color=q[1];
        $('a-25').textContent=a.pm25;$('a-10').textContent=a.pm10;
        $('a-st').textContent='PM1.0: '+a.pm1;
        $('a-pip').style.marginLeft=q[2]+'%';
      }
    }

    if(d.Pomodoro){
      var p=d.Pomodoro;
      $('pw').value=p.wm;$('pwv').textContent=p.wm;
      $('pb').value=p.bm;$('pbv').textContent=p.bm;
      if(p.run){
        $('p-t').textContent=pad(p.min)+':'+pad(p.sec);
        $('p-s').textContent=p.work?'Working':'Break';
        $('p-s').style.color=p.work?'var(--org)':'var(--grn)';
      }else{
        $('p-t').textContent='--:--';
        $('p-s').textContent='Stopped';$('p-s').style.color='';
      }
    }
  }).catch(function(){$('pip').classList.remove('on');$('stxt').textContent='Offline'});
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