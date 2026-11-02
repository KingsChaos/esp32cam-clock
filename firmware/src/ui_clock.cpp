#include "ui_clock.h"
#include "layout.h"
#include "display.h"
#include "net_weather.h"
#include "ui_wx_icon.h"
#include "lunar.h"

#include <U8g2lib.h>
#include <stdio.h>
#include <string.h>

// ------------------------------------------------------------
//  v6 排版：时间（黄区）/ 日期+星期 / 虚线 / 天气 / 状态行
//  字体：时间 8x13B，日期拉丁 6x12，中文 wqy12（必须 drawUTF8），状态 5x7
// ------------------------------------------------------------
static const char* kWeekday[] = {"周日", "周一", "周二", "周三", "周四", "周五", "周六"};

static void drawDashed(U8G2& d, int y, int x0, int x1) {
  for (int x = x0; x < x1; x += 7) {
    int xe = x + 4;
    if (xe > x1) xe = x1;
    d.drawBox(x, y, xe - x, 1);
  }
}

static void drawTimeText(U8G2& d, const struct tm* t, bool ok) {
  char buf[10];
  if (ok && t) snprintf(buf, sizeof(buf), "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
  else         snprintf(buf, sizeof(buf), "--:--:--");

  d.setFont(u8g2_font_8x13B_tf);
  int w = d.getStrWidth(buf);
  d.drawStr((128 - w) / 2, L_TIME_BASELINE, buf);
}

static void drawDate(U8G2& d, const struct tm* t, bool ok) {
  char lat[8];
  const char* wd;

  if (ok && t) {
    snprintf(lat, sizeof(lat), "%02d-%02d", t->tm_mon + 1, t->tm_mday);
    wd = kWeekday[t->tm_wday % 7];
  } else {
    snprintf(lat, sizeof(lat), "--:--");
    wd = "---";
  }

  // 农历（公历日期有效时才显示）
  char lunar[24] = "";
  if (ok && t) {
    LunarDate ld;
    if (lunarFromSolar(t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, &ld)) {
      lunarToCn(ld, lunar, sizeof(lunar));
    }
  }

  d.setFont(u8g2_font_6x12_tf);
  int wLat = d.getStrWidth(lat);

  // 中文是 UTF-8 多字节，必须用 drawUTF8 / getUTF8Width（drawStr 会按字节拆开 → 乱码）
  d.setFont(u8g2_font_wqy12_t_gb2312);
  int wCn = d.getUTF8Width(wd);
  int wLu = lunar[0] ? d.getUTF8Width(lunar) : 0;

  int gap   = 4;
  int total = wLat + gap + wCn + (wLu ? gap + wLu : 0);
  int x0    = (128 - total) / 2;
  if (x0 < 1) x0 = 1;

  d.setFont(u8g2_font_6x12_tf);
  d.drawStr(x0, L_DATE_BASELINE, lat);

  d.setFont(u8g2_font_wqy12_t_gb2312);
  d.drawUTF8(x0 + wLat + gap, L_DATE_BASELINE, wd);
  if (wLu) d.drawUTF8(x0 + wLat + gap + wCn + gap, L_DATE_BASELINE, lunar);
}

// 天气区：图标 + 温度（左），H/L 与 降雨%/风（右两行，右对齐 125）
static void drawWeather(U8G2& d) {
  const WeatherData& w = g_weather;

  // 图标：28x28 的框内用图元现画（不再用 XBM 位图）
  uiDrawWeatherIcon(d, w.valid ? w.icon : WX_CLOUD2, 2, L_WEATHER_TOP, 24);

  // 温度（大字，保留一位小数：17.6 而不是 18）
  char tbuf[10];
  if (w.valid) {
    int t10 = w.temp10;
    bool neg = t10 < 0;
    if (neg) t10 = -t10;
    snprintf(tbuf, sizeof(tbuf), neg ? "-%d.%d" : "%d.%d", t10 / 10, t10 % 10);
  } else {
    snprintf(tbuf, sizeof(tbuf), "--.-");
  }
  d.setFont(u8g2_font_8x13B_tf);
  d.drawStr(33, L_TEMP_BASELINE, tbuf);

  // 右侧两行（5x7）
  char l1a[8], l1b[8], l2a[8], l2b[8];
  if (w.valid) {
    snprintf(l1a, sizeof(l1a), "H %d", w.hi);
    snprintf(l1b, sizeof(l1b), "L %d", w.lo);
    snprintf(l2a, sizeof(l2a), "%d%%", w.humidity);   // 湿度
    snprintf(l2b, sizeof(l2b), "%s%d", windDirLetter(w.windDir), w.windLevel);
  } else {
    snprintf(l1a, sizeof(l1a), "H --");
    snprintf(l1b, sizeof(l1b), "L --");
    snprintf(l2a, sizeof(l2a), "--%%");
    snprintf(l2b, sizeof(l2b), "--");
  }

  d.setFont(u8g2_font_5x7_tf);
  d.drawStr(82, 39, l1a);
  d.drawStr(L_RIGHT_X - d.getStrWidth(l1b), 39, l1b);
  d.drawStr(82, 49, l2a);
  d.drawStr(L_RIGHT_X - d.getStrWidth(l2b), 49, l2b);
}

static void drawStatus(U8G2& d, bool wifiOk) {
  d.setFont(u8g2_font_5x7_tf);
  d.drawStr(0, L_STATUS_BASELINE, wifiOk ? "WiFi OK" : "WiFi NO");

  char up[8] = "--:--";
  if (g_weather.valid && g_weather.updated > 0) {
    struct tm tmv;
    localtime_r(&g_weather.updated, &tmv);
    snprintf(up, sizeof(up), "%02d:%02d", tmv.tm_hour, tmv.tm_min);
  }
  d.drawStr(L_RIGHT_X - d.getStrWidth(up), L_STATUS_BASELINE, up);
}

void uiDrawAll(const struct tm* t, bool timeOk, bool wifiOk) {
  U8G2& d = displayRef();
  if (!displayReady()) return;

  d.clearBuffer();
  drawTimeText(d, t, timeOk);
  drawDate(d, t, timeOk);
  drawDashed(d, L_DIV_Y, L_DIV_X0, L_DIV_X1);
  drawWeather(d);
  drawStatus(d, wifiOk);
  d.sendBuffer();
}

void uiDrawTime(const struct tm* t, bool timeOk) {
  U8G2& d = displayRef();
  if (!displayReady()) return;

  // 只清黄区（0–15 行）再重画时间，保留日期/天气/状态内容
  d.setDrawColor(0);
  d.drawBox(0, 0, 128, L_TIME_CLEAR_H);
  d.setDrawColor(1);
  drawTimeText(d, t, timeOk);
  d.sendBuffer();
}
