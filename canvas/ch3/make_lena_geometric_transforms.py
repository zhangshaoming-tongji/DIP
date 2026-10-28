# -*- coding: utf-8 -*-
"""Lena demo: Euclidean, similarity, affine, and projective transforms (slide 11)."""
from __future__ import annotations

import json
from pathlib import Path

import cv2
import matplotlib.pyplot as plt
import numpy as np
from matplotlib import font_manager
from PIL import Image

ROOT = Path(r"D:\works\Others\Teaching\3-Under-DigitalImageProcessing\课件v2")
SRC = ROOT / "canvas" / "ch2" / "lena_src.png"
OUT_DIR = ROOT / "canvas" / "ch3" / "lena_geometric_transforms"

NAVY = "#1B4E79"
MUTED = "#5A5A5A"


def fp(size=14, weight="normal"):
    return font_manager.FontProperties(fname=r"C:\Windows\Fonts\msyh.ttc", size=size, weight=weight)


def fp_bold(size=14):
    return font_manager.FontProperties(fname=r"C:\Windows\Fonts\msyhbd.ttc", size=size)


def T_translate(tx: float, ty: float) -> np.ndarray:
    return np.array([[1, 0, tx], [0, 1, ty], [0, 0, 1]], dtype=np.float64)


def T_rotate(theta_deg: float) -> np.ndarray:
    t = np.deg2rad(theta_deg)
    c, s = np.cos(t), np.sin(t)
    return np.array([[c, -s, 0], [s, c, 0], [0, 0, 1]], dtype=np.float64)


def T_scale(sx: float, sy: float | None = None) -> np.ndarray:
    if sy is None:
        sy = sx
    return np.array([[sx, 0, 0], [0, sy, 0], [0, 0, 1]], dtype=np.float64)


def T_shear(shx: float, shy: float = 0.0) -> np.ndarray:
    return np.array([[1, shx, 0], [shy, 1, 0], [0, 0, 1]], dtype=np.float64)


def rotate_about(H: int, W: int, theta_deg: float, tx: float, ty: float) -> np.ndarray:
    cx, cy = (W - 1) / 2.0, (H - 1) / 2.0
    return T_translate(tx, ty) @ T_translate(cx, cy) @ T_rotate(theta_deg) @ T_translate(-cx, -cy)


def warp_forward(img: np.ndarray, H_fwd: np.ndarray) -> np.ndarray:
    h, w = img.shape[:2]
    H_inv = np.linalg.inv(H_fwd)
    if np.allclose(H_fwd[2, :2], 0) and H_fwd[2, 2] == 1:
        M = H_inv[:2, :]
        return cv2.warpAffine(
            img,
            M,
            (w, h),
            flags=cv2.INTER_LINEAR,
            borderMode=cv2.BORDER_CONSTANT,
            borderValue=(0, 0, 0),
        )
    return cv2.warpPerspective(
        img,
        H_inv,
        (w, h),
        flags=cv2.INTER_LINEAR,
        borderMode=cv2.BORDER_CONSTANT,
        borderValue=(0, 0, 0),
    )


def fmt_matrix(H: np.ndarray) -> str:
    rows = []
    for r in range(3):
        cells = []
        for c in range(3):
            v = H[r, c]
            if abs(v - round(v)) < 1e-6:
                cells.append(f"{int(round(v))}")
            else:
                cells.append(f"{v: .4f}".strip())
        rows.append("  ".join(f"{x:>8}" for x in cells))
    return "\n".join(rows)


def main():
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    if not SRC.is_file():
        raise FileNotFoundError(SRC)
    img = np.asarray(Image.open(SRC).convert("RGB"))
    h, w = img.shape[:2]

    # --- parameters (forward: source pixel -> output coordinates) ---
    eucl = {
        "name": "欧式变换",
        "dof": 3,
        "params": {"theta_deg": 25.0, "tx": 45.0, "ty": 30.0},
    }
    H_eucl = rotate_about(h, w, **eucl["params"])

    sim = {
        "name": "相似变换",
        "dof": 4,
        "params": {"theta_deg": 25.0, "scale": 0.72, "tx": 70.0, "ty": 55.0},
    }
    cx, cy = (w - 1) / 2.0, (h - 1) / 2.0
    H_sim = (
        T_translate(sim["params"]["tx"], sim["params"]["ty"])
        @ T_translate(cx, cy)
        @ T_rotate(sim["params"]["theta_deg"])
        @ T_scale(sim["params"]["scale"])
        @ T_translate(-cx, -cy)
    )

    aff = {
        "name": "仿射变换",
        "dof": 6,
        "params": {
            "theta_deg": 12.0,
            "sx": 0.95,
            "sy": 0.78,
            "shear_x": 0.38,
            "tx": 35.0,
            "ty": 42.0,
        },
    }
    H_aff = (
        T_translate(aff["params"]["tx"], aff["params"]["ty"])
        @ T_translate(cx, cy)
        @ T_rotate(aff["params"]["theta_deg"])
        @ T_shear(aff["params"]["shear_x"])
        @ T_scale(aff["params"]["sx"], aff["params"]["sy"])
        @ T_translate(-cx, -cy)
    )

    # Projective: move top edge inward (keystone / perspective).
    proj = {
        "name": "射影变换",
        "dof": 8,
        "params": {
            "top_inset_px": 95,
            "bottom_outset_px": 25,
            "vertical_skew": 0.06,
        },
    }
    ti = proj["params"]["top_inset_px"]
    bo = proj["params"]["bottom_outset_px"]
    vs = proj["params"]["vertical_skew"]
    src_pts = np.float32([[0, 0], [w - 1, 0], [w - 1, h - 1], [0, h - 1]])
    dst_pts = np.float32(
        [
            [ti, vs * h],
            [w - 1 - ti, vs * h],
            [w - 1 + bo, h - 1],
            [-bo, h - 1],
        ]
    )
    H_proj = cv2.getPerspectiveTransform(src_pts, dst_pts)

    cases = [
        (eucl, H_eucl),
        (sim, H_sim),
        (aff, H_aff),
        (proj, H_proj),
    ]

    meta = []
    results = []
    for info, H_fwd in cases:
        out = warp_forward(img, H_fwd)
        stem = {
            "欧式变换": "euclidean",
            "相似变换": "similarity",
            "仿射变换": "affine",
            "射影变换": "projective",
        }[info["name"]]
        path = OUT_DIR / f"lena_{stem}.png"
        Image.fromarray(out).save(path)
        root_path = ROOT / "lab" / f"lena几何变换_{info['name'].replace('变换', '')}.png"
        Image.fromarray(out).save(root_path)
        results.append((info, H_fwd, out))
        meta.append(
            {
                "name": info["name"],
                "dof": info["dof"],
                "params": info.get("params"),
                "matrix_forward": H_fwd.tolist(),
                "matrix_inverse_for_warp": np.linalg.inv(H_fwd).tolist(),
                "output": str(path.relative_to(ROOT)),
            }
        )

    (OUT_DIR / "transform_meta.json").write_text(
        json.dumps(meta, ensure_ascii=False, indent=2), encoding="utf-8"
    )

    plt.rcParams["axes.unicode_minus"] = False
    fig, axes = plt.subplots(2, 3, figsize=(15.5, 10.2), dpi=160, facecolor="white")
    fig.suptitle("Lena 几何变换（逆向映射 + 双线性插值）", fontproperties=fp_bold(20), color=NAVY, y=0.98)

    axes[0, 0].imshow(img)
    axes[0, 0].set_title("原图", fontproperties=fp_bold(13))
    axes[0, 0].axis("off")

    slots = [(0, 1), (0, 2), (1, 0), (1, 1)]
    for (info, H_fwd, out), (r, c) in zip(results, slots):
        ax = axes[r, c]
        ax.imshow(out)
        p = info["params"]
        if info["name"] == "射影变换":
            param_txt = (
                f"上边内收 {p['top_inset_px']} px，下边外扩 {p['bottom_outset_px']} px\n"
                f"由四角对应点求 H（8 自由度）"
            )
        elif info["name"] == "欧式变换":
            param_txt = f"θ={p['theta_deg']}°, tx={p['tx']}, ty={p['ty']}"
        elif info["name"] == "相似变换":
            param_txt = (
                f"θ={p['theta_deg']}°, s={p['scale']}, tx={p['tx']}, ty={p['ty']}"
            )
        else:
            param_txt = (
                f"θ={p['theta_deg']}°, sx={p['sx']}, sy={p['sy']}, "
                f"shear={p['shear_x']}, tx={p['tx']}, ty={p['ty']}"
            )
        ax.set_title(f"{info['name']}（{info['dof']} 自由度）", fontproperties=fp_bold(12))
        ax.text(
            0.5,
            -0.06,
            param_txt,
            transform=ax.transAxes,
            ha="center",
            va="top",
            fontproperties=fp(10),
            color=MUTED,
        )
        ax.axis("off")

    ax_m = axes[1, 2]
    ax_m.axis("off")
    lines = ["正向齐次矩阵 H（p′ = H p）：", ""]
    for info, H_fwd, _ in results:
        lines.append(f"【{info['name']}】")
        lines.append(fmt_matrix(H_fwd))
        lines.append("")
    ax_m.text(
        0.02,
        0.98,
        "\n".join(lines).rstrip(),
        transform=ax_m.transAxes,
        ha="left",
        va="top",
        fontproperties=font_manager.FontProperties(
            fname=r"C:\Windows\Fonts\msyh.ttc", size=10
        ),
        color="#222222",
    )

    fig.tight_layout(rect=[0, 0, 1, 0.96])
    combo = ROOT / "lab" / "lena几何变换_四合一对比.png"
    fig.savefig(combo, dpi=170, bbox_inches="tight", facecolor="white", pad_inches=0.12)
    fig.savefig(OUT_DIR / "lena_four_panel.png", dpi=170, bbox_inches="tight", facecolor="white")
    plt.close(fig)

    print("Saved:")
    for p in [
        combo,
        *[ROOT / "lab" / f"lena几何变换_{info['name'].replace('变换', '')}.png" for info, _, _ in results],
    ]:
        print(" ", p)
    print(" meta:", OUT_DIR / "transform_meta.json")


if __name__ == "__main__":
    main()
