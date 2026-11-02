# -*- coding: utf-8 -*-
"""
v2 布局（定稿方向）：128x64 黄蓝双色 OLED 单屏
  0-14   时间 16px 黄
  15     黄色虚线分隔
  17-26  日期 + 星期 12px 蓝（中文）
  27     蓝色虚线分隔（黄区以下只能是蓝）
  28-47  天气行：20x20 图标 + 16px 温度 + 12px 天气词
  48-55  H30° L20° 60% E2  (8px)
  56-63  WiFi OK ....... 23:45 (8px，右下为最后更新时间)

运行：python _make_mockups_v2.py
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

YELLOW_END = 15
Y_DIV1 = 15
Y_DIV2 = 27


def ink_mask(text, font_path, size, threshold=96):
    font = ImageFont.truetype(font_path, size)
    tmp = Image.new("L", (700, 110), 0)
    ImageDraw.Draw(tmp).text((8, 8), text, font=font, fill=255, anchor="la")
    bbox = tmp.getbbox()
    if not bbox:
        return None, 0, 0
    crop = tmp.crop(bbox)
    return crop.point(lambda v: 255 if v > threshold else 0).convert("1"), crop.width, crop.height


def fit(text, font_path, target_h, lo=6, hi=44):
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


def put(canvas, mask, x, top, color=BLUE):
    if mask is None:
        return
    layer = Image.new("RGB", mask.size, color)
    canvas.paste(layer, (x, top), mask)


def dashed(canvas, y, color):
    d = ImageDraw.Draw(canvas)
    x = 3
    while x < W - 3:
        d.line([(x, y), (min(x + 4, W - 3), y)], fill=color)
        x += 7


def cloud(canvas, x, y, size=20, color=BLUE):
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


def build(online=True):
    c = Image.new("RGB", (W, H), BG)

    # 时间：16px 字体，墨迹 13px，水平居中
    tm, tw, th = fit("23:45:07", F_LATIN, 13)
    put(c, tm, (W - tw) // 2, 14 - th, YELLOW)

    dashed(c, Y_DIV1, YELLOW)

    # 日期：拉丁 9px + 中文 10px，底对齐 y=26
    dm, dw, dh = fit("11-02", F_LATIN, 9)
    hm, hw, hh = fit("周一", F_HAN, 10)
    gap = 5
    put(c, dm, 3, 25 - dh)
    put(c, hm, 3 + dw + gap, 25 - hh)

    dashed(c, Y_DIV2, BLUE)

    # 天气行：图标 20x20 + 温度 16px + 天气词
    cloud(c, 3, 28, 20)
    tm2, tw2, th2 = fit("23°", F_LATIN, 13)
    put(c, tm2, 27, 43 - th2)
    wm, ww, wh = fit("多云", F_HAN, 11)
    put(c, wm, 27 + tw2 + 6, 43 - wh)

    # 数据行：H/L + 降雨概率 + 风力
    m, w, h = fit("H30° L20°  60% E2", F_LATIN, 7)
    put(c, m, 3, 55 - h)

    # 状态行：WiFi + 最后更新时间
    sm, sw, sh = fit("WiFi OK" if online else "WiFi NO", F_LATIN, 7)
    put(c, sm, 3, 63 - sh)
    upm, upw, uph = fit("23:45" if online else "23:02", F_LATIN, 7)
    put(c, upm, W - 3 - upw, 63 - uph)

    # 物理约束：0-15 只能黄，16-63 只能蓝
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


def annotate(img):
    s = 7
    big = img.resize((W * s, H * s), Image.NEAREST)
    pad_l, pad_r, pad_t, pad_b = 205, 40, 74, 104
    cv = Image.new("RGB", (W * s + pad_l + pad_r, H * s + pad_t + pad_b), (24, 26, 30))
    cv.paste(big, (pad_l, pad_t))
    d = ImageDraw.Draw(cv)
    f = ImageFont.truetype(F_HAN, 15)
    fb = ImageFont.truetype(F_HAN, 18)

    def ry(r):
        return pad_t + r * s

    bands = [
        (1, 14, "时间 16px", YELLOW),
        (17, 25, "日期 12px", BLUE),
        (28, 47, "天气行", BLUE),
        (48, 55, "数据行", BLUE),
        (56, 63, "状态行", BLUE),
    ]
    for r0, r1, label, col in bands:
        y0, y1 = ry(r0), ry(r1)
        d.line([(pad_l - 12, y0), (pad_l - 12, y1)], fill=col, width=2)
        d.line([(pad_l - 17, y0), (pad_l - 7, y0)], fill=col, width=2)
        d.line([(pad_l - 17, y1), (pad_l - 7, y1)], fill=col, width=2)
        d.text((pad_l - 24, (y0 + y1) // 2), label, font=f, fill=col, anchor="rm")

    for r, lab, col in ((0, "y0", (140, 146, 154)), (15, "y15 ─ 黄虚线", YELLOW),
                        (27, "y27 ─ 蓝虚线", BLUE), (63, "y63", (140, 146, 154))):
        d.text((pad_l - 34, ry(r) + s // 2), lab, font=f, fill=col, anchor="rm")

    marks = [(3, 23, "图标 20×20"), (27, 55, "温度 16px"), (56, 82, "天气词 12px"),
             (3, 105, "H/L + 降雨概率 + 风力"), (3, 45, "WiFi OK / NO"), (96, 125, "最后更新时间")]
    for i, (x0, x1, label) in enumerate(marks[:4]):
        yy = pad_t + H * s + 16 + i * 22
        d.line([(pad_l + x0 * s, pad_t + H * s + 2), (pad_l + x0 * s, yy - 4)], fill=(96, 102, 110))
        d.text((pad_l + x0 * s, yy), "x%d–%d  %s" % (x0, x1, label), font=f, fill=(190, 196, 204), anchor="la")

    d.text((pad_l, 20), "128×64 单屏布局 v2 · 无边框 · 双虚线分区 · 底部状态行",
           font=fb, fill=(235, 238, 242), anchor="la")
    return cv


def main():
    on = build(True)
    off = build(False)
    on.resize((W * SCALE, H * SCALE), Image.NEAREST).save(os.path.join(HERE, "05-布局v2-在线.png"))
    off.resize((W * SCALE, H * SCALE), Image.NEAREST).save(os.path.join(HERE, "06-布局v2-离线.png"))
    annotate(on).save(os.path.join(HERE, "07-布局v2-规格.png"))
    print("ok")


if __name__ == "__main__":
    main()

