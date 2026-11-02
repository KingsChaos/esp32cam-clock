// ============================================================
//  ESP32-CAM + SSD1315 OLED 桌面时钟
//  M0：显示自检 + WiFi + ArduinoOTA 通路
//  详细进度见 docs/05-开发计划.md
// ============================================================
#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <string.h>

#include "config.h"
#include "display.h"
#include "net_time.h"
#include "net_weather.h"
#include "web_portal.h"
#include "net_config.h"
#include <Preferences.h>
#include "ui_clock.h"

// ---------------- 运行状态 ----------------
static bool     s_wifiOk    = false;
static bool     s_otaActive = false;
static int      s_otaPct    = 0;
static char     s_ip[16]    = "";
static uint32_t s_bootMs    = 0;
static uint32_t s_lastDrawS = 0xFFFFFFFF;
static uint32_t s_lastWifiMs = 0;
static uint32_t s_lastLogMs  = 0;
static bool     s_otaStarted = false;
static bool     s_forceRedraw = false;

uint32_t g_bootCount      = 0;
uint32_t g_wifiReconnects = 0;
uint32_t g_minHeap        = 0xFFFFFFFFu;
static Preferences s_stat;

// ---------------- 板载红灯 ----------------
static void ledSet(bool on) {
  digitalWrite(PIN_LED_RED, on ? LOW : HIGH);   // 低电平点亮
}

// 用闪灯次数表示启动阶段（没有串口也能看出卡在哪）
// 1=屏幕就绪 2=自检完成 3=WiFi 完成 4=OTA 就绪
static void blinkCode(int n) {
  for (int i = 0; i < n; i++) {
    ledSet(true);  delay(120);
    ledSet(false); delay(180);
  }
  delay(300);
}

// ---------------- WiFi ----------------
static void wifiStart(const char* ssid, const char* pass) {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(OTA_HOSTNAME);
  WiFi.begin(ssid, pass);
  Serial.printf("[wifi] 连接 \"%s\" ...\n", ssid);

  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - t0) < WIFI_CONNECT_TIMEOUT_MS) {
    delay(200);
    ledSet(((millis() / 200) & 1) != 0);   // 连接中快闪
  }

  s_wifiOk = (WiFi.status() == WL_CONNECTED);
  if (s_wifiOk) {
    snprintf(s_ip, sizeof(s_ip), "%s", WiFi.localIP().toString().c_str());
    Serial.printf("[wifi] 已连接 %s  rssi=%d dBm\n", s_ip, (int)WiFi.RSSI());
    ledSet(false);
  } else {
    Serial.println("[wifi] 连接失败（15s 超时），继续离线运行");
    ledSet(false);
  }
  WiFi.setAutoReconnect(true);
}

// ---------------- OTA ----------------
static void otaSetup() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  if (sizeof(OTA_PASSWORD) > 1) ArduinoOTA.setPassword(OTA_PASSWORD);

  ArduinoOTA.onStart([]() {
    s_otaActive = true;
    s_otaPct    = 0;
    Serial.println("[ota] 开始");
  });
  ArduinoOTA.onProgress([](unsigned int done, unsigned int total) {
    s_otaPct = total ? (int)((done * 100ULL) / total) : 0;
  });
  ArduinoOTA.onEnd([]() {
    s_otaPct = 100;
    displayOtaScreen(100);
    Serial.println("[ota] 完成，重启");
    delay(300);
    ESP.restart();
  });
  ArduinoOTA.onError([](ota_error_t err) {
    s_otaActive = false;
    Serial.printf("[ota] 错误 %u\n", (unsigned)err);
  });

  ArduinoOTA.begin();
  Serial.printf("[ota] 就绪：主机名 %s.local（%s）\n",
                OTA_HOSTNAME, s_wifiOk ? s_ip : "当前离线");
}

// ---------------- setup ----------------
void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(200);

  pinMode(PIN_LED_RED, OUTPUT);
  ledSet(false);

  Serial.println();
  Serial.println("=====================================");
  Serial.printf("  %s  %s   by %s\n", FW_NAME, FW_VERSION, FW_AUTHOR);
  Serial.println("=====================================");
  Serial.printf("chip   : %s rev%d, %d cores @ %u MHz\n",
                ESP.getChipModel(), ESP.getChipRevision(), ESP.getChipCores(),
                (unsigned)getCpuFrequencyMhz());
  Serial.printf("flash  : %u KB\n", (unsigned)(ESP.getFlashChipSize() / 1024));
  Serial.printf("heap   : %u bytes\n", (unsigned)ESP.getFreeHeap());

  // 0) 开机统计（NVS）
  s_stat.begin("stat", false);
  g_bootCount = s_stat.getUInt("boots", 0) + 1;
  s_stat.putUInt("boots", g_bootCount);
  s_stat.end();

  // 1) 屏幕
  if (displayBegin()) {
    Serial.printf("[oled] 初始化成功，地址 0x%02X\n", displayAddress());
    Serial.flush();
    blinkCode(1);
    displaySplash();
    Serial.println("[oled] 自检完成");
    Serial.flush();
    blinkCode(2);
  } else {
    Serial.println("[oled] 未找到屏幕！检查 SDA=15 / SCL=14 / 3V3 / GND");
    Serial.println("[oled] 固件仍会继续运行（LED 闪 + OTA 可用）");
  }

  // 先画一次信息屏：确认"绘制→显示"这条通路是通的，再去做网络
  displayMainScreen(0, "", false);
  s_bootMs = millis();

  // 2) WiFi：优先 NVS 保存的凭证，其次 config.h 默认值；都没有就开热点配网
  char ssid[33] = {0};
  char pass[65] = {0};
  bool haveCfg = cfgLoad(ssid, sizeof(ssid), pass, sizeof(pass));
  if (!haveCfg && strlen(WIFI_SSID) > 0) {
    snprintf(ssid, sizeof(ssid), "%s", WIFI_SSID);
    snprintf(pass, sizeof(pass), "%s", WIFI_PASS);
    haveCfg = true;
  }

  if (haveCfg) {
    Serial.println("[boot] wifi start");
    Serial.flush();
    wifiStart(ssid, pass);
  } else {
    Serial.println("[boot] 没有 WiFi 凭证 → 配网模式");
  }
  blinkCode(3);

  if (!s_wifiOk) {
    cfgPortalBegin();
    displayPortalScreen();
    Serial.println("[boot] 配网模式：主循环只跑配网页");
  }

  // 2.5) 时间：SNTP + CST-8（没网会一直重试，先显示 --:--:--）
  timeBegin();

  // 2.6) 天气（首次 5 秒后拉，之后 30min；失败退避）
  weatherBegin();

  // 2.7) Web 状态页（/ 和 /status，调试用）
  if (s_wifiOk) webBegin();

  // 3) OTA —— 只有连上 WiFi 才启动，避免无网络时 MDNS/UDP 初始化把板子搞挂
  if (s_wifiOk) {
    otaSetup();
    s_otaStarted = true;
    blinkCode(4);
  } else {
    Serial.println("[ota] 跳过（无 WiFi）：配好 config.h 后重烧即可");
  }

  Serial.println("[boot] 进入主循环");
  Serial.flush();
}

// ---------------- loop ----------------
void loop() {
  // 没有 WiFi 时不跑 OTA handle（这是之前黑屏的可疑原因）
  if (s_otaStarted) ArduinoOTA.handle();

  // 每 2s 复查 WiFi 状态
  if (millis() - s_lastWifiMs > 2000) {
    s_lastWifiMs = millis();
    bool ok = (WiFi.status() == WL_CONNECTED);
    if (ok != s_wifiOk) {
      s_wifiOk = ok;
      if (ok) {
        g_wifiReconnects++;
        snprintf(s_ip, sizeof(s_ip), "%s", WiFi.localIP().toString().c_str());
        Serial.printf("[wifi] 恢复 %s\n", s_ip);
      } else {
        s_ip[0] = '\0';
        Serial.println("[wifi] 断开");
      }
      s_lastDrawS = 0xFFFFFFFF;   // 强制重画
      // WiFi 恢复后补启 OTA / Web，并立刻补拉天气和时间
      if (s_wifiOk) {
        if (!s_otaStarted) { otaSetup(); s_otaStarted = true; }
        webBegin();
        weatherBegin();
        timeBegin();
      }
    }
  }

  // OTA 进行中：只画进度条
  if (s_otaActive) {
    displayOtaScreen(s_otaPct);
    delay(50);
    return;
  }

  // 配网模式：只跑配网页
  if (cfgPortalActive()) {
    cfgPortalTick();
    delay(5);
    return;
  }

  // 运行统计：最小空闲堆
  uint32_t freeHeap = ESP.getFreeHeap();
  if (freeHeap < g_minHeap) g_minHeap = freeHeap;

  // 软看门狗：堆太低持续 5 分钟，或 WiFi 断超 30 分钟 → 重启自愈
  static uint32_t lowHeapSince = 0;
  static uint32_t wifiDownSince = 0;
  if (freeHeap < 20000) {
    if (!lowHeapSince) lowHeapSince = millis();
    else if (millis() - lowHeapSince > 300000UL) { Serial.println("[wdt] 堆过低，重启"); ESP.restart(); }
  } else lowHeapSince = 0;

  if (!s_wifiOk) {
    if (!wifiDownSince) wifiDownSince = millis();
    else if (millis() - wifiDownSince > 1800000UL) { Serial.println("[wdt] WiFi 长时间断开，重启"); ESP.restart(); }
  } else wifiDownSince = 0;

  // ---- M1：真实时间 ----
  timeTick();
  weatherTick();
  webTick();

  // 图标放大预览中：不刷正常界面，等它超时（webTick 必须在这之前，否则页面会卡）
  static bool previewWas = false;
  bool previewNow = webIconPreviewActive();
  if (previewNow) { previewWas = true; delay(30); return; }
  if (previewWas) { previewWas = false; s_forceRedraw = true; }

  // 天气刚更新 → 整屏重画
  static time_t lastWxUpd = 0;
  if (g_weather.updated != lastWxUpd) { lastWxUpd = g_weather.updated; s_forceRedraw = true; }

  struct tm now;
  bool haveTime = timeNow(&now);
  bool timeOk   = haveTime && timeSynced();

  static int  lastSec   = -1;
  static int  lastMin   = -1;
  static bool lastWifi  = false;
  static bool firstDraw = true;

  int sec = haveTime ? now.tm_sec : -1;
  int min = haveTime ? now.tm_min : -1;

  if (firstDraw || min != lastMin || s_wifiOk != lastWifi || s_forceRedraw) {
    uiDrawAll(haveTime ? &now : nullptr, timeOk, s_wifiOk);
    lastSec   = sec;
    lastMin   = min;
    lastWifi  = s_wifiOk;
    firstDraw = false;
    s_forceRedraw = false;
  } else if (sec != lastSec) {
    uiDrawTime(haveTime ? &now : nullptr, timeOk);
    lastSec = sec;
    if (sec >= 0) ledSet((sec & 1) != 0);  // 秒闪
  }

  uint32_t uptimeSec = (millis() - s_bootMs) / 1000;

  // 每 10s 心跳日志
  if (millis() - s_lastLogMs > 10000) {
    s_lastLogMs = millis();
    Serial.printf("[hb] up=%lus heap=%u wifi=%d ip=%s\n",
                  (unsigned long)uptimeSec, (unsigned)ESP.getFreeHeap(),
                  (int)s_wifiOk, s_ip);
  }

  delay(20);
}
