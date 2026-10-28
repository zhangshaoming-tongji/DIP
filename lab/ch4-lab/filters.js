/* Spatial filters used by the chapter 4 classroom page. Gray values are 0–255. */
(function () {
  function clamp(v, lo, hi) {
    return v < lo ? lo : v > hi ? hi : v;
  }

  function integral(src, w, h) {
    const ii = new Float64Array((w + 1) * (h + 1));
    const stride = w + 1;
    for (let y = 0; y < h; y++) {
      let row = 0;
      const srcRow = y * w;
      for (let x = 0; x < w; x++) {
        row += src[srcRow + x];
        ii[(y + 1) * stride + (x + 1)] = ii[y * stride + (x + 1)] + row;
      }
    }
    return ii;
  }

  function rectSum(ii, w, x0, y0, x1, y1) {
    const stride = w + 1;
    return (
      ii[(y1 + 1) * stride + (x1 + 1)] -
      ii[y0 * stride + (x1 + 1)] -
      ii[(y1 + 1) * stride + x0] +
      ii[y0 * stride + x0]
    );
  }

  function boxMean(src, w, h, r) {
    const ii = integral(src, w, h);
    const out = new Float32Array(w * h);
    for (let y = 0; y < h; y++) {
      const y0 = Math.max(0, y - r);
      const y1 = Math.min(h - 1, y + r);
      for (let x = 0; x < w; x++) {
        const x0 = Math.max(0, x - r);
        const x1 = Math.min(w - 1, x + r);
        const n = (x1 - x0 + 1) * (y1 - y0 + 1);
        out[y * w + x] = rectSum(ii, w, x0, y0, x1, y1) / n;
      }
    }
    return out;
  }

  function mul(a, b) {
    const out = new Float32Array(a.length);
    for (let i = 0; i < a.length; i++) out[i] = a[i] * b[i];
    return out;
  }

  function extremum(src, w, h, r, mode) {
    const out = new Float32Array(w * h);
    const wantMax = mode === "max";
    for (let y = 0; y < h; y++) {
      for (let x = 0; x < w; x++) {
        let best = wantMax ? -Infinity : Infinity;
        let bx = x;
        let by = y;
        const y0 = Math.max(0, y - r);
        const y1 = Math.min(h - 1, y + r);
        const x0 = Math.max(0, x - r);
        const x1 = Math.min(w - 1, x + r);
        for (let yy = y0; yy <= y1; yy++) {
          const row = yy * w;
          for (let xx = x0; xx <= x1; xx++) {
            const v = src[row + xx];
            if (wantMax ? v > best : v < best) {
              best = v;
              bx = xx;
              by = yy;
            }
          }
        }
        out[y * w + x] = best;
      }
    }
    return out;
  }

  function windowAt(src, w, h, x, y, r) {
    const n = 2 * r + 1;
    const win = [];
    for (let dy = -r; dy <= r; dy++) {
      const yy = clamp(y + dy, 0, h - 1);
      const row = [];
      for (let dx = -r; dx <= r; dx++) {
        const xx = clamp(x + dx, 0, w - 1);
        row.push(src[yy * w + xx]);
      }
      win.push(row);
    }
    return { n, win };
  }

  function gaussKernel1d(sigma) {
    const rad = Math.max(1, Math.ceil(3 * sigma));
    const k = new Float64Array(2 * rad + 1);
    let sum = 0;
    const s2 = 2 * sigma * sigma;
    for (let i = -rad; i <= rad; i++) {
      const v = Math.exp(-(i * i) / s2);
      k[i + rad] = v;
      sum += v;
    }
    for (let i = 0; i < k.length; i++) k[i] /= sum;
    return k;
  }

  function convSeparablePasses(src, w, h, k) {
    const rad = (k.length - 1) >> 1;
    const tmp = new Float32Array(w * h);
    const out = new Float32Array(w * h);
    for (let y = 0; y < h; y++) {
      for (let x = 0; x < w; x++) {
        let s = 0;
        const row = y * w;
        for (let i = 0; i < k.length; i++) {
          const xx = clamp(x + i - rad, 0, w - 1);
          s += src[row + xx] * k[i];
        }
        tmp[row + x] = s;
      }
    }
    for (let y = 0; y < h; y++) {
      for (let x = 0; x < w; x++) {
        let s = 0;
        for (let i = 0; i < k.length; i++) {
          const yy = clamp(y + i - rad, 0, h - 1);
          s += tmp[yy * w + x] * k[i];
        }
        out[y * w + x] = s;
      }
    }
    return { tmp, image: out };
  }

  function convSeparable(src, w, h, k) {
    return convSeparablePasses(src, w, h, k).image;
  }

  function conv2d(src, w, h, k) {
    const rad = (k.length - 1) >> 1;
    const n = k.length;
    const k2 = new Float64Array(n * n);
    for (let j = 0; j < n; j++) {
      for (let i = 0; i < n; i++) k2[j * n + i] = k[j] * k[i];
    }
    const out = new Float32Array(w * h);
    for (let y = 0; y < h; y++) {
      for (let x = 0; x < w; x++) {
        let s = 0;
        for (let j = 0; j < n; j++) {
          const yy = clamp(y + j - rad, 0, h - 1);
          const row = yy * w;
          const kr = j * n;
          for (let i = 0; i < n; i++) {
            const xx = clamp(x + i - rad, 0, w - 1);
            s += src[row + xx] * k2[kr + i];
          }
        }
        out[y * w + x] = s;
      }
    }
    return out;
  }

  function gaussian(src, w, h, sigma) {
    const k = gaussKernel1d(sigma);
    return { image: convSeparable(src, w, h, k), kernel: k };
  }

  function bilateralPair(values, range, w, h, sigmaS, sigmaR, radiusCap) {
    const cap = radiusCap || 8;
    const rad = Math.max(1, Math.min(cap, Math.ceil(2 * sigmaS)));
    const spatial = [];
    const s2 = 2 * sigmaS * sigmaS;
    for (let dy = -rad; dy <= rad; dy++) {
      for (let dx = -rad; dx <= rad; dx++) {
        spatial.push(Math.exp(-(dx * dx + dy * dy) / s2));
      }
    }
    const r2 = 2 * sigmaR * sigmaR;
    const out = new Float32Array(w * h);
    for (let y = 0; y < h; y++) {
      for (let x = 0; x < w; x++) {
        const center = range[y * w + x];
        let acc = 0;
        let norm = 0;
        let t = 0;
        for (let dy = -rad; dy <= rad; dy++) {
          const yy = clamp(y + dy, 0, h - 1);
          const row = yy * w;
          for (let dx = -rad; dx <= rad; dx++) {
            const xx = clamp(x + dx, 0, w - 1);
            const diff = range[row + xx] - center;
            const weight = spatial[t++] * Math.exp(-(diff * diff) / r2);
            acc += weight * values[row + xx];
            norm += weight;
          }
        }
        out[y * w + x] = acc / norm;
      }
    }
    return { image: out, radius: rad };
  }

  function bilateral(src, w, h, sigmaS, sigmaR, radiusCap) {
    return bilateralPair(src, src, w, h, sigmaS, sigmaR, radiusCap);
  }

  function bilateralWeights(range, w, h, x, y, sigmaS, sigmaR) {
    const rad = Math.max(1, Math.min(8, Math.ceil(2 * sigmaS)));
    const s2 = 2 * sigmaS * sigmaS;
    const r2 = 2 * sigmaR * sigmaR;
    const center = range[y * w + x];
    const weights = [];
    let maxW = 0;
    for (let dy = -rad; dy <= rad; dy++) {
      const row = [];
      const yy = clamp(y + dy, 0, h - 1);
      for (let dx = -rad; dx <= rad; dx++) {
        const xx = clamp(x + dx, 0, w - 1);
        const diff = range[yy * w + xx] - center;
        const weight = Math.exp(-(dx * dx + dy * dy) / s2) * Math.exp(-(diff * diff) / r2);
        row.push(weight);
        if (weight > maxW) maxW = weight;
      }
      weights.push(row);
    }
    return { radius: rad, weights, maxW, center };
  }

  function guided(p, guide, w, h, radius, eps) {
    const meanI = boxMean(guide, w, h, radius);
    const meanP = boxMean(p, w, h, radius);
    const corrI = boxMean(mul(guide, guide), w, h, radius);
    const corrIp = boxMean(mul(guide, p), w, h, radius);
    const a = new Float32Array(w * h);
    const b = new Float32Array(w * h);
    for (let i = 0; i < w * h; i++) {
      const varI = Math.max(0, corrI[i] - meanI[i] * meanI[i]);
      const cov = corrIp[i] - meanI[i] * meanP[i];
      const ai = cov / (varI + eps);
      a[i] = ai;
      b[i] = meanP[i] - ai * meanI[i];
    }
    const meanA = boxMean(a, w, h, radius);
    const meanB = boxMean(b, w, h, radius);
    const out = new Float32Array(w * h);
    for (let i = 0; i < w * h; i++) out[i] = meanA[i] * guide[i] + meanB[i];
    return { image: out, a: meanA };
  }

  window.DIPFilters = {
    clamp,
    boxMean,
    extremum,
    windowAt,
    gaussKernel1d,
    gaussian,
    convSeparable,
    convSeparablePasses,
    conv2d,
    bilateral,
    bilateralPair,
    bilateralWeights,
    guided,
  };
})();
