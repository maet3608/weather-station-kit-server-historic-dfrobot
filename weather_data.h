#pragma once

struct WeatherData {
  int   windDir;
  float windAvg, windMax;
  float tempC;
  int   humidity;
  float pressure;
  float rain1h, rain24h;
  bool  valid = false;
};