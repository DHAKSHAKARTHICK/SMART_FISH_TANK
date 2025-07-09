//Next tft display to be included
#include <WiFi.h>
#include <WebServer.h>
#include <time.h>
#include <Preferences.h>
#include <ESP32Servo.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include "RTClib.h"

#define WATER_LEVEL_PIN 35  // Adjust based on your wiring
int waterLevelPercent = 0;

RTC_DS3231 rtc;

#define ONE_WIRE_BUS 19  // GPIO 19

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

float currentTemp = 0.0;
unsigned long lastTempUpdate = 0;

Preferences preferences;

const char* ssid = "DYAN";
const char* password = "yathiabi";

WebServer server(80);

const int relayPins[2] = {5, 18};
bool relayState[2] = {false, false};
bool scheduleEnabled[2] = {true, true};
bool manualOverride[2] = {false, false};
unsigned long lastScheduleUpdate[2] = {0, 0};

Servo servo;
const int servoPin = 4;
bool servoActive = false;
unsigned long servoStartTime = 0;
int servoStage = 0;
int servoOnHour1, servoOnMinute1;
int servoOnHour2, servoOnMinute2;
bool servoScheduleEnabled = true;

String currentRtcTimeStr = "";
const int phPin = 36;        // Replace with the actual GPIO you're using for the pH sensor
float phValue = 0.0;


struct Schedule {
  int onHour;
  int onMinute;
  int offHour;
  int offMinute;
};

Schedule schedules[2];

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800;
const int daylightOffset_sec = 0;

void updateWaterLevel() {
  int rawValue = analogRead(WATER_LEVEL_PIN);
  
  const int minValue = 400;   // Adjust based on dry sensor
  const int maxValue = 1000;  // Adjust based on full submersion

  rawValue = constrain(rawValue, minValue, maxValue);
  waterLevelPercent = map(rawValue, minValue, maxValue, 0, 100);
}

void updatePh() {
  int rawValue = analogRead(phPin);
  float voltage = rawValue * (3.3 / 4095.0);
  phValue = 7 + ((2.5 - voltage) / 0.18);

  Serial.print("pH Sensor Raw: ");
  Serial.print(rawValue);
  Serial.print(" -> Voltage: ");
  Serial.print(voltage, 3);
  Serial.print(" -> pH: ");
  Serial.println(phValue, 2);
}

void loadSettings() {
  preferences.begin("relaycfg", true);
  for (int i = 0; i < 2; i++) {
    schedules[i].onHour = preferences.getInt((String("r")+i+"onH").c_str(), 7);
    schedules[i].onMinute = preferences.getInt((String("r")+i+"onM").c_str(), 0);
    schedules[i].offHour = preferences.getInt((String("r")+i+"offH").c_str(), 19);
    schedules[i].offMinute = preferences.getInt((String("r")+i+"offM").c_str(), 0);
    scheduleEnabled[i] = preferences.getBool((String("r")+i+"en").c_str(), true);
  }
  preferences.end();

  // Load servo schedule
  preferences.begin("servocfg", true);
  servoOnHour1 = preferences.getInt((String("s") + "1onH").c_str(), 8);
  servoOnMinute1 = preferences.getInt((String("s") + "1onM").c_str(), 0);
  servoOnHour2 = preferences.getInt((String("s") + "2onH").c_str(), 20);
  servoOnMinute2 = preferences.getInt((String("s") + "2onM").c_str(), 0);
  servoScheduleEnabled = preferences.getBool((String("s") + "en").c_str(), true);
  preferences.end();
}

void saveScheduleToPrefs(int i) {
  preferences.begin("relaycfg", false);
  preferences.putInt((String("r")+i+"onH").c_str(), schedules[i].onHour);
  preferences.putInt((String("r")+i+"onM").c_str(), schedules[i].onMinute);
  preferences.putInt((String("r")+i+"offH").c_str(), schedules[i].offHour);
  preferences.putInt((String("r")+i+"offM").c_str(), schedules[i].offMinute);
  preferences.putBool((String("r")+i+"en").c_str(), scheduleEnabled[i]);
  preferences.end();
}

void saveRelayState(int i) {
  preferences.begin("relayStates", false);
  preferences.putBool(("relay" + String(i)).c_str(), relayState[i]);
  preferences.putBool(("manual" + String(i)).c_str(), manualOverride[i]);
  preferences.end();
}

void saveServoSchedule() {
  preferences.begin("servocfg", false);
  preferences.putInt((String("s") + "1onH").c_str(), servoOnHour1);
  preferences.putInt((String("s") + "1onM").c_str(), servoOnMinute1);
  preferences.putInt((String("s") + "2onH").c_str(), servoOnHour2);
  preferences.putInt((String("s") + "2onM").c_str(), servoOnMinute2);
  preferences.putBool((String("s") + "en").c_str(), servoScheduleEnabled);
  preferences.end();
}

void loadRelayState() {
  preferences.begin("relayStates", true);
  for (int i = 0; i < 2; i++) {
    relayState[i] = preferences.getBool(("relay" + String(i)).c_str(), false);
    manualOverride[i] = preferences.getBool(("manual" + String(i)).c_str(), false);
    digitalWrite(relayPins[i], relayState[i] ? LOW : HIGH);
  }
  preferences.end();
}

void setup() {
  Serial.begin(115200);
  sensors.begin();

  if (!rtc.begin()) {
    Serial.println("❌ Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("⚠️ RTC lost power, setting time from NTP");
    
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    struct tm timeinfo;
    while (!getLocalTime(&timeinfo)) {
      Serial.print(".");
      delay(500);
    }

    // Set RTC with current NTP time
    rtc.adjust(DateTime(
      timeinfo.tm_year + 1900,
      timeinfo.tm_mon + 1,
      timeinfo.tm_mday,
      timeinfo.tm_hour,
      timeinfo.tm_min,
      timeinfo.tm_sec
    ));

    Serial.println("\n✅ RTC time set from NTP");
  } else {
    Serial.println("✅ RTC time is valid");
  }

  for (int i = 0; i < 2; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], HIGH);
  }

  servo.attach(servoPin);
  servo.write(0);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.println(WiFi.localIP());

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  Serial.print("Waiting for NTP time sync");
  struct tm timeinfo;
  int retry = 0;
  const int maxRetries = 20;
  while (!getLocalTime(&timeinfo) && retry < maxRetries) {
    Serial.print(".");
    delay(500);
    retry++;
  }
  if (retry < maxRetries) {
    Serial.println("\nTime sync successful");
  } else {
    Serial.println("\nFailed to sync time. Check internet access.");
  }

  //struct tm timeinfo;

  if (getLocalTime(&timeinfo)) {
    Serial.println("NTP time obtained. Setting RTC...");
    rtc.adjust(DateTime(
      timeinfo.tm_year + 1900,
      timeinfo.tm_mon + 1,
      timeinfo.tm_mday,
      timeinfo.tm_hour,
      timeinfo.tm_min,
      timeinfo.tm_sec
    ));
  } else {
    Serial.println("Failed to get NTP time");
  }

  loadRelayState();
  loadSettings();

  server.on("/", HTTP_GET, handleRoot);

  for (int i = 1; i <= 2; i++) {
    server.on(String("/toggle") + i, [i]() {
      int idx = i - 1;
      relayState[idx] = !relayState[idx];
      digitalWrite(relayPins[idx], relayState[idx] ? LOW : HIGH);
      manualOverride[idx] = true;
      saveRelayState(idx);
      server.send(200, "text/plain", "OK");
    });
  }

  server.on("/status", HTTP_GET, []() {
    String json = "{";
    for (int i = 0; i < 2; i++) {
      json += "\"relay" + String(i + 1) + "\":" + (relayState[i] ? "true" : "false") + ",";
      json += "\"enabled" + String(i + 1) + "\":" + (scheduleEnabled[i] ? "true" : "false") + ",";
    }
    json += "\"servoRunning\":" + String(servoActive ? "true" : "false") + ",";
    json += "\"servoEnabled\":" + String(servoScheduleEnabled ? "true" : "false") + ",";
    json += "\"servoOnH1\":" + String(servoOnHour1) + ",";
    json += "\"servoOnM1\":" + String(servoOnMinute1) + ",";
    json += "\"servoOnH2\":" + String(servoOnHour2) + ",";
    json += "\"servoOnM2\":" + String(servoOnMinute2);
    json += ",\"temperature\":" + String(currentTemp, 2);
    json += ",\"rtcTime\":\"" + currentRtcTimeStr + "\"";
    json += ",\"waterLevel\":" + String(waterLevelPercent);
    if (isnan(phValue) || phValue < 0 || phValue > 14) {
      json += ",\"ph\":null";
    } else {
      json += ",\"ph\":" + String(phValue, 2);
    }
    json += "}";

    server.send(200, "application/json", json);
  });

  server.on("/getschedule", HTTP_GET, []() {
    String json = "{";
    for (int i = 0; i < 2; i++) {
      json += "\"relay" + String(i + 1) + "\":{";
      json += "\"onHour\":" + String(schedules[i].onHour) + ",";
      json += "\"onMinute\":" + String(schedules[i].onMinute) + ",";
      json += "\"offHour\":" + String(schedules[i].offHour) + ",";
      json += "\"offMinute\":" + String(schedules[i].offMinute) + ",";
      json += "\"enabled\":" + String(scheduleEnabled[i] ? "true" : "false");
      json += "}";
      if (i < 1) json += ",";
    }
    json += "}";
    server.send(200, "application/json", json);
  });

  server.on("/setschedule", HTTP_POST, []() {
    if (server.hasArg("relay") && server.hasArg("onHour") && server.hasArg("onMinute") &&
        server.hasArg("offHour") && server.hasArg("offMinute")) {
      int r = server.arg("relay").toInt() - 1;
      if (r >= 0 && r < 2) {
        schedules[r].onHour = server.arg("onHour").toInt();
        schedules[r].onMinute = server.arg("onMinute").toInt();
        schedules[r].offHour = server.arg("offHour").toInt();
        schedules[r].offMinute = server.arg("offMinute").toInt();
        if (server.hasArg("enabled")) {
          scheduleEnabled[r] = server.arg("enabled") == "true";
          manualOverride[r] = !scheduleEnabled[r];
          lastScheduleUpdate[r] = millis();
        }
        saveScheduleToPrefs(r);
        saveRelayState(r);
        server.send(200, "text/plain", "Schedule updated");
        return;
      }
    }
    server.send(400, "text/plain", "Invalid parameters");
  });

  server.on("/triggerServo", HTTP_GET, []() {
    if (!servoActive) {
      servoActive = true;
      servoStartTime = millis();
      servoStage = 0;
    }
    server.send(200, "text/plain", "Servo triggered");
  });

  server.on("/setservoschedule", HTTP_POST, []() {
    if (server.hasArg("onHour1") && server.hasArg("onMinute1") &&
        server.hasArg("onHour2") && server.hasArg("onMinute2")) {
      servoOnHour1 = server.arg("onHour1").toInt();
      servoOnMinute1 = server.arg("onMinute1").toInt();
      servoOnHour2 = server.arg("onHour2").toInt();
      servoOnMinute2 = server.arg("onMinute2").toInt();
      servoScheduleEnabled = server.hasArg("enabled") && server.arg("enabled") == "true";
      saveServoSchedule();
      server.send(200, "text/plain", "Servo schedule updated");
    } else {
      server.send(400, "text/plain", "Invalid parameters");
    }
  });


  server.begin();
}

void loop() {
  server.handleClient();

  unsigned long currentmillis = millis();
  static unsigned long lastTempUpdate = 0;
  static unsigned long lastRtcPrint = 0;
  const unsigned long rtcPrintInterval = 1000; // 1 seconds
  if (currentmillis - lastTempUpdate >= 1000) {  // Update every 1 second
    sensors.requestTemperatures();
    currentTemp = sensors.getTempCByIndex(0);
    Serial.println("Temperature: " + String(currentTemp) + " °C");
    lastTempUpdate = currentmillis;
  }
  if (currentmillis - lastRtcPrint >= rtcPrintInterval) {
    lastRtcPrint = currentmillis;

    DateTime now = rtc.now();
    char buf[20];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d %02d/%02d/%04d",
           now.hour(), now.minute(), now.second(),
           now.day(), now.month(), now.year());
    currentRtcTimeStr = String(buf);

    Serial.println("RTC Time: " + currentRtcTimeStr);

  }

  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 1000) {
    lastCheck = millis();

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      Serial.println("Failed to get time");
      return;
    }

    int currentHour = timeinfo.tm_hour;
    int currentMinute = timeinfo.tm_min;

    for (int i = 0; i < 2; i++) {
      if (scheduleEnabled[i] && !manualOverride[i]) {
        if (millis() - lastScheduleUpdate[i] > 10000) {
          bool shouldBeOn = isTimeBetween(currentHour, currentMinute,
                                          schedules[i].onHour, schedules[i].onMinute,
                                          schedules[i].offHour, schedules[i].offMinute);
          if (relayState[i] != shouldBeOn) {
            relayState[i] = shouldBeOn;
            digitalWrite(relayPins[i], shouldBeOn ? LOW : HIGH);
            saveRelayState(i);
            Serial.printf("Relay %d turned %s by schedule\n", i + 1, shouldBeOn ? "ON" : "OFF");
          }
        }
      }
    }

    static int lastCheckedMinute = -1;
    if (servoScheduleEnabled && timeinfo.tm_min != lastCheckedMinute) {
      lastCheckedMinute = timeinfo.tm_min;
      bool match1 = timeinfo.tm_hour == servoOnHour1 && timeinfo.tm_min == servoOnMinute1;
      bool match2 = timeinfo.tm_hour == servoOnHour2 && timeinfo.tm_min == servoOnMinute2;
      if ((match1 || match2) && !servoActive) {
        servoActive = true;
        servoStartTime = millis();
        servoStage = 0;
        Serial.println("Servo triggered by schedule");
      }
    }


  }

  if (servoActive) {
    unsigned long currentmillis = millis();
    if (servoStage == 0 && currentmillis - servoStartTime >= 0) {
      servo.write(90);
      servoStage = 1;
      servoStartTime = currentmillis;
    } else if (servoStage == 1 && currentmillis - servoStartTime >= 1000) {
      servo.write(0);
      servoActive = false;
    }
  }
  updateWaterLevel();
  updatePh();
}

bool isTimeBetween(int curH, int curM, int onH, int onM, int offH, int offM) {
  int curTotal = curH * 60 + curM;
  int onTotal = onH * 60 + onM;
  int offTotal = offH * 60 + offM;
  if (onTotal <= offTotal)
    return curTotal >= onTotal && curTotal < offTotal;
  else
    return curTotal >= onTotal || curTotal < offTotal;
}

void handleRoot() {
  String page = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset="UTF-8">
<meta name='viewport' content='width=device-width, initial-scale=1'>
<style>
body { font-family: Arial; text-align: center; padding: 20px; }
button { padding: 10px 20px; margin: 5px; font-size: 16px; border-radius: 6px; }
.on { background-color: #4CAF50; color: white; }
.off { background-color: #f44336; color: white; }
input[type=time], input[type=checkbox] { margin: 5px; }
.relay-block { border: 1px solid #ccc; padding: 10px; margin: 10px auto; width: 300px; border-radius: 8px; }
</style>
</head><body>
<h2>Relay Scheduler</h2>
<div id="tempCard" style="margin-bottom:20px; padding:15px; background-color:#f2f2f2; border-radius:10px; box-shadow:0 2px 6px rgba(0,0,0,0.1); display:inline-block;">
  <h3 style="margin:0; font-size:18px; color:#333;">Water Temperature</h3>
  <p style="font-size:24px; margin:5px 0; color:#0077cc;"><span id="temperature">--</span> °C</p>
</div>
<div id="rtcCard" style="margin-bottom:20px; padding:15px; background-color:#f2f2f2; border-radius:10px; box-shadow:0 2px 6px rgba(0,0,0,0.1); display:inline-block;">
  <h3 style="margin:0; font-size:18px; color:#333;">RTC Time</h3>
  <p style="font-size:24px; margin:5px 0; color:#0077cc;"><span id="rtcTime">--:--:--</span></p>
</div>
<div id="waterCard" style='margin-bottom:20px; padding:15px; background-color:#f2f2f2; border-radius:10px; box-shadow:0 2px 6px rgba(0,0,0,0.1); display:inline-block;'>
  <h3 style='margin:0; font-size:18px; color:#333;'>Water Level</h3>
  <p style='font-size:24px; margin:5px 0; color:#0077cc;'><span id="waterLevel">--</span>%</p>
</div>
<div id="phCard" style='margin-bottom:20px; padding:15px; background-color:#f2f2f2; border-radius:10px; box-shadow:0 2px 6px rgba(0,0,0,0.1); display:inline-block;'>
  <h3 style='margin:0; font-size:18px; color:#333;'>pH Value</h3>
  <p style='font-size:24px; margin:5px 0; color:#0077cc;'><span id="phValue">--</span></p>
</div>
)rawliteral";

  for (int i = 1; i <= 2; i++) {
    page += "<div class='relay-block'>";
    page += "<h3>Relay " + String(i) + "</h3>";
    page += "<button id='btn" + String(i) + "' onclick='toggleRelay(" + String(i) + ")'>Loading...</button><br>";
    page += "Schedule ON: <input type='time' id='onTime" + String(i) + "'><br>";
    page += "Schedule OFF: <input type='time' id='offTime" + String(i) + "'><br>";
    page += "<label><input type='checkbox' id='enable" + String(i) + "'> Enable Schedule</label><br>";
    page += "<button onclick='saveSchedule(" + String(i) + ")'>Save Schedule</button>";
    page += "</div>";
  }

  // Servo control
  page += R"rawliteral(
  <div class='relay-block'>
    <h3>Feeding Servo</h3>
    <button onclick='triggerServo()'>Feed Now</button>
    <p>Schedule 1: <input type='time' id='servoOn1'><br>
    Schedule 2: <input type='time' id='servoOn2'><br>
    <label><input type='checkbox' id='servoEnable'> Enable Servo Schedule</label><br>
    <button onclick='saveServoSchedule()'>Save Servo Schedule</button></p>
  </div>
<script>
function toggleRelay(n) {
  fetch("/toggle" + n).then(updateStatus);
}

function triggerServo() {
  fetch("/triggerServo").then(r => {
    if (r.ok) alert("Feeding triggered");
    else alert("Failed to trigger");
  });
}

function saveServoSchedule() {
  const time1 = document.getElementById('servoOn1').value.split(":");
  const time2 = document.getElementById('servoOn2').value.split(":");
  const enabled = document.getElementById('servoEnable').checked;

  const params = 
    "onHour1=" + time1[0] + "&onMinute1=" + time1[1] +
    "&onHour2=" + time2[0] + "&onMinute2=" + time2[1] +
    "&enabled=" + (enabled ? "true" : "false");

  console.log("Sending servo schedule params:", params); // helpful debug
  fetch("/setservoschedule", {
    method: "POST",
    headers: { "Content-Type": "application/x-www-form-urlencoded" },
    body: params
  }).then(r => {
    if (r.ok) alert("Servo schedule saved");
    else alert("Failed to save");
  });
}

function updateStatus() {
  fetch("/status").then(r => r.json()).then(data => {
    // Update relay buttons
    for (let i = 1; i <= 2; i++) {
      const btn = document.getElementById("btn" + i);
      btn.innerHTML = data["relay" + i] ? "TURN OFF" : "TURN ON";
      btn.className = data["relay" + i] ? "on" : "off";
    }
    const servoEnableBox = document.getElementById("servoEnable");
    if (document.activeElement !== servoEnableBox) {
      servoEnableBox.checked = data.servoEnabled;
    }
    document.getElementById("temperature").innerText = data.temperature;
    document.getElementById("rtcTime").innerText = data.rtcTime;
    document.getElementById("waterLevel").innerText = data.waterLevel;
    document.getElementById("phValue").innerText = (data.ph === null ? "--" : data.ph.toFixed(2));
  });
}


function loadSchedules() {
  fetch("/getschedule").then(r => r.json()).then(data => {
    for (let i = 1; i <= 2; i++) {
      const s = data["relay" + i];
      document.getElementById("onTime" + i).value = ("0" + s.onHour).slice(-2) + ":" + ("0" + s.onMinute).slice(-2);
      document.getElementById("offTime" + i).value = ("0" + s.offHour).slice(-2) + ":" + ("0" + s.offMinute).slice(-2);
      document.getElementById("enable" + i).checked = s.enabled;
    }
  });
}

function saveSchedule(n) {
  const on = document.getElementById("onTime" + n).value.split(":");
  const off = document.getElementById("offTime" + n).value.split(":");
  const enabled = document.getElementById("enable" + n).checked;

  const params = new URLSearchParams({
    relay: n,
    onHour: on[0],
    onMinute: on[1],
    offHour: off[0],
    offMinute: off[1],
    enabled: enabled
  });

  fetch("/setschedule", {
    method: "POST",
    body: params
  }).then(r => {
    if (r.ok) {
      alert("Schedule saved");
      loadSchedules();
    } else alert("Failed to save");
  });
}

setInterval(updateStatus, 2000);
window.onload = () => { updateStatus(); loadSchedules(); };
</script>
</body></html>
)rawliteral";

  server.send(200, "text/html", page);
}
