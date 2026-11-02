#include "ui_wx_icon.h"
#include "net_weather.h"

#include <U8g2lib.h>

// ------------------------------------------------------------
//  天气图标：全部用 U8g2 图元现画
//  之前的 XBM 位图在屏上显示成"三条竖带"，改用图元后所见即所得
//  坐标系：图标框左上角 (x0,y0)，边长 size（默认 28）
// ------------------------------------------------------------

static void sunIcon(U8G2& d, int cx, int cy, int r) {
  d.drawDisc(cx, cy, r);
  // 射线与圆盘之间留 2px 空隙，看起来更精致（贴在一起会糊成一团）
  int gap = r + 3;
  int len = r + 6;
  d.drawLine(cx, cy - gap, cx, cy - len);
  d.drawLine(cx, cy + gap, cx, cy + len);
  d.drawLine(cx - gap, cy, cx - len, cy);
  d.drawLine(cx + gap, cy, cx + len, cy);
}

static void smallSun(U8G2& d, int cx, int cy, int r) {
  d.drawDisc(cx, cy, r);
}

// 平底云朵
static void cloudShape(U8G2& d, int x, int base, int w, int h) {
  d.drawDisc(x + w * 24 / 100, base - h * 34 / 100, h * 34 / 100);
  d.drawDisc(x + w * 50 / 100, base - h * 56 / 100, h * 50 / 100);
  d.drawDisc(x + w * 77 / 100, base - h * 32 / 100, h * 33 / 100);
  d.drawBox(x + w * 16 / 100, base - h * 34 / 100, w * 70 / 100, h * 34 / 100);
}

// 云下 1/2/3 根竖线
static void rainBars(U8G2& d, int cx, int y, int n, int len) {
  if (n == 1) {
    d.drawLine(cx, y, cx, y + len);
    d.drawLine(cx + 1, y, cx + 1, y + len);
  } else if (n == 2) {
    for (int i = -1; i <= 1; i += 2) {
      int x = cx + i * 5;
      d.drawLine(x, y, x, y + len);
      d.drawLine(x + 1, y, x + 1, y + len);
    }
  } else {
    for (int i = -1; i <= 1; i++) {
      int x = cx + i * 6;
      d.drawLine(x, y, x, y + len);
      d.drawLine(x + 1, y, x + 1, y + len);
    }
  }
}

static void snowFlakes(U8G2& d, int cx, int y) {
  for (int i = -1; i <= 1; i++) {
    int x = cx + i * 6;
    d.drawLine(x - 2, y + 2, x + 2, y + 2);
    d.drawLine(x, y, x, y + 4);
  }
}

static void boltShape(U8G2& d, int cx, int cy) {
  // 两道错开的折线，看起来是一条粗闪电
  int pts[5][2] = {
    {cx + 5, cy - 13}, {cx - 4, cy - 1}, {cx + 2, cy - 1}, {cx - 6, cy + 13}, {0, 0}
  };
  for (int i = 0; i < 3; i++) {
    d.drawLine(pts[i][0], pts[i][1], pts[i + 1][0], pts[i + 1][1]);
    d.drawLine(pts[i][0] + 1, pts[i][1], pts[i + 1][0] + 1, pts[i + 1][1]);
  }
  d.drawLine(cx + 1, cy - 6, cx - 1, cy + 2);
}

void uiDrawWeatherIcon(U8G2& d, uint8_t icon, int x0, int y0, int size) {
  const int cx = x0 + size / 2;
  const int cy = y0 + size / 2;
  const int R  = size;                 // 便于按比例取尺寸

  switch (icon) {
    case WX_SUN:
      sunIcon(d, cx, cy, R * 22 / 100);   // 圆盘小一点，射线才有空隙
      break;

    case WX_SUN_CLOUD:
      smallSun(d, x0 + R * 28 / 100, y0 + R * 22 / 100, R * 14 / 100);
      cloudShape(d, x0 + R * 8 / 100, y0 + R * 88 / 100, R * 82 / 100, R * 46 / 100);
      break;

    case WX_CLOUD_SUN:
      smallSun(d, x0 + R * 74 / 100, y0 + R * 18 / 100, R * 14 / 100);
      cloudShape(d, x0 + R * 2 / 100, y0 + R * 88 / 100, R * 86 / 100, R * 48 / 100);
      break;

    case WX_CLOUD2:
      cloudShape(d, x0 + R * 4 / 100, y0 + R * 52 / 100, R * 66 / 100, R * 40 / 100);
      cloudShape(d, x0 + R * 18 / 100, y0 + R * 92 / 100, R * 80 / 100, R * 44 / 100);
      break;

    case WX_FOG:
      for (int i = 0; i < 3; i++) {
        int y = y0 + R * (28 + i * 22) / 100;
        d.drawLine(x0 + R * 8 / 100, y, x0 + R * 92 / 100, y);
        d.drawLine(x0 + R * 8 / 100, y + 1, x0 + R * 92 / 100, y + 1);
      }
      break;

    case WX_DRIZZLE:
      cloudShape(d, x0 + R * 8 / 100, y0 + R * 58 / 100, R * 84 / 100, R * 46 / 100);
      rainBars(d, cx, y0 + R * 66 / 100, 1, R * 24 / 100);
      break;

    case WX_RAIN:
      cloudShape(d, x0 + R * 4 / 100, y0 + R * 56 / 100, R * 92 / 100, R * 46 / 100);
      rainBars(d, cx, y0 + R * 64 / 100, 2, R * 30 / 100);
      break;

    case WX_HEAVY_RAIN:
      cloudShape(d, x0 + R * 2 / 100, y0 + R * 54 / 100, R * 96 / 100, R * 46 / 100);
      rainBars(d, cx, y0 + R * 62 / 100, 3, R * 34 / 100);
      break;

    case WX_SNOW:
      cloudShape(d, x0 + R * 4 / 100, y0 + R * 56 / 100, R * 92 / 100, R * 46 / 100);
      snowFlakes(d, cx, y0 + R * 68 / 100);
      break;

    case WX_SHOWER:
      smallSun(d, x0 + R * 76 / 100, y0 + R * 16 / 100, R * 14 / 100);
      cloudShape(d, x0 + R * 4 / 100, y0 + R * 68 / 100, R * 84 / 100, R * 44 / 100);
      rainBars(d, cx, y0 + R * 74 / 100, 2, R * 20 / 100);
      break;

    case WX_THUNDER:
      boltShape(d, cx, cy);
      break;

    case WX_FREEZING:
      cloudShape(d, x0 + R * 4 / 100, y0 + R * 54 / 100, R * 92 / 100, R * 42 / 100);
      rainBars(d, cx, y0 + R * 62 / 100, 3, R * 16 / 100);
      for (int i = -1; i <= 1; i++) {
        d.drawDisc(cx + i * 6, y0 + R * 88 / 100, 1);
      }
      break;

    default:
      cloudShape(d, x0 + R * 8 / 100, y0 + R * 62 / 100, R * 84 / 100, R * 44 / 100);
      break;
  }
}
