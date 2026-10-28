# -*- coding: utf-8 -*-
"""Eight Lena quantization results for the four categories on slide 21."""
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw, ImageFont
import matplotlib.pyplot as plt
from matplotlib import font_manager

ROOT = Path(r"D:\works\Others\Teaching\3-Under-DigitalImageProcessing\课件v2")
SRC = ROOT / "canvas" / "ch2" / "lena_src.png"
OUT = ROOT / "lab" / "lena_quantization"
OUT.mkdir(parents=True, exist_ok=True)

NAVY = (31, 78, 121)
WHITE = (255, 255, 255)


def chinese_font_mpl():
    for name in ("Microsoft YaHei", "SimHei"):
        try:
            return font_manager.FontProperties(family=name)
        except Exception:
            continue
    return font_manager.FontProperties()


def pil_font(size):
    for p in (
        r"C:\Windows\Fonts\msyh.ttc",
        r"C:\Windows\Fonts\simhei.ttf",
        r"C:\Windows\Fonts\msyhbd.ttc",
    ):
        if Path(p).exists():
            return ImageFont.truetype(p, size=size)
    return ImageFont.load_default()


def load_lena(size=512):
    img = Image.open(SRC).convert("RGB").resize((size, size), Image.Resampling.BICUBIC)
    return np.asarray(img).astype(np.float64) / 255.0


def psnr(a, b):
    mse = np.mean((a - b) ** 2)
    if mse <= 1e-12:
        return 99.0
    return float(10.0 * np.log10(1.0 / mse))


def nearest_levels(x, levels):
    idx = np.abs(x[..., None] - levels).argmin(axis=-1)
    return levels[idx]


def uniform_levels(n, lo=0.0, hi=1.0):
    return np.linspace(lo, hi, n)


def uniform_quantize(img, n):
    levels = uniform_levels(n)
    out = np.empty_like(img)
    for c in range(img.shape[2]):
        out[..., c] = nearest_levels(img[..., c], levels)
    return np.clip(out, 0.0, 1.0)


def lloyd_max_levels(samples, n, iters=18):
    x = samples.reshape(-1)
    qs = np.quantile(x, np.linspace(0.0, 1.0, n))
    qs = np.unique(qs)
    if len(qs) < n:
        qs = np.linspace(x.min(), x.max(), n)
    for _ in range(iters):
        if len(qs) == 1:
            break
        bounds = np.concatenate([[-np.inf], 0.5 * (qs[:-1] + qs[1:]), [np.inf]])
        new_qs = []
        for i in range(len(qs)):
            m = (x >= bounds[i]) & (x < bounds[i + 1])
            new_qs.append(x[m].mean() if np.any(m) else qs[i])
        qs = np.array(new_qs)
    return np.sort(qs)


def nonuniform_lloyd(img, n):
    out = np.empty_like(img)
    for c in range(img.shape[2]):
        qs = lloyd_max_levels(img[..., c], n)
        out[..., c] = nearest_levels(img[..., c], qs)
    return np.clip(out, 0.0, 1.0)


def symmetric_midtread(img, n):
    """Symmetric mid-tread about 0.5. Even n: reconstruction symmetric, includes a dead-zone around mid-gray."""
    y = 2.0 * img - 1.0
    delta = 2.0 / n
    q = delta * np.round(y / delta)
    q = np.clip(q, -1.0 + 0.5 * delta, 1.0 - 0.5 * delta)
    return np.clip(0.5 * (q + 1.0), 0.0, 1.0)


def asymmetric_quantize(img, n_dark=6, n_bright=2):
    """More reconstruction levels in shadows than in highlights (not symmetric about 0.5)."""
    out = np.empty_like(img)
    dark_lv = uniform_levels(n_dark, 0.0, 0.5)
    bright_lv = uniform_levels(n_bright, 0.5, 1.0)
    # avoid duplicating 0.5
    if n_bright > 1:
        bright_lv = uniform_levels(n_bright, 0.5 + 0.5 / n_bright, 1.0)
    levels = np.unique(np.concatenate([dark_lv, bright_lv]))
    for c in range(img.shape[2]):
        out[..., c] = nearest_levels(img[..., c], levels)
    return np.clip(out, 0.0, 1.0)


def dpcm_quantize(img, n):
    """Horizontal DPCM: left-neighbor predictor, small-step residual codebook."""
    rec = np.empty_like(img)
    if n == 4:
        res_levels = np.array([-0.36, -0.10, 0.10, 0.36])
    else:
        res_levels = np.linspace(-0.45, 0.45, n)
    rec[:, 0] = nearest_levels(img[:, 0], uniform_levels(n))
    for j in range(1, img.shape[1]):
        pred = rec[:, j - 1]
        rec[:, j] = np.clip(pred + nearest_levels(img[:, j] - pred, res_levels), 0.0, 1.0)
    return rec


def kmeans_codebook(X, k, iters=12, seed=0, batch=8192):
    rng = np.random.default_rng(seed)
    n = X.shape[0]
    cents = X[rng.choice(n, size=k, replace=False)].astype(np.float64).copy()
    labels = np.empty(n, dtype=np.int32)
    for _ in range(iters):
        for s in range(0, n, batch):
            e = min(s + batch, n)
            d = ((X[s:e, None, :] - cents[None, :, :]) ** 2).sum(axis=2)
            labels[s:e] = d.argmin(axis=1)
        for j in range(k):
            m = labels == j
            if np.any(m):
                cents[j] = X[m].mean(axis=0)
            else:
                cents[j] = X[rng.integers(0, n)]
    return cents, labels


def vector_quantize_rgb(img, k=64):
    h, w, c = img.shape
    X = img.reshape(-1, 3)
    cents, labels = kmeans_codebook(X, k)
    rec = cents[labels].reshape(h, w, 3)
    return np.clip(rec, 0.0, 1.0)


def add_banner(img, title, subtitle):
    arr = (np.clip(img, 0, 1) * 255).astype(np.uint8)
    im = Image.fromarray(arr, mode="RGB")
    bar_h = 58
    canvas = Image.new("RGB", (im.width, im.height + bar_h), NAVY)
    canvas.paste(im, (0, bar_h))
    draw = ImageDraw.Draw(canvas)
    font_t = pil_font(22)
    font_s = pil_font(15)
    draw.text((16, 8), title, font=font_t, fill=WHITE)
    draw.text((16, 34), subtitle, font=font_s, fill=(210, 222, 235))
    return canvas


def save_pair(img, orig, stem, title, subtitle):
    val = psnr(img, orig)
    bannered = add_banner(img, title, f"{subtitle}    PSNR = {val:.2f} dB")
    path = OUT / f"{stem}.png"
    bannered.save(path, optimize=True)
    Image.fromarray((np.clip(img, 0, 1) * 255).astype(np.uint8)).save(OUT / f"{stem}_纯图.png")
    return path, val


def main():
    orig = load_lena(512)

    jobs = []

    q1a = uniform_quantize(orig, 8)
    jobs.append(save_pair(
        q1a, orig, "Q1a_均匀量化",
        "（1）均匀量化",
        "每通道 8 个等间隔电平",
    ))

    q1b = nonuniform_lloyd(orig, 8)
    jobs.append(save_pair(
        q1b, orig, "Q1b_非均匀量化",
        "（1）非均匀量化",
        "Lloyd-Max，每通道 8 电平",
    ))

    q2a = symmetric_midtread(orig, 8)
    jobs.append(save_pair(
        q2a, orig, "Q2a_对称量化",
        "（2）对称量化",
        "中央平稳型，关于 0.5 对称",
    ))

    q2b = asymmetric_quantize(orig, n_dark=6, n_bright=2)
    jobs.append(save_pair(
        q2b, orig, "Q2b_非对称量化",
        "（2）非对称量化",
        "暗部 6 电平 / 亮部 2 电平",
    ))

    q3a = uniform_quantize(orig, 4)
    jobs.append(save_pair(
        q3a, orig, "Q3a_无记忆量化",
        "（3）无记忆量化",
        "像素独立量化，每通道 4 电平",
    ))

    q3b = dpcm_quantize(orig, 4)
    jobs.append(save_pair(
        q3b, orig, "Q3b_有记忆量化",
        "（3）有记忆量化",
        "DPCM 左邻预测，残差 4 电平",
    ))

    q4a = uniform_quantize(orig, 3)
    jobs.append(save_pair(
        q4a, orig, "Q4a_标量量化",
        "（4）标量量化",
        "RGB 逐个量化，每通道 3 电平（27 色）",
    ))

    q4b = vector_quantize_rgb(orig, k=27)
    jobs.append(save_pair(
        q4b, orig, "Q4b_向量量化",
        "（4）向量量化",
        "RGB 联合量化，码书 27 色",
    ))

    fp = chinese_font_mpl()
    plt.rcParams["axes.unicode_minus"] = False
    imgs = [q1a, q1b, q2a, q2b, q3a, q3b, q4a, q4b]
    titles = [
        "均匀量化", "非均匀量化",
        "对称量化", "非对称量化",
        "无记忆量化", "有记忆量化",
        "标量量化", "向量量化",
    ]
    fig, axes = plt.subplots(2, 4, figsize=(14.2, 7.6), facecolor="white")
    for ax, im, t, (_, p) in zip(axes.ravel(), imgs, titles, jobs):
        ax.imshow(im)
        ax.set_title(f"{t}\nPSNR {p:.2f} dB", fontproperties=fp, fontsize=11, color="#1f4e79")
        ax.set_xticks([])
        ax.set_yticks([])
        for sp in ax.spines.values():
            sp.set_color("#1f4e79")
    fig.suptitle("四种量化分类对比（Lena）", fontproperties=fp, fontsize=16, color="#1f4e79")
    fig.tight_layout(rect=[0, 0, 1, 0.95])
    fig.savefig(OUT / "Q0_八图总览.png", dpi=150, bbox_inches="tight", facecolor="white")
    fig.savefig(ROOT / "lab" / "lena四种量化分类对比_八图.png", dpi=150, bbox_inches="tight", facecolor="white")
    plt.close(fig)

    for path, val in jobs:
        print(f"{val:6.2f} dB  {path.name}")


if __name__ == "__main__":
    main()
