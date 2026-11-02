#include "lunar.h"

#include <stdio.h>
#include <string.h>

// 1900–2099 农历信息表
//  bit0-3  : 闰月月份（0 = 无闰月）
//  bit4-15 : 十二个月大小（1 = 30 天，0 = 29 天）
//  bit16   : 闰月天数（1 = 30 天）
static const uint32_t LUNAR_INFO[201] = {   // 1900–2100
  0x04bd8, 0x04ae0, 0x0a570, 0x054d5, 0x0d260, 0x0d950, 0x16554, 0x056a0, 0x09ad0, 0x055d2,
  0x04ae0, 0x0a5b6, 0x0a4d0, 0x0d250, 0x1d255, 0x0b540, 0x0d6a0, 0x0ada2, 0x095b0, 0x14977,
  0x04970, 0x0a4b0, 0x0b4b5, 0x06a50, 0x06d40, 0x1ab54, 0x02b60, 0x09570, 0x052f2, 0x04970,
  0x06566, 0x0d4a0, 0x0ea50, 0x06e95, 0x05ad0, 0x02b60, 0x186e3, 0x092e0, 0x1c8d7, 0x0c950,
  0x0d4a0, 0x1d8a6, 0x0b550, 0x056a0, 0x1a5b4, 0x025d0, 0x092d0, 0x0d2b2, 0x0a950, 0x0b557,
  0x06ca0, 0x0b550, 0x15355, 0x04da0, 0x0a5b0, 0x14573, 0x052b0, 0x0a9a8, 0x0e950, 0x06aa0,
  0x0aea6, 0x0ab50, 0x04b60, 0x0aae4, 0x0a570, 0x05260, 0x0f263, 0x0d950, 0x05b57, 0x056a0,
  0x096d0, 0x04dd5, 0x04ad0, 0x0a4d0, 0x0d4d4, 0x0d250, 0x0d558, 0x0b540, 0x0b6a0, 0x195a6,
  0x095b0, 0x049b0, 0x0a974, 0x0a4b0, 0x0b27a, 0x06a50, 0x06d40, 0x0af46, 0x0ab60, 0x09570,
  0x04af5, 0x04970, 0x064b0, 0x074a3, 0x0ea50, 0x06b58, 0x055c0, 0x0ab60, 0x096d5, 0x092e0,
  0x0c960, 0x0d954, 0x0d4a0, 0x0da50, 0x07552, 0x056a0, 0x0abb7, 0x025d0, 0x092d0, 0x0cab5,
  0x0a950, 0x0b4a0, 0x0baa4, 0x0ad50, 0x055d9, 0x04ba0, 0x0a5b0, 0x15176, 0x052b0, 0x0a930,
  0x07954, 0x06aa0, 0x0ad50, 0x05b52, 0x04b60, 0x0a6e6, 0x0a4e0, 0x0d260, 0x0ea65, 0x0d530,
  0x05aa0, 0x076a3, 0x096d0, 0x04afb, 0x04ad0, 0x0a4d0, 0x1d0b6, 0x0d250, 0x0d520, 0x0dd45,
  0x0b5a0, 0x056d0, 0x055b2, 0x049b0, 0x0a577, 0x0a4b0, 0x0aa50, 0x1b255, 0x06d20, 0x0ada0,
  0x14b63, 0x09370, 0x049f8, 0x04970, 0x064b0, 0x168a6, 0x0ea50, 0x06b20, 0x1a6c4, 0x0aae0,
  0x092e0, 0x0d2e3, 0x0c960, 0x0d557, 0x0d4a0, 0x0da50, 0x05d55, 0x056a0, 0x0a6d0, 0x055d4,
  0x052d0, 0x0a9b8, 0x0a950, 0x0b4a0, 0x0b6a6, 0x0ad50, 0x055a0, 0x0aba4, 0x0a5b0, 0x052b0,
  0x0b273, 0x06930, 0x07337, 0x06aa0, 0x0ad50, 0x14b55, 0x04b60, 0x0a570, 0x054e4, 0x0d160,
  0x0e968, 0x0d520, 0x0daa0, 0x16aa6, 0x056d0, 0x04ae0, 0x0a9d4, 0x0a2d0, 0x0d150, 0x0f252,
  0x0d520
};

// 公历年月日 -> 儒略日数（Howard Hinnant 算法，避免依赖时区/mktime）
static long daysFromCivil(int y, int m, int d) {
  y -= (m <= 2) ? 1 : 0;
  const int  era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = (unsigned)(y - era * 400);
  const unsigned doy = (153u * (unsigned)(m + (m > 2 ? -3 : 9)) + 2u) / 5u + (unsigned)d - 1u;
  const unsigned doe = yoe * 365u + yoe / 4u - yoe / 100u + doy;
  return (long)era * 146097L + (long)doe - 719468L;
}

static int leapMonthOf(int y) { return (int)(LUNAR_INFO[y - 1900] & 0x0f); }

static int leapDaysOf(int y) {
  if (leapMonthOf(y) == 0) return 0;
  return (LUNAR_INFO[y - 1900] & 0x10000) ? 30 : 29;
}

static int monthDaysOf(int y, int m) {
  return (LUNAR_INFO[y - 1900] & (0x10000u >> m)) ? 30 : 29;
}

static int yearDaysOf(int y) {
  int sum = 0;
  for (int m = 1; m <= 12; m++) sum += monthDaysOf(y, m);
  return sum + leapDaysOf(y);
}

bool lunarFromSolar(int y, int m, int d, LunarDate* out) {
  if (!out || y < 1900 || y > 2099) return false;

  long offset = daysFromCivil(y, m, d) - daysFromCivil(1900, 1, 31);
  if (offset < 0) return false;

  int ly = 1900;
  while (ly < 2100) {
    int yd = yearDaysOf(ly);
    if (offset < yd) break;
    offset -= yd;
    ly++;
  }
  if (ly >= 2100) return false;

  int lm = 1;
  bool leap = false;
  while (lm <= 12) {
    int dm = leap ? leapDaysOf(ly) : monthDaysOf(ly, lm);
    if (offset < dm) break;
    offset -= dm;
    if (leap) {
      leap = false;
      lm++;
    } else if (lm == leapMonthOf(ly) && leapMonthOf(ly) > 0) {
      leap = true;
    } else {
      lm++;
    }
  }

  out->year  = ly;
  out->month = lm;
  out->day   = (int)offset + 1;
  out->leap  = leap;
  return true;
}

void lunarToCn(const LunarDate& l, char* buf, size_t n) {
  static const char* kMon[] = {"正", "二", "三", "四", "五", "六",
                               "七", "八", "九", "十", "冬", "腊"};
  static const char* kNum[] = {"一", "二", "三", "四", "五", "六", "七", "八", "九", "十"};

  char day[16];
  int d = l.day;
  if (d == 10)       snprintf(day, sizeof(day), "初十");
  else if (d == 20)  snprintf(day, sizeof(day), "二十");
  else if (d == 30)  snprintf(day, sizeof(day), "三十");
  else if (d < 10)   snprintf(day, sizeof(day), "初%s", kNum[d - 1]);
  else if (d < 20)   snprintf(day, sizeof(day), "十%s", kNum[d - 11]);
  else               snprintf(day, sizeof(day), "廿%s", kNum[d - 21]);

  int mi = (l.month >= 1 && l.month <= 12) ? l.month - 1 : 0;
  snprintf(buf, n, "%s%s月%s", l.leap ? "闰" : "", kMon[mi], day);
}
