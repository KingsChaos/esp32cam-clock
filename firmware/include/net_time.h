#pragma once
#include <Arduino.h>
#include <time.h>

// SNTP 校时：时区 CST-8，每 12 小时重新校准一次
bool  timeBegin();
bool  timeSynced();
bool  timeNow(struct tm* out);   // 取本地时间（线程安全 localtime_r）
void  timeTick();                // 主循环调用：到点自动重新校时
