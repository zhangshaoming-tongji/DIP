# -*- coding: utf-8 -*-
"""Slide 18 demo: square LPF reconstruction -> blur; circular LPF -> moire."""
from pathlib import Path
import numpy as np
from PIL import Image
import matplotlib.pyplot as plt
from matplotlib import font_manager
from matplotlib.patches import Rectangle, Circle

ROOT = Path(r"D:\works\Others\Teaching\3-Under-DigitalImageProcessing\课件v2")
SRC = ROOT / "canvas" / "ch2" / "lena_src.png"
OUT = ROOT / "canvas" / "ch2" / "lena_sampling_demo"
OUT.mkdir(parents=True, exist_ok=True)


def chinese_font():
    for name in ("Microsoft YaHei", "SimHei", "Noto Sans CJK SC", "Source Han Sans SC"):
        try:
            return font_manager.FontProperties(family=name)
        except Exception:
            continue
    return font_manager.FontProperties()


def to_float_rgb(path, size=512):
    img = Image.open(path).convert("RGB")
    img = img.resize((size, size), Image.Resampling.BICUBIC)
    return np.asarray(img).astype(np.float64) / 255.0


def add_highfreq_texture(img, amp=0.13):
    """Grating at ~0.371 cycle/px. After stride-4 sampling, the copy nearest
    DC sits at ~62 bins: outside the square LPF, inside the circular LPF."""
    h, w = img.shape[:2]
    y, x = np.mgrid[0:h, 0:w].astype(np.float64)
    grating = np.cos(2 * np.pi * (0.371 * x + 0.035 * y))
    out = img + amp * grating[..., None]
    return np.clip(out, 0.0, 1.0)


def impulse_sample(img, stride):
    sampled = np.zeros_like(img)
    sampled[::stride, ::stride] = img[::stride, ::stride]
    return sampled


def fft_channel(ch):
    return np.fft.fftshift(np.fft.fft2(ch))


def ifft_channel(F):
    rec = np.fft.ifft2(np.fft.ifftshift(F))
    return np.real(rec)


def coord_grids(h, w):
    v = np.arange(h) - h // 2
    u = np.arange(w) - w // 2
    U, V = np.meshgrid(u, v)
    return U, V


def square_mask(h, w, half):
    U, V = coord_grids(h, w)
    return (np.abs(U) <= half) & (np.abs(V) <= half)


def circle_mask(h, w, radius):
    U, V = coord_grids(h, w)
    return (U * U + V * V) <= radius * radius


def reconstruct(sampled, mask, gain):
    rec = np.zeros_like(sampled)
    for c in range(sampled.shape[2]):
        F = fft_channel(sampled[..., c])
        rec[..., c] = ifft_channel(F * mask) * gain
    return np.clip(rec, 0.0, 1.0)


def save_rgb(arr, path):
    Image.fromarray(np.clip(arr * 255, 0, 255).astype(np.uint8)).save(path)


def log_spectrum_rgb(img):
    mag = None
    for c in range(3):
        F = fft_channel(img[..., c])
        m = np.log1p(np.abs(F))
        mag = m if mag is None else mag + m
    mag /= 3.0
    mag = mag - mag.min()
    if mag.max() > 0:
        mag = mag / mag.max()
    return mag


def main():
    fp = chinese_font()
    plt.rcParams["axes.unicode_minus"] = False

    original_clean = to_float_rgb(SRC, 512)
    original = add_highfreq_texture(original_clean, amp=0.14)

    stride = 4
    sampled = impulse_sample(original, stride)
    h, w = original.shape[:2]
    nyquist_half = h / (2.0 * stride)  # 64 bins
    replica = h / float(stride)        # 128 bins

    # Keep a gap: aliased grating ~ (62, 18) bins from DC
    square_half = 48.0
    circle_r = 92.0

    sq = square_mask(h, w, square_half)
    circ = circle_mask(h, w, circle_r)
    gain = stride * stride

    blur = reconstruct(sampled, sq, gain)
    moire = reconstruct(sampled, circ, gain)

    save_rgb(original, OUT / "lena_original.png")
    save_rgb(blur, OUT / "lena_blur.png")
    save_rgb(moire, OUT / "lena_moire.png")
    save_rgb(original_clean, OUT / "lena_original_clean.png")

    # Also copy to course root with Chinese names
    save_rgb(original, ROOT / "lab" / "lena原图.png")
    save_rgb(blur, ROOT / "lab" / "lena模糊图.png")
    save_rgb(moire, ROOT / "lab" / "lena莫尔条纹图.png")

    spec_s = log_spectrum_rgb(sampled)
    spec_o = log_spectrum_rgb(original)

    fig, axes = plt.subplots(2, 3, figsize=(12.8, 8.4), facecolor="white")
    titles = ["原图（含高频纹理）", "方框 LPF 重建：模糊", "圆形 LPF 重建：莫尔条纹"]
    images = [original, blur, moire]
    for ax, im, t in zip(axes[0], images, titles):
        ax.imshow(im)
        ax.set_title(t, fontproperties=fp, fontsize=13, color="#1f4e79", pad=8)
        ax.set_xticks([])
        ax.set_yticks([])
        for spine in ax.spines.values():
            spine.set_color("#1f4e79")
            spine.set_linewidth(1.2)

    # Spectra: original, sampled with square, sampled with circle
    axes[1, 0].imshow(spec_o, cmap="magma", origin="upper")
    axes[1, 0].set_title("原图频谱", fontproperties=fp, fontsize=12, color="#1f4e79")

    axes[1, 1].imshow(spec_s, cmap="magma", origin="upper")
    rect = Rectangle(
        (w / 2 - square_half, h / 2 - square_half),
        2 * square_half,
        2 * square_half,
        fill=False,
        edgecolor="black",
        linewidth=1.8,
    )
    axes[1, 1].add_patch(rect)
    axes[1, 1].set_title("采样后频谱 + 黑色方框 LPF", fontproperties=fp, fontsize=12, color="#1f4e79")

    axes[1, 2].imshow(spec_s, cmap="magma", origin="upper")
    circ_p = Circle((w / 2, h / 2), circle_r, fill=False, edgecolor="red", linewidth=1.8)
    axes[1, 2].add_patch(circ_p)
    axes[1, 2].set_title("采样后频谱 + 红色圆 LPF", fontproperties=fp, fontsize=12, color="#1f4e79")

    for ax in axes[1]:
        ax.set_xticks([])
        ax.set_yticks([])

    fig.suptitle(
        "采样间隔不满足奈奎斯特条件时的两种重建",
        fontproperties=fp,
        fontsize=16,
        color="#1f4e79",
        y=0.98,
    )
    fig.tight_layout(rect=[0, 0, 1, 0.95])
    compare_path = OUT / "lena_blur_moire_compare.png"
    fig.savefig(compare_path, dpi=160, bbox_inches="tight", facecolor="white")
    fig.savefig(ROOT / "lab" / "lena模糊与莫尔条纹对比.png", dpi=160, bbox_inches="tight", facecolor="white")
    plt.close(fig)

    # Three-image strip only (for PPT insertion)
    fig2, axes2 = plt.subplots(1, 3, figsize=(13.5, 4.6), facecolor="white")
    labels = ["① 原图", "② 模糊（方框 LPF）", "③ 莫尔条纹（圆形 LPF）"]
    for ax, im, t in zip(axes2, images, labels):
        ax.imshow(im)
        ax.set_title(t, fontproperties=fp, fontsize=14, color="#1f4e79", pad=6)
        ax.axis("off")
    fig2.tight_layout()
    fig2.savefig(OUT / "lena_three_panel.png", dpi=170, bbox_inches="tight", facecolor="white")
    fig2.savefig(ROOT / "lab" / "lena三图对比_原图模糊莫尔.png", dpi=170, bbox_inches="tight", facecolor="white")
    plt.close(fig2)

    print("square_half", square_half, "circle_r", circle_r, "replica", replica)
    print("wrote", OUT)


if __name__ == "__main__":
    main()
