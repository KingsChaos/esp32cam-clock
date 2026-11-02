# -*- coding: utf-8 -*-
"""
v5 布局：128x64 黄蓝双色 OLED 单屏
  2-15   时间：7 段数码风格，黄
  16-18  空行（时间与日期之间的分隔，不再画虚线）
  19-28  日期 + 星期 + 农历 12px 蓝
  30     蓝色虚线
  31-54  天气行：左 24x24 图标 + 16px 温度；右两行（H 30 / L 20，60% / E2）
         两行左端 x=62、右端 x=125 对齐；与图标上下对齐
  56-62  状态行：WiFi OK  |  23:45

运行：python _make_v5.py
"""
import os
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "v5")
W, H = 128, 64
SCALE = 6
YELLOW = (255, 198, 20)
BLUE = (69, 210, 255)
BG = (5, 5, 6)

F_BOLD = r"C:\Windows\Fonts\consolab.ttf"
F_REG = r"C:\Windows\Fonts\consola.ttf"
F_HAN = r"C:\Windows\Fonts\msyh.ttc"

YELLOW_END = 15

# 7 段数码：数字 -> 亮起的段
SEG = {
    "0": "abcdef", "1": "bc", "2": "abdeg", "3": "abcdg", "4": "bcfg",
    "5": "acdfg", "6": "acdefg", "7": "abc", "8": "abcdefg", "9": "abcdfg",
}


def ink_mask(text, font_path, size, threshold=96):
    font = ImageFont.truetype(font_path, size)
    tmp = Image.new("L", (760, 120), 0)
    ImageDraw.Draw(tmp).text((8, 8), text, font=font, fill=255, anchor="la")
    bbox = tmp.getbbox()
    if not bbox:
        return None, 0, 0
    crop = tmp.crop(bbox)
    return crop.point(lambda v: 255 if v > threshold else 0).convert("1"), crop.width, crop.height


def fit(text, font_path, target_h, lo=5, hi=44):
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


# ---------------- 7 段数码 ----------------
DIG_W, DIG_H, TH, GAP, COL_W = 10, 13, 2, 2, 3


def seg_rects(x, y, ch):
    """返回某个字符的段矩形列表（相对左上角）"""
    w, h, t = DIG_W, DIG_H, TH
    rects = {
        "a": (x + 1, y, x + w - 1, y + t),
        "g": (x + 1, y + h // 2 - t // 2 - 1, x + w - 1, y + h // 2 + t // 2 + 1),
        "d": (x + 1, y + h - t, x + w - 1, y + h),
        "f": (x, y + 1, x + t, y + h // 2 - 1),
        "b": (x + w - t, y + 1, x + w, y + h // 2 - 1),
        "e": (x, y + h // 2 + 1, x + t, y + h - 1),
        "c": (x + w - t, y + h // 2 + 1, x + w, y + h - 1),
    }
    if ch == ":":
        return [(x, y + 3, x + 2, y + 6), (x, y + 9, x + 2, y + 12)]
    if ch not in SEG:
        return []
    return [rects[s] for s in SEG[ch]]


def seg_width(ch):
    return COL_W if ch == ":" else DIG_W


def text7_width(s):
    return sum(seg_width(c) for c in s) + GAP * (len(s) - 1)


def draw_text7(canvas, s, x, y, color=YELLOW):
    m = Image.new("L", (text7_width(s) + 2, DIG_H + 2), 0)
    d = ImageDraw.Draw(m)
    cx = 0
    for ch in s:
        for r in seg_rects(cx, 0, ch):
            d.rectangle(r, fill=255)
        cx += seg_width(ch) + GAP
    mask = m.point(lambda v: 255 if v > 96 else 0).convert("1")
    put(canvas, mask, x, y, color)


def build(online=True):
    c = Image.new("RGB", (W, H), BG)

    # 时间（7 段数码）2..14
    tw = text7_width("23:45:07")
    draw_text7(c, "23:45:07", (W - tw) // 2, 2, YELLOW)

    # 空行 15..17 → 日期行 18..27（居中）
    dm, dw, dh = fit("11-02", F_REG, 9)
    hm, hw, hh = fit("周一 九月廿四", F_HAN, 10)
    gap = 4
    total = dw + gap + hw
    x0 = (W - total) // 2
    put(c, dm, x0, 28 - dh)
    put(c, hm, x0 + dw + gap, 28 - hh)

    dashed(c, 31, BLUE)

    # 天气行 32..53：图标 22 + 温度
    cloud(c, 3, 33, 22)
    tm, tw2, th2 = fit("23", F_BOLD, 13)
    put(c, tm, 27, 49 - th2)

    # 右栏两行 34..52：左端 64、右端 125 对齐
    XL, XR = 62, 125
    for top, left, right in ((35, "H 30", "L 20"), (45, "60%", "E2")):
        lm, lw, lh = fit(left, F_REG, 7)
        rm, rw, rh = fit(right, F_REG, 7)
        put(c, lm, XL, top)
        put(c, rm, XR - rw, top + (lh - rh))

    # 状态行 56..62
    sm, sw, sh = fit("WiFi OK" if online else "WiFi NO", F_REG, 7)
    put(c, sm, 3, 62 - sh)
    upm, upw, uph = fit("23:45" if online else "23:02", F_REG, 7)
    put(c, upm, W - 3 - upw, 62 - uph)

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
    pad_l, pad_r, pad_t, pad_b = 235, 40, 74, 104
    cv = Image.new("RGB", (W * s + pad_l + pad_r, H * s + pad_t + pad_b), (24, 26, 30))
    cv.paste(big, (pad_l, pad_t))
    d = ImageDraw.Draw(cv)
    f = ImageFont.truetype(F_HAN, 15)
    fb = ImageFont.truetype(F_HAN, 18)

    def ry(r):
        return pad_t + r * s

    bands = [
        (2, 15, "时间 7 段数码 12px", YELLOW),
        (16, 18, "空行（分隔）", (140, 146, 154)),
        (19, 28, "日期 + 星期 + 农历", BLUE),
        (33, 54, "天气行", BLUE),
        (56, 62, "状态行", BLUE),
    ]
    for r0, r1, label, col in bands:
        y0, y1 = ry(r0), ry(r1)
        d.line([(pad_l - 12, y0), (pad_l - 12, y1)], fill=col, width=2)
        d.line([(pad_l - 17, y0), (pad_l - 7, y0)], fill=col, width=2)
        d.line([(pad_l - 17, y1), (pad_l - 7, y1)], fill=col, width=2)
        d.text((pad_l - 24, (y0 + y1) // 2), label, font=f, fill=col, anchor="rm")

    for r, lab, col in ((0, "y0", (140, 146, 154)), (31, "y31 ─ 蓝虚线", BLUE),
                        (63, "y63", (140, 146, 154))):
        d.text((pad_l - 34, ry(r) + s // 2), lab, font=f, fill=col, anchor="rm")

    marks = [(3, 24, "图标 22×22"), (27, 54, "温度 16px"), (62, 125, "右栏两行 7px（头尾对齐）")]
    for i, (x0, x1, label) in enumerate(marks):
        yy = pad_t + H * s + 16 + i * 22
        d.line([(pad_l + x0 * s, pad_t + H * s + 2), (pad_l + x0 * s, yy - 4)], fill=(96, 102, 110))
        d.text((pad_l + x0 * s, yy), "x%d–%d  %s" % (x0, x1, label), font=f, fill=(190, 196, 204), anchor="la")

    d.text((pad_l, 20), "128×64 单屏布局 v5 · 7 段数码时间 · 无虚线分隔 · 右栏头尾对齐",
           font=fb, fill=(235, 238, 242), anchor="la")
    return cv


def main():
    os.makedirs(OUT, exist_ok=True)
    on, off = build(True), build(False)
    on.resize((W * SCALE, H * SCALE), Image.NEAREST).save(os.path.join(OUT, "01-在线.png"))
    off.resize((W * SCALE, H * SCALE), Image.NEAREST).save(os.path.join(OUT, "02-离线.png"))
    annotate(on).save(os.path.join(OUT, "03-规格.png"))
    print("ok")


if __name__ == "__main__":
    main()










