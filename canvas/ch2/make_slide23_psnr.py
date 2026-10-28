# -*- coding: utf-8 -*-
"""Render Ch2 slide 23 content: PSNR + eight quantizer formulas."""
from pathlib import Path
import io
import numpy as np
from PIL import Image, ImageDraw, ImageFont, ImageOps
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = Path(r"D:\works\Others\Teaching\3-Under-DigitalImageProcessing\课件v2")
QDIR = ROOT / "lab" / "lena_quantization"
OUT = ROOT / "canvas" / "ch2" / "slide23_psnr_formulas.png"

NAVY = (27, 78, 121)
NAVY2 = (21, 62, 99)
RED = (180, 32, 36)
DARK = (40, 40, 40)
GRAY = (90, 90, 90)
CARD_BG = (248, 250, 252)
LINE = (210, 220, 230)
PSNR_BG = (232, 241, 248)
WHITE = (255, 255, 255)

W, H = 1920, 900


def font(size, bold=False):
    path = r"C:\Windows\Fonts\msyhbd.ttc" if bold else r"C:\Windows\Fonts\msyh.ttc"
    return ImageFont.truetype(path, size=size)


def math_img(tex, fontsize=15, dpi=200, color="#222222", max_w=None, max_h=None):
    fig = plt.figure(figsize=(0.02, 0.02))
    fig.patch.set_alpha(0.0)
    fig.text(0.0, 0.0, tex, fontsize=fontsize, color=color, ha="left", va="bottom")
    buf = io.BytesIO()
    fig.savefig(buf, format="png", dpi=dpi, bbox_inches="tight", pad_inches=0.02, transparent=True)
    plt.close(fig)
    buf.seek(0)
    im = Image.open(buf).convert("RGBA")
    a = np.array(im.split()[-1])
    ys, xs = np.where(a > 8)
    if xs.size:
        im = im.crop((int(xs.min()), int(ys.min()), int(xs.max()) + 1, int(ys.max()) + 1))
    nw, nh = im.size
    scale = 1.0
    if max_w and nw > max_w:
        scale = min(scale, max_w / nw)
    if max_h and nh > max_h:
        scale = min(scale, max_h / nh)
    if scale < 1.0:
        im = im.resize((max(1, int(nw * scale)), max(1, int(nh * scale))), Image.Resampling.LANCZOS)
    return im


def paste_center(base, im, box):
    x0, y0, x1, y1 = box
    bw, bh = x1 - x0, y1 - y0
    im = im.convert("RGBA")
    scale = min(bw / im.width, bh / im.height, 1.0)
    nw, nh = max(1, int(im.width * scale)), max(1, int(im.height * scale))
    im = im.resize((nw, nh), Image.Resampling.LANCZOS)
    x = x0 + (bw - nw) // 2
    y = y0 + (bh - nh) // 2
    base.paste(im, (x, y), im)


def rounded_rect(draw, xy, r, fill, outline=None, width=1):
    draw.rounded_rectangle(xy, radius=r, fill=fill, outline=outline, width=width)


def load_thumb(name, size=108):
    p = QDIR / name
    im = Image.open(p).convert("RGB")
    im = ImageOps.fit(im, (size, size), Image.Resampling.LANCZOS)
    return im


def draw_card(canvas, box, title, psnr, principle, tex_list, thumb_name, tag):
    x0, y0, x1, y1 = box
    d = ImageDraw.Draw(canvas)
    rounded_rect(d, box, 12, CARD_BG, outline=LINE, width=2)
    d.rectangle([x0, y0, x0 + 8, y1], fill=NAVY)

    tw = 78
    thumb = load_thumb(thumb_name, tw)
    canvas.paste(thumb, (x0 + 18, y0 + 14))
    d.rectangle([x0 + 17, y0 + 13, x0 + 18 + tw, y0 + 14 + tw], outline=NAVY, width=1)

    tx = x0 + 18 + tw + 12
    d.text((tx, y0 + 10), tag, font=font(12, True), fill=NAVY)
    d.text((tx, y0 + 28), title, font=font(22, True), fill=NAVY2)
    d.text((tx, y0 + 56), f"PSNR = {psnr:.2f} dB", font=font(15, True), fill=RED)

    text_top = y0 + 14 + tw + 8
    pf = font(14)
    max_text_w = (x1 - x0) - 32
    # wrap principle
    lines, cur = [], ""
    for ch in principle:
        trial = cur + ch
        if d.textlength(trial, font=pf) <= max_text_w:
            cur = trial
        else:
            lines.append(cur)
            cur = ch
    if cur:
        lines.append(cur)
    yy = text_top
    for line in lines[:2]:
        d.text((x0 + 16, yy), line, font=pf, fill=DARK)
        yy += 22

    my = yy + 4
    max_w = (x1 - x0) - 28
    remain = max(22, y1 - my - 8)
    each_h = remain // max(1, len(tex_list))
    for tex in tex_list:
        mi = math_img(tex, fontsize=13.5, max_w=max_w, max_h=each_h)
        canvas.paste(mi, (x0 + 16, my), mi)
        my += mi.height + 2


def main():
    canvas = Image.new("RGB", (W, H), WHITE)
    d = ImageDraw.Draw(canvas)

    # ----- PSNR banner -----
    rounded_rect(d, (16, 8, W - 16, 188), 14, PSNR_BG, outline=(176, 201, 220), width=2)
    d.text((32, 18), "峰值信噪比 PSNR（Peak Signal-to-Noise Ratio）", font=font(24, True), fill=NAVY)
    explain = (
        "把量化误差当作“噪声”，比较图像峰值功率与均方误差。单位：分贝（dB）。"
        "PSNR 越大，量化结果越接近原图。像素取 [0,1] 时 MAX=1；8 bit 图 MAX=255，两种写法数值相同。"
    )
    d.text((32, 54), explain, font=font(16), fill=DARK)

    mse = math_img(
        r"$\mathrm{MSE}=(1/N)\sum(f-\hat{f})^{2},\ \ N=HWC$",
        fontsize=17, max_h=56,
    )
    psnr = math_img(
        r"$\mathrm{PSNR}=10\log_{10}(\mathrm{MAX}^{2}/\mathrm{MSE})$",
        fontsize=17, max_h=56,
    )
    canvas.paste(mse, (40, 102), mse)
    canvas.paste(psnr, (980, 102), psnr)
    d.text((980, 158), "本实验 MAX = 1（归一化）    8 bit 时 MAX = 255", font=font(15), fill=GRAY)

    # ----- 2x4 cards -----
    gap_x, gap_y = 12, 10
    grid_top, grid_bot = 204, H - 12
    grid_left, grid_right = 16, W - 16
    cw = (grid_right - grid_left - 3 * gap_x) // 4
    ch = (grid_bot - grid_top - gap_y) // 2

    cards = [
        dict(
            title="均匀量化", tag="（1）均匀 / 非均匀", psnr=27.53,
            principle="等间隔分成 K=8 档（线性量化）",
            tex=[
                r"$\Delta=\frac{1}{K-1},\quad \hat{f}=\Delta\cdot\mathrm{round}(f/\Delta)$",
            ],
            thumb="Q1a_均匀量化_纯图.png",
        ),
        dict(
            title="非均匀量化", tag="（1）均匀 / 非均匀", psnr=32.07,
            principle="Lloyd-Max：按灰度分布使均方误差最小",
            tex=[
                r"$z_i=\frac{q_{i-1}+q_i}{2},\quad q_i=E[z|z_i\leq z<z_{i+1}]$",
            ],
            thumb="Q1b_非均匀量化_纯图.png",
        ),
        dict(
            title="对称量化", tag="（2）对称 / 非对称", psnr=29.08,
            principle="中央平稳型，关于中灰 0.5 对称",
            tex=[
                r"$y=2f-1,\ \Delta=2/K,\ \hat{y}=\Delta\cdot\mathrm{round}(y/\Delta)$",
            ],
            thumb="Q2a_对称量化_纯图.png",
        ),
        dict(
            title="非对称量化", tag="（2）对称 / 非对称", psnr=25.39,
            principle="暗部 6 档、亮部 2 档（关于 0.5 不对称）",
            tex=[
                r"$K_{\mathrm{dark}}=6,\ K_{\mathrm{bright}}=2\ \ (f\leq 0.5\ /\ f>0.5)$",
            ],
            thumb="Q2b_非对称量化_纯图.png",
        ),
        dict(
            title="无记忆量化", tag="（3）无记忆 / 有记忆", psnr=20.29,
            principle="各像素独立量化，不考虑邻域（K=4）",
            tex=[
                r"$\hat{f}(x,y)=Q(f(x,y))$",
            ],
            thumb="Q3a_无记忆量化_纯图.png",
        ),
        dict(
            title="有记忆量化", tag="（3）无记忆 / 有记忆", psnr=24.67,
            principle="DPCM：用左邻预测，只量化残差",
            tex=[
                r"$e=f(x,y)-\hat{f}(x,y-1)$",
                r"$\hat{f}=\hat{f}_{\mathrm{pred}}+Q_e(e)$",
            ],
            thumb="Q3b_有记忆量化_纯图.png",
        ),
        dict(
            title="标量量化", tag="（4）标量 / 向量", psnr=16.47,
            principle="R、G、B 分别量化（每通道 3 电平）",
            tex=[
                r"$\hat{\mathbf{c}}=(Q(R),\,Q(G),\,Q(B)),\ \ 3^3=27$",
            ],
            thumb="Q4a_标量量化_纯图.png",
        ),
        dict(
            title="向量量化", tag="（4）标量 / 向量", psnr=30.74,
            principle="RGB 当作三维向量，一次映射到码书",
            tex=[
                r"$\hat{\mathbf{c}}=\arg\min_{c_k}\| \mathbf{c}-c_k\|_2,\ \ |\mathcal{C}|=27$",
            ],
            thumb="Q4b_向量量化_纯图.png",
        ),
    ]

    for i, card in enumerate(cards):
        r, c = divmod(i, 4)
        x0 = grid_left + c * (cw + gap_x)
        y0 = grid_top + r * (ch + gap_y)
        draw_card(
            canvas, (x0, y0, x0 + cw, y0 + ch),
            card["title"], card["psnr"], card["principle"], card["tex"], card["thumb"], card["tag"],
        )

    canvas.save(OUT, "PNG", optimize=True)
    print("wrote", OUT, canvas.size)


if __name__ == "__main__":
    main()
