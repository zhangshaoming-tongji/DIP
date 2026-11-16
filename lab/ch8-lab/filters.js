/* 频域分析课堂演示里实际运行的算法。右侧代码面板显示的就是这些函数。 */
(function (root, factory) {
  const api = factory();
  if (typeof module === "object" && module.exports) module.exports = api;
  else root.Filters = api;
})(typeof globalThis !== "undefined" ? globalThis : this, function () {
  function modulate(re, im, n) {
    // 乘 (−1)^{x+y}，把直流从左上角移到频谱中心。再乘一次就移回去。
    for (let y = 0; y < n; y++) {
      for (let x = 0; x < n; x++) {
        if ((x + y) & 1) {
          const i = y * n + x;
          re[i] = -re[i];
          im[i] = -im[i];
        }
      }
    }
  }

  function centeredDft(gray, n) {
    const re = new Float64Array(gray);
    const im = new Float64Array(n * n);
    modulate(re, im, n);
    globalThis.Freq.fft2(re, im, n, false);
    return { re, im };
  }

  function rawDft(gray, n) {
    const re = new Float64Array(gray);
    const im = new Float64Array(n * n);
    globalThis.Freq.fft2(re, im, n, false);
    return { re, im };
  }

  function reconstructFrom(re, im, n) {
    const outRe = new Float64Array(re);
    const outIm = new Float64Array(im);
    globalThis.Freq.fft2(outRe, outIm, n, true);
    modulate(outRe, outIm, n);
    return outRe;
  }

  function synthesize(spec, n, mode) {
    // mode: full 原频谱；mag 相位清零；phase 幅度置 1；random 幅度保留、相位随机。
    const re = spec.re;
    const im = spec.im;
    const outRe = new Float64Array(re.length);
    const outIm = new Float64Array(re.length);
    if (mode === "random") {
      const phase = new Float64Array(re.length);
      for (let v = 0; v < n; v++) {
        for (let u = 0; u < n; u++) {
          const u2 = (n - u) % n;
          const v2 = (n - v) % n;
          if (v > v2 || (v === v2 && u > u2)) continue;
          let p = Math.random() * Math.PI * 2;
          if (u === u2 && v === v2) p = 0;
          phase[v * n + u] = p;
          phase[v2 * n + u2] = -p;
        }
      }
      for (let i = 0; i < re.length; i++) {
        const m = Math.hypot(re[i], im[i]);
        outRe[i] = m * Math.cos(phase[i]);
        outIm[i] = m * Math.sin(phase[i]);
      }
    } else {
      for (let i = 0; i < re.length; i++) {
        const m = Math.hypot(re[i], im[i]);
        if (mode === "mag") {
          outRe[i] = m;
        } else if (mode === "phase") {
          if (m > 1e-8) {
            outRe[i] = re[i] / m;
            outIm[i] = im[i] / m;
          }
        } else {
          outRe[i] = re[i];
          outIm[i] = im[i];
        }
      }
    }
    return reconstructFrom(outRe, outIm, n);
  }

  function mixMagPhase(magSpec, phaseSpec, n) {
    // 用一幅图的幅度、另一幅图的相位组成新频谱，再逆变换。
    const re = new Float64Array(magSpec.re.length);
    const im = new Float64Array(magSpec.re.length);
    for (let i = 0; i < re.length; i++) {
      const m = Math.hypot(magSpec.re[i], magSpec.im[i]);
      const p = Math.hypot(phaseSpec.re[i], phaseSpec.im[i]);
      if (p < 1e-8) continue;
      re[i] = m * phaseSpec.re[i] / p;
      im[i] = m * phaseSpec.im[i] / p;
    }
    return reconstructFrom(re, im, n);
  }

  function checkerImage(gray, n) {
    const out = new Float64Array(n * n);
    for (let y = 0; y < n; y++) {
      for (let x = 0; x < n; x++) {
        const i = y * n + x;
        out[i] = ((x + y) & 1) ? -gray[i] : gray[i];
      }
    }
    return out;
  }

  function applyH(spec, H, n) {
    // 频谱已经中心化。H 与之相乘，逆变换，再乘 (−1)^{x+y} 回到空域。
    const re = new Float64Array(spec.re);
    const im = new Float64Array(spec.im);
    for (let i = 0; i < re.length; i++) {
      re[i] *= H[i];
      im[i] *= H[i];
    }
    globalThis.Freq.fft2(re, im, n, true);
    modulate(re, im, n);
    return re;
  }

  function lowpassH(n, d0, order, type) {
    // 理想：圆内为 1，圆外为 0。高斯与巴特沃斯在截止频率处平滑下降。
    const H = new Float64Array(n * n);
    const c = n / 2;
    const d = Math.max(d0, 0.5);
    for (let v = 0; v < n; v++) {
      for (let u = 0; u < n; u++) {
        const D = Math.hypot(u - c, v - c);
        const i = v * n + u;
        if (type === "ideal") H[i] = D <= d ? 1 : 0;
        else if (type === "gaussian") H[i] = Math.exp(-(D * D) / (2 * d * d));
        else H[i] = 1 / (1 + Math.pow(D / d, 2 * order));
      }
    }
    return H;
  }

  function emphasis(Hlow, keepLow) {
    // keepLow = 0 时就是 1 − H_low；增大它会把一部分原图低频加回来。
    const H = new Float64Array(Hlow.length);
    const a = keepLow;
    for (let i = 0; i < H.length; i++) H[i] = a + (1 - a) * (1 - Hlow[i]);
    return H;
  }

  function bandRejectH(n, d0, width, order, type) {
    const H = new Float64Array(n * n);
    const c = n / 2;
    const W = Math.max(width, 1);
    for (let v = 0; v < n; v++) {
      for (let u = 0; u < n; u++) {
        const D = Math.hypot(u - c, v - c);
        let h = 1;
        if (type === "ideal") {
          h = Math.abs(D - d0) <= W / 2 ? 0 : 1;
        } else if (type === "gaussian") {
          if (D < 1e-6) h = 1;
          else {
            const t = (D * D - d0 * d0) / (D * W);
            h = 1 - Math.exp(-(t * t));
          }
        } else if (Math.abs(D * D - d0 * d0) < 1e-8) {
          h = 0;
        } else {
          const t = (D * W) / (D * D - d0 * d0);
          h = 1 / (1 + Math.pow(Math.abs(t), 2 * order));
        }
        H[v * n + u] = h;
      }
    }
    return H;
  }

  function notchH(n, freq, radius) {
    // 正弦噪声在 (±f, 0) 和 (0, ±f)。共轭对称，四个点一起挖掉。
    const H = new Float64Array(n * n);
    H.fill(1);
    const c = n / 2;
    const pts = [[c + freq, c], [c - freq, c], [c, c + freq], [c, c - freq]];
    const r = Math.max(radius, 0);
    for (let v = 0; v < n; v++) {
      for (let u = 0; u < n; u++) {
        for (let p = 0; p < pts.length; p++) {
          if (Math.hypot(u - pts[p][0], v - pts[p][1]) <= r) {
            H[v * n + u] = 0;
            break;
          }
        }
      }
    }
    return H;
  }

  function addPeriodic(gray, n, freq, amp) {
    const out = new Float64Array(n * n);
    for (let y = 0; y < n; y++) {
      for (let x = 0; x < n; x++) {
        const i = y * n + x;
        out[i] = gray[i]
          + amp * Math.sin((2 * Math.PI * freq * x) / n)
          + amp * Math.sin((2 * Math.PI * freq * y) / n);
      }
    }
    return out;
  }

  function spatialFromCenteredH(H, n) {
    // 先把直流移回 (0,0) 再逆变换，最后把原点搬到画面中心以便观察。
    const re = new Float64Array(n * n);
    const im = new Float64Array(n * n);
    const c = n >> 1;
    for (let v = 0; v < n; v++) {
      for (let u = 0; u < n; u++) {
        re[v * n + u] = H[((v + c) % n) * n + ((u + c) % n)];
      }
    }
    globalThis.Freq.fft2(re, im, n, true);
    const out = new Float64Array(n * n);
    for (let y = 0; y < n; y++) {
      for (let x = 0; x < n; x++) {
        out[((y + c) % n) * n + ((x + c) % n)] = re[y * n + x];
      }
    }
    return out;
  }

  function cropCenter(src, n, m) {
    const out = new Float64Array(m * m);
    const c = n >> 1;
    const h = m >> 1;
    for (let y = 0; y < m; y++) {
      for (let x = 0; x < m; x++) {
        out[y * m + x] = src[(c - h + y) * n + (c - h + x)];
      }
    }
    return out;
  }

  function energyKept(re, im, H) {
    let num = 0;
    let den = 0;
    for (let i = 0; i < re.length; i++) {
      const p = re[i] * re[i] + im[i] * im[i];
      den += p;
      num += p * H[i] * H[i];
    }
    return den === 0 ? 0 : num / den;
  }

  function kernelSpectrum(kernel, n) {
    const re = new Float64Array(n * n);
    const im = new Float64Array(n * n);
    const kh = kernel.length;
    const kw = kernel[0].length;
    const cy = (kh - 1) >> 1;
    const cx = (kw - 1) >> 1;
    for (let y = 0; y < kh; y++) {
      for (let x = 0; x < kw; x++) {
        const yy = (y - cy + n) % n;
        const xx = (x - cx + n) % n;
        re[yy * n + xx] = kernel[y][x];
      }
    }
    modulate(re, im, n);
    globalThis.Freq.fft2(re, im, n, false);
    return { re, im };
  }

  function convolveSpatial(gray, n, kernel) {
    // 边界外补 0，核的中心对准当前像素。这是线性卷积的 same 结果。
    const kh = kernel.length;
    const kw = kernel[0].length;
    const cy = (kh - 1) >> 1;
    const cx = (kw - 1) >> 1;
    const out = new Float64Array(n * n);
    for (let y = 0; y < n; y++) {
      for (let x = 0; x < n; x++) {
        let s = 0;
        for (let ky = 0; ky < kh; ky++) {
          const yy = y + ky - cy;
          if (yy < 0 || yy >= n) continue;
          const row = kernel[ky];
          for (let kx = 0; kx < kw; kx++) {
            const xx = x + kx - cx;
            if (xx < 0 || xx >= n) continue;
            s += gray[yy * n + xx] * row[kx];
          }
        }
        out[y * n + x] = s;
      }
    }
    return out;
  }

  function nextPow2(v) {
    let p = 1;
    while (p < v) p <<= 1;
    return p;
  }

  function convolveDFT(gray, n, kernel, mode) {
    // circular：不补零，边界卷绕。linear：补零到不少于 N+K−1，再取 2 的幂。
    const kh = kernel.length;
    const kw = kernel[0].length;
    const cy = (kh - 1) >> 1;
    const cx = (kw - 1) >> 1;
    const N = mode === "linear" ? nextPow2(n + Math.max(kh, kw) - 1) : n;
    const reA = new Float64Array(N * N);
    const imA = new Float64Array(N * N);
    const reB = new Float64Array(N * N);
    const imB = new Float64Array(N * N);
    for (let y = 0; y < n; y++) {
      for (let x = 0; x < n; x++) reA[y * N + x] = gray[y * n + x];
    }
    for (let y = 0; y < kh; y++) {
      for (let x = 0; x < kw; x++) {
        const yy = (y - cy + N) % N;
        const xx = (x - cx + N) % N;
        reB[yy * N + xx] = kernel[y][x];
      }
    }
    globalThis.Freq.fft2(reA, imA, N, false);
    globalThis.Freq.fft2(reB, imB, N, false);
    for (let i = 0; i < reA.length; i++) {
      const a = reA[i];
      const b = imA[i];
      const c = reB[i];
      const d = imB[i];
      reA[i] = a * c - b * d;
      imA[i] = a * d + b * c;
    }
    globalThis.Freq.fft2(reA, imA, N, true);
    const out = new Float64Array(n * n);
    for (let y = 0; y < n; y++) {
      for (let x = 0; x < n; x++) out[y * n + x] = reA[y * N + x];
    }
    return out;
  }

  function conv1Linear(x, h) {
    const c = (h.length - 1) >> 1;
    const y = [];
    for (let i = 0; i < x.length; i++) {
      let s = 0;
      for (let k = 0; k < h.length; k++) {
        const j = i + k - c;
        if (j >= 0 && j < x.length) s += x[j] * h[k];
      }
      y.push(s);
    }
    return y;
  }

  function conv1Circular(x, h) {
    const c = (h.length - 1) >> 1;
    const n = x.length;
    const y = [];
    for (let i = 0; i < n; i++) {
      let s = 0;
      for (let k = 0; k < h.length; k++) {
        const j = (i + k - c + n) % n;
        s += x[j] * h[k];
      }
      y.push(Math.round(s * 1000) / 1000);
    }
    return y;
  }

  function conv1Padded(x, h) {
    // 两侧各补 (K−1)/2 个零后做循环卷积，中间一段等于线性卷积。
    const c = (h.length - 1) >> 1;
    const xp = Array(c).fill(0).concat(x, Array(c).fill(0));
    const full = conv1Circular(xp, h);
    return full.slice(c, c + x.length);
  }

  function boxKernel(k) {
    const v = 1 / (k * k);
    return Array.from({ length: k }, () => Array(k).fill(v));
  }

  function gaussianKernel(k, sigma) {
    const c = (k - 1) / 2;
    const ker = [];
    let s = 0;
    for (let y = 0; y < k; y++) {
      const row = [];
      for (let x = 0; x < k; x++) {
        const v = Math.exp(-((x - c) ** 2 + (y - c) ** 2) / (2 * sigma * sigma));
        row.push(v);
        s += v;
      }
      ker.push(row);
    }
    for (const row of ker) {
      for (let i = 0; i < row.length; i++) row[i] /= s;
    }
    return ker;
  }

  function motionKernel(k) {
    return [Array(k).fill(1 / k)];
  }

  function circShift(gray, n, dx, dy) {
    const out = new Float64Array(n * n);
    for (let y = 0; y < n; y++) {
      const ys = (y - dy + n * 4) % n;
      for (let x = 0; x < n; x++) {
        const xs = (x - dx + n * 4) % n;
        out[y * n + x] = gray[ys * n + xs];
      }
    }
    return out;
  }

  function zeroShift(gray, n, dx, dy) {
    const out = new Float64Array(n * n);
    for (let y = 0; y < n; y++) {
      const ys = y - dy;
      if (ys < 0 || ys >= n) continue;
      for (let x = 0; x < n; x++) {
        const xs = x - dx;
        if (xs < 0 || xs >= n) continue;
        out[y * n + x] = gray[ys * n + xs];
      }
    }
    return out;
  }

  function sampleBilinear(gray, n, x, y) {
    if (x < 0 || y < 0 || x > n - 1 || y > n - 1) return 0;
    const x0 = Math.floor(x);
    const y0 = Math.floor(y);
    const x1 = Math.min(x0 + 1, n - 1);
    const y1 = Math.min(y0 + 1, n - 1);
    const tx = x - x0;
    const ty = y - y0;
    const i00 = gray[y0 * n + x0];
    const i10 = gray[y0 * n + x1];
    const i01 = gray[y1 * n + x0];
    const i11 = gray[y1 * n + x1];
    return (1 - tx) * (1 - ty) * i00 + tx * (1 - ty) * i10 + (1 - tx) * ty * i01 + tx * ty * i11;
  }

  function rotateImage(gray, n, deg) {
    // 逆映射：输出像素到原图取样，绕中心逆时针旋转。出界填 0。
    const rad = deg * Math.PI / 180;
    const c = Math.cos(rad);
    const s = Math.sin(rad);
    const cx = (n - 1) / 2;
    const out = new Float64Array(n * n);
    for (let y = 0; y < n; y++) {
      const dy = y - cx;
      for (let x = 0; x < n; x++) {
        const dx = x - cx;
        const xs = c * dx - s * dy + cx;
        const ys = s * dx + c * dy + cx;
        out[y * n + x] = sampleBilinear(gray, n, xs, ys);
      }
    }
    return out;
  }

  function dominantAngle(re, im, n) {
    const c = n / 2;
    const r0 = Math.max(4, Math.round(n * 0.08));
    const r1 = Math.round(n * 0.42);
    let bestE = -1;
    let bestA = 0;
    for (let a = 0; a < 180; a++) {
      const rad = a * Math.PI / 180;
      const dx = Math.cos(rad);
      const dy = Math.sin(rad);
      let e = 0;
      for (let r = r0; r < r1; r += 2) {
        const u = Math.round(c + r * dx);
        const v = Math.round(c + r * dy);
        const i = v * n + u;
        e += re[i] * re[i] + im[i] * im[i];
      }
      if (e > bestE) {
        bestE = e;
        bestA = a;
      }
    }
    return bestA;
  }

  function logMag(re, im) {
    const out = new Float64Array(re.length);
    for (let i = 0; i < re.length; i++) out[i] = Math.log(1 + Math.hypot(re[i], im[i]));
    return out;
  }

  function phaseImage(re, im) {
    const out = new Float64Array(re.length);
    for (let i = 0; i < re.length; i++) {
      out[i] = (Math.atan2(im[i], re[i]) + Math.PI) / (2 * Math.PI);
    }
    return out;
  }

  function maxAbsDiff(a, b) {
    let m = 0;
    for (let i = 0; i < a.length; i++) {
      const d = Math.abs(a[i] - b[i]);
      if (d > m) m = d;
    }
    return m;
  }

  function maxAbs(a) {
    let m = 0;
    for (let i = 0; i < a.length; i++) {
      const d = Math.abs(a[i]);
      if (d > m) m = d;
    }
    return m;
  }

  const COS = [];
  for (let u = 0; u < 8; u++) {
    COS[u] = [];
    for (let x = 0; x < 8; x++) COS[u][x] = Math.cos(((2 * x + 1) * u * Math.PI) / 16);
  }

  function dct8(vec, out) {
    // JPEG 正变换的一维部分：系数 1/2 · C(u)，二维合起来是 1/4。
    for (let u = 0; u < 8; u++) {
      const cu = u === 0 ? Math.SQRT1_2 : 1;
      let s = 0;
      const row = COS[u];
      for (let x = 0; x < 8; x++) s += vec[x] * row[x];
      out[u] = 0.5 * cu * s;
    }
  }

  function idct8(vec, out) {
    for (let x = 0; x < 8; x++) {
      let s = 0;
      for (let u = 0; u < 8; u++) {
        const cu = u === 0 ? Math.SQRT1_2 : 1;
        s += cu * vec[u] * COS[u][x];
      }
      out[x] = 0.5 * s;
    }
  }

  function zigzagCoords() {
    const coords = [];
    for (let s = 0; s <= 14; s++) {
      if (s % 2 === 0) {
        for (let i = 0; i <= s; i++) {
          const u = i;
          const v = s - i;
          if (u < 8 && v < 8) coords.push([u, v]);
        }
      } else {
        for (let i = 0; i <= s; i++) {
          const u = s - i;
          const v = i;
          if (u < 8 && v < 8) coords.push([u, v]);
        }
      }
    }
    return coords;
  }

  const ZIG = zigzagCoords();

  function compressDCT(gray, n, keep, blockX, blockY) {
    // 8×8 分块。先按行再按列做 DCT，只保留折线顺序的前 K 个系数。
    const out = new Float64Array(n * n);
    const diff = new Float64Array(n * n);
    const coeff = Array.from({ length: 8 }, () => new Float64Array(8));
    const keptBlock = Array.from({ length: 8 }, () => new Float64Array(8));
    const rowT = Array.from({ length: 8 }, () => new Float64Array(8));
    const rec = Array.from({ length: 8 }, () => new Float64Array(8));
    const tmp = new Float64Array(8);
    const col = new Float64Array(8);
    let mse = 0;
    const k = Math.max(1, Math.min(64, keep | 0));
    for (let by = 0; by < n; by += 8) {
      for (let bx = 0; bx < n; bx += 8) {
        for (let y = 0; y < 8; y++) {
          for (let x = 0; x < 8; x++) tmp[x] = gray[(by + y) * n + bx + x];
          dct8(tmp, rowT[y]);
        }
        for (let u = 0; u < 8; u++) {
          for (let y = 0; y < 8; y++) col[y] = rowT[y][u];
          dct8(col, tmp);
          for (let v = 0; v < 8; v++) coeff[v][u] = tmp[v];
        }
        const kept = Array.from({ length: 8 }, () => new Float64Array(8));
        for (let t = 0; t < k; t++) {
          const u = ZIG[t][0];
          const v = ZIG[t][1];
          kept[v][u] = coeff[v][u];
        }
        if (bx === blockX && by === blockY) {
          for (let v = 0; v < 8; v++) {
            for (let u = 0; u < 8; u++) keptBlock[v][u] = coeff[v][u];
          }
        }
        for (let u = 0; u < 8; u++) {
          for (let v = 0; v < 8; v++) col[v] = kept[v][u];
          idct8(col, tmp);
          for (let y = 0; y < 8; y++) rec[y][u] = tmp[y];
        }
        for (let y = 0; y < 8; y++) {
          idct8(rec[y], tmp);
          for (let x = 0; x < 8; x++) {
            const val = tmp[x];
            const i = (by + y) * n + bx + x;
            out[i] = val;
            const d = val - gray[i];
            diff[i] = d;
            mse += d * d;
          }
        }
      }
    }
    mse /= n * n;
    const psnr = mse < 1e-8 ? Infinity : 10 * Math.log10((255 * 255) / mse);
    return { out, diff, psnr, mse, coeff: keptBlock };
  }

  function makeZone(n) {
    const g = new Float64Array(n * n);
    const c = (n - 1) / 2;
    for (let y = 0; y < n; y++) {
      for (let x = 0; x < n; x++) {
        const r2 = (x - c) * (x - c) + (y - c) * (y - c);
        g[y * n + x] = 127.5 + 127.5 * Math.sin(r2 * 0.04);
      }
    }
    return g;
  }

  function makeStripes(n) {
    const g = new Float64Array(n * n);
    g.fill(235);
    for (let y = 0; y < n; y++) {
      for (let x = 0; x < n; x++) {
        const i = y * n + x;
        if (x < 16) g[i] = 25;
        if (y % 16 < 3) g[i] = 15;
        if (x > n * 0.78 && x < n * 0.78 + 14 && y > 36 && y < n - 36) g[i] = 40;
      }
    }
    return g;
  }

  function makeRect(n) {
    const g = new Float64Array(n * n);
    g.fill(16);
    for (let y = 0; y < n; y++) {
      for (let x = 0; x < n; x++) {
        if (x > n * 0.18 && x < n * 0.82 && y > n * 0.38 && y < n * 0.62) g[y * n + x] = 230;
      }
    }
    return g;
  }

  function makeGeom(n) {
    const g = new Float64Array(n * n);
    g.fill(24);
    for (let y = 0; y < n; y++) {
      for (let x = 0; x < n; x++) {
        const i = y * n + x;
        const dx = x - 78;
        const dy = y - 128;
        if (dx * dx + dy * dy < 52 * 52) g[i] = 210;
        if (x > 130 && x < 230 && y > 36 && y < 210) g[i] = 150;
        if (Math.abs((x - 40) - (y - 20)) < 7 && x > 40 && x < 220) g[i] = 245;
      }
    }
    return g;
  }

  const KERNELS = {
    smooth: {
      name: "课件平滑核",
      text: "低通：低频系数大",
      k: [
        [0, 1 / 6, 0],
        [1 / 6, 1 / 3, 1 / 6],
        [0, 1 / 6, 0],
      ],
    },
    box: {
      name: "均值 9×9",
      text: "低通，频率响应是二维 sinc，有旁瓣",
      k: boxKernel(9),
    },
    gauss: {
      name: "高斯 5×5",
      text: "低通，频率响应仍接近高斯",
      k: gaussianKernel(5, 1),
    },
    sharp: {
      name: "课件锐化核",
      text: "高频增强：直流增益为 1，高频增益更大",
      k: [
        [-1, -1, -1],
        [-1, 9, -1],
        [-1, -1, -1],
      ],
    },
    lap: {
      name: "拉普拉斯",
      text: "高通：直流增益为 0",
      k: [
        [0, 1, 0],
        [1, -4, 1],
        [0, 1, 0],
      ],
    },
    sobel: {
      name: "Sobel 水平",
      text: "定向高通",
      k: [
        [-1, 0, 1],
        [-2, 0, 2],
        [-1, 0, 1],
      ],
    },
  };

  return {
    modulate, centeredDft, rawDft, reconstructFrom, synthesize, mixMagPhase, checkerImage,
    applyH, lowpassH, emphasis, bandRejectH, notchH, addPeriodic, spatialFromCenteredH,
    cropCenter, energyKept, kernelSpectrum, convolveSpatial, convolveDFT, nextPow2,
    conv1Linear, conv1Circular, conv1Padded, boxKernel, gaussianKernel, motionKernel,
    circShift, zeroShift, rotateImage, dominantAngle, logMag, phaseImage, maxAbsDiff, maxAbs,
    dct8, idct8, zigzagCoords, compressDCT, makeZone, makeStripes, makeRect, makeGeom, KERNELS,
  };
});
