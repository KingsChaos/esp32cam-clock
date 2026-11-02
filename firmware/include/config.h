#pragma once
// ============================================================
//  全局配置（M0）
//  所有"会变的数字"都集中在这里，后面 M2 会把 WiFi 挪到 NVS
// ============================================================

// ---------- 固件标识 ----------
#define FW_NAME    "ESP32-CAM CLOCK"
#define FW_AUTHOR  "shane"
#define FW_VERSION "1.0.0-esp32cam"

// ---------- 屏幕：SSD1315 / SSD1306 兼容 128x64，I2C ----------
// ESP32-CAM 没有硬件 I2C 引脚，用 GPIO14/15（原 SD 卡脚，不用 SD 卡即可）
#define PIN_OLED_SDA 15   // D15 -> SDA
#define PIN_OLED_SCL 14   // D14 -> SCL
#define OLED_I2C_HZ  100000

// ---------- 板载灯 ----------
#define PIN_LED_RED   33  // 板载红灯，低电平点亮
#define PIN_LED_FLASH  4  // 补光灯（本固件不用）

// ---------- WiFi（M0 先写死；2.4GHz only） ----------
// 填上你的 SSID/密码即可；留空则跳过联网，只做本地显示自检
#ifndef WIFI_SSID
#define WIFI_SSID       ""              // 留空 = 开机进配网热点模式（推荐）
#endif
#ifndef WIFI_PASS
#define WIFI_PASS       ""
#endif
#define WIFI_CONNECT_TIMEOUT_MS 15000

// ---------- OTA ----------
#define OTA_HOSTNAME "esp32-clock"
#define OTA_PASSWORD ""           // 留空 = 不校验密码（局域网内自用足够）

// ---------- 串口 ----------
#define SERIAL_BAUD 115200
// ---------- 天气：Open-Meteo（苏州，免 Key） ----------
#define WX_LAT          "31.2989"
#define WX_LON          "120.5853"
#define WX_TZ           "Asia%2FShanghai"
#define WX_REFRESH_MIN  30          // 正常刷新间隔（分钟）
