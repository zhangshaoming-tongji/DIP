# -*- coding: utf-8 -*-
"""Ch4 figures: kernel g, continuous 1-D / 2-D convolution GIFs, and the five properties on slide 10.

Outputs in this folder:
  conv1d_kernel_g.png
  conv1d_continuous.gif / .png
  conv2d_continuous.gif / .png
  卷积性质_交换率.png  卷积性质_分配率.png  卷积性质_结合率.png
  卷积性质_求导性.png  卷积性质_平移不变性.png / .gif
  卷积性质_总览.png
"""
from pathlib import Path
import numpy as np
import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib import font_manager
from matplotlib.colors import LinearSegmentedColormap
from matplotlib.patches import FancyBboxPatch, FancyArrowPatch
from PIL import Image

HERE = Path(__file__).resolve().parent
DPI_GIF = 120
DPI_PNG = 150

NAVY = "#1B4E79"
RED = "#B42024"
BLUE = "#1565C0"
GREEN = "#2E7D32"
ORANGE = "#E07A00"
DARK = "#222222"
MUTED = "#5A5A5A"
GRID = "#D5DEE6"
YELLOW = "#FFF3D6"

CMAP_F = LinearSegmentedColormap.from_list("fblue", ["#F4F8FC", "#90CAF9", "#1565C0", "#0D2C54"])
CMAP_P = LinearSegmentedColormap.from_list("prod", ["#FFFFFF", "#FFE0B2", "#F4A261", "#B42024"])

T = np.linspace(-2.2, 6.2, 1681)
DT = float(T[1] - T[0])


def fp(size=16):
    return font_manager.FontProperties(fname=r"C:\Windows\Fonts\msyh.ttc", size=size)


def fp_bold(size=16):
    return font_manager.FontProperties(fname=r"C:\Windows\Fonts\msyhbd.ttc", size=size)


def rect(t, a, b, h=1.0):
    return np.where((t >= a) & (t <= b), h, 0.0).astype(float)


def gauss(t, mu, sig, h=1.0):
    return h * np.exp(-0.5 * ((t - mu) / sig) ** 2)


def conv(f, g):
    """Linear convolution on the shared grid T. Both samples use time 0 at the same index."""
    iz = int(np.argmin(np.abs(T)))
    full = np.convolve(f, g)
    return full[iz:iz + len(T)] * DT


def shifted(y, t0):
    return np.interp(T - t0, T, y, left=0.0, right=0.0)


def style_time(ax, xlim, ylim):
    ax.set_xlim(*xlim)
    ax.set_ylim(*ylim)
    ax.axhline(0, color=GRID, lw=0.8, zorder=0)
    ax.grid(ls=":", color=GRID, alpha=0.9)
    ax.tick_params(labelsize=9, colors="#333333")
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)
    for s in ("left", "bottom"):
        ax.spines[s].set_color("#8AA0B4")


def new_fig():
    return plt.figure(figsize=(12.8, 7.2), dpi=DPI_PNG, facecolor="white")


def save_png(fig, name):
    path = HERE / name
    fig.savefig(path, dpi=DPI_PNG, facecolor="white")
    plt.close(fig)
    print("png", path.name)
    return path


def render(fig, dpi=DPI_GIF):
    fig.set_dpi(dpi)
    fig.canvas.draw()
    buf = np.asarray(fig.canvas.buffer_rgba())
    img = Image.fromarray(buf[:, :, :3].copy())
    plt.close(fig)
    return img


def save_gif(frames, durs, name, apng=False):
    n = min(6, len(frames))
    idxs = [int(round(i * (len(frames) - 1) / (n - 1))) for i in range(n)] if n > 1 else [0]
    thumbs = [frames[i].resize((280, 158)) for i in idxs]
    sheet = Image.new("RGB", (280 * n, 158))
    for i, th in enumerate(thumbs):
        sheet.paste(th, (280 * i, 0))
    pal = sheet.quantize(colors=256, method=Image.Quantize.MEDIANCUT)
    gframes = [fr.quantize(palette=pal, dither=Image.Dither.NONE) for fr in frames]
    gif_path = HERE / name
    gframes[0].save(
        gif_path, save_all=True, append_images=gframes[1:],
        duration=durs, loop=0, optimize=False, disposal=2,
    )
    print("gif", gif_path.name, len(frames), "frames", gif_path.stat().st_size)
    if apng:
        png_path = gif_path.with_suffix(".png")
        frames[0].save(
            png_path, save_all=True, append_images=frames[1:],
            duration=durs, loop=0, optimize=False,
        )
        print("apng", png_path.name)


# ---------------------------------------------------------------------------
# Signals used by the property figures
# ---------------------------------------------------------------------------
F_EX = rect(T, 0.0, 1.6)
G_EX = np.where((T >= 0.0) & (T <= 1.0), T, 0.0)  # ramp, asymmetric
Y_FG = conv(F_EX, G_EX)
Y_GF = conv(G_EX, F_EX)

F_DI = rect(T, 0.0, 1.2)
G_DI = rect(T, 0.0, 0.8, 1.0)
H_DI = rect(T, 0.4, 1.3, 0.7)
Y_DI_L = conv(F_DI, G_DI + H_DI)
Y_DI_R = conv(F_DI, G_DI) + conv(F_DI, H_DI)

F_AS = rect(T, 0.0, 1.2)
G_AS = rect(T, 0.0, 0.8)
H_AS = rect(T, 0.0, 0.8)
K_AS = conv(G_AS, H_AS)
Y_AS_L = conv(conv(F_AS, G_AS), H_AS)
Y_AS_R = conv(F_AS, K_AS)

MU, SIG, GW = 1.55, 0.50, 0.90
F_DV = gauss(T, MU, SIG)
FP_DV = -(T - MU) / SIG ** 2 * F_DV
G_DV = rect(T, 0.0, GW)
Y_DV = conv(F_DV, G_DV)
D_Y = np.gradient(Y_DV, DT)
D_FG = conv(FP_DV, G_DV)
D_GF = F_DV - shifted(F_DV, GW)  # f * (δ(t) - δ(t-w))

F_SH = 0.55 * gauss(T, 0.7, 0.28) + gauss(T, 1.7, 0.22)
G_SH = gauss(T, 0.0, 0.20)
G_SH = G_SH / (G_SH.sum() * DT)
Y_SH = conv(F_SH, G_SH)


def check_math():
    def y_ramp(t):
        a, b = max(0.0, t - 1.0), min(2.0, t)
        if a >= b:
            return 0.0
        return (t * b - b * b / 2) - (t * a - a * a / 2)

    # continuous 1-D ramp example used by the animation, checked analytically
    tau = np.linspace(-1.5, 4.0, 4001)
    dtau = tau[1] - tau[0]
    f = ((tau >= 0) & (tau <= 2)).astype(float)
    for t, expect in ((0.5, 0.125), (1.5, 0.5), (2.5, 0.375)):
        g = np.where((t - tau >= 0) & (t - tau <= 1), t - tau, 0.0)
        num = np.trapezoid(f * g, dx=dtau)
        assert abs(num - expect) < 2e-3, (t, num, expect)
        assert abs(y_ramp(t) - expect) < 1e-9

    assert np.max(np.abs(Y_FG - Y_GF)) < 0.02
    assert np.max(np.abs(Y_DI_L - Y_DI_R)) < 0.03
    assert np.max(np.abs(Y_AS_L - Y_AS_R)) < 0.04
    mask = (T > 0.15) & (T < 3.6)
    assert np.max(np.abs(D_Y[mask] - D_FG[mask])) < 0.06, np.max(np.abs(D_Y[mask] - D_FG[mask]))
    assert np.max(np.abs(D_FG[mask] - D_GF[mask])) < 0.06
    t0 = 1.1
    err = np.max(np.abs(conv(shifted(F_SH, t0), G_SH) - shifted(Y_SH, t0)))
    assert err < 0.03, err
    print("math checks ok")


# ---------------------------------------------------------------------------
# Kernel g of the discrete 1-D animation
# ---------------------------------------------------------------------------
def fig_kernel():
    fig = new_fig()
    fig.text(0.5, 0.955, "一维离散卷积所用的核  g[k]", ha="center", va="top",
             fontproperties=fp_bold(24), color=NAVY)
    fig.text(0.5, 0.905,
             "卷积先把核左右翻转，再沿信号滑动。动画里的红色竖线就是翻转后的 g[−k]。",
             ha="center", va="top", fontproperties=fp(14), color=MUTED)

    def stems(rect, title, xs, ys, note):
        ax = fig.add_axes(rect)
        style_time(ax, (-1.8, 1.8), (0, 0.72))
        ax.set_xticks([-1, 0, 1])
        ax.vlines(xs, 0, ys, color=RED, lw=3.2, zorder=3)
        ax.plot(xs, ys, "o", color=RED, ms=9, zorder=4)
        for x, y in zip(xs, ys):
            ax.text(x, y + 0.04, f"{y:.1f}", ha="center", color=RED, fontsize=14, fontweight="bold")
        ax.set_title(title, fontproperties=fp_bold(16), color=NAVY, pad=6)
        if note:
            ax.text(0.5, -0.28, note, transform=ax.transAxes, ha="center",
                    fontproperties=fp(12), color=MUTED)

    stems([0.07, 0.60, 0.36, 0.24], "原核   g[k]",
          [-1, 0, 1], [0.5, 0.3, 0.2], "g[-1]=0.5    g[0]=0.3    g[1]=0.2")
    stems([0.57, 0.60, 0.36, 0.24], "翻转后   g[-k]",
          [-1, 0, 1], [0.2, 0.3, 0.5], "动画中滑动的权重  0.2,  0.3,  0.5")

    axa = fig.add_axes([0.44, 0.64, 0.12, 0.14])
    axa.axis("off")
    axa.annotate("", xy=(0.92, 0.55), xytext=(0.08, 0.55),
                 arrowprops=dict(arrowstyle="-|>", color=NAVY, lw=2.2, mutation_scale=16))
    axa.text(0.5, 0.85, "翻转", ha="center", fontproperties=fp_bold(14), color=NAVY)
    axa.text(0.5, 0.18, "k  →  −k", ha="center", fontsize=13, color=NAVY)

    fig.text(0.5, 0.47,
             r"$y[n]=\sum_k f[k]\,g[n-k]=f[n-1]\cdot 0.2+f[n]\cdot 0.3+f[n+1]\cdot 0.5$",
             ha="center", va="center", fontsize=16, color=DARK)
    fig.text(0.5, 0.41,
             "与 f[n−1]、f[n]、f[n+1] 相乘的，分别是翻转后落在这三格上的权重",
             ha="center", fontproperties=fp(13), color=MUTED)

    # tie to the animation frame n = 4
    ax = fig.add_axes([0.18, 0.08, 0.64, 0.20])
    style_time(ax, (2.2, 5.8), (0, 9.6))
    xs, fs = [3, 4, 5], [2, 6, 6]
    ws = [0.2, 0.3, 0.5]
    ax.vlines(xs, 0, fs, color=NAVY, lw=8, zorder=2)
    for x, fv, w in zip(xs, fs, ws):
        ax.text(x, fv + 0.28, f"f[{x}]={fv}", ha="center", fontproperties=fp(11), color=NAVY)
        ax.text(x, fv + 1.15, f"×{w}", ha="center", color=RED, fontsize=12, fontweight="bold")
    ax.set_xticks(xs)
    ax.tick_params(axis="x", labelsize=11)
    ax.set_title("对照动画中的 n = 4 ：  0.2×2 + 0.3×6 + 0.5×6 = 5.2",
                 fontproperties=fp(13), color=DARK, pad=6)
    save_png(fig, "conv1d_kernel_g.png")


# ---------------------------------------------------------------------------
# Continuous 1-D
# ---------------------------------------------------------------------------
TAU = np.linspace(-1.6, 4.0, 1401)
DTAU = float(TAU[1] - TAU[0])
F_TAU = ((TAU >= 0.0) & (TAU <= 2.0)).astype(float)


def g_fun(tau):
    return np.where((tau >= 0.0) & (tau <= 1.0), tau, 0.0)


def y_cont(t):
    g = g_fun(t - TAU)
    return float(np.trapezoid(F_TAU * g, dx=DTAU))


_YT = np.linspace(-0.45, 3.45, 420)
_YV = np.array([y_cont(v) for v in _YT])


def frame_cont1d(t, phase):
    fig = plt.figure(figsize=(12.8, 7.2), dpi=DPI_GIF, facecolor="white")
    fig.text(0.5, 0.968, "一维连续卷积：翻转 · 平移 · 相乘 · 积分",
             ha="center", va="top", fontproperties=fp_bold(22), color=NAVY)
    fig.text(0.5, 0.925,
             r"$y(t)=\int f(\tau)\,g(t-\tau)\,d\tau$"
             "        f(τ) = 1（0≤τ≤2），   g(τ) = τ（0≤τ≤1，右高左低）",
             ha="center", va="top", fontproperties=fp(13), color=MUTED)

    ax = fig.add_axes([0.07, 0.63, 0.70, 0.23])
    style_time(ax, (-1.5, 3.9), (0, 1.35))
    ax.fill_between(TAU, F_TAU, color=BLUE, alpha=0.18, zorder=1)
    ax.plot(TAU, F_TAU, color=BLUE, lw=2.0, label="f(τ)")
    g = g_fun(t - TAU)
    prod = F_TAU * g
    ax.fill_between(TAU, prod, color=ORANGE, alpha=0.85, zorder=2, label="乘积")
    ax.plot(TAU, g, color=RED, lw=2.4, zorder=3, label="g(t−τ)")
    ax.axvline(t, color="#7A8794", ls="--", lw=1.0, zorder=0)
    ax.text(t, 1.26, "t", ha="center", color="#7A8794", fontsize=11)
    ax.legend(prop=fp(10), loc="upper right", frameon=False)
    ax.set_xlabel("τ", fontsize=12)
    ax.set_ylabel("幅度", fontproperties=fp(12))

    axg = fig.add_axes([0.80, 0.56, 0.17, 0.28])
    tg = np.linspace(-0.15, 1.25, 200)
    style_time(axg, (-0.15, 1.25), (0, 1.25))
    axg.plot(tg, g_fun(tg), color=NAVY, lw=2.2)
    axg.fill_between(tg, g_fun(tg), color=NAVY, alpha=0.12)
    axg.set_title("原核 g(τ)", fontproperties=fp_bold(11), color=NAVY, pad=3)
    axg.text(0.5, -0.28, "翻转后变成左高右低", transform=axg.transAxes,
             ha="center", fontproperties=fp(9), color=RED)
    axg.tick_params(labelsize=8)

    val = y_cont(t)
    if phase == "intro":
        msg = "先把 g(τ) 绕原点翻转，再沿 τ 轴向右平移；橙色区域的面积就是 y(t)"
    elif phase == "done":
        msg = "卷积完成。核完全落在平台内时面积恒为 1/2，离开平台后积分下降到 0"
    else:
        msg = f"当前  t = {t:.2f} ，   橙色面积  y(t) = {val:.3f}"
    fig.text(0.5, 0.50, msg, ha="center", va="center", fontproperties=fp(15), color=DARK)

    ax2 = fig.add_axes([0.07, 0.07, 0.86, 0.36])
    style_time(ax2, (-0.6, 3.6), (0, 0.68))
    sel = _YT <= t + 1e-9
    if np.any(sel):
        ax2.plot(_YT[sel], _YV[sel], color=NAVY, lw=2.3)
        ax2.plot([t], [val], "o", color=RED, ms=7, zorder=4)
        ax2.text(t, val + 0.05, f"{val:.3f}", ha="center", color=RED, fontsize=11, fontweight="bold")
    ax2.set_xlabel("t", fontsize=12)
    ax2.set_ylabel("y(t)", fontsize=12)
    return render(fig)


# ---------------------------------------------------------------------------
# Continuous 2-D
# ---------------------------------------------------------------------------
ALPHA = np.linspace(-2.6, 2.6, 261)
BETA = np.linspace(-2.6, 2.6, 261)
DA = float(ALPHA[1] - ALPHA[0])
AA, BB = np.meshgrid(ALPHA, BETA)
F2 = (np.exp(-((AA + 0.40) ** 2 + (BB - 0.20) ** 2) / (2 * 0.48 ** 2))
      + 0.70 * np.exp(-((AA - 0.75) ** 2 + (BB + 0.40) ** 2) / (2 * 0.34 ** 2)))

S = np.linspace(-1.2, 1.2, 121)
DS = float(S[1] - S[0])
SS, TT = np.meshgrid(S, S)


def w_raw(s, t):
    r = np.exp(-((s - 0.28) ** 2) / (2 * 0.34 ** 2) - ((t + 0.15) ** 2) / (2 * 0.22 ** 2))
    return np.where((np.abs(s) <= 1.05) & (np.abs(t) <= 0.85), r, 0.0)


_WNORM = float(np.sum(w_raw(SS, TT)) * DS * DS)


def w_fun(s, t):
    return w_raw(s, t) / _WNORM


W2 = w_fun(SS, TT)
W_FLIP = w_fun(-SS, -TT)


def conv2_same(F, W):
    sf, sw = F.shape, W.shape
    osh = (sf[0] + sw[0] - 1, sf[1] + sw[1] - 1)
    spec = np.fft.rfft2(F, s=osh) * np.fft.rfft2(W, s=osh)
    full = np.fft.irfft2(spec, s=osh)
    sy, sx = (sw[0] - 1) // 2, (sw[1] - 1) // 2
    return np.real(full[sy:sy + sf[0], sx:sx + sf[1]]) * DA * DA


G2 = conv2_same(F2, W2)
EXT = [ALPHA[0], ALPHA[-1], BETA[0], BETA[-1]]
XS = np.linspace(-1.20, 1.20, 8)
YS = np.linspace(1.05, -1.05, 5)  # top to bottom
SCAN = [(float(x), float(y)) for y in YS for x in XS]
HX = float(abs(XS[1] - XS[0]) / 2)
HY = float(abs(YS[0] - YS[1]) / 2)


def direct_at(x, y):
    return float(np.sum(F2 * w_fun(x - AA, y - BB)) * DA * DA)


def check_2d():
    x, y = SCAN[len(SCAN) // 3]
    j = int(np.argmin(np.abs(ALPHA - x)))
    i = int(np.argmin(np.abs(BETA - y)))
    err = abs(G2[i, j] - direct_at(ALPHA[j], BETA[i]))
    assert err < 0.02, err
    assert abs(np.sum(W2) * DS * DS - 1) < 1e-6
    print("2d check ok", round(err, 5))


def frame_cont2d(step):
    """step None = intro, -1 = finished, otherwise index into SCAN."""
    fig = plt.figure(figsize=(12.8, 7.2), dpi=DPI_GIF, facecolor="white")
    fig.text(0.5, 0.972, "二维连续卷积：翻转核，再在平面上平移并积分",
             ha="center", va="top", fontproperties=fp_bold(22), color=NAVY)
    fig.text(0.5, 0.932,
             r"$g(x,y)=\iint f(\alpha,\beta)\,w(x-\alpha,\,y-\beta)\,d\alpha\,d\beta$"
             "        核的权重之和为 1",
             ha="center", va="top", fontproperties=fp(12.5), color=MUTED)

    cur = SCAN[step] if isinstance(step, int) and step >= 0 else None
    panels = []
    for rect, title in (
        ([0.035, 0.24, 0.30, 0.62], "输入  f(α, β)"),
        ([0.355, 0.24, 0.30, 0.62], "乘积  f · w(x−α, y−β)"),
        ([0.675, 0.24, 0.30, 0.62], "输出  g(x, y)"),
    ):
        ax = fig.add_axes(rect)
        ax.set_xlim(EXT[0], EXT[1])
        ax.set_ylim(EXT[2], EXT[3])
        ax.set_aspect("equal")
        ax.set_xticks([-2, -1, 0, 1, 2])
        ax.set_yticks([-2, -1, 0, 1, 2])
        ax.tick_params(labelsize=8)
        ax.set_title(title, fontproperties=fp_bold(13), color=NAVY, pad=6)
        panels.append(ax)
    axL, axM, axR = panels
    vmax_f = float(F2.max())
    axL.imshow(F2, extent=EXT, origin="lower", cmap=CMAP_F, vmin=0, vmax=vmax_f, zorder=1)

    if cur is not None:
        x, y = cur
        WS = w_fun(x - AA, y - BB)
        prod = F2 * WS
        kn = WS / (WS.max() + 1e-12)
        rgba = np.zeros((*kn.shape, 4))
        rgba[..., 0], rgba[..., 1], rgba[..., 2] = 0.706, 0.125, 0.141
        rgba[..., 3] = 0.50 * kn
        axL.imshow(rgba, extent=EXT, origin="lower", zorder=3)
        axL.plot([x], [y], marker="+", color=RED, ms=16, mew=2.2, zorder=4)
        axM.imshow(prod, extent=EXT, origin="lower", cmap=CMAP_P, vmin=0, vmax=float(prod.max()) + 1e-6)
        val = direct_at(x, y)
        # reveal output in scan order: finished rows (higher β) plus current row up to x
        visible = ((BB > y + HY) | ((np.abs(BB - y) <= HY) & (AA <= x + HX)))
        visible &= (np.abs(AA) <= XS[-1] + HX) & (np.abs(BB) <= YS[0] + HY)
        show = np.ma.array(G2, mask=~visible)
        cmap = CMAP_F.copy()
        cmap.set_bad("#FFFFFF")
        axR.imshow(show, extent=EXT, origin="lower", cmap=cmap, vmin=0, vmax=vmax_f)
        axR.plot([x], [y], marker="o", color=RED, ms=8, zorder=4)
        fig.text(0.5, 0.175, f"g({x:.2f}, {y:.2f})  =  {val:.3f}",
                 ha="center", fontproperties=fp_bold(18), color=RED)
        fig.text(0.90, 0.055, f"第 {step + 1} / {len(SCAN)} 个位置",
                 ha="center", fontproperties=fp(11), color=MUTED)
    elif step is None:
        axM.text(0, 0, "平移到一处后\n逐点相乘", ha="center", va="center", fontproperties=fp(14), color=MUTED)
        axR.text(0, 0, "积分写入输出", ha="center", va="center", fontproperties=fp(14), color=MUTED)
        fig.text(0.5, 0.175, "红点是当前位置，红色云团是翻转后的核（峰值不在红点上）",
                 ha="center", fontproperties=fp(14), color=DARK)
    else:
        visible = (np.abs(AA) <= XS[-1] + HX) & (np.abs(BB) <= YS[0] + HY)
        show = np.ma.array(G2, mask=~visible)
        cmap = CMAP_F.copy()
        cmap.set_bad("#FFFFFF")
        axR.imshow(show, extent=EXT, origin="lower", cmap=cmap, vmin=0, vmax=vmax_f)
        fig.text(0.5, 0.175, "卷积完成。核归一化后，输出是输入的加权平均，亮斑被抹开",
                 ha="center", fontproperties=fp_bold(15), color=NAVY)

    def thumb(rect, data, title):
        ax = fig.add_axes(rect)
        ax.imshow(data, extent=[-1.2, 1.2, -1.2, 1.2], origin="lower", cmap=CMAP_P)
        ax.set_xticks([])
        ax.set_yticks([])
        ax.set_title(title, fontproperties=fp(9), color=NAVY, pad=2)
        iy, ix = np.unravel_index(int(np.argmax(data)), data.shape)
        ax.plot([S[ix]], [S[iy]], "o", color="white", ms=4, mew=0)

    thumb([0.035, 0.025, 0.09, 0.145], W2, "原核 w")
    thumb([0.145, 0.025, 0.09, 0.145], W_FLIP, "翻转后")
    fig.text(0.132, 0.09, "→", ha="center", va="center", fontsize=16, color=NAVY)
    return render(fig)


# ---------------------------------------------------------------------------
# Five properties (slide 10 wording: 交换率 / 分配率 / 结合率 / 求导性 / 平移不变性)
# ---------------------------------------------------------------------------
def overlap(ax, curves, xlim, note):
    """curves: list of (y, color, ls, lw, label)."""
    ymax = max(float(np.max(y)) for y, *_ in curves)
    style_time(ax, xlim, (min(0, min(float(np.min(y)) for y, *_ in curves) * 1.15) - 0.02 * ymax,
                          ymax * 1.22 + 1e-6))
    for y, color, ls, lw, label in curves:
        ax.plot(T, y, color=color, ls=ls, lw=lw, label=label, zorder=3)
    ax.legend(prop=fp(11), loc="upper right", frameon=False)
    ax.set_xlabel("t", fontsize=11)
    ax.text(0.0, -0.22, note, transform=ax.transAxes, fontproperties=fp(12.5), color=GREEN)


def fig_exchange():
    fig = new_fig()
    fig.text(0.5, 0.955, "1   交换率", ha="center", va="top", fontproperties=fp_bold(24), color=NAVY)
    fig.text(0.5, 0.900, r"$(f*g)(t)=(g*f)(t)$", ha="center", va="top", fontsize=20, color=DARK)
    axf = fig.add_axes([0.08, 0.52, 0.38, 0.30])
    axg = fig.add_axes([0.55, 0.52, 0.38, 0.30])
    for ax, y, c, title in (
        (axf, F_EX, BLUE, "f(t)：矩形脉冲"),
        (axg, G_EX, RED, "g(t)：上升斜坡（不对称）"),
    ):
        style_time(ax, (-0.4, 3.2), (0, 1.8 if c == BLUE else 1.25))
        ax.fill_between(T, y, color=c, alpha=0.25)
        ax.plot(T, y, color=c, lw=2.2)
        ax.set_title(title, fontproperties=fp_bold(14), color=NAVY)
        ax.set_xlabel("t", fontsize=11)
    ax = fig.add_axes([0.08, 0.14, 0.84, 0.30])
    overlap(ax, [
        (Y_FG, NAVY, "-", 2.6, "f * g"),
        (Y_GF, RED, "--", 2.2, "g * f"),
    ], (-0.4, 3.2), "两条曲线重合：谁在前谁在后，结果相同。计算时先翻转哪一个函数都可以。")
    save_png(fig, "卷积性质_交换率.png")


def fig_distribute():
    fig = new_fig()
    fig.text(0.5, 0.955, "2   分配率", ha="center", va="top", fontproperties=fp_bold(24), color=NAVY)
    fig.text(0.5, 0.900, r"$f*(g+h)=f*g+f*h$", ha="center", va="top", fontsize=20, color=DARK)
    specs = [
        (F_DI, BLUE, "f(t)"),
        (G_DI, RED, "g(t)"),
        (H_DI, GREEN, "h(t)"),
        (G_DI + H_DI, ORANGE, "g(t)+h(t)"),
    ]
    for i, (y, c, title) in enumerate(specs):
        ax = fig.add_axes([0.05 + i * 0.24, 0.55, 0.21, 0.26])
        style_time(ax, (-0.3, 2.6), (0, 2.0))
        ax.fill_between(T, y, color=c, alpha=0.28)
        ax.plot(T, y, color=c, lw=2.0)
        ax.set_title(title, fontproperties=fp_bold(13), color=NAVY)
    ax = fig.add_axes([0.08, 0.14, 0.84, 0.32])
    overlap(ax, [
        (Y_DI_L, NAVY, "-", 2.6, "f * (g+h)"),
        (Y_DI_R, RED, "--", 2.2, "f*g + f*h"),
    ], (-0.3, 3.0), "两条曲线重合：对和做卷积，等于分别卷积再相加。线性滤波因此满足叠加。")
    save_png(fig, "卷积性质_分配率.png")


def fig_associate():
    fig = new_fig()
    fig.text(0.5, 0.955, "3   结合率", ha="center", va="top", fontproperties=fp_bold(24), color=NAVY)
    fig.text(0.5, 0.900, r"$(f*g)*h=f*(g*h)$", ha="center", va="top", fontsize=20, color=DARK)
    fig.text(0.5, 0.855, "上一路：f 先与 g 卷积，再与 h 卷积          下一路：先把 g、h 合成一个核，再与 f 卷积一次",
             ha="center", va="top", fontproperties=fp(13), color=MUTED)
    ax = fig.add_axes([0.07, 0.46, 0.40, 0.28])
    style_time(ax, (-0.3, 2.2), (0, 1.05))
    ax.plot(T, G_AS, color=RED, lw=2.2, label="g")
    ax.plot(T, H_AS, color=GREEN, lw=2.2, ls="--", label="h")
    ax.set_title("两个核", fontproperties=fp_bold(14), color=NAVY)
    ax.legend(prop=fp(11), frameon=False)
    axk = fig.add_axes([0.55, 0.46, 0.40, 0.28])
    style_time(axk, (-0.3, 2.2), (0, 1.05))
    axk.fill_between(T, K_AS, color=ORANGE, alpha=0.35)
    axk.plot(T, K_AS, color=ORANGE, lw=2.4)
    axk.set_title("合成核   k = g * h    （三角形）", fontproperties=fp_bold(14), color=NAVY)
    ax = fig.add_axes([0.08, 0.14, 0.84, 0.30])
    overlap(ax, [
        (Y_AS_L, NAVY, "-", 2.6, "(f * g) * h"),
        (Y_AS_R, RED, "--", 2.2, "f * k"),
    ], (-0.3, 3.2), "两条曲线重合：多次模板可以先合成一个模板，再对图像只做一次卷积。")
    save_png(fig, "卷积性质_结合率.png")


def fig_derivative():
    fig = new_fig()
    fig.text(0.5, 0.955, "4   求导性", ha="center", va="top", fontproperties=fp_bold(24), color=NAVY)
    fig.text(0.5, 0.900, r"$\frac{d}{dt}(f*g)=f'*g=f*g'$",
             ha="center", va="top", fontsize=20, color=DARK)
    axf = fig.add_axes([0.06, 0.48, 0.28, 0.28])
    style_time(axf, (-0.2, 3.6), (-1.35, 1.45))
    axf.plot(T, F_DV, color=BLUE, lw=2.2, label="f")
    axf.plot(T, FP_DV, color=BLUE, lw=1.6, ls="--", label="f'")
    axf.legend(prop=fp(10), frameon=False)
    axf.set_title("光滑信号 f 及其导数", fontproperties=fp_bold(13), color=NAVY)
    axg = fig.add_axes([0.38, 0.48, 0.26, 0.28])
    style_time(axg, (-0.4, 1.6), (0, 1.25))
    axg.fill_between(T, G_DV, color=RED, alpha=0.25)
    axg.plot(T, G_DV, color=RED, lw=2.2)
    axg.set_title("平滑核 g：宽 0.9 的矩形", fontproperties=fp_bold(13), color=NAVY)
    axp = fig.add_axes([0.68, 0.48, 0.27, 0.28])
    style_time(axp, (-0.4, 1.6), (-1.4, 1.4))
    axp.annotate("", xy=(0, 1.05), xytext=(0, 0),
                 arrowprops=dict(arrowstyle="-|>", color=RED, lw=2.2))
    axp.annotate("", xy=(GW, -1.05), xytext=(GW, 0),
                 arrowprops=dict(arrowstyle="-|>", color=NAVY, lw=2.2))
    axp.text(0.08, 1.08, "+1", color=RED, fontsize=12, fontweight="bold")
    axp.text(GW + 0.08, -1.25, "−1", color=NAVY, fontsize=12, fontweight="bold")
    axp.set_title("g'：一正一负两个冲激", fontproperties=fp_bold(13), color=NAVY)
    ax = fig.add_axes([0.08, 0.14, 0.84, 0.30])
    overlap(ax, [
        (D_Y, NAVY, "-", 2.8, "(f * g)'"),
        (D_FG, RED, "--", 2.2, "f' * g"),
        (D_GF, GREEN, ":", 2.4, "f * g'"),
    ], (-0.2, 3.8), "三条曲线重合：只需求导其中一路。平滑核的导数是边缘模板，用它卷积等于先平滑再求导。")
    save_png(fig, "卷积性质_求导性.png")


def draw_shift(t0, phase):
    fig = plt.figure(figsize=(12.8, 7.2), dpi=DPI_GIF if phase != "still" else DPI_PNG, facecolor="white")
    fig.text(0.5, 0.955, "5   平移不变性", ha="center", va="top", fontproperties=fp_bold(24), color=NAVY)
    fig.text(0.5, 0.900, "若  y = f * g ，则  f(t − t0) * g(t) = y(t − t0)",
             ha="center", va="top", fontproperties=fp(16), color=DARK)
    f2 = shifted(F_SH, t0)
    y2 = shifted(Y_SH, t0)
    ax1 = fig.add_axes([0.08, 0.52, 0.84, 0.30])
    ax2 = fig.add_axes([0.08, 0.16, 0.84, 0.30])
    style_time(ax1, (-0.6, 4.6), (0, 1.25))
    style_time(ax2, (-0.6, 4.6), (0, 1.25))
    ax1.plot(T, F_SH, color="#B7C3CE", lw=1.6, label="f(t)")
    ax1.plot(T, f2, color=BLUE, lw=2.4, label="f(t − t0)")
    ax2.plot(T, Y_SH, color="#B7C3CE", lw=1.6, label="y(t)")
    ax2.plot(T, y2, color=RED, lw=2.4, label="y(t − t0)")
    ax1.legend(prop=fp(11), loc="upper right", frameon=False)
    ax2.legend(prop=fp(11), loc="upper right", frameon=False)
    ax1.set_ylabel("输入", fontproperties=fp(12))
    ax2.set_ylabel("输出", fontproperties=fp(12))
    ax2.set_xlabel("t", fontsize=12)
    # mark the tall peak
    if t0 > 0.04:
        p0, p1 = 1.7, 1.7 + t0
        for ax in (ax1, ax2):
            ax.annotate("", xy=(p1, 1.08), xytext=(p0, 1.08),
                        arrowprops=dict(arrowstyle="<->", color=ORANGE, lw=1.6))
            ax.text((p0 + p1) / 2, 1.13, f"t0 = {t0:.2f}", ha="center", color=ORANGE, fontsize=11)
    if phase == "intro":
        msg = "核 g 固定不动。输入向右平移时，输出整体跟着平移同样的距离"
    elif phase == "still":
        msg = "系统不随时间改变：输入晚到 t0，输出也晚到 t0，形状不变"
    else:
        msg = "输入的峰移多少，输出的峰就移多少"
    fig.text(0.5, 0.075, msg, ha="center", fontproperties=fp(14), color=GREEN)
    if phase == "still":
        save_png(fig, "卷积性质_平移不变性.png")
        return None
    return render(fig)


def fig_overview():
    fig = new_fig()
    fig.text(0.5, 0.965, "卷积的五条性质", ha="center", va="top", fontproperties=fp_bold(24), color=NAVY)
    items = [
        ("1  交换率", r"$(f*g)(t)=(g*f)(t)$",
         [(Y_FG, NAVY, "-", "f*g"), (Y_GF, RED, "--", "g*f")], (-0.3, 3.0), "实线与虚线重合"),
        ("2  分配率", r"$f*(g+h)=f*g+f*h$",
         [(Y_DI_L, NAVY, "-", "f*(g+h)"), (Y_DI_R, RED, "--", "f*g+f*h")], (-0.2, 2.8), "实线与虚线重合"),
        ("3  结合率", r"$(f*g)*h=f*(g*h)$",
         [(Y_AS_L, NAVY, "-", "(f*g)*h"), (Y_AS_R, RED, "--", "f*(g*h)")], (-0.2, 3.0), "实线与虚线重合"),
        ("4  求导性", r"$(f*g)'=f'*g=f*g'$",
         [(D_Y, NAVY, "-", "(f*g)'"), (D_FG, RED, "--", "f'*g"), (D_GF, GREEN, ":", "f*g'")],
         (0.2, 3.4), "三条曲线重合"),
        ("5  平移不变性", r"$f(t-t_0)*g=y(t-t_0)$",
         [(Y_SH, "#90A4AE", "-", "y(t)"), (shifted(Y_SH, 1.0), RED, "-", "y(t−t0)")],
         (-0.4, 4.2), "输入输出同移"),
    ]
    # 3 on the first row, 2 on the second
    slots = [
        (0.04, 0.50, 0.30, 0.38),
        (0.355, 0.50, 0.30, 0.38),
        (0.67, 0.50, 0.30, 0.38),
        (0.12, 0.06, 0.36, 0.38),
        (0.52, 0.06, 0.36, 0.38),
    ]
    for (title, formula, curves, xlim, note), rect in zip(items, slots):
        x, y, w, h = rect
        ax = fig.add_axes([x, y, w, h * 0.72])
        ymax = max(float(np.max(c[0])) for c in curves)
        ymin = min(0.0, min(float(np.min(c[0])) for c in curves))
        style_time(ax, xlim, (ymin * 1.2 - 0.02, ymax * 1.72 + 1e-6))
        for yy, c, ls, lab in curves:
            ax.plot(T, yy, color=c, ls=ls, lw=2.0, label=lab)
        ax.legend(prop=fp(8), loc="upper right", frameon=False, handlelength=1.4)
        ax.tick_params(labelsize=7)
        fig.text(x + w / 2, y + h * 0.98, title, ha="center", va="top",
                 fontproperties=fp_bold(14), color=NAVY)
        fig.text(x + w / 2, y + h * 0.86, formula, ha="center", va="top", fontsize=12, color=DARK)
        fig.text(x + w / 2, y + h * 0.74, note, ha="center", va="top",
                 fontproperties=fp(11), color=GREEN)
    save_png(fig, "卷积性质_总览.png")


def main():
    plt.rcParams["axes.unicode_minus"] = False
    plt.rcParams["mathtext.fontset"] = "stix"
    check_math()
    check_2d()

    fig_kernel()
    fig_exchange()
    fig_distribute()
    fig_associate()
    fig_derivative()
    draw_shift(1.0, "still")
    fig_overview()

    ts = list(np.linspace(-0.35, 3.35, 40))
    frames = [frame_cont1d(ts[0], "intro")]
    frames += [frame_cont1d(t, "run") for t in ts[1:-1]]
    frames.append(frame_cont1d(ts[-1], "done"))
    durs = [1500] + [90] * (len(frames) - 2) + [2200]
    save_gif(frames, durs, "conv1d_continuous.gif", apng=True)

    frames = [frame_cont2d(None)]
    frames += [frame_cont2d(i) for i in range(len(SCAN))]
    frames.append(frame_cont2d(-1))
    durs = [1600] + [110] * len(SCAN) + [2400]
    save_gif(frames, durs, "conv2d_continuous.gif", apng=True)

    t0s = list(np.linspace(0.0, 1.5, 16))
    sframes = [draw_shift(t0s[0], "intro")]
    sframes += [draw_shift(t, "run") for t in t0s[1:]]
    sframes.append(draw_shift(t0s[-1], "run"))
    sdurs = [1200] + [120] * (len(sframes) - 2) + [1600]
    save_gif(sframes, sdurs, "卷积性质_平移不变性.gif", apng=False)
    print("done", HERE)


if __name__ == "__main__":
    main()
