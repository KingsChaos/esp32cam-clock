#include "display.h"
#include "config.h"

#include <Wire.h>
#include <U8g2lib.h>
#include <stdio.h>
#include <string.h>

// ------------------------------------------------------------
// U8g2 软件 I2C：clock = SCL, data = SDA（顺序别搞反）
// ------------------------------------------------------------
static U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(
    U8G2_R0,
    /* clock= */ PIN_OLED_SCL,
    /* data  = */ PIN_OLED_SDA,
    /* reset = */ U8X8_PIN_NONE);

static uint8_t s_addr  = 0;
static bool    s_ready = false;

// ------------------------------------------------------------
// 探测 I2C 地址：先试常见地址，再全量扫描（排错时串口会打印）
// ------------------------------------------------------------
static uint8_t i2cProbe(uint8_t addr) {
  Wire.beginTransmission(addr);
  return (Wire.endTransmission() == 0) ? addr : 0;
}

static uint8_t i2cScan() {
  uint8_t a = i2cProbe(0x3C);
  if (a) return a;
  a = i2cProbe(0x3D);
  if (a) return a;

  Serial.println("[i2c] 0x3C/0x3D 没响应，全量扫描：");
  for (uint8_t addr = 0x08; addr < 0x78; addr++) {
    if (i2cProbe(addr)) {
      Serial.printf("[i2c]   found 0x%02X\n", addr);
      return addr;
    }
  }
  return 0;
}

bool displayBegin() {
  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
  Wire.setClock(OLED_I2C_HZ);
  delay(50);                        // 等屏上电稳定

  s_addr = i2cScan();
  if (!s_addr) {
    s_ready = false;
    return false;
  }

  // 扫描用硬件 I2C，接下来交给 U8g2 的软件 I2C，先把 Wire 释放掉
  Wire.end();

  u8g2.setI2CAddress(s_addr << 1);  // U8g2 要 8 位地址
  u8g2.begin();
  u8g2.setContrast(180);
  u8g2.clearBuffer();
  u8g2.sendBuffer();
  s_ready = true;
  return true;
}

bool displayReady()   { return s_ready; }
U8G2& displayRef()    { return u8g2; }
uint8_t displayAddress() { return s_addr; }

void displayClear() {
  if (!s_ready) return;
  u8g2.clearBuffer();
  u8g2.sendBuffer();
}

// ------------------------------------------------------------
// 自检图
// 1) 全屏点亮：用来确认面板颜色分区（顶部 16 行发黄 = 黄蓝双色）
// 2) 棋盘格：看有没有坏点/整行不亮
// 3) 边框 + y=15 分界线 + 文字：确认像素坐标没有偏移
// ------------------------------------------------------------
void displaySplash() {
  if (!s_ready) return;

  u8g2.clearBuffer();
  u8g2.drawBox(0, 0, 128, 64);
  u8g2.sendBuffer();
  delay(800);

  u8g2.clearBuffer();
  for (uint8_t y = 0; y < 64; y += 8)
    for (uint8_t x = 0; x < 128; x += 8)
      if (((x / 8) + (y / 8)) & 1) u8g2.drawBox(x, y, 8, 8);
  u8g2.sendBuffer();
  delay(400);

  u8g2.clearBuffer();
  u8g2.drawFrame(0, 0, 128, 64);
  u8g2.drawHLine(0, 15, 128);          // 黄蓝分界线的位置参考
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(4, 12, "TOP 16 ROWS");
  u8g2.drawStr(4, 30, "should be YELLOW");
  u8g2.drawStr(4, 44, "if dual-color panel");
  u8g2.drawStr(4, 58, FW_VERSION);
  u8g2.sendBuffer();
  delay(1400);
}

// ------------------------------------------------------------
// M0 信息屏：标题 / 运行时长 / I2C+WiFi / IP
// ------------------------------------------------------------
void displayMainScreen(uint32_t uptimeSec, const char* ip, bool wifiOk) {
  if (!s_ready) return;

  char buf[24];

  u8g2.clearBuffer();

  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 9, "ESP32-CAM CLOCK");
  u8g2.drawStr(116, 9, "M0");

  // 运行时长（M0 没有 NTP，用开机秒数演示刷新）
  snprintf(buf, sizeof(buf), "%02u:%02u:%02u",
           (unsigned)((uptimeSec / 3600) % 100),
           (unsigned)((uptimeSec / 60) % 60),
           (unsigned)(uptimeSec % 60));
  u8g2.setFont(u8g2_font_9x15_tf);
  uint8_t w = u8g2.getStrWidth(buf);
  u8g2.drawStr((128 - w) / 2, 32, buf);

  // 状态行
  u8g2.setFont(u8g2_font_6x10_tf);
  snprintf(buf, sizeof(buf), "I2C 0x%02X  %s", s_addr, wifiOk ? "WIFI OK" : "WIFI NO");
  u8g2.drawStr(0, 46, buf);

  const char* last = (ip && ip[0]) ? ip : (wifiOk ? "no ip yet" : "no wifi config");
  u8g2.drawStr(0, 58, last);

  u8g2.sendBuffer();
}

// ------------------------------------------------------------
// OTA 进度屏
// ------------------------------------------------------------
void displayOtaScreen(int percent) {
  if (!s_ready) return;
  if (percent < 0)   percent = 0;
  if (percent > 100) percent = 100;

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 10, "OTA UPDATE");

  u8g2.drawFrame(0, 22, 128, 14);
  u8g2.drawBox(2, 24, (uint8_t)((124 * percent) / 100), 10);

  char buf[8];
  snprintf(buf, sizeof(buf), "%d%%", percent);
  u8g2.drawStr(4, 50, buf);
  u8g2.sendBuffer();
}
// 配网模式：告诉用户连哪个热点、打开哪个地址
void displayPortalScreen() {
  if (!s_ready) return;
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawStr(4, 12, "CONFIG MODE");
  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.drawStr(4, 26, "WiFi: ESP32-Clock");
  u8g2.drawStr(4, 38, "Open: 192.168.4.1");
  u8g2.drawStr(4, 50, "save & reboot");
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawStr(4, 62, FW_VERSION);
  u8g2.sendBuffer();
}
