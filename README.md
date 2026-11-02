# ESP32-CAM 桌面时钟 · esp32cam-clock

> 把吃灰的 **ESP32-CAM** 拔掉摄像头当普通 ESP32 用，外接一块 **128×64 SSD1315 OLED**，
> 做成一台显示 **时间 / 农历 / 天气** 的桌面小屏。

**固件版本 1.0.0-esp32cam ｜ 作者 shane**

```
        18:07:14            ← 黄色（面板 0–15 行为黄区）
      11-02 周一 九月廿四
   ─ ─ ─ ─ ─ ─ ─ ─
    ☀   16.3      H 20   L 13
                  68%     N1
WiFi OK                  18:07
```

---

## 功能

| 功能 | 说明 |
|---|---|
| 时钟 | SNTP（阿里云/腾讯/国际池），时区 `CST-8`，每 12h 重校 |
| 日期 | 公历 + 星期 + **农历**（1900–2100，支持闰月） |
| 天气 | Open-Meteo（苏州），30 分钟刷新，失败 1→5→30 分钟退避 |
| 天气内容 | 12 种图标 + 当前温度（一位小数）+ 今日最高/最低 + 相对湿度 + 风向风级 |
| 离线态 | 数据超 2h 加 `!` 标记；断网保留旧数据 |
| 配网 | 无凭证自动开热点 `ESP32-Clock`，手机打开 `192.168.4.1` 配网 |
| 升级 | 串口 / ArduinoOTA / **浏览器上传 .bin** |
| 自愈 | 堆过低 5 分钟、WiFi 断 30 分钟 → 自动重启 |

---

## 硬件

| 部件 | 规格 |
|---|---|
| 主控 | ESP32-CAM（AI-Thinker，4MB Flash + 4MB PSRAM，无 USB） |
| 屏幕 | OLED 128×64 **SSD1315**（I2C 四针，黄蓝双色） |
| 接线 | `SDA=GPIO15`、`SCL=GPIO14`、`VCC=3V3`、`GND=GND` |
| 供电 | 5V / 2A + 100µF（**不要**用 3V3 给板子供电） |

> I2C 只能挂 15/14 —— 这是 ESP32-CAM 10 个可用 GPIO 里唯一不踩启动 strapping 的组合，代价是 SD 卡不可用。

---

## 快速开始

```bat
:: 1) 编译
cd firmware
tools\build.cmd

:: 2) 烧录（三选一）
tools\build.cmd -Upload COM5              :: 串口（需 IO0 接地）
tools\build.cmd -Upload 192.168.1.23      :: OTA（需同网，无需 IO0）
:: 或浏览器打开 http://<设备IP>/update 上传 .bin
```

**首次配网**：上电 → 手机连热点 `ESP32-Clock` → 打开 `http://192.168.4.1` → 选 WiFi、填密码 → 自动重启。

> `config.h` 里的 WiFi 默认留空，所以开机会先进配网模式；也可以直接填好再烧。

---

## 文档

| 文档 | 内容 |
|---|---|
| [docs/README](docs/README.md) | 总览与索引 |
| [docs/11-软件架构](docs/11-软件架构.md) | **模块划分、启动/主循环流程、数据流、设计决策、资源占用** |
| [docs/10-HTTP接口](docs/10-HTTP接口.md) | **全部 HTTP API 与 curl 用法** |
| [docs/08-接线与刷机教程](docs/08-接线与刷机教程.md) | 接线图、三种烧录方式、配网、FAQ |
| [docs/04-单屏布局设计](docs/04-单屏布局设计.md) | 像素坐标、图标规则、黄区限制 |
| [docs/07-开发记录](docs/07-开发记录.md) | R01–R75 时间线与踩坑表 |
| [docs/05-开发计划](docs/05-开发计划.md) | 里程碑 M0–M6 与验收标准 |

---

## 目录结构

```
docs/           01–11 设计与开发文档
firmware/
  src/          应用源码（9 个 .cpp）
  include/      头文件 + config.h（WiFi/版本/作者/坐标）
  tools/        build.cmd / build.ps1（arduino-cli 封装）
  platformio.ini
archive/        里程碑快照（不纳入版本库）
```

---

## License

MIT © 2026 shane
