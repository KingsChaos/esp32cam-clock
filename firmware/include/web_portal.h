#pragma once
#include <Arduino.h>

// ------------------------------------------------------------
//  极简 Web 服务（端口 80）
//  /         状态页（人看）
//  /status   JSON 状态（机器看，调试用）
//  -----------------------------------------------------------
void webBegin();
bool webIconPreviewActive();   // /debug/icon 放大预览中（此时不要刷正常界面）
void webTick();
