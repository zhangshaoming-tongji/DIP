# -*- coding: utf-8 -*-
"""Three digital-image distances: Euclidean, city-block (D4), chessboard (D8)."""
from pathlib import Path
import numpy as np
import matplotlib.pyplot as plt
from matplotlib import font_manager
from matplotlib.patches import FancyBboxPatch, Rectangle, Circle, Polygon
from matplotlib.lines import Line2D

ROOT = Path(r"D:\works\Others\Teaching\3-Under-DigitalImageProcessing\课件v2")
OUT = ROOT / "canvas" / "ch2" / "图像距离_欧式街区棋盘.png"

NAVY = "#1B4E79"
RED = "#B42024"
BLUE = "#1565C0"
GREEN = "#2E7D32"
DARK = "#222222"
MUTED = "#5A5A5A"
GRID = "#C5CDD4"

# 9x9 so the D4=4 diamond vertices lie on the grid
HALF = 4
P = (0, 0)
Q = (2, 2)
D4_PATH = [(0, 0), (1, 0), (2, 0), (2, 1), (2, 2)]
D8_PATH = [(0, 0), (1, 1), (2, 2)]


def fp(size=16, weight="normal"):
    return font_manager.FontProperties(fname=r"C:\Windows\Fonts\msyh.ttc", size=size, weight=weight)


def fp_bold(size=16):
    return font_manager.FontProperties(fname=r"C:\Windows\Fonts\msyhbd.ttc", size=size)


def cell_center(x, y):
    return x + 0.5, y + 0.5


def draw_grid(ax):
    for i in range(-HALF, HALF + 1):
        for j in range(-HALF, HALF + 1):
            ax.add_patch(
                Rectangle((i, j), 1, 1, facecolor="white", edgecolor=GRID, linewidth=0.9, zorder=1)
            )
    ax.set_xlim(-HALF, HALF + 1)
    ax.set_ylim(-HALF, HALF + 1)
    ax.set_aspect("equal")
    ax.axis("off")


def fill_cell(ax, x, y, color, z=2):
    ax.add_patch(
        Rectangle((x, y), 1, 1, facecolor=color, edgecolor=GRID, linewidth=0.9, zorder=z)
    )


def mark_pq(ax):
    fill_cell(ax, *P, RED, z=6)
    fill_cell(ax, *Q, BLUE, z=6)
    ax.text(*cell_center(*P), "p", ha="center", va="center",
            color="white", fontsize=15, fontweight="bold", zorder=7)
    ax.text(*cell_center(*Q), "q", ha="center", va="center",
            color="white", fontsize=15, fontweight="bold", zorder=7)


def draw_path(ax, pts, color):
    for a, b in zip(pts[:-1], pts[1:]):
        ax.annotate(
            "",
            xy=cell_center(*b),
            xytext=cell_center(*a),
            arrowprops=dict(
                arrowstyle="-|>",
                color=color,
                lw=2.4,
                mutation_scale=13,
                shrinkA=11,
                shrinkB=11,
            ),
            zorder=8,
        )


def write_dist(ax, x, y, val, color="#444444"):
    if (x, y) in (P, Q):
        return
    ax.text(*cell_center(x, y), str(val), ha="center", va="center",
            fontsize=10, color=color, zorder=4)


def panel_card(fig, rect, title_cn, title_math, formula, note, result, paint):
    ax_box = fig.add_axes(rect)
    ax_box.set_xlim(0, 1)
    ax_box.set_ylim(0, 1)
    ax_box.axis("off")
    ax_box.add_patch(
        FancyBboxPatch(
            (0.012, 0.012), 0.976, 0.976,
            boxstyle="round,pad=0.01,rounding_size=0.018",
            facecolor="#F7FAFC",
            edgecolor="#D5DEE6",
            linewidth=1.15,
        )
    )
    ax_box.text(0.42, 0.955, title_cn, ha="right", va="top", fontproperties=fp_bold(16), color=NAVY)
    ax_box.text(0.45, 0.948, title_math, ha="left", va="top", fontsize=16, color=NAVY)
    ax_box.text(0.50, 0.118, formula, ha="center", va="center", fontsize=12.5, color=DARK)
    ax_box.text(0.50, 0.068, note, ha="center", va="center", fontproperties=fp(11.5), color=MUTED)
    ax_box.text(0.50, 0.028, result, ha="center", va="center", fontsize=12.5, color=RED, fontweight="bold")

    gx, gy, gw, gh = rect
    ax = fig.add_axes([gx + 0.028, gy + 0.145, gw - 0.056, gh - 0.230])
    draw_grid(ax)
    paint(ax)
    mark_pq(ax)
    return ax


def paint_euclidean(ax):
    cx, cy = cell_center(*P)
    for r, lw, ls, alpha in (
        (1.0, 1.1, "-", 0.35),
        (2.0, 1.1, "-", 0.40),
        (np.hypot(2, 2), 2.0, "--", 0.85),
    ):
        ax.add_patch(Circle(
            (cx, cy), r, fill=False, edgecolor=RED, linewidth=lw,
            linestyle=ls, alpha=alpha, zorder=3,
        ))
    x0, y0 = cell_center(*P)
    x1, y1 = cell_center(*Q)
    ax.add_line(Line2D([x0, x1], [y0, y1], color=RED, lw=2.4, zorder=5))


def paint_cityblock(ax):
    colors = {1: "#FFF6C8", 2: "#FFE08A", 3: "#FFC04D", 4: "#F4A261"}
    for x in range(-HALF, HALF + 1):
        for y in range(-HALF, HALF + 1):
            d = abs(x) + abs(y)
            if (x, y) not in (P, Q) and d in colors:
                fill_cell(ax, x, y, colors[d])
            write_dist(ax, x, y, d, "#5C4033")
    for pt in D4_PATH[1:-1]:
        fill_cell(ax, *pt, "#90CAF9", z=3)
        write_dist(ax, *pt, abs(pt[0]) + abs(pt[1]), "#0D47A1")
    # complete discrete diamond through q (D4 = 4)
    diamond = [(4.5, 0.5), (0.5, 4.5), (-3.5, 0.5), (0.5, -3.5)]
    ax.add_patch(Polygon(
        diamond, closed=True, fill=False, edgecolor=BLUE, linewidth=2.0, zorder=5,
    ))
    draw_path(ax, D4_PATH, BLUE)


def paint_chessboard(ax):
    colors = {1: "#A5D6A7", 2: "#C8E6C9", 3: "#E8F5E9", 4: "#F1F8E9"}
    for x in range(-HALF, HALF + 1):
        for y in range(-HALF, HALF + 1):
            d = max(abs(x), abs(y))
            if (x, y) not in (P, Q) and d in colors:
                fill_cell(ax, x, y, colors[d])
            write_dist(ax, x, y, d, "#1B5E20")
    mid = (1, 1)
    fill_cell(ax, *mid, "#81C784", z=3)
    write_dist(ax, *mid, 1, "#1B5E20")
    square = [(-1.5, -1.5), (2.5, -1.5), (2.5, 2.5), (-1.5, 2.5)]
    ax.add_patch(Polygon(
        square, closed=True, fill=False, edgecolor=GREEN, linewidth=2.0, zorder=5,
    ))
    draw_path(ax, D8_PATH, GREEN)


def main():
    plt.rcParams["mathtext.fontset"] = "stix"
    plt.rcParams["axes.unicode_minus"] = False
    fig = plt.figure(figsize=(16.2, 8.0), dpi=180, facecolor="white")

    fig.text(0.5, 0.965, "图像中的三种距离", ha="center", va="top",
             fontproperties=fp_bold(26), color=NAVY)
    fig.text(0.5, 0.918, "从同一对像素 p → q = (2, 2) 比较三种距离；中心格只标 p",
             ha="center", va="top", fontproperties=fp(13), color=MUTED)

    w, gap = 0.305, 0.022
    x0 = (1 - 3 * w - 2 * gap) / 2
    y, h = 0.035, 0.86
    panel_card(
        fig, [x0, y, w, h],
        "欧式距离", r"$D_e$",
        r"$D_e(p,q)=\sqrt{(x-s)^2+(y-t)^2}$",
        "等距线为圆，直线最短",
        r"$D_e(p,q)=\sqrt{8}\approx 2.83$",
        paint_euclidean,
    )
    panel_card(
        fig, [x0 + w + gap, y, w, h],
        "街区距离", r"$D_4$",
        r"$D_4(p,q)=|x-s|+|y-t|$",
        "等距线为菱形，只走四邻接  ↑↓←→",
        r"$D_4(p,q)=2+2=4$",
        paint_cityblock,
    )
    panel_card(
        fig, [x0 + 2 * (w + gap), y, w, h],
        "棋盘距离", r"$D_8$",
        r"$D_8(p,q)=\max(|x-s|,|y-t|)$",
        "等距线为正方形，可走八邻接",
        r"$D_8(p,q)=\max(2,2)=2$",
        paint_chessboard,
    )

    fig.savefig(OUT, dpi=190, bbox_inches="tight", facecolor="white", pad_inches=0.16)
    plt.close(fig)
    print(OUT)


if __name__ == "__main__":
    main()
