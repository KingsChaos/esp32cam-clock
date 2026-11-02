#pragma once
#include <Arduino.h>
#include <time.h>

// ------------------------------------------------------------
//  天气：Open-Meteo（苏州，免 Key）
// ------------------------------------------------------------

enum WxIcon {
  WX_SUN = 0, WX_SUN_CLOUD, WX_CLOUD_SUN, WX_CLOUD2, WX_FOG, WX_DRIZZLE,
  WX_RAIN, WX_HEAVY_RAIN, WX_SNOW, WX_SHOWER, WX_THUNDER, WX_FREEZING
};

struct WeatherData {
  bool    valid;      // 是否成功取到过
  int8_t  temp;       // 当前温度（℃）取整
  int16_t temp10;     // 当前温度 x10（17.6℃ -> 176），屏幕显示一位小数
  int8_t  hi, lo;     // 今日最高/最低
  uint8_t icon;       // WxIcon
  uint8_t rainPct;    // 当前小时降雨概率 0..100
  uint8_t rainToday;  // 今日降雨概率（全天最大）0..100
  uint8_t humidity;   // 相对湿度 0..100
  uint8_t windDir;    // 0=N 1=NE 2=E 3=SE 4=S 5=SW 6=W 7=NW
  uint8_t windLevel;  // 蒲福风级 0..12
  time_t  updated;    // 最后一次成功取数的时刻（本地）

  // ---- 调试字段（供 /status 查看）----
  int   dbgHttp;      // HTTP 状态码
  int   dbgJsonErr;   // ArduinoJson 错误码
  int   dbgWmo;       // 原始天气码
  float dbgRawT, dbgRawHi, dbgRawLo, dbgRawWs, dbgRawWd, dbgRawRp;
  int   dbgBodyLen;   // 响应体长度
  int   dbgCalc2;     // 计算过程中的方向索引
  int   dbgDeg;       // 归一化后的角度
  char  dbgHead[40];  // 响应体开头（前 32 字符）
};

extern WeatherData g_weather;

void weatherBegin();    // 立刻尝试拉一次
void weatherTick();     // 调度：正常 30min；失败 1/5/30min 退避
const char* windDirLetter(uint8_t dir);   // "N"/"NE"/...
bool weatherStale();    // 数据超过 2 小时没更新（离线/请求失败）
