#include "web_portal.h"
#include "net_weather.h"
#include "display.h"
#include "ui_wx_icon.h"
#include <U8g2lib.h>
#include <Update.h>
#include <Preferences.h>
#include "lunar.h"
#include "net_config.h"

// 运行统计（main.cpp 维护）
extern uint32_t g_bootCount;
extern uint32_t g_wifiReconnects;
extern uint32_t g_minHeap;
#include "config.h"

#include <WebServer.h>
#include <WiFi.h>
#include <time.h>

static WebServer s_web(80);
static uint32_t s_previewUntil = 0;   // /debug/icon 放大预览的截止时间

static const char* kJson =
    "{\"fw\":\"" FW_VERSION "\",\"author\":\"" FW_AUTHOR "\",\"ip\":\"%s\",\"rssi\":%d,\"heap\":%u,"
    "\"time\":%ld,\"timeStr\":\"%02d:%02d:%02d\",\"lunar\":\"%s\",\"uptime\":%lu,\"boots\":%u,\"reconnects\":%u,\"minHeap\":%u,"
    "\"wx\":{\"valid\":%d,\"http\":%d,\"jsonErr\":%d,\"wmo\":%d,"
    "\"temp\":%d,\"tempTenth\":%d,\"hi\":%d,\"lo\":%d,\"rain\":%d,\"rainToday\":%d,\"humidity\":%d,\"windDir\":%d,\"windLevel\":%d,"
    "\"rawT\":%.1f,\"rawHi\":%.1f,\"rawLo\":%.1f,\"rawWs\":%.1f,\"rawWd\":%.1f,\"rawRp\":%.1f,"
    "\"bodyLen\":%d,\"dirCalc\":%d,\"deg\":%d,\"calc2\":%d,"
    "\"updated\":%ld}}";

static void handleStatus() {
  const WeatherData& w = g_weather;
  struct tm tmv = {};
  time_t now = time(nullptr);
  localtime_r(&now, &tmv);

  char lunarBuf[24] = "";
  LunarDate ld;
  if (lunarFromSolar(tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday, &ld)) {
    lunarToCn(ld, lunarBuf, sizeof(lunarBuf));
  }

  char buf[700];
  snprintf(buf, sizeof(buf), kJson,
           WiFi.localIP().toString().c_str(), (int)WiFi.RSSI(), (unsigned)ESP.getFreeHeap(),
           (long)now, tmv.tm_hour, tmv.tm_min, tmv.tm_sec, lunarBuf,
           (unsigned long)(millis() / 1000UL), g_bootCount, g_wifiReconnects, g_minHeap,
           (int)w.valid, w.dbgHttp, w.dbgJsonErr, w.dbgWmo,
           (int)w.temp, (int)w.temp10, (int)w.hi, (int)w.lo, (int)w.rainPct, (int)w.rainToday, (int)w.humidity, (int)w.windDir, (int)w.windLevel,
           w.dbgRawT, w.dbgRawHi, w.dbgRawLo, w.dbgRawWs, w.dbgRawWd, w.dbgRawRp,
           w.dbgBodyLen,
           (int)(((((int)(w.dbgRawWd + 0.5) % 360) + 22) / 45) % 8),
           w.dbgDeg, w.dbgCalc2,
           (long)w.updated);
  s_web.send(200, "application/json; charset=utf-8", buf);
}

static void handleRoot() {
  const WeatherData& w = g_weather;
  char buf[900];
  snprintf(buf, sizeof(buf),
           "<!doctype html><meta charset=utf-8><meta name=viewport content='width=device-width'>"
           "<title>ESP32-CAM Clock</title>"
           "<style>body{font-family:system-ui;margin:24px;line-height:1.6}"
           "td{padding:2px 10px}</style>"
           "<h2>ESP32-CAM Clock</h2>"
           "<table>"
           "<tr><td>固件</td><td>%s</td></tr>"
           "<tr><td>IP</td><td>%s</td></tr>"
           "<tr><td>信号</td><td>%d dBm</td></tr>"
           "<tr><td>内存</td><td>%u B</td></tr>"
           "<tr><td>天气有效</td><td>%s</td></tr>"
           "<tr><td>温度 / 今日</td><td>%d ℃ / %d ~ %d ℃</td></tr>"
           "<tr><td>降雨概率</td><td>%d %%</td></tr>"
           "<tr><td>风</td><td>dir=%d level=%d</td></tr>"
           "<tr><td>HTTP / JSON</td><td>%d / %d</td></tr>"
           "</table>"
           "<p><a href='/status'>/status</a> · <a href='/debug/wx'>重新取天气</a></p>",
           FW_VERSION, WiFi.localIP().toString().c_str(), (int)WiFi.RSSI(), (unsigned)ESP.getFreeHeap(),
           w.valid ? "是" : "否", (int)w.temp, (int)w.hi, (int)w.lo, (int)w.rainPct,
           (int)w.windDir, (int)w.windLevel, w.dbgHttp, w.dbgJsonErr);
  s_web.send(200, "text/html; charset=utf-8", buf);
}

static void handleWxRefresh() {
  weatherBegin();          // 下一轮 loop 立刻重试
  s_web.send(200, "text/plain; charset=utf-8", "queued");
}

// /debug/icon?n=0..11 —— 把某个图标放大画满屏，15 秒后自动恢复
static void handleIconDebug() {
  int n = s_web.arg("n").toInt();
  n = constrain(n, 0, 11);
  static const char* kName[12] = {"sun", "sun+cloud", "cloud+sun", "cloud2", "fog", "drizzle",
                                  "rain", "heavy rain", "snow", "shower", "thunder", "freezing"};
  U8G2& d = displayRef();
  if (displayReady()) {
    d.clearBuffer();
    // 屏的上 16 行是黄色区，图标必须整个画在蓝区（16 行以下），否则会被黄蓝切成两截
    d.setFont(u8g2_font_6x12_tf);
    d.drawStr(2, 12, kName[n]);
    char buf[12];
    snprintf(buf, sizeof(buf), "%d/11", n);
    d.drawStr(126 - d.getStrWidth(buf), 12, buf);
    uiDrawWeatherIcon(d, (uint8_t)n, 40, 16, 48);
    d.sendBuffer();
  }
  s_previewUntil = millis() + 15000;
  s_web.send(200, "text/plain; charset=utf-8", "preview 15s");
}

bool webIconPreviewActive() {
  return s_previewUntil != 0 && millis() < s_previewUntil;
}
// ---------------- Web OTA：浏览器上传 .bin ----------------
static const char* kUpdatePage =
    "<!doctype html><meta charset=utf-8><meta name=viewport content='width=device-width'>"
    "<title>OTA 升级</title><style>body{font-family:system-ui;margin:24px;max-width:520px}"
    "input,button{margin-top:14px;font-size:16px;width:100%;padding:10px;box-sizing:border-box}"
    "</style><h2>固件升级</h2>"
    "<form method=POST action=/update enctype=multipart/form-data>"
    "<input type=file name=firmware accept=.bin required>"
    "<button type=submit>上传并重启</button></form>"
    "<p><a href='/'>返回状态页</a> · 当前版本 " FW_VERSION "</p>";

static void handleUpdatePage() { s_web.send(200, "text/html; charset=utf-8", kUpdatePage); }

static void handleUpdateDone() {
  bool ok = !Update.hasError();
  s_web.send(200, "text/html; charset=utf-8",
             ok ? "<meta charset=utf-8><h3>升级成功，正在重启…</h3>"
                : "<meta charset=utf-8><h3>升级失败</h3>");
  delay(400);
  if (ok) ESP.restart();
}

static void handleUpdateUpload() {
  HTTPUpload& up = s_web.upload();
  if (up.status == UPLOAD_FILE_START) {
    Serial.printf("[ota-web] 开始接收 %s\n", up.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) Update.printError(Serial);
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (Update.write(up.buf, up.currentSize) != up.currentSize) Update.printError(Serial);
  } else if (up.status == UPLOAD_FILE_END) {
    if (Update.end(true)) Serial.printf("[ota-web] 完成 %u 字节\n", up.totalSize);
    else Update.printError(Serial);
  }
}
void webBegin() {
  s_web.on("/", handleRoot);
  s_web.on("/status", handleStatus);
  s_web.on("/debug/wx", handleWxRefresh);
  s_web.on("/debug/icon", handleIconDebug);
  s_web.on("/update", HTTP_GET, handleUpdatePage);
  s_web.on("/update", HTTP_POST, handleUpdateDone, handleUpdateUpload);
  s_web.on("/debug/portal", []() {
    s_web.send(200, "text/plain; charset=utf-8", "portal on: GET / (15s 内可看配网页), /reboot 退出");
    delay(400);
    s_web.stop();
    cfgPortalBegin();
  });
  s_web.onNotFound([]() { s_web.send(404, "text/plain", "not found"); });
  s_web.begin();
  Serial.printf("[web] http://%s/\n", WiFi.localIP().toString().c_str());
}

void webTick() {
  s_web.handleClient();
}
