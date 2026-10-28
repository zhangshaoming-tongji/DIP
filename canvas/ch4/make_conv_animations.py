# -*- coding: utf-8 -*-
"""Animated schematics of 1-D and 2-D convolution (APNG + GIF) for ch4 slides.

Outputs (canvas/ch4/):
  conv1d_animation.png / conv1d_animation.gif  -- flip & slide, multiply, sum
  conv2d_animation.png / conv2d_animation.gif  -- sliding 3x3 window on 6x6 image
  _preview_conv1d.png, _preview_conv2d.png     -- one static frame for checking
"""
from pathlib import Path
import numpy as np
import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib import font_manager
from matplotlib.patches import Rectangle
from PIL import Image

ROOT = Path(r"D:\works\Others\Teaching\3-Under-DigitalImageProcessing\课件v2")
OUTDIR = ROOT / "canvas" / "ch4"
DPI = 150  # 12.8 x 7.2 in -> 1920 x 1080, fits 16:9 slides

NAVY = "#1B4E79"
RED = "#B42024"
DARK = "#222222"
MUTED = "#5A5A5A"
GRID = "#C5CDD4"
YELLOW = "#FFF3D6"
GHOST = "#9AA5B1"


def fp(size=16, weight="normal"):
    return font_manager.FontProperties(fname=r"C:\Windows\Fonts\msyh.ttc", size=size, weight=weight)


def fp_bold(size=16):
    return font_manager.FontProperties(fname=r"C:\Windows\Fonts\msyhbd.ttc", size=size)


def fmt(x):
    r = round(x)
    return str(int(r)) if abs(x - r) < 1e-9 else f"{x:.1f}"


def mix(h1, h2, t):
    a = [int(h1[i:i + 2], 16) for i in (1, 3, 5)]
    b = [int(h2[i:i + 2], 16) for i in (1, 3, 5)]
    c = [round(a[i] + (b[i] - a[i]) * t) for i in range(3)]
    return "#" + "".join(f"{v:02X}" for v in c)


def heat(v):
    return mix("#FFFFFF", "#1565C0", 0.10 + 0.80 * (v / 9.0))


def render(fig):
    fig.canvas.draw()
    buf = np.asarray(fig.canvas.buffer_rgba())
    img = Image.fromarray(buf[:, :, :3].copy())
    plt.close(fig)
    return img


def save_anim(frames, durs, png_path, gif_path):
    frames[0].save(png_path, save_all=True, append_images=frames[1:],
                   duration=durs, loop=0, optimize=False)
    gframes = [fr.convert("P", palette=Image.ADAPTIVE) for fr in frames]
    gframes[0].save(gif_path, save_all=True, append_images=gframes[1:],
                    duration=durs, loop=0, optimize=False)
    print(png_path, Image.open(png_path).n_frames, "frames")
    print(gif_path)


# ----------------------------------------------------------------------
# 1-D convolution:  y[n] = sum_k f[k] g[n-k]
# kernel g[-1..1] = [0.5, 0.3, 0.2]; flipped view sliding on f is [0.2, 0.3, 0.5]
F = [2, 2, 2, 2, 6, 6, 6, 6, 2, 2, 2, 2, 4, 4, 4, 4]
W = [0.2, 0.3, 0.5]           # weights over f[n-1], f[n], f[n+1] (kernel already flipped)
N = len(F)
Y = [sum(w * (F[n + d] if 0 <= n + d < N else 0)
         for w, d in zip(W, (-1, 0, 1))) for n in range(N)]


def comp_str(n):
    tf, tn, tp = [], [], []
    for w, d in zip(W, (-1, 0, 1)):
        k = n + d
        fk = F[k] if 0 <= k < N else 0
        tf.append(f"{w}·f[{k}]")
        tn.append(f"{w}×{fk}")
        tp.append(fmt(w * fk))
    s = f"y[{n}] = " + " + ".join(tf) + " = " + " + ".join(tn) \
        + " = " + " + ".join(tp) + f" = {fmt(Y[n])}"
    if n == 0 or n == N - 1:
        s += "   (边界补零)"
    return s


def frame_1d(step):
    """step: -1 intro, 0..N-1 compute, N final."""
    fig = plt.figure(figsize=(12.8, 7.2), dpi=DPI, facecolor="white")
    fig.text(0.5, 0.965, "一维卷积：翻转 · 平移 · 相乘 · 求和",
             ha="center", va="top", fontproperties=fp_bold(23), color=NAVY)
    fig.text(0.5, 0.922,
             "y[n] = Σ f[k]·g[n-k]；核 g = [g[-1], g[0], g[1]] = [0.5, 0.3, 0.2]，"
             "翻转后 [0.2, 0.3, 0.5]（红色）沿 f 逐点滑动，边界补零",
             ha="center", va="top", fontproperties=fp(12.5), color=MUTED)

    # upper: signal + sliding flipped kernel
    ax1 = fig.add_axes([0.06, 0.545, 0.87, 0.28])
    ax2 = ax1.twinx()
    ax1.set_xlim(-2, 17)
    ax1.set_ylim(0, 7.4)
    ax2.set_xlim(-2, 17)
    ax2.set_ylim(0, 0.74)
    ax2.patch.set_visible(False)
    ax1.set_xticks(range(-1, 17))
    ax1.tick_params(labelsize=9)
    ax2.set_yticks([0.2, 0.3, 0.5])
    ax2.tick_params(labelsize=8, colors=RED)
    ax1.grid(axis="y", ls=":", alpha=0.4)
    ax1.vlines(range(N), 0, F, colors=NAVY, lw=2.2)
    ax1.plot(range(N), F, "o", color=NAVY, ms=5)
    ax1.set_ylabel("信号 f", fontproperties=fp(12), color=NAVY)
    ax2.set_ylabel("核 g（翻转后）", fontproperties=fp(12), color=RED)

    if 0 <= step < N:
        n = step
        ax1.axvspan(n - 1.5, n + 1.5, color=YELLOW, zorder=0)
        for d, w in zip((-1, 0, 1), W):
            ax2.vlines(n + d, 0, w, colors=RED, lw=3)
            ax2.plot(n + d, w, "o", color=RED, ms=6)
            ax2.text(n + d, w + 0.035, f"{w}", ha="center", color=RED,
                     fontsize=11, fontweight="bold")
        for k in (n - 1, n + 1):
            if k < 0 or k >= N:
                ax1.vlines(k, 0, 0.45, colors=GHOST, lw=1.6, linestyles="--")
                ax1.text(k, 0.65, "补零", ha="center", fontproperties=fp(10), color=GHOST)

    if step == -1:
        txt = "把核 g 左右翻转，从 n = 0 开始逐点滑动：每到一个位置，对应点相乘再求和"
    elif step == N:
        txt = "卷积完成：输出 y[n] 与输入 f 等长（same 方式，边界补零）"
    else:
        txt = comp_str(step)
    fig.text(0.5, 0.475, txt, ha="center", va="center", fontproperties=fp(14.5), color=DARK)

    # lower: output drawn progressively
    ax3 = fig.add_axes([0.06, 0.09, 0.87, 0.30])
    ax3.set_xlim(-2, 17)
    ax3.set_ylim(0, 7.4)
    ax3.set_xticks(range(-1, 17))
    ax3.tick_params(labelsize=9)
    ax3.grid(axis="y", ls=":", alpha=0.4)
    ax3.set_ylabel("输出 y", fontproperties=fp(12), color=NAVY)
    ax3.set_xlabel("n", fontproperties=fp(12))
    if step >= 0:
        hi = min(step, N - 1)
        for x in range(hi + 1):
            cur = (x == hi) and (step < N)
            c = RED if cur else NAVY
            ax3.vlines(x, 0, Y[x], colors=c, lw=2.4)
            ax3.plot(x, Y[x], "o", color=c, ms=5)
        if step < N:
            ax3.text(hi, Y[hi] + 0.35, fmt(Y[hi]), ha="center", color=RED,
                     fontsize=12, fontweight="bold")
    return render(fig)


# ----------------------------------------------------------------------
# 2-D convolution: 6x6 image, 3x3 weighted kernel (sum=16), valid -> 4x4
IMG = [
    [0, 0, 1, 2, 1, 0],
    [0, 2, 4, 6, 4, 1],
    [1, 4, 8, 8, 6, 2],
    [2, 5, 8, 9, 7, 3],
    [1, 3, 6, 7, 5, 2],
    [0, 1, 2, 3, 2, 1],
]
K = [[1, 2, 1],
     [2, 4, 2],
     [1, 2, 1]]
KSUM = 16
SUM2 = [[sum(IMG[i + di][j + dj] * K[di][dj] for di in range(3) for dj in range(3))
         for j in range(4)] for i in range(4)]
OUT2 = [[s / KSUM for s in row] for row in SUM2]


def draw_grid(ax, x0, ytop, vals, cell, fsize, face=None, edge=GRID, lw=1.0,
              tcolor=None, bold=False, fmtv=fmt):
    rows, cols = len(vals), len(vals[0])
    for r in range(rows):
        for c in range(cols):
            v = vals[r][c]
            fc = face(r, c, v) if callable(face) else (face or "white")
            ax.add_patch(Rectangle((x0 + c * cell, ytop - (r + 1) * cell), cell, cell,
                                   facecolor=fc, edgecolor=edge, lw=lw, zorder=2))
            if v is not None:
                tc = tcolor(r, c, v) if callable(tcolor) else (tcolor or DARK)
                ax.text(x0 + (c + 0.5) * cell, ytop - (r + 0.5) * cell, fmtv(v),
                        ha="center", va="center", zorder=3,
                        fontproperties=fp_bold(fsize) if bold else fp(fsize), color=tc)


def frame_2d(step):
    """step: -1 intro, 0..15 compute (raster order), 16 final."""
    fig = plt.figure(figsize=(12.8, 7.2), dpi=DPI, facecolor="white")
    fig.text(0.5, 0.965, "二维卷积：滑动窗口 —— 对应元素相乘再求和",
             ha="center", va="top", fontproperties=fp_bold(23), color=NAVY)
    fig.text(0.5, 0.922,
             "g[x, y] = ( ΣΣ w[s, t]·f[x+s, y+t] ) ÷ 16；3×3 加权核（权重和 = 16），"
             "valid 方式（无填充）：6×6 → 4×4",
             ha="center", va="top", fontproperties=fp(12.5), color=MUTED)

    cur = divmod(step, 4) if 0 <= step < 16 else None

    # left: input image with active window
    axL = fig.add_axes([0.02, 0.05, 0.30, 0.80])
    axL.set_xlim(-0.2, 6.4)
    axL.set_ylim(-1.75, 8.15)
    axL.set_aspect("equal")
    axL.axis("off")
    axL.text(3, 7.35, "输入 f（6×6）", ha="center", fontproperties=fp_bold(15), color=NAVY)
    draw_grid(axL, 0, 6.4, IMG, 1.0, 12,
              face=lambda r, c, v: heat(v),
              tcolor=lambda r, c, v: "#FFFFFF" if v >= 5 else "#12303F")
    if cur:
        i, j = cur
        for di in range(3):
            for dj in range(3):
                axL.add_patch(Rectangle((j + dj, 6.4 - (i + di + 1)), 1, 1,
                                        facecolor=RED, alpha=0.28, zorder=4))
        axL.add_patch(Rectangle((j, 6.4 - (i + 3)), 3, 3, fill=False,
                                edgecolor=RED, lw=2.6, zorder=5))
    axL.text(3, -0.95, "红框 = 当前 3×3 窗口", ha="center", fontproperties=fp(11), color=MUTED)

    # middle: patch (x) kernel = products, then sum
    axM = fig.add_axes([0.335, 0.05, 0.355, 0.80])
    axM.set_xlim(0, 11.4)
    axM.set_ylim(-2.9, 11.55)
    axM.set_aspect("equal")
    axM.axis("off")
    cell, top = 0.95, 8.6
    x_patch, x_kern, x_prod = 0.1, 3.95, 7.75
    patch = [[IMG[cur[0] + di][cur[1] + dj] for dj in range(3)] for di in range(3)] if cur \
        else [[None] * 3 for _ in range(3)]
    prod = [[patch[di][dj] * K[di][dj] for dj in range(3)] for di in range(3)] if cur \
        else [[None] * 3 for _ in range(3)]
    axM.text(x_patch + 1.4, 9.15, "当前窗口", ha="center", fontproperties=fp_bold(13), color=NAVY)
    axM.text(x_kern + 1.4, 9.15, "核 w", ha="center", fontproperties=fp_bold(13), color=NAVY)
    axM.text(x_prod + 1.4, 9.15, "逐点积", ha="center", fontproperties=fp_bold(13), color=NAVY)
    draw_grid(axM, x_patch, top, patch, cell, 12,
              face=lambda r, c, v: heat(v) if v is not None else "white",
              tcolor=lambda r, c, v: "#FFFFFF" if v >= 5 else "#12303F")
    draw_grid(axM, x_kern, top, K, cell, 12, face="#F2F6FA", tcolor=NAVY)
    draw_grid(axM, x_prod, top, prod, cell, 11.5, face=YELLOW, tcolor=DARK)
    axM.text(3.52, 7.05, r"$\otimes$", ha="center", va="center", fontsize=24, color=DARK)
    axM.text(7.32, 7.05, "=", ha="center", va="center", fontsize=24, color=DARK)
    if cur:
        i, j = cur
        axM.text(5.7, 4.5, f"逐点积求和：Σ = {SUM2[i][j]}", ha="center",
                 fontproperties=fp(15), color=DARK)
        axM.text(5.7, 3.0, f"g[{i},{j}] = {SUM2[i][j]} ÷ 16 = {fmt(OUT2[i][j])}",
                 ha="center", fontproperties=fp_bold(17), color=RED)
    elif step == -1:
        axM.text(5.7, 3.6, "对应元素相乘 → 求和 → ÷16 → 写入输出", ha="center",
                 fontproperties=fp(14), color=DARK)
    else:
        axM.text(5.7, 4.3, "卷积完成！", ha="center", fontproperties=fp_bold(17), color=NAVY)
        axM.text(5.7, 2.9, "输出尺寸 = 6 - 3 + 1 = 4（valid）", ha="center",
                 fontproperties=fp(13), color=MUTED)
    axM.text(5.7, -1.9, "核按步长 1 从左到右、从上到下滑动，共 4×4 = 16 个位置",
             ha="center", fontproperties=fp(11.5), color=MUTED)

    # right: output grid filled progressively
    axR = fig.add_axes([0.715, 0.05, 0.265, 0.80])
    axR.set_xlim(-0.15, 4.95)
    axR.set_ylim(-1.95, 6.71)
    axR.set_aspect("equal")
    axR.axis("off")
    axR.text(2.4, 6.2, "输出 g（4×4）", ha="center", fontproperties=fp_bold(15), color=NAVY)
    ocell, otop, ox0 = 1.15, 5.5, 0.1
    for i in range(4):
        for j in range(4):
            idx = i * 4 + j
            x, y = ox0 + j * ocell, otop - (i + 1) * ocell
            if step == 16 or (0 <= step < 16 and idx < step):
                fc, ec, lw, tc, bold = "#E8F1FA", GRID, 1.0, NAVY, False
                v = OUT2[i][j]
            elif 0 <= step < 16 and idx == step:
                fc, ec, lw, tc, bold = YELLOW, RED, 2.4, RED, True
                v = OUT2[i][j]
            else:
                fc, ec, lw, tc, bold = "white", GRID, 1.0, DARK, False
                v = None
            axR.add_patch(Rectangle((x, y), ocell, ocell, facecolor=fc, edgecolor=ec, lw=lw, zorder=2))
            if v is not None:
                axR.text(x + ocell / 2, y + ocell / 2, fmt(v), ha="center", va="center",
                         fontproperties=fp_bold(12) if bold else fp(12), color=tc, zorder=3)
    if step == -1:
        note = "共 16 个位置，逐点计算"
    elif step == 16:
        note = "全部 16 个输出已填完"
    else:
        note = f"第 {step + 1} / 16 个输出"
    axR.text(2.4, 0.1, note, ha="center", fontproperties=fp(11), color=MUTED)
    return render(fig)


def main():
    plt.rcParams["axes.unicode_minus"] = False

    frame_1d(4).save(OUTDIR / "_preview_conv1d.png")
    frame_2d(5).save(OUTDIR / "_preview_conv2d.png")

    steps1 = [-1] + list(range(N)) + [N]
    durs1 = [1600] + [1000] * N + [2600]
    frames1 = [frame_1d(s) for s in steps1]
    save_anim(frames1, durs1, OUTDIR / "conv1d_animation.png", OUTDIR / "conv1d_animation.gif")

    steps2 = [-1] + list(range(16)) + [16]
    durs2 = [1800] + [1100] * 16 + [2800]
    frames2 = [frame_2d(s) for s in steps2]
    save_anim(frames2, durs2, OUTDIR / "conv2d_animation.png", OUTDIR / "conv2d_animation.gif")


if __name__ == "__main__":
    main()
