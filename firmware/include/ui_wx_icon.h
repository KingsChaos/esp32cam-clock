#pragma once
#include <Arduino.h>

class U8G2;

// 用 U8g2 图元直接画天气图标（不用 XBM 位图，避免位图步长/字节序问题）
// 图标区为 size x size 的方框，(x0,y0) 是左上角
void uiDrawWeatherIcon(U8G2& d, uint8_t icon, int x0, int y0, int size);
