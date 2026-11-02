# -*- coding: utf-8 -*-
"""生成接线图：FTDI -> ESP32-CAM -> OLED（docs/images/08-接线图.png）"""
import os
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
W, H = 1500, 980
F = r"C:\Windows\Fonts\msyh.ttc"
FB = r"C:\Windows\Fonts\msyhbd.ttc"

BG = (255, 255, 255)
BOX = (245, 247, 250)
EDGE = (60, 70, 85)
TXT = (28, 32, 38)
SUB = (100, 108, 120)
C5V = (214, 60, 60)
CGND = (40, 44, 50)
CDATA = (40, 110, 210)
CIO0 = (230, 140, 20)
C3V3 = (30, 140, 90)


def f(size, bold=False):
    return ImageFont.truetype(FB if bold else F, size)


def box(d, x, y, w, h, title, subtitle=""):
    d.rounded_rectangle([x, y, x + w, y + h], radius=14, fill=BOX, outline=EDGE, width=3)
    d.text((x + w / 2, y + 26), title, font=f(26, True), fill=TXT, anchor="ma")
    if subtitle:
        d.text((x + w / 2, y + 62), subtitle, font=f(18), fill=SUB, anchor="ma")


def pin(d, x, y, label, color=TXT, side="right"):
    d.ellipse([x - 7, y - 7, x + 7, y + 7], fill=color, outline=EDGE, width=2)
    if side == "right":
        d.text((x + 14, y), label, font=f(20), fill=TXT, anchor="lm")
    else:
        d.text((x - 14, y), label, font=f(20), fill=TXT, anchor="rm")


def route(d, pts, color, width=4):
    d.line(pts, fill=color, width=width, joint="curve")


def note(d, x, y, text, color=SUB, size=18, anchor="lm"):
    d.text((x, y), text, font=f(size), fill=color, anchor=anchor)


def main():
    img = Image.new("RGB", (W, H), BG)
    d = ImageDraw.Draw(img)

    d.text((40, 28), "ESP32-CAM  ·  SSD1315 OLED  ·  FTDI 接线图（M0）",
           font=f(32, True), fill=TXT, anchor="la")

    # ---------------- FTDI ----------------
    fx, fw = 60, 220
    box(d, fx, 250, fw, 300, "FTDI 编程器", "USB 转串口")
    fy5, fyg, fyt, fyr = 330, 390, 450, 510
    pin(d, fx + fw, fy5, "5V", C5V, "left")
    pin(d, fx + fw, fyg, "GND", CGND, "left")
    pin(d, fx + fw, fyt, "TX", CDATA, "left")
    pin(d, fx + fw, fyr, "RX", CDATA, "left")

    # ---------------- ESP32-CAM ----------------
    ex, ew = 600, 330
    box(d, ex, 160, ew, 500, "ESP32-CAM", "AI-Thinker · 拔掉摄像头")
    e5, eg, er, et, ei = 280, 340, 400, 460, 540
    pin(d, ex, e5, "5V", C5V, "right")
    pin(d, ex, eg, "GND", CGND, "right")
    pin(d, ex, er, "U0R", CDATA, "right")
    pin(d, ex, et, "U0T", CDATA, "right")
    pin(d, ex, ei, "IO0", CIO0, "right")

    e3v, e14, e15, eg2 = 280, 400, 460, 540
    pin(d, ex + ew, e3v, "3V3", C3V3, "left")
    pin(d, ex + ew, e14, "GPIO14", CDATA, "left")
    pin(d, ex + ew, e15, "GPIO15", CDATA, "left")
    pin(d, ex + ew, eg2, "GND", CGND, "left")

    # ---------------- OLED ----------------
    ox, ow = 1180, 240
    box(d, ox, 250, ow, 300, "SSD1315 OLED", "128×64  I2C")
    ov, ocl, oda, og = 330, 390, 450, 510
    pin(d, ox, ov, "VCC", C3V3, "right")
    pin(d, ox, ocl, "SCL", CDATA, "right")
    pin(d, ox, oda, "SDA", CDATA, "right")
    pin(d, ox, og, "GND", CGND, "right")

    # ---------------- 左侧：FTDI ↔ ESP32-CAM ----------------
    route(d, [(fx + fw, fy5), (470, fy5), (470, e5), (ex, e5)], C5V)
    route(d, [(fx + fw, fyg), (490, fyg), (490, eg), (ex, eg)], CGND)
    route(d, [(fx + fw, fyt), (510, fyt), (510, er), (ex, er)], CDATA)
    route(d, [(fx + fw, fyr), (530, fyr), (530, et), (ex, et)], CDATA)

    # IO0 -> 接地符号
    gx = 470
    route(d, [(ex, ei), (gx, ei), (gx, 640)], CIO0)
    for i, wdt in enumerate((56, 38, 20)):
        d.line([(gx - wdt // 2, 650 + i * 12), (gx + wdt // 2, 650 + i * 12)], fill=CIO0, width=4)
    note(d, gx + 42, 596, "IO0 只在烧录时接 GND", CIO0, 19)
    note(d, gx + 42, 624, "烧完拔掉，按 RST 运行", CIO0, 18)

    # ---------------- 右侧：ESP32-CAM ↔ OLED ----------------
    route(d, [(ex + ew, e3v), (1040, e3v), (1040, ov), (ox, ov)], C3V3)
    route(d, [(ex + ew, e14), (1060, e14), (1060, ocl), (ox, ocl)], CDATA)
    route(d, [(ex + ew, e15), (1080, e15), (1080, oda), (ox, oda)], CDATA)
    route(d, [(ex + ew, eg2), (1100, eg2), (1100, og), (ox, og)], CGND)

    # ---------------- 标注 ----------------
    note(d, 300, 250, "5V（FTDI 跳线选 5V）", C5V, 18)
    note(d, 900, 238, "VCC 接 3V3", C3V3, 18)
    note(d, 880, 374, "SCL = GPIO14", CDATA, 18)
    note(d, 880, 488, "SDA = GPIO15", CDATA, 18)

    # ---------------- 底部说明 ----------------
    lines = [
        "① 屏只接 4 根线：VCC→3V3、GND→GND、SCL→GPIO14、SDA→GPIO15（模块丝印顺序可能不同，按丝印接）",
        "② FTDI 交叉接：TX→U0R、RX→U0T；供电 5V/2A（不要用 3V3 供板子），建议 5V-GND 并 100µF",
        "③ 接屏后 SD 卡不可用（14/15 被 I2C 占用）；摄像头可保留，但固件不要初始化它",
    ]
    y = 806
    for t in lines:
        d.text((60, y), t, font=f(19), fill=SUB, anchor="la")
        y += 46

    out = os.path.join(HERE, "08-接线图.png")
    img.save(out)
    print("saved", out)


if __name__ == "__main__":
    main()

