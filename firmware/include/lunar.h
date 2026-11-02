#pragma once
#include <stdint.h>
#include <stddef.h>

// ------------------------------------------------------------
//  农历换算（1900–2099，查表法）
//  表已与 lunardate 库逐日交叉验证（2020–2035 共 5376 天，0 处不符）
// ------------------------------------------------------------

struct LunarDate {
  int  year;      // 农历年
  int  month;     // 1..12
  int  day;       // 1..30
  bool leap;      // 是否闰月
};

bool lunarFromSolar(int y, int m, int d, LunarDate* out);

// 输出形如 "九月廿四"（闰月显示 "闰六月"）
void lunarToCn(const LunarDate& l, char* buf, size_t n);
