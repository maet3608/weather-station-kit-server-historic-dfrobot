#pragma once
#include <Arduino.h>
#include "weather_data.h"

#define HISTORY_SIZE    240                     // 30-min samples × 5 days
#define RECORD_INTERVAL (30UL * 60UL * 1000UL)   // 30 min in ms


struct HistoryRecord {
  uint32_t ts;                // seconds since boot
  float    tempC;
  uint8_t  humidity;
  float    pressure;
  uint16_t windDir;
  float    windAvg, windMax;
  float    rain1h,  rain24h;
};

HistoryRecord histBuf[HISTORY_SIZE];
int           histHead  = 0;
int           histCount = 0;
unsigned long lastRecMs = 0;

void recordHistory(const WeatherData& d) {
  if (!d.valid) return;
  unsigned long now = millis();
  if (histCount > 0 && now - lastRecMs < RECORD_INTERVAL) return;
  histBuf[histHead] = {
    (uint32_t)(now / 1000),
    d.tempC, (uint8_t)d.humidity, d.pressure,
    (uint16_t)d.windDir, d.windAvg, d.windMax,
    d.rain1h, d.rain24h
  };
  histHead  = (histHead + 1) % HISTORY_SIZE;
  if (histCount < HISTORY_SIZE) histCount++;
  lastRecMs = now;
}