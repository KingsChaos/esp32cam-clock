#pragma once
#include <Arduino.h>

// ------------------------------------------------------------
//  WiFi 凭证存储（NVS）+ 配网热点
//  - 凭证优先级：NVS > config.h 里的编译期默认值
//  - 两者都没有 → 开热点 "ESP32-Clock"，手机连上后打开 192.168.4.1 配网
// ------------------------------------------------------------

bool cfgLoad(char* ssid, size_t n, char* pass, size_t m);   // 从 NVS 读；没有则 false
void cfgSave(const char* ssid, const char* pass);           // 写入 NVS
void cfgClear();                                            // 清除（恢复出厂配网）

void cfgPortalBegin();      // 开热点 + 配网页（要在 status WebServer 之前决定用哪个）
void cfgPortalTick();       // 主循环调用（DNS + HTTP）
bool cfgPortalActive();
