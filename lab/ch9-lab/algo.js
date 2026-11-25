// 第9章 边缘检测核心算法（纯 JS，无外部依赖）
// 包含：一阶微分算子、拉普拉斯/LoG/DoG 过零点、Canny 全流程

function toGrayFloat(imageData) {
  const { width, height, data } = imageData;
  const out = new Float64Array(width * height);
  for (let i = 0; i < width * height; i++) {
    const idx = i * 4;
    out[i] = 0.299 * data[idx] + 0.587 * data[idx + 1] + 0.114 * data[idx + 2];
  }
  return out;
}

function addGaussianNoise(src, sigma, seed = 42) {
  if (sigma <= 0) return new Float64Array(src);
  const out = new Float64Array(src.length);
  let s = seed >>> 0;
  function rand() {
    s = (1664525 * s + 1013904223) >>> 0;
    return (s + 0.5) / 4294967296;
  }
  for (let i = 0; i < src.length; i += 2) {
    const u1 = Math.max(1e-12, rand());
    const u2 = rand();
    const z0 = Math.sqrt(-2 * Math.log(u1)) * Math.cos(2 * Math.PI * u2);
    const z1 = Math.sqrt(-2 * Math.log(u1)) * Math.sin(2 * Math.PI * u2);
    out[i] = Math.min(255, Math.max(0, src[i] + z0 * sigma));
    if (i + 1 < src.length) {
      out[i + 1] = Math.min(255, Math.max(0, src[i + 1] + z1 * sigma));
    }
  }
  return out;
}

function convolve2d(src, w, h, kernel, kw, kh) {
  const out = new Float64Array(w * h);
  const rx = Math.floor(kw / 2);
  const ry = Math.floor(kh / 2);
  for (let y = 0; y < h; y++) {
    for (let x = 0; x < w; x++) {
      let sum = 0;
      for (let ky = 0; ky < kh; ky++) {
        const sy = Math.min(h - 1, Math.max(0, y + ky - ry));
        for (let kx = 0; kx < kw; kx++) {
          const sx = Math.min(w - 1, Math.max(0, x + kx - rx));
          sum += src[sy * w + sx] * kernel[ky * kw + kx];
        }
      }
      out[y * w + x] = sum;
    }
  }
  return out;
}

function gaussianKernel1D(sigma) {
  const r = Math.max(1, Math.ceil(3 * sigma));
  const size = 2 * r + 1;
  const k = new Float64Array(size);
  let sum = 0;
  const s2 = 2 * sigma * sigma;
  for (let i = -r; i <= r; i++) {
    const v = Math.exp(-(i * i) / s2);
    k[i + r] = v;
    sum += v;
  }
  for (let i = 0; i < size; i++) k[i] /= sum;
  return { k, r, size };
}

function gaussianSmooth(src, w, h, sigma) {
  if (sigma <= 0.1) return new Float64Array(src);
  const { k, r } = gaussianKernel1D(sigma);
  const tmp = new Float64Array(w * h);
  const out = new Float64Array(w * h);
  for (let y = 0; y < h; y++) {
    for (let x = 0; x < w; x++) {
      let sum = 0;
      for (let i = -r; i <= r; i++) {
        const sx = Math.min(w - 1, Math.max(0, x + i));
        sum += src[y * w + sx] * k[i + r];
      }
      tmp[y * w + x] = sum;
    }
  }
  for (let y = 0; y < h; y++) {
    for (let x = 0; x < w; x++) {
      let sum = 0;
      for (let i = -r; i <= r; i++) {
        const sy = Math.min(h - 1, Math.max(0, y + i));
        sum += tmp[sy * w + x] * k[i + r];
      }
      out[y * w + x] = sum;
    }
  }
  return out;
}

const FIRST_ORDER_OPS = {
  roberts: {
    name: "Roberts (2×2)",
    kw: 2, kh: 2,
    hA: [1, 0, 0, -1],
    hB: [0, 1, -1, 0],
    text: "hA = [ 1  0;  0 -1 ],  hB = [ 0  1; -1  0 ]"
  },
  prewitt: {
    name: "Prewitt (3×3)",
    kw: 3, kh: 3,
    hA: [-1, -1, -1, 0, 0, 0, 1, 1, 1],
    hB: [-1, 0, 1, -1, 0, 1, -1, 0, 1],
    text: "hA = [-1 -1 -1;  0  0  0;  1  1  1]\nhB = [-1  0  1; -1  0  1; -1  0  1]"
  },
  sobel: {
    name: "Sobel (3×3)",
    kw: 3, kh: 3,
    hA: [-1, -2, -1, 0, 0, 0, 1, 2, 1],
    hB: [-1, 0, 1, -2, 0, 2, -1, 0, 1],
    text: "hA = [-1 -2 -1;  0  0  0;  1  2  1]\nhB = [-1  0  1; -2  0  2; -1  0  1]"
  },
  freichen: {
    name: "Frei-Chen (3×3)",
    kw: 3, kh: 3,
    hA: [-1, -Math.SQRT2, -1, 0, 0, 0, 1, Math.SQRT2, 1],
    hB: [-1, 0, 1, -Math.SQRT2, 0, Math.SQRT2, -1, 0, 1],
    text: "hA = [-1 -√2 -1;  0  0  0;  1  √2  1]\nhB = [-1  0  1; -√2  0  √2; -1  0  1]"
  }
};

function firstOrderDetect(src, w, h, opKey) {
  const op = FIRST_ORDER_OPS[opKey];
  const gx = convolve2d(src, w, h, op.hB, op.kw, op.kh);
  const gy = convolve2d(src, w, h, op.hA, op.kw, op.kh);
  const mag = new Float64Array(w * h);
  for (let i = 0; i < w * h; i++) {
    mag[i] = Math.hypot(gx[i], gy[i]);
  }
  return { gx, gy, mag };
}

function laplacianRaw(src, w, h) {
  const k = [0, 1, 0, 1, -4, 1, 0, 1, 0];
  return convolve2d(src, w, h, k, 3, 3);
}

function logResponse(src, w, h, sigma) {
  const r = Math.max(2, Math.ceil(3 * sigma));
  const size = 2 * r + 1;
  const k = new Float64Array(size * size);
  const s2 = sigma * sigma;
  const s4 = s2 * s2;
  let mean = 0;
  for (let y = -r; y <= r; y++) {
    for (let x = -r; x <= r; x++) {
      const r2 = x * x + y * y;
      const val = ((r2 - 2 * s2) / (Math.PI * s4 * 2)) * Math.exp(-r2 / (2 * s2));
      k[(y + r) * size + (x + r)] = val;
      mean += val;
    }
  }
  mean /= size * size;
  for (let i = 0; i < k.length; i++) k[i] -= mean;
  return convolve2d(src, w, h, k, size, size);
}

function dogResponse(src, w, h, sigma, kRatio) {
  const g1 = gaussianSmooth(src, w, h, sigma);
  const g2 = gaussianSmooth(src, w, h, sigma * kRatio);
  const out = new Float64Array(w * h);
  for (let i = 0; i < out.length; i++) out[i] = g1[i] - g2[i];
  return out;
}

function zeroCrossings(resp, w, h, slopeThresh) {
  const edges = new Uint8Array(w * h);
  for (let y = 1; y < h - 1; y++) {
    for (let x = 1; x < w - 1; x++) {
      const idx = y * w + x;
      const pairs = [
        [resp[idx - 1], resp[idx + 1]],
        [resp[idx - w], resp[idx + w]],
        [resp[idx - w - 1], resp[idx + w + 1]],
        [resp[idx - w + 1], resp[idx + w - 1]],
      ];
      let maxDiff = 0;
      let isZC = false;
      for (const [a, b] of pairs) {
        if ((a > 0 && b < 0) || (a < 0 && b > 0)) {
          const d = Math.abs(a - b);
          if (d > maxDiff) maxDiff = d;
          isZC = true;
        }
      }
      if (isZC && maxDiff >= slopeThresh) {
        edges[idx] = 255;
      }
    }
  }
  return edges;
}

function cannyPipeline(src, w, h, sigma, tLow, tHigh) {
  const smooth = gaussianSmooth(src, w, h, sigma);
  const { gx, gy, mag } = firstOrderDetect(smooth, w, h, "sobel");
  const nms = new Float64Array(w * h);

  for (let y = 1; y < h - 1; y++) {
    for (let x = 1; x < w - 1; x++) {
      const idx = y * w + x;
      const m = mag[idx];
      if (m === 0) continue;
      let angle = (Math.atan2(gy[idx], gx[idx]) * 180) / Math.PI;
      if (angle < 0) angle += 180;
      let q = 0, r = 0;
      if (angle < 22.5 || angle >= 157.5) {
        q = mag[idx + 1];
        r = mag[idx - 1];
      } else if (angle < 67.5) {
        q = mag[idx + w - 1];
        r = mag[idx - w + 1];
      } else if (angle < 112.5) {
        q = mag[idx + w];
        r = mag[idx - w];
      } else {
        q = mag[idx - w - 1];
        r = mag[idx + w + 1];
      }
      nms[idx] = (m >= q && m >= r) ? m : 0;
    }
  }

  const state = new Uint8Array(w * h); // 0=none, 1=weak, 2=strong
  const queue = [];
  let strongCount = 0;
  let weakCount = 0;
  for (let i = 0; i < w * h; i++) {
    if (nms[i] >= tHigh) {
      state[i] = 2;
      queue.push(i);
      strongCount++;
    } else if (nms[i] >= tLow) {
      state[i] = 1;
      weakCount++;
    }
  }

  const finalEdges = new Uint8Array(w * h);
  for (const idx of queue) finalEdges[idx] = 255;

  let head = 0;
  let linkedWeak = 0;
  while (head < queue.length) {
    const cur = queue[head++];
    const cx = cur % w;
    const cy = Math.floor(cur / w);
    for (let dy = -1; dy <= 1; dy++) {
      for (let dx = -1; dx <= 1; dx++) {
        if (dx === 0 && dy === 0) continue;
        const nx = cx + dx;
        const ny = cy + dy;
        if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
          const nidx = ny * w + nx;
          if (state[nidx] === 1 && finalEdges[nidx] === 0) {
            finalEdges[nidx] = 255;
            queue.push(nidx);
            linkedWeak++;
          }
        }
      }
    }
  }

  return { smooth, gx, gy, mag, nms, state, finalEdges, strongCount, weakCount, linkedWeak };
}
