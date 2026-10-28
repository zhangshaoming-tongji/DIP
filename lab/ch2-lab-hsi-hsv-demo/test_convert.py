# -*- coding: utf-8 -*-
"""Round-trip checks for the demo's HSI / HSV formulas."""
import math

def clamp01(x):
    return 0.0 if x < 0 else 1.0 if x > 1 else x

def wrap_hue(h):
    return h % 360.0

def rgb_to_hsv(r, g, b):
    mx, mn = max(r, g, b), min(r, g, b)
    d = mx - mn
    h = 0.0
    if d > 1e-8:
        if mx == r:
            h = 60 * (((g - b) / d) % 6)
        elif mx == g:
            h = 60 * ((b - r) / d + 2)
        else:
            h = 60 * ((r - g) / d + 4)
    if h < 0:
        h += 360
    s = 0.0 if mx < 1e-8 else d / mx
    return h, s, mx

def hsv_to_rgb(h, s, v):
    h = wrap_hue(h)
    c = v * s
    x = c * (1 - abs((h / 60) % 2 - 1))
    m = v - c
    if h < 60:
        r, g, b = c, x, 0
    elif h < 120:
        r, g, b = x, c, 0
    elif h < 180:
        r, g, b = 0, c, x
    elif h < 240:
        r, g, b = 0, x, c
    elif h < 300:
        r, g, b = x, 0, c
    else:
        r, g, b = c, 0, x
    return clamp01(r + m), clamp01(g + m), clamp01(b + m)

def rgb_to_hsi(r, g, b):
    ssum = r + g + b
    i = ssum / 3
    if ssum < 1e-8:
        return 0.0, 0.0, 0.0
    s = 1 - 3 * min(r, g, b) / ssum
    num = 0.5 * ((r - g) + (r - b))
    den = math.sqrt((r - g) ** 2 + (r - b) * (g - b))
    h = 0.0
    if den > 1e-8:
        theta = math.acos(min(1, max(-1, num / den)))
        h = math.degrees(theta) if b <= g else 360 - math.degrees(theta)
    return h, clamp01(s), clamp01(i)

def hsi_to_rgb(h, s, i):
    h = wrap_hue(h)
    s, i = clamp01(s), clamp01(i)
    def sector(hh):
        return i * (1 + (s * math.cos(math.radians(hh))) / math.cos(math.radians(60 - hh)))
    if h < 120:
        b = i * (1 - s)
        r = sector(h)
        g = 3 * i - (r + b)
    elif h < 240:
        r = i * (1 - s)
        g = sector(h - 120)
        b = 3 * i - (r + g)
    else:
        g = i * (1 - s)
        b = sector(h - 240)
        r = 3 * i - (g + b)
    return clamp01(r), clamp01(g), clamp01(b)

def err(a, b):
    return max(abs(x - y) for x, y in zip(a, b))

samples = [
    (1, 0, 0),
    (0, 1, 0),
    (0, 0, 1),
    (1, 1, 1),
    (0, 0, 0),
    (0.8, 0.2, 0.15),  # baboon-like red nose
    (0.25, 0.45, 0.75),  # blue cheek
    (0.4, 0.55, 0.2),
]

ok = True
for rgb in samples:
    back = hsv_to_rgb(*rgb_to_hsv(*rgb))
    e = err(rgb, back)
    print(f"HSV {rgb} -> {tuple(round(x,4) for x in back)} err={e:.4f}")
    if e > 1e-6:
        ok = False
    back = hsi_to_rgb(*rgb_to_hsi(*rgb))
    e = err(rgb, back)
    print(f"HSI {rgb} -> {tuple(round(x,4) for x in back)} err={e:.4f}")
    if e > 2e-2:  # HSI inverse is slightly less tight at extremes
        ok = False

# I vs V on a saturated red
hsi = rgb_to_hsi(0.8, 0.15, 0.12)
hsv = rgb_to_hsv(0.8, 0.15, 0.12)
print("red-nose HSI I=%.3f  HSV V=%.3f" % (hsi[2], hsv[2]))
assert hsi[2] < hsv[2], "I should be lower than V on a saturated color"
print("PASS" if ok else "FAIL")
