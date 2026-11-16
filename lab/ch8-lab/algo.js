/* 第 8 章课堂演示所用算法：离散傅里叶变换、频谱泄漏、幅度与相位交换、快速傅里叶变换图像滤波以及哈尔小波遥感图像融合。 */
(function (root, factory) {
  const api = factory();
  if (typeof module === "object" && module.exports) module.exports = api;
  else root.Algo = api;
})(typeof globalThis !== "undefined" ? globalThis : this, function () {
  const FFT = () => globalThis.Freq;

  function modulate(re, im, n) {
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
    FFT().fft2(re, im, n, false);
    return { re, im };
  }

  function reconstructFrom(re, im, n) {
    const outRe = new Float64Array(re);
    const outIm = new Float64Array(im);
    FFT().fft2(outRe, outIm, n, true);
    modulate(outRe, outIm, n);
    return outRe;
  }

  function synthesize(spec, n, mode) {
    const re = spec.re;
    const im = spec.im;
    const outRe = new Float64Array(re.length);
    const outIm = new Float64Array(re.length);
    for (let i = 0; i < re.length; i++) {
      const m = Math.hypot(re[i], im[i]);
      if (mode === "mag") outRe[i] = m;
      else if (mode === "phase") {
        if (m > 1e-8) {
          outRe[i] = re[i] / m;
          outIm[i] = im[i] / m;
        }
      } else {
        outRe[i] = re[i];
        outIm[i] = im[i];
      }
    }
    return reconstructFrom(outRe, outIm, n);
  }

  function mixMagPhase(magSpec, phaseSpec, n) {
    const re = new Float64Array(magSpec.re.length);
    const im = new Float64Array(magSpec.re.length);
    for (let i = 0; i < re.length; i++) {
      const m = Math.hypot(magSpec.re[i], magSpec.im[i]);
      const p = Math.hypot(phaseSpec.re[i], phaseSpec.im[i]);
      if (p < 1e-8) continue;
      re[i] = (m * phaseSpec.re[i]) / p;
      im[i] = (m * phaseSpec.im[i]) / p;
    }
    return reconstructFrom(re, im, n);
  }

  function logMagnitude(re, im) {
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

  function dftMag1d(x) {
    const n = x.length;
    const re = new Float64Array(x);
    const im = new Float64Array(n);
    FFT().fft1(re, im, false);
    const mag = new Float64Array(n);
    for (let i = 0; i < n; i++) mag[i] = Math.hypot(re[i], im[i]);
    return mag;
  }

  function makeCos1d(n, cycles) {
    const x = new Float64Array(n);
    for (let i = 0; i < n; i++) x[i] = Math.cos((2 * Math.PI * cycles * i) / n);
    return x;
  }

  function makeCos2d(n, cycles) {
    const g = new Float64Array(n * n);
    for (let y = 0; y < n; y++) {
      for (let x = 0; x < n; x++) {
        g[y * n + x] = 128 + 127 * Math.cos((2 * Math.PI * cycles * x) / n);
      }
    }
    return g;
  }

  function dirichletKernel(delta, n) {
    const wrapped = delta - n * Math.round(delta / n);
    if (Math.abs(wrapped) < 1e-9) return { re: n, im: 0 };
    const mag = Math.sin(Math.PI * delta) / Math.sin((Math.PI * delta) / n);
    const phase = (-Math.PI * delta * (n - 1)) / n;
    return { re: mag * Math.cos(phase), im: mag * Math.sin(phase) };
  }

  // 实余弦信号的离散傅里叶变换幅度包络：正、负两个中心频率处的狄利克雷核（周期 sinc 函数）复数叠加后再取模。
  function cosineSpectrumEnvelope(k, cycles, n) {
    const pos = dirichletKernel(k - cycles, n);
    const neg = dirichletKernel(k + cycles, n);
    return 0.5 * Math.hypot(pos.re + neg.re, pos.im + neg.im);
  }

  function sideLobeRatio(mag, peakBin) {
    let peak = 0;
    let side = 0;
    const half = mag.length >> 1;
    for (let k = 0; k <= half; k++) {
      const e = mag[k] * mag[k];
      if (Math.abs(k - peakBin) <= 1) peak += e;
      else side += e;
    }
    return side / Math.max(peak, 1e-18);
  }

  function nextPow2(v) {
    let p = 1;
    while (p < v) p <<= 1;
    return p;
  }

  function wrap(i, n) {
    let r = i % n;
    if (r < 0) r += n;
    return r;
  }

  function gaussianKernel(k, sigma) {
    const odd = k % 2 === 0 ? k + 1 : k;
    const c = (odd / 2) | 0;
    const s2 = 2 * sigma * sigma;
    const ker = [];
    let sum = 0;
    for (let y = 0; y < odd; y++) {
      const row = [];
      for (let x = 0; x < odd; x++) {
        const v = Math.exp(-((x - c) * (x - c) + (y - c) * (y - c)) / s2);
        row.push(v);
        sum += v;
      }
      ker.push(row);
    }
    for (let y = 0; y < odd; y++) for (let x = 0; x < odd; x++) ker[y][x] /= sum;
    return ker;
  }

  function horizGradKernel() {
    return [
      [-1, 0, 1],
      [-2, 0, 2],
      [-1, 0, 1],
    ];
  }

  function convolveSpatial(gray, n, kernel) {
    const kh = kernel.length;
    const kw = kernel[0].length;
    const cy = (kh / 2) | 0;
    const cx = (kw / 2) | 0;
    const out = new Float64Array(n * n);
    for (let y = 0; y < n; y++) {
      for (let x = 0; x < n; x++) {
        let s = 0;
        for (let ky = 0; ky < kh; ky++) {
          const yy = y + ky - cy;
          if (yy < 0 || yy >= n) continue;
          for (let kx = 0; kx < kw; kx++) {
            const xx = x + kx - cx;
            if (xx < 0 || xx >= n) continue;
            s += gray[yy * n + xx] * kernel[ky][kx];
          }
        }
        out[y * n + x] = s;
      }
    }
    return out;
  }

  // 为使频域滤波结果与空域中卷积核中心对齐的滤波结果一致，需要将卷积核系数放置到 (cy−ky, cx−kx) 的循环位置。
  function filterByFft(gray, n, kernel) {
    const kh = kernel.length;
    const kw = kernel[0].length;
    const cy = (kh / 2) | 0;
    const cx = (kw / 2) | 0;
    const N = nextPow2(n + Math.max(kh, kw) - 1);
    const reA = new Float64Array(N * N);
    const imA = new Float64Array(N * N);
    const reB = new Float64Array(N * N);
    const imB = new Float64Array(N * N);
    for (let y = 0; y < n; y++) {
      for (let x = 0; x < n; x++) reA[y * N + x] = gray[y * n + x];
    }
    for (let y = 0; y < kh; y++) {
      for (let x = 0; x < kw; x++) {
        const yy = wrap(cy - y, N);
        const xx = wrap(cx - x, N);
        reB[yy * N + xx] = kernel[y][x];
      }
    }
    const padImage = new Float64Array(reA);
    const padKernel = new Float64Array(reB);
    FFT().fft2(reA, imA, N, false);
    FFT().fft2(reB, imB, N, false);
    const fftImageMag = logMagnitude(reA, imA);
    const fftKernelMag = logMagnitude(reB, imB);
    for (let i = 0; i < reA.length; i++) {
      const a = reA[i];
      const b = imA[i];
      const c = reB[i];
      const d = imB[i];
      reA[i] = a * c - b * d;
      imA[i] = a * d + b * c;
    }
    const fftProductMag = logMagnitude(reA, imA);
    FFT().fft2(reA, imA, N, true);
    const out = new Float64Array(n * n);
    for (let y = 0; y < n; y++) {
      for (let x = 0; x < n; x++) out[y * n + x] = reA[y * N + x];
    }
    return { out, N, padImage, padKernel, fftImageMag, fftKernelMag, fftProductMag };
  }

  function maxAbsDiff(a, b) {
    let m = 0;
    for (let i = 0; i < a.length; i++) m = Math.max(m, Math.abs(a[i] - b[i]));
    return m;
  }

  function haarDwt2(src, n) {
    const h = n >> 1;
    const LL = new Float64Array(h * h);
    const LH = new Float64Array(h * h);
    const HL = new Float64Array(h * h);
    const HH = new Float64Array(h * h);
    for (let y = 0; y < h; y++) {
      for (let x = 0; x < h; x++) {
        const a = src[(2 * y) * n + 2 * x];
        const b = src[(2 * y) * n + 2 * x + 1];
        const c = src[(2 * y + 1) * n + 2 * x];
        const d = src[(2 * y + 1) * n + 2 * x + 1];
        const i = y * h + x;
        // 低频取四像素平均，数值范围与原图像一致，才能直接用彩色图像替换。
        LL[i] = 0.25 * (a + b + c + d);
        LH[i] = 0.25 * (a + b - c - d);
        HL[i] = 0.25 * (a - b + c - d);
        HH[i] = 0.25 * (a - b - c + d);
      }
    }
    return { LL, LH, HL, HH, h };
  }

  function haarIdwt2(LL, LH, HL, HH, h) {
    const n = h * 2;
    const out = new Float64Array(n * n);
    for (let y = 0; y < h; y++) {
      for (let x = 0; x < h; x++) {
        const i = y * h + x;
        const ll = LL[i];
        const lh = LH[i];
        const hl = HL[i];
        const hh = HH[i];
        out[(2 * y) * n + 2 * x] = ll + lh + hl + hh;
        out[(2 * y) * n + 2 * x + 1] = ll + lh - hl - hh;
        out[(2 * y + 1) * n + 2 * x] = ll - lh + hl - hh;
        out[(2 * y + 1) * n + 2 * x + 1] = ll - lh - hl + hh;
      }
    }
    return out;
  }

  function resizeNearest(src, sw, sh, dw, dh) {
    const out = new Float64Array(dw * dh);
    for (let y = 0; y < dh; y++) {
      const sy = Math.min(sh - 1, Math.floor((y + 0.5) * sh / dh));
      for (let x = 0; x < dw; x++) {
        const sx = Math.min(sw - 1, Math.floor((x + 0.5) * sw / dw));
        out[y * dw + x] = src[sy * sw + sx];
      }
    }
    return out;
  }

  function resizeArea(src, sw, sh, dw, dh) {
    // 当目标尺寸更小时，按区域求平均值实现下采样；当目标尺寸更大时，采用双线性插值实现上采样。
    if (dw === sw && dh === sh) return new Float64Array(src);
    if (dw < sw || dh < sh) {
      const out = new Float64Array(dw * dh);
      const xRatio = sw / dw;
      const yRatio = sh / dh;
      for (let y = 0; y < dh; y++) {
        const y0 = Math.floor(y * yRatio);
        const y1 = Math.min(sh, Math.floor((y + 1) * yRatio));
        for (let x = 0; x < dw; x++) {
          const x0 = Math.floor(x * xRatio);
          const x1 = Math.min(sw, Math.floor((x + 1) * xRatio));
          let s = 0;
          let c = 0;
          for (let yy = y0; yy < y1; yy++) {
            for (let xx = x0; xx < x1; xx++) {
              s += src[yy * sw + xx];
              c++;
            }
          }
          out[y * dw + x] = c ? s / c : 0;
        }
      }
      return out;
    }
    const out = new Float64Array(dw * dh);
    for (let y = 0; y < dh; y++) {
      const fy = ((y + 0.5) * sh) / dh - 0.5;
      const y0 = Math.floor(fy);
      const y1 = Math.min(sh - 1, y0 + 1);
      const ty = fy - y0;
      for (let x = 0; x < dw; x++) {
        const fx = ((x + 0.5) * sw) / dw - 0.5;
        const x0 = Math.floor(fx);
        const x1 = Math.min(sw - 1, x0 + 1);
        const tx = fx - x0;
        const yy0 = Math.max(0, Math.min(sh - 1, y0));
        const xx0 = Math.max(0, Math.min(sw - 1, x0));
        const a = src[yy0 * sw + xx0];
        const b = src[yy0 * sw + x1];
        const c = src[y1 * sw + xx0];
        const d = src[y1 * sw + x1];
        out[y * dw + x] =
          (1 - tx) * (1 - ty) * a + tx * (1 - ty) * b + (1 - tx) * ty * c + tx * ty * d;
      }
    }
    return out;
  }

  function fuseWavelet(pan, panN, msR, msG, msB, msN) {
    const { LL, LH, HL, HH, h } = haarDwt2(pan, panN);
    const msUpR = resizeArea(msR, msN, msN, panN, panN);
    const msUpG = resizeArea(msG, msN, msN, panN, panN);
    const msUpB = resizeArea(msB, msN, msN, panN, panN);
    // 彩色图边长已经是低频子带的大小时，直接替换，不再放大后再缩小。
    const llR = msN === h ? new Float64Array(msR) : resizeArea(msR, msN, msN, h, h);
    const llG = msN === h ? new Float64Array(msG) : resizeArea(msG, msN, msN, h, h);
    const llB = msN === h ? new Float64Array(msB) : resizeArea(msB, msN, msN, h, h);
    const fusedR = haarIdwt2(llR, LH, HL, HH, h);
    const fusedG = haarIdwt2(llG, LH, HL, HH, h);
    const fusedB = haarIdwt2(llB, LH, HL, HH, h);
    return { fusedR, fusedG, fusedB, LL, LH, HL, HH, h, llR, llG, llB, msUpR, msUpG, msUpB };
  }

  return {
    modulate,
    centeredDft,
    reconstructFrom,
    synthesize,
    mixMagPhase,
    logMagnitude,
    phaseImage,
    dftMag1d,
    makeCos1d,
    makeCos2d,
    dirichletKernel,
    cosineSpectrumEnvelope,
    sideLobeRatio,
    gaussianKernel,
    horizGradKernel,
    convolveSpatial,
    filterByFft,
    maxAbsDiff,
    haarDwt2,
    haarIdwt2,
    resizeNearest,
    resizeArea,
    fuseWavelet,
  };
});
