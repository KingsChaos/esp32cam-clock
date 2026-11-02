# 10 · HTTP 接口

设备联网后自带一个 Web 服务（端口 **80**）。下面所有接口都可以用浏览器、`curl` 或任意 HTTP 客户端访问。

> 把 `<IP>` 换成设备实际地址（本机示例 `192.168.124.24`），也可用 mDNS 名 `esp32-clock.local`。
> 查 IP 的方法：串口日志、路由器后台，或 `curl http://esp32-clock.local/status`。

---

## 1. 接口总览

| 方法 | 路径 | 作用 | 返回 |
|---|---|---|---|
| `GET` | `/` | 状态页（人看的 HTML） | `text/html` |
| `GET` | `/status` | 状态 JSON（机器读、调试用） | `application/json` |
| `GET` | `/update` | 固件升级页面（表单） | `text/html` |
| `POST` | `/update` | 上传 `.bin` 固件并重启 | `text/html` |
| `GET` | `/reboot` | 重启设备 | `text/plain` |
| `GET` | `/debug/wx` | 立刻重新拉一次天气 | `text/plain` |
| `GET` | `/debug/icon?n=0..11` | 把第 n 个天气图标放大到 48×48 显示 15 秒 | `text/plain` |
| `GET` | `/debug/portal` | 切到配网模式（停掉状态页，开热点） | `text/plain` |

> 配网模式（热点 `ESP32-Clock`）下是另一套极简服务：`/` 配网页、`POST /save` 保存、`/reboot` 重启。

---

## 2. `/status` 字段说明

```bash
curl http://192.168.124.24/status
```

```json
{
  "fw": "1.0.0-esp32cam",
  "author": "shane",
  "ip": "192.168.124.24",
  "rssi": -63,
  "heap": 200628,
  "time": 1793610806,
  "timeStr": "17:13:26",
  "lunar": "九月廿四",
  "uptime": 16,
  "boots": 4,
  "reconnects": 0,
  "minHeap": 203136,
  "wx": {
    "valid": 1,
    "http": 200,
    "jsonErr": 0,
    "wmo": 0,
    "temp": 17,
    "tempTenth": 172,
    "hi": 20,
    "lo": 13,
    "rain": 0,
    "rainToday": 100,
    "humidity": 65,
    "windDir": 0,
    "windLevel": 1,
    "rawT": 17.2,
    "rawHi": 20.0,
    "rawLo": 12.8,
    "rawWs": 1.2,
    "rawWd": 5.0,
    "rawRp": 0.0,
    "dirCalc": 0,
    "deg": 5,
    "calc2": 0,
    "bodyLen": 914,
    "updated": 1793610806
  }
}
```

### 顶层字段

| 字段 | 含义 |
|---|---|
| `fw` | 固件版本 |
| `author` | 作者 |
| `ip` / `rssi` / `heap` | 设备 IP、WiFi 信号（dBm）、当前空闲堆（字节） |
| `time` / `timeStr` | Unix 时间戳 / 本地时间 |
| `lunar` | 农历（如 `九月廿四`，闰月显示 `闰六月`） |
| `uptime` | 本次开机运行秒数 |
| `boots` | 累计开机次数（存 NVS，可用来判断有没有意外重启） |
| `reconnects` | WiFi 重连次数 |
| `minHeap` | 观测到的最小空闲堆（长稳看这个） |

### `wx` 天气字段

| 字段 | 含义 |
|---|---|
| `valid` | 是否成功取到过数据 |
| `http` / `jsonErr` | 上次请求的 HTTP 状态码 / JSON 解析错误码（0=OK） |
| `wmo` | WMO 天气码（0=晴、3=阴、61=小雨…） |
| `temp` / `tempTenth` | 当前温度整数 / ×10（`172` = 17.2℃） |
| `hi` / `lo` | 今日最高 / 最低 |
| `rain` | **当前小时**降雨概率 |
| `rainToday` | **今天**最大降雨概率 |
| `humidity` | 相对湿度 |
| `windDir` / `windLevel` | 风向索引（0=N…7=NW）/ 蒲福风级 |
| `raw*` | 原始浮点值（排查用） |
| `updated` | 上次成功取数的 Unix 时间 |

---

## 3. 常用操作

### 3.1 看设备是否活着

```bash
curl -s http://192.168.124.24/status | jq .fw
```

### 3.2 强制刷新天气（不用等 30 分钟）

```bash
curl http://192.168.124.24/debug/wx
# → queued
```

下一次主循环（约 5 秒内）就会重新请求，然后 `/status` 的 `updated` 会变。

### 3.3 逐个验收天气图标

```bash
# n = 0..11，屏幕上放大显示 15 秒
curl "http://192.168.124.24/debug/icon?n=0"
```

| n | 图标 | n | 图标 |
|---|---|---|---|
| 0 | 晴 | 6 | 雨 |
| 1 | 少云 | 7 | 大雨 |
| 2 | 多云 | 8 | 雪 |
| 3 | 阴 | 9 | 阵雨 |
| 4 | 雾 | 10 | 雷 |
| 5 | 毛毛雨 | 11 | 冻雨 |

### 3.4 浏览器 OTA 升级

```bash
# 1) 打开上传页
#    http://192.168.124.24/update
# 2) 选 firmware/.build/out/esp32clock.ino.bin
# 3) 点「上传并重启」
```

命令行等价写法：

```bash
curl -F "firmware=@firmware/.build/out/esp32clock.ino.bin" \
     http://192.168.124.24/update
```

### 3.5 重启

```bash
curl http://192.168.124.24/reboot
```

### 3.6 进入配网模式（换 WiFi）

```bash
curl http://192.168.124.24/debug/portal
```

然后手机连热点 **`ESP32-Clock`** → 打开 `http://192.168.4.1` → 选 WiFi、填密码 → 保存后自动重启。

---

## 4. 配网模式下的接口（热点 `192.168.4.1`）

| 方法 | 路径 | 作用 |
|---|---|---|
| `GET` | `/` | 配网页（自动扫描周边 WiFi，列出下拉框） |
| `POST` | `/save` | 保存 `ssid` + `pass` 到 NVS 并重启 |
| `GET` | `/reboot` | 不保存，直接重启 |
| 任意 | 其他路径 | 强制跳转配网页（captive portal） |
