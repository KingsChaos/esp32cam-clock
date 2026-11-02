#include "net_weather.h"
#include "config.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

WeatherData g_weather = {};

static uint32_t s_lastTry  = 0;
static uint32_t s_interval = 5;      // 秒；首次失败后按 60/300/1800 退避
static uint8_t  s_fails    = 0;
static bool     s_started  = false;

static const char* kUrl =
    "https://api.open-meteo.com/v1/forecast"
    "?latitude=" WX_LAT "&longitude=" WX_LON
    "&current=temperature_2m,relative_humidity_2m,weather_code,wind_speed_10m,wind_direction_10m"
    "&daily=temperature_2m_max,temperature_2m_min,precipitation_probability_max"
    "&hourly=precipitation_probability&forecast_hours=1"
    "&wind_speed_unit=ms&timezone=" WX_TZ "&forecast_days=1";

// WMO 天气码 -> 图标
static uint8_t wmoToIcon(int c) {
  if (c == 0) return WX_SUN;
  if (c == 1) return WX_SUN_CLOUD;
  if (c == 2) return WX_CLOUD_SUN;
  if (c == 3) return WX_CLOUD2;
  if (c == 45 || c == 48) return WX_FOG;
  if (c >= 51 && c <= 55) return WX_DRIZZLE;
  if (c == 56 || c == 57) return WX_FREEZING;
  if (c >= 61 && c <= 65) return (c >= 65) ? WX_HEAVY_RAIN : WX_RAIN;
  if (c == 66 || c == 67) return WX_FREEZING;
  if (c >= 71 && c <= 77) return WX_SNOW;
  if (c >= 80 && c <= 82) return WX_SHOWER;
  if (c == 85 || c == 86) return WX_SNOW;
  if (c >= 95) return WX_THUNDER;
  return WX_CLOUD2;
}

// m/s -> 蒲福风级
static uint8_t beaufort(float ms) {
  static const float lim[] = {0.3f, 1.6f, 3.4f, 5.5f, 8.0f, 10.8f, 13.9f, 17.2f, 20.8f, 24.5f, 28.5f, 32.7f};
  for (uint8_t i = 0; i < 12; i++) {
    if (ms < lim[i]) return i;
  }
  return 12;
}

const char* windDirLetter(uint8_t dir) {
  static const char* kDir[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
  return kDir[dir & 7];
}

static bool fetch() {
  WiFiClientSecure client;
  client.setInsecure();          // v1：不校验证书（后续换成内置根证书）
  client.setTimeout(8);

  HTTPClient http;
  http.setTimeout(8000);
  http.setConnectTimeout(8000);
  if (!http.begin(client, kUrl)) {
    Serial.println("[wx] begin 失败");
    return false;
  }

  int code = http.GET();
  g_weather.dbgHttp = code;
  if (code != HTTP_CODE_OK) {
    Serial.printf("[wx] HTTP %d\n", code);
    http.end();
    return false;
  }

  String body = http.getString();
  http.end();

  g_weather.dbgBodyLen = body.length();
  memset(g_weather.dbgHead, 0, sizeof(g_weather.dbgHead));
  strncpy(g_weather.dbgHead, body.c_str(), sizeof(g_weather.dbgHead) - 1);

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);

  g_weather.dbgJsonErr = (int)err.code();
  if (err) {
    Serial.printf("[wx] JSON 解析失败: %s\n", err.c_str());
    return false;
  }

  float t   = doc["current"]["temperature_2m"]     | 0.0f;
  int   wmo = doc["current"]["weather_code"]       | 3;
  float ws  = doc["current"]["wind_speed_10m"]     | 0.0f;
  int   wd  = doc["current"]["wind_direction_10m"] | 0;
  float hi  = doc["daily"]["temperature_2m_max"][0]  | 0.0f;
  float lo  = doc["daily"]["temperature_2m_min"][0]  | 0.0f;
  // 降雨概率取"当前这一小时"，不是全天最大值（全天最大值常是 100%，会和晴天图标矛盾）
  int   rp  = doc["hourly"]["precipitation_probability"][0] | 0;
  int   rh  = doc["current"]["relative_humidity_2m"]         | 0;
  int   rpd = doc["daily"]["precipitation_probability_max"][0] | 0;

  g_weather.dbgWmo   = wmo;
  g_weather.dbgRawT  = t;  g_weather.dbgRawHi = hi; g_weather.dbgRawLo = lo;
  g_weather.dbgRawWs = ws; g_weather.dbgRawWd = wd; g_weather.dbgRawRp = rp;

  g_weather.temp      = (int8_t)lroundf(t);
  g_weather.temp10    = (int16_t)lroundf(t * 10.0f);
  g_weather.hi        = (int8_t)lroundf(hi);
  g_weather.lo        = (int8_t)lroundf(lo);
  g_weather.icon      = wmoToIcon(wmo);
  g_weather.rainPct   = (uint8_t)constrain(rp, 0, 100);
  g_weather.rainToday = (uint8_t)constrain(rpd, 0, 100);
  g_weather.humidity  = (uint8_t)constrain(rh, 0, 100);
  // 注意：deg 不能用 uint8_t——353 会被截成 97
  int     deg         = ((wd % 360) + 360) % 360;
  uint8_t d2          = (uint8_t)(((deg + 22) / 45) % 8);
  g_weather.windDir   = d2;
  g_weather.windLevel = beaufort(ws);
  g_weather.dbgDeg    = (int)deg;
  g_weather.dbgCalc2  = (int)d2;
  g_weather.updated   = time(nullptr);
  g_weather.valid     = true;

  Serial.printf("[wx] %dC %d-%d %d%% %s%d icon=%d\n",
                g_weather.temp, g_weather.hi, g_weather.lo, g_weather.rainPct,
                windDirLetter(g_weather.windDir), g_weather.windLevel, g_weather.icon);
  return true;
}

bool weatherStale() {
  if (!g_weather.valid) return true;
  time_t now = time(nullptr);
  if (now < 1600000000L) return false;      // 时间还没同步，先不判
  return (now - g_weather.updated) > 2L * 3600L;
}

void weatherBegin() {
  s_started  = true;
  s_lastTry  = 0;
  s_interval = 5;      // 5 秒后第一次尝试（等 WiFi 稳一点）
  s_fails    = 0;
}

void weatherTick() {
  if (!s_started) return;
  if (WiFi.status() != WL_CONNECTED) return;
  if (millis() - s_lastTry < s_interval * 1000UL) return;

  s_lastTry = millis();
  if (fetch()) {
    s_fails    = 0;
    s_interval = (uint32_t)WX_REFRESH_MIN * 60UL;
  } else {
    if (s_fails < 3) s_fails++;
    s_interval = (s_fails <= 1) ? 60 : ((s_fails == 2) ? 300 : 1800);
  }
}
