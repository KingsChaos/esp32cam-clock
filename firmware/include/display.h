#pragma once
#include <Arduino.h>

// 屏幕封装：初始化 / 地址探测 / 自检图 / M0 信息屏
// 说明：ESP32-CAM 没有硬件 I2C 引脚，这里用 U8g2 的软件 I2C，
//       引脚在 config.h 里（SDA=15, SCL=14）。

bool     displayBegin();      // 扫描 0x3C/0x3D 并初始化；成功返回 true
bool     displayReady();
uint8_t  displayAddress();    // 探测到的地址（0 表示没找到）

void     displaySplash();     // 自检：全屏点亮 -> 棋盘 -> 边框+黄区分界线
void     displayMainScreen(uint32_t uptimeSec, const char* ip, bool wifiOk);
void     displayOtaScreen(int percent);
void     displayPortalScreen();   // 配网模式提示屏（热点名 + 192.168.4.1）
void     displayClear();

// 给 ui_* 用的底层句柄（U8g2 实例在 display.cpp 里）
class U8G2;
U8G2&    displayRef();
