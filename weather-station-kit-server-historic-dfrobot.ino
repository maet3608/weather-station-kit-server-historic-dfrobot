// board:
// - Nologo ESP32C3 Super Mini
// tools:
// - USB CDC On Boot : Enabled
// - Partition Scheme : Default 4MB with spiffs
// esp32 core:
// - version: 4.0.0-alpha1


#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "credentials.h" 
#include "weather_server.h"    

WeatherData wxData;        // extern-declared in weather_server.h
WebServer   server(80);    // extern-declared in weather_server.h

char databuffer[35];

// ── UART ──────────────────────────────────────────────────────────────────────

bool getBuffer() {
  int index = 0;
  unsigned long start = millis();
  while (index < 35) {
    if (millis() - start > 5000) {
      Serial.println("[ERROR] Sensor timeout - check GPIO20/21 wiring");
      return false;
    }
    if (Serial0.available()) {
      databuffer[index] = Serial0.read();
      if (databuffer[0] != 'c') index = -1;
      index++;
    } else {
      serverHandle();          // keep HTTP responsive while waiting for sensor bytes
    }
  }
  return true;
}

// ── PARSING ───────────────────────────────────────────────────────────────────

int parseInt(int start, int stop) {
  int r = 0;
  for (int i = start; i <= stop; i++) r = r * 10 + (databuffer[i] - '0');
  return r;
}

int parseTempF() {
  return (databuffer[13] == '-')
    ? -(((databuffer[14]-'0')*10) + (databuffer[15]-'0'))
    :   ((databuffer[13]-'0')*100) + ((databuffer[14]-'0')*10) + (databuffer[15]-'0');
}

// ── MEASUREMENTS ──────────────────────────────────────────────────────────────

int   WindDirection()    { return parseInt(1, 3); }
float WindSpeedAverage() { return 0.44704f * parseInt(5, 7); }
float WindSpeedMax()     { return 0.44704f * parseInt(9, 11); }
float Temperature()      { return (parseTempF() - 32.0f) * 5.0f / 9.0f; }
float RainfallOneHour()  { return parseInt(17, 19) * 0.2540f; }
float RainfallOneDay()   { return parseInt(21, 23) * 0.2540f; }
int   Humidity()         { return parseInt(25, 26); }
float BarPressure()      { return parseInt(28, 32) / 10.0f; }

// ── WIFI ──────────────────────────────────────────────────────────────────────

void wifiConnect() {
  Serial.printf("[WiFi] Connecting to %s", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.printf("\n[WiFi] Connected: http://%s\n", WiFi.localIP().toString().c_str());
  if (MDNS.begin(HOSTNAME))
    Serial.printf("[mDNS] http://%s.local\n", HOSTNAME);
}

// ── SETUP / LOOP ──────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  unsigned long t = millis();
  while (!Serial && millis() - t < 3000);
  Serial.println("[INFO] ESP32-C3 weather station starting...");
  Serial0.begin(9600, SERIAL_8N1, 20, 21);
  wifiConnect();
  serverSetup();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) wifiConnect();

  if (!getBuffer()) { delay(2000); return; }

  wxData = {
    WindDirection(), WindSpeedAverage(), WindSpeedMax(),
    Temperature(), Humidity(), BarPressure(),
    RainfallOneHour(), RainfallOneDay(), true
  };

  recordHistory(wxData);     // stores a point every 30 min

  // Serial.printf("Wind Dir: %d°  Avg: %.2f m/s  Max: %.2f m/s\n",
  //               wxData.windDir, wxData.windAvg, wxData.windMax);
  // Serial.printf("Temp: %.1f°C  Hum: %d%%  Pressure: %.1f hPa\n",
  //               wxData.tempC, wxData.humidity, wxData.pressure);
  // Serial.printf("Rain 1h: %.2f mm  Rain 24h: %.2f mm\n\n",
  //               wxData.rain1h, wxData.rain24h);
}