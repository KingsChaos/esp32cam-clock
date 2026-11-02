# -*- coding: utf-8 -*-
"""
生成 128x64 单屏布局示意图（黄蓝双色 OLED）。
运行：python _make_mockups.py
输出：01-mockup-A-双色.png / 02-mockup-B-温度入黄区.png / 03-布局规格.png
"""
import os
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
W, H = 128, 64
SCALE = 6
YELLOW = (255, 198, 20)
BLUE = (69, 210, 255)
BG = (5, 5, 6)

F_LATIN = r"C:\Windows\Fonts\consolab.ttf"
F_HAN = r"C:\Windows\Fonts\msyh.ttc"
F_ANNO = r"C:\Windows\Fonts\msyh.ttc"

YELLOW_END = 15   # 0..15 = 黄色区（16 行）
Y_DIV = 15
Y_BOX = 30
BOX_X0, BOX_X1 = 1, 126


def ink_mask(text, font_path, size, threshold=96):
    font = ImageFont.truetype(font_path, size)
    tmp = Image.new("L", (600, 100), 0)
    ImageDraw.Draw(tmp).text((6, 6), text, font=font, fill=255, anchor="la")
    bbox = tmp.getbbox()
    if not bbox:
        return None, 0, 0
    crop = tmp.crop(bbox)
    return crop.point(lambda v: 255 if v > threshold else 0).convert("1"), crop.width, crop.height


def fit(text, font_path, target_h, lo=6, hi=40):
    """二分找一个字号，使渲染后墨迹高度尽量等于 target_h"""
    best = None
    for size in range(lo, hi + 1):
        m, w, h = ink_mask(text, font_path, size)
        if m is None:
            continue
        if best is None or abs(h - target_h) < abs(best[2] - target_h):
            best = (m, w, h)
        if h >= target_h:
            break
    return best


def put(canvas, mask, x, top, color):
    if mask is None:
        return
    layer = Image.new("RGB", mask.size, color)
    canvas.paste(layer, (x, top), mask)


def dashed(canvas, y, color=YELLOW):
    d = ImageDraw.Draw(canvas)
    x = 3
    while x < W - 3:
        d.line([(x, y), (min(x + 4, W - 3), y)], fill=color)
        x += 7


def cloud(canvas, x, y, size=24, color=BLUE):
    m = Image.new("L", (size, size), 0)
    d = ImageDraw.Draw(m)
    s = size / 24.0
    d.ellipse([2 * s, 11 * s, 12 * s, 21 * s], fill=255)
    d.ellipse([8 * s, 5 * s, 21 * s, 18 * s], fill=255)
    d.ellipse([15 * s, 10 * s, 24 * s, 21 * s], fill=255)
    d.rectangle([5 * s, 15 * s, 22 * s, 21 * s], fill=255)
    d.rectangle([0, 21 * s, size, size], fill=0)
    mask = m.point(lambda v: 255 if v > 96 else 0).convert("1")
    put(canvas, mask, x, y, color)


def line_row(canvas, text, x, top, target_h, color, font=F_LATIN):
    m, w, h = fit(text, font, target_h)
    put(canvas, m, x, top, color)
    return w, h


def build(layout="A", han="周日 多云"):
    c = Image.new("RGB", (W, H), BG)

    # ---- 黄区：时间（+ 方案 B 的温度）----
    if layout in ("A", "C"):
        tm0, tw, th = fit("23:45:07", F_LATIN, 13)
        put(c, tm0, (W - tw) // 2, 14 - th, YELLOW)
    else:
        tm0, tw, th = fit("23:45:07", F_LATIN, 13)
        put(c, tm0, 3, 14 - th, YELLOW)
        tm, tmw, tmh = fit("23°", F_LATIN, 13)
        put(c, tm, W - 4 - tmw, 14 - tmh, YELLOW)

    dashed(c, Y_DIV)

    # ---- 蓝区：日期行（拉丁数字 + 中文）----
    dm, dw, dh = fit("11-01", F_LATIN, 9)
    hm, hw, hh = fit(han, F_HAN, 11)
    gap = 4
    total = dw + gap + hw
    x0 = max(2, (W - total) // 2)
    put(c, dm, x0, 28 - dh, BLUE)
    put(c, hm, x0 + dw + gap, 28 - hh, BLUE)

    # ---- 蓝区：天气框 ----
    ImageDraw.Draw(c).rounded_rectangle([BOX_X0, Y_BOX, BOX_X1, H - 1], radius=2, outline=BLUE, width=1)

    if layout in ("A", "C"):
        cloud(c, 4, 34, 24)
        line_row(c, "23°", 32, 40, 13, BLUE)
        xs = 70
    else:
        cloud(c, 8, 33, 27)
        xs = 50

    # ---- 蓝区：右侧三行 ----
    for i, t in enumerate(("H 30°", "L 20°", "E 2")):
        line_row(c, t, xs, 32 + i * 10, 7, BLUE)

    # ---- 物理约束：黄区只能黄，蓝区只能蓝 ----
    px = c.load()
    for y in range(H):
        for x in range(W):
            r, g, b = px[x, y]
            if r + g + b < 40:
                px[x, y] = (0, 0, 0)
            elif y <= YELLOW_END:
                px[x, y] = YELLOW
            else:
                px[x, y] = BLUE
    return c


def build_annotation(img):
    s = 7
    big = img.resize((W * s, H * s), Image.NEAREST)
    pad_l, pad_r, pad_t, pad_b = 185, 40, 74, 96
    cv = Image.new("RGB", (W * s + pad_l + pad_r, H * s + pad_t + pad_b), (24, 26, 30))
    cv.paste(big, (pad_l, pad_t))
    d = ImageDraw.Draw(cv)
    f = ImageFont.truetype(F_ANNO, 15)
    fb = ImageFont.truetype(F_ANNO, 18)

    def ry(row):
        return pad_t + row * s

    bands = [
        (0, 15, "时间 16px 字体 黄", YELLOW),
        (17, 29, "日期 12px 字体 蓝", BLUE),
        (30, 63, "天气框 蓝", BLUE),
    ]
    for r0, r1, label, col in bands:
        y0, y1 = ry(r0), ry(r1) - 1
        d.line([(pad_l - 12, y0), (pad_l - 12, y1)], fill=col, width=2)
        d.line([(pad_l - 17, y0), (pad_l - 7, y0)], fill=col, width=2)
        d.line([(pad_l - 17, y1), (pad_l - 7, y1)], fill=col, width=2)
        d.text((pad_l - 22, (y0 + y1) // 2), label, font=f, fill=col, anchor="rm")

    for r, lab, col in ((0, "y0", (140, 146, 154)), (15, "y15 虚线", YELLOW),
                        (17, "y17", (140, 146, 154)), (30, "y30", (140, 146, 154)),
                        (63, "y63", (140, 146, 154))):
        d.text((pad_l - 32, ry(r) + s // 2), lab, font=f, fill=col, anchor="rm")

    marks = [(4, 28, "图标 24×24", 0), (32, 62, "实时温度 16px 字体", 1), (70, 125, "三行 8px 字体 行距10", 2)]
    for x0, x1, label, i in marks:
        yy = pad_t + H * s + 18 + i * 25
        d.line([(pad_l + x0 * s, pad_t + H * s + 2), (pad_l + x0 * s, yy - 4)], fill=(96, 102, 110))
        d.line([(pad_l + x1 * s, pad_t + H * s + 2), (pad_l + x1 * s, yy - 4)], fill=(96, 102, 110))
        d.text((pad_l + x0 * s, yy), "x%d–%d  %s" % (x0, x1, label), font=f, fill=(190, 196, 204), anchor="la")

    d.text((pad_l, 20), "128×64 单屏布局 · 方案 A（黄蓝双色面板：0–15 行黄 / 16–63 行蓝）",
           font=fb, fill=(235, 238, 242), anchor="la")
    d.text((pad_l + W * s - 2, pad_t + H * s + 2), "128 px", font=f, fill=(128, 134, 142), anchor="ra")
    return cv


def main():
    a, b = build("A"), build("B")
    c = build("C", han="周日 十月初八")
    a.resize((W * SCALE, H * SCALE), Image.NEAREST).save(os.path.join(HERE, "01-mockup-A-双色.png"))
    b.resize((W * SCALE, H * SCALE), Image.NEAREST).save(os.path.join(HERE, "02-mockup-B-温度入黄区.png"))
    c.resize((W * SCALE, H * SCALE), Image.NEAREST).save(os.path.join(HERE, "04-mockup-C-农历版.png"))
    build_annotation(a).save(os.path.join(HERE, "03-布局规格.png"))
    print("ok")


if __name__ == "__main__":
    main()






