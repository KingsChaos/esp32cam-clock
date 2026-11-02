#pragma once
#include <Arduino.h>

// ------------------------------------------------------------
//  v6 布局的像素常量（见 docs/04-单屏布局设计.md）
//  改这里 = 改全屏排版
// ------------------------------------------------------------

// 时间（黄色区 0–15）
#define L_TIME_BASELINE   12     // 8x13 字体基线
#define L_YELLOW_ROWS     16     // 黄区行数（双色屏 0–15）
#define L_TIME_CLEAR_H    14     // 每秒只清这 14 行（0–13），不能再多，否则会把日期顶上削掉

// 日期行
#define L_DATE_BASELINE   27    // 墨迹必须在 16 行以下，否则会染上黄区颜色

// 分区线
#define L_DIV_Y           29
#define L_DIV_X0          3
#define L_DIV_X1          125

// 天气行（M1 只放占位符）
#define L_WEATHER_TOP     30
#define L_TEMP_X          28
#define L_TEMP_BASELINE   47

// 数据行 / 状态行
#define L_STATUS_BASELINE 63
#define L_RIGHT_X         127    // 右对齐基准
