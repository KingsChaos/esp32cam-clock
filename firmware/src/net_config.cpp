#include "net_config.h"

#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

static Preferences s_prefs;
static WebServer   s_web(80);
static DNSServer   s_dns;
static bool        s_portal = false;

static const char* kApName = "ESP32-Clock";

bool cfgLoad(char* ssid, size_t n, char* pass, size_t m) {
  s_prefs.begin("wifi", true);
  String s = s_prefs.getString("ssid", "");
  String p = s_prefs.getString("pass", "");
  s_prefs.end();

  if (s.length() == 0) return false;
  snprintf(ssid, n, "%s", s.c_str());
  snprintf(pass, m, "%s", p.c_str());
  return true;
}

void cfgSave(const char* ssid, const char* pass) {
  s_prefs.begin("wifi", false);
  s_prefs.putString("ssid", ssid);
  s_prefs.putString("pass", pass ? pass : "");
  s_prefs.end();
  Serial.printf("[cfg] 已保存 SSID=%s\n", ssid);
}

void cfgClear() {
  s_prefs.begin("wifi", false);
  s_prefs.clear();
  s_prefs.end();
  Serial.println("[cfg] 凭证已清除");
}

// 配网页：列出扫描到的 WiFi + 输入密码
static String portalPage() {
  int n = WiFi.scanNetworks();
  String opt;
  for (int i = 0; i < n; i++) {
    String ss = WiFi.SSID(i);
    if (!ss.length()) continue;
    opt += "<option value=\"";
    opt += ss;
    opt += "\">";
    opt += ss;
    opt += (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " (开放)" : "";
    opt += "</option>";
  }
  if (!opt.length()) opt = "<option value=''>（没扫到，请手动输入）</option>";

  String page;
  page.reserve(1400);
  page += F("<!doctype html><meta charset=utf-8>"
            "<meta name=viewport content='width=device-width,initial-scale=1'>"
            "<title>ESP32-Clock 配网</title>"
            "<style>body{font-family:system-ui;margin:24px;max-width:520px}"
            "label{display:block;margin:14px 0 6px;font-weight:600}"
            "select,input{width:100%;padding:10px;font-size:16px;box-sizing:border-box}"
            "button{margin-top:18px;padding:12px 20px;font-size:16px;width:100%}"
            ".hint{color:#666;font-size:13px;margin-top:10px}</style>"
            "<h2>ESP32-Clock 配网</h2>"
            "<form method=POST action=/save>"
            "<label>WiFi 名称（2.4GHz）</label>"
            "<input name=ssid list=aps placeholder='选择或输入 SSID' required>"
            "<datalist id=aps>");
  page += opt;
  page += F("</datalist>"
            "<label>密码</label>"
            "<input name=pass type=password placeholder='留空表示开放网络'>"
            "<button type=submit>保存并重启</button>"
            "</form>"
            "<div class=hint>ESP32 只支持 2.4GHz；保存后设备会重启并连接该网络。</div>");
  return page;
}

static void handleRoot()  { s_web.send(200, "text/html; charset=utf-8", portalPage()); }

static void handleSave() {
  String ssid = s_web.arg("ssid");
  String pass = s_web.arg("pass");
  ssid.trim();
  if (!ssid.length()) {
    s_web.send(400, "text/plain; charset=utf-8", "SSID 不能为空");
    return;
  }
  cfgSave(ssid.c_str(), pass.c_str());
  s_web.send(200, "text/html; charset=utf-8",
             "<meta charset=utf-8><h3>已保存，正在重启…</h3>");
  delay(800);
  ESP.restart();
}

void cfgPortalBegin() {
  if (s_portal) return;

  // AP + STA 同时开：AP 供手机连接，STA 保留以便扫描
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(kApName);
  delay(200);

  s_dns.start(53, "*", WiFi.softAPIP());

  s_web.on("/", handleRoot);
  s_web.on("/save", HTTP_POST, handleSave);
  s_web.on("/reboot", []() {
    s_web.send(200, "text/plain; charset=utf-8", "rebooting");
    delay(400);
    ESP.restart();
  });
  // Captive portal：任何未知路径都弹配网页
  s_web.onNotFound(handleRoot);
  s_web.begin();

  s_portal = true;
  Serial.printf("[cfg] 配网热点已开：%s  http://%s/\n",
                kApName, WiFi.softAPIP().toString().c_str());
}

void cfgPortalTick() {
  if (!s_portal) return;
  s_dns.processNextRequest();
  s_web.handleClient();
}

bool cfgPortalActive() { return s_portal; }
