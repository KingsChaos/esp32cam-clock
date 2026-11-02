#include "net_time.h"
#include "config.h"

// ------------------------------------------------------------
//  SNTP：阿里云 / 腾讯 / 国际池 三家轮询
//  时区用 POSIX 串 "CST-8"（中国无夏令时）
//  12 小时重新校时一次（自己计时，避免依赖不同 core 版本的 API）
// ------------------------------------------------------------
static const char* kNtp[] = {"ntp.aliyun.com", "ntp.tencent.com", "pool.ntp.org"};

static bool     s_synced   = false;
static uint32_t s_lastSync = 0;

static const uint32_t kResyncMs = 12UL * 60UL * 60UL * 1000UL;   // 12h

static void applySntp() {
  configTime(0, 0, kNtp[0], kNtp[1], kNtp[2]);
  // 必须在 configTime 之后覆盖时区，否则会被改回 UTC
  setenv("TZ", "CST-8", 1);
  tzset();
}

bool timeBegin() {
  applySntp();
  s_lastSync = millis();
  s_synced   = false;
  Serial.println("[time] SNTP 已启动（CST-8），每 12h 校准");
  return true;
}

bool timeSynced() {
  if (s_synced) return true;
  if (time(nullptr) > 1600000000L) {      // 2020-09 之后视为已同步
    s_synced = true;
    Serial.println("[time] 已同步");
  }
  return s_synced;
}

bool timeNow(struct tm* out) {
  time_t now = time(nullptr);
  if (now < 1600000000L) return false;
  localtime_r(&now, out);
  return true;
}

void timeTick() {
  if (millis() - s_lastSync >= kResyncMs) {
    Serial.println("[time] 12h 到，重新校时");
    applySntp();
    s_lastSync = millis();
    s_synced   = false;
  }
}
