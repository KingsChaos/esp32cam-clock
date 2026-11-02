#pragma once
#include <Arduino.h>
#include <time.h>

// ------------------------------------------------------------
//  v6 单屏布局绘制（M1 版本）
//  时间（黄区） / 日期+星期 / 分区虚线 / 天气占位 / 状态行
// ------------------------------------------------------------

// 整屏重画（状态变化、跨分钟、首次上电）
void uiDrawAll(const struct tm* t, bool timeOk, bool wifiOk);

// 只重画时间（每秒）—— 目前内部仍是整帧刷新，等 M4 再切局部刷新
void uiDrawTime(const struct tm* t, bool timeOk);
