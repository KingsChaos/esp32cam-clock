# firmware · ESP32-CAM 桌面时钟

**版本 1.0.0-esp32cam ｜ 作者 shane ｜ 状态：功能完整**

| 文档 | 内容 |
|---|---|
| [README](../docs/README.md) | 项目总览、快速上手 |
| [11-软件架构](../docs/11-软件架构.md) | 模块划分、启动流程、数据流、设计决策 |
| [10-HTTP接口](../docs/10-HTTP接口.md) | 全部 API 与 curl 用法 |
| [08-接线与刷机教程](../docs/08-接线与刷机教程.md) | 接线、烧录、配网、FAQ |
| [04-单屏布局设计](../docs/04-单屏布局设计.md) | 像素坐标与图标规则 |

## 一条命令编译 / 烧录

```bat
cd /d D:\CS\esp32display\firmware

tools\build.cmd                                  :: 只编译
tools\build.cmd -Upload COM5                     :: 串口烧录（IO0 接地）
tools\build.cmd -Upload 192.168.124.24           :: OTA（同网，无需 IO0）
tools\build.cmd -Export arduino\esp32clock       :: 导出给 Arduino IDE
tools\build.cmd -Clean                           :: 清缓存重编
```

产物：`firmware/.build/out/esp32clock.ino.bin`（约 1.37 MB）

也可以用浏览器：打开 `http://<设备IP>/update` 上传这个 `.bin`。

---
## 1. 接线（AI-Thinker ESP32-CAM ↔ SSD1315 OLED）

| OLED | ESP32-CAM | 说明 |
|---|---|---|
| `GND` | `GND` | 任一 GND |
| `VCC` | `3V3` | 屏驱动约 10–20mA |
| `SCL` | **GPIO14（D14）** | 四针模块丝印可能是 `SCK`/`CLK` |
| `SDA` | **GPIO15（D15）** | 丝印可能是 `SDA`/`DIN` |

> - 接屏后 **SD 卡不可用**（14/15 被 I2C 占用），本项目不需要 SD。
> - 四针模块的针脚顺序不统一，**接线前看丝印**。
> - 供电走 **5V/2A**，5V-GND 之间建议并 100µF，避免 HTTPS 握手时 brownout。
> - 板载红灯 = GPIO33（低电平点亮），固件用它做心跳。

---

## 2. 先填 WiFi（可选）

编辑 `include/config.h`：

```c
#define WIFI_SSID "你的2.4G WiFi"
#define WIFI_PASS "密码"
```

> 留空也能跑：会跳过联网，只做屏幕自检和本地秒表（OTA 不可用）。
> **ESP32 只支持 2.4GHz**，5GHz 连不上。

---

## 3. 编译与烧录

> ⚠️ 本机实测：PlatformIO 的包 CDN 不通（`pio run` 卡在 `Downloading 0%`）。
> **当前请用 3.0 的一行命令**（走 arduino-cli，已验证可编译出 491 KB 固件）。

### 3.0 一行命令（推荐，Windows）

```powershell
# 只编译
powershell -File tools\build.ps1

# 编译 + 烧录（COM3 换成你的端口）
powershell -File tools\build.ps1 -Upload COM3

# 编译 + 烧录 + 打开串口
powershell -File tools\build.ps1 -Upload COM3 -Monitor COM3
```

产物：`firmware/.build/out/esp32clock.ino.bin`

脚本自动做的事：把 `src/*.cpp` + `include/*.h` 拼成临时 sketch → 用
`esp32:esp32:esp32cam` + `PartitionScheme=default`（1.2MB APP ×2，OTA 必需）编译。

### 3.1 PlatformIO（备用，等 CDN 恢复）

```bash
cd firmware
pio run                  # 编译
pio run -t upload        # 烧录（需要 FTDI + GPIO0 接地）
pio device monitor       # 看日志（115200）
```

FTDI 接线：

| ESP32-CAM | FTDI |
|---|---|
| `5V` | `VCC`（跳线选 5V） |
| `GND` | `GND` |
| `U0R` | `TX` |
| `U0T` | `RX` |
| `IO0` | `GND`（**只在烧录时**接，烧完拔掉，按 RST 运行） |

### 3.2 Arduino IDE（备用）

1. 开发板选 **AI Thinker ESP32-CAM**
2. 分区表选 **Default 4MB with spiffs**（双 app，OTA 需要）
3. 库管理器装 **U8g2**
4. 打开 `src/main.cpp` 上传（`include/` 下的头文件同一目录会被识别）

---

## 4. 首次上电会看到什么（M0 自检）

1. **全屏点亮 0.8s**
   → 用来确认面板类型：**顶部约 16 行发黄、下面发蓝 = 黄蓝双色屏**；全白/全蓝 = 单色屏。
2. **棋盘格 0.4s** → 检查坏点/整行不亮。
3. **边框 + y=15 分界线 + 文字** → 确认像素坐标没有偏移。
4. **信息屏**（每秒刷新）：
   ```
   ESP32-CAM CLOCK              M0
              00:00:12
   I2C 0x3C  WIFI OK
   192.168.1.23
   ```
   - 第三行：探测到的 I2C 地址 + WiFi 状态
   - 第四行：IP；没联网时显示 `no wifi config` / `no ip yet`

串口 115200 会同时打印：芯片型号、Flash 大小、I2C 扫描结果、WiFi 连接过程、每 10s 一次心跳。

---

## 5. OTA 升级

联网后设备主机名是 **`esp32-clock`**：

```bash
# PlatformIO
pio run -t upload --upload-port esp32-clock.local
```

或者用 Arduino IDE：工具 → 端口里会出现 `esp32-clock (xxx.xxx.x.x)`，选它上传即可。
升级过程中屏幕上会显示进度条。

> 如果 OTA 连不上：确认电脑与设备在同一网段、路由器没开 AP 隔离；实在不行就插 FTDI 重刷（`IO0` 接地）。

---

## 6. 常见问题

| 现象 | 处理 |
|---|---|
| 串口打印 `未找到屏幕` | 检查 SDA=15 / SCL=14 是否接反、屏是否 3V3 供电；串口会打印全量扫描结果 |
| 屏全黑但扫到了 0x3C | 七针模块可能没配成 I2C 模式；或需要 `setContrast` 调高 |
| 烧录卡在 `Connecting...` | `IO0` 没接地 / FTDI TX-RX 没交叉 / 按一下 RST |
| 运行中反复重启 | 供电不足（brownout）：换 5V/2A，并 100µF 电容 |
| 花屏/雪花 | I2C 线太长或接触不良，换短线 |
| WiFi 一直连不上 | 确认是 2.4GHz、SSID 没有中文与空格 |

---

## 7. 文件结构

```
firmware/
├── platformio.ini
├── include/
│   ├── config.h      # 引脚 / WiFi / OTA / 版本
│   └── display.h
└── src/
    ├── main.cpp      # 启动流程、WiFi、OTA、主循环
    └── display.cpp   # U8g2 软件 I2C、自检图、M0 信息屏、OTA 进度屏
```

---

## 8. 下一步（M1）

- 用 SNTP 取真实时间（`CST-8`），替换 M0 的开机秒表
- 按 v6 布局绘制时间/日期/星期
- 1 秒局部刷新（`updateDisplayArea`），确认不闪


