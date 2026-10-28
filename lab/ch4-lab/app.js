/* Chapter 4 classroom demo: max/min, mean, Gaussian, bilateral, joint bilateral, guided. */
(function () {
  const F = window.DIPFilters;
  const MAX_SIDE = 380;

  const FILTERS = {
    maxmin: {
      name: "最大 / 最小滤波",
      formula: "g(x, y) = max 或 min { f(x+s, y+t) }，(s, t) 在邻域内",
      note: "课件用的是盐椒噪声，不是高斯噪声。最大值只去掉孤立黑点（椒），白点会留下；最小值只去掉孤立白点（盐）。两种一起出现时，要先做一次最小、再做一次最大（或反过来），黑点和白点才会一起去掉。点击输入图可看窗口里被选中的像素。",
      fields: [
        { key: "mode", type: "choice", label: "替换为", options: [
          { value: "max", label: "最大值（去黑点）" },
          { value: "min", label: "最小值（去白点）" },
          { value: "minThenMax", label: "先最小再最大" },
          { value: "maxThenMin", label: "先最大再最小" },
        ], value: "max" },
        { key: "r", label: "邻域半径 r", min: 1, max: 5, step: 1, value: 1, fmt: (v) => (2 * v + 1) + "×" + (2 * v + 1) },
      ],
      scene: { source: "lena", noise: "pepper", amt: 0.06, mode: "max", r: 1 },
      presets: [
        { name: "黑点 → 最大", mode: "max", r: 1, source: "lena", noise: "pepper", amt: 0.06 },
        { name: "白点 → 最小", mode: "min", r: 1, source: "lena", noise: "salt", amt: 0.06 },
        { name: "椒盐 → 先最小再最大", mode: "minThenMax", r: 1, source: "lena", noise: "sp", amt: 0.08 },
      ],
    },
    mean: {
      name: "均值滤波",
      formula: "g(x, y) = (1/N) Σ f(x+s, y+t)<br>N 为邻域内像素个数",
      note: "课件里的模型是理想灰度加上零均值高斯噪声，邻域平均把上下浮动抵消掉。窗口越大，噪声越低，边缘也一起被抹平。椒盐点不是零均值小波动，平均后会留下灰斑，所以这里不用椒盐做例子。",
      fields: [
        { key: "r", label: "邻域半径 r", min: 1, max: 8, step: 1, value: 1, fmt: (v) => (2 * v + 1) + "×" + (2 * v + 1) },
      ],
      scene: { source: "lena", noise: "gauss", amt: 22, r: 1 },
      presets: [
        { name: "3×3", r: 1, source: "lena", noise: "gauss", amt: 22 },
        { name: "5×5", r: 2, source: "lena", noise: "gauss", amt: 22 },
        { name: "9×9", r: 4, source: "lena", noise: "gauss", amt: 22 },
      ],
    },
    gauss: {
      name: "高斯滤波",
      formula: "G(s, t) = exp(−(s²+t²) / 2σ²)，再归一化后卷积<br>可分离：先按行、再按列与一维高斯核卷积",
      note: "课件强调的是核本身：权重只随到中心的距离变化，与图像内容无关，各方向平滑程度相同。默认用没有噪声的阶跃和圆，圆会被各向同性地抹开，阶跃变成斜坡。要看它压低零均值噪声，用下面的“加高斯噪声”。",
      fields: [
        { key: "sigma", label: "σ", min: 0.4, max: 4, step: 0.1, value: 1.6, fmt: (v) => v.toFixed(1) },
      ],
      scene: { source: "edge", noise: "none", amt: 0, sigma: 1.6 },
      presets: [
        { name: "σ=0.8 只看核", sigma: 0.8, source: "edge", noise: "none", amt: 0 },
        { name: "σ=3 强模糊", sigma: 3, source: "edge", noise: "none", amt: 0 },
        { name: "加高斯噪声", sigma: 1.2, source: "lena", noise: "gauss", amt: 18 },
      ],
    },
    separable: {
      name: "可分离卷积",
      formula: "二维核 G(s, t) = g(s) · g(t)<br>先行：t(x, y) = Σ g(i) f(x+i, y)<br>再列：q(x, y) = Σ g(j) t(x, y+j)",
      note: "高斯核和均值核都能写成两个一维核的乘积。先沿每一行卷积，中间图只在水平方向变糊；再沿每一列卷积，结果与直接做二维卷积相同。每个像素的乘法次数从 K×K 降到 K+K，理论加速比是 K/2，核越大越明显。",
      fields: [
        { key: "kind", type: "choice", label: "一维核", options: [
          { value: "gauss", label: "高斯核" },
          { value: "box", label: "均值核" },
        ], value: "gauss" },
        { key: "sigma", label: "高斯 σ", min: 0.6, max: 4, step: 0.1, value: 2, fmt: (v) => v.toFixed(1) },
        { key: "r", label: "均值核半径", min: 1, max: 8, step: 1, value: 4, fmt: (v) => (2 * v + 1) + "×" + (2 * v + 1) },
      ],
      scene: { source: "edge", noise: "none", amt: 0, kind: "gauss", sigma: 2, r: 4 },
      presets: [
        { name: "高斯 σ=1", kind: "gauss", sigma: 1, source: "edge", noise: "none", amt: 0 },
        { name: "高斯 σ=2.5", kind: "gauss", sigma: 2.5, source: "edge", noise: "none", amt: 0 },
        { name: "均值 9×9", kind: "box", r: 4, source: "edge", noise: "none", amt: 0 },
      ],
    },
    bilateral: {
      name: "双边滤波",
      formula: "w = exp(−(s²+t²) / 2σ<sub>s</sub>²) · exp(−(ΔI)² / 2σ<sub>r</sub>²)<br>g = Σ w f / Σ w",
      note: "课件上的例子是一条带噪声的阶跃：平坦处接近高斯滤波，跨过阶跃时灰度差权重大幅下降，边缘留得住。高斯滤波在同一张图上会把阶跃抹成斜坡。σs 管邻域远近，σr 管允许多大的灰度差。点击阶跃附近可看权重被切成两半。",
      fields: [
        { key: "sigmaS", label: "空间尺度 σs", min: 0.6, max: 4, step: 0.1, value: 2, fmt: (v) => v.toFixed(1) },
        { key: "sigmaR", label: "灰度尺度 σr", min: 8, max: 80, step: 1, value: 22, fmt: (v) => String(v) },
      ],
      scene: { source: "edge", noise: "gauss", amt: 18, sigmaS: 2, sigmaR: 22 },
      presets: [
        { name: "阶跃保边", sigmaS: 2, sigmaR: 22, source: "edge", noise: "gauss", amt: 18 },
        { name: "σr 放大后边缘也糊", sigmaS: 2.4, sigmaR: 70, source: "edge", noise: "gauss", amt: 18 },
        { name: "人像：双边与高斯对比", sigmaS: 2, sigmaR: 28, source: "lena", noise: "gauss", amt: 18, compare: true },
      ],
    },
    joint: {
      name: "联合双边滤波",
      formula: "w = exp(−(s²+t²) / 2σ<sub>s</sub>²) · exp(−(ΔG)² / 2σ<sub>r</sub>²)<br>g = Σ w f / Σ w，ΔG 取自引导图，f 是待滤波图",
      note: "Petschnigg 等人的闪光灯例子：关闪光的图有噪声，是被平均的像素；开闪光的图边缘清楚、颜色偏灰，只用来算灰度差。空间权重仍然只看距离。默认把阶跃加比较强的高斯噪声，引导图用没有噪声的阶跃，对应课件里那张“noisy p + 清晰 guide”的示意图。引导图改成输入自身时，就退回普通双边滤波。",
      fields: [
        { key: "sigmaS", label: "空间尺度 σs", min: 0.6, max: 4, step: 0.1, value: 2, fmt: (v) => v.toFixed(1) },
        { key: "sigmaR", label: "灰度尺度 σr", min: 8, max: 80, step: 1, value: 22, fmt: (v) => String(v) },
        { key: "guide", type: "choice", label: "引导图", options: [
          { value: "flash", label: "偏灰但边缘清楚（开闪光）" },
          { value: "clean", label: "无噪声原图" },
          { value: "self", label: "输入自身（等于双边）" },
        ], value: "clean" },
      ],
      scene: { source: "edge", noise: "gauss", amt: 28, sigmaS: 2, sigmaR: 16, guide: "clean" },
      presets: [
        { name: "清晰阶跃作引导", sigmaS: 2, sigmaR: 16, guide: "clean", source: "edge", noise: "gauss", amt: 28 },
        { name: "人像 · 开闪光引导", sigmaS: 2, sigmaR: 22, guide: "flash", source: "lena", noise: "gauss", amt: 24 },
        { name: "引导改用带噪图", sigmaS: 2, sigmaR: 16, guide: "self", source: "edge", noise: "gauss", amt: 28 },
      ],
    },
    guided: {
      name: "引导滤波",
      formula: "细节增强：增强图 = 输入 + 倍数 × (输入 − 滤波结果)<br>结构迁移：q = mean(a)·I + mean(b)，I 是另一幅引导图",
      note: "课件有两例。郁金香是细节增强：引导图就是原图，把滤掉的细节加回去。双边滤波在花瓣边缘会反色，引导滤波的梯度方向与原图一致。结构迁移把硬掩模当输入、毛发照片当引导图，细毛会出现在掩模边上。",
      fields: [
        { key: "example", type: "choice", label: "课件例子", options: [
          { value: "tulip", label: "郁金香 · 细节增强" },
          { value: "fur", label: "结构迁移" },
        ], value: "tulip" },
        { key: "r", label: "引导滤波半径 r", min: 1, max: 16, step: 1, value: 8, fmt: (v) => (2 * v + 1) + "×" + (2 * v + 1) },
        { key: "eps", label: "正则化 ε", min: 0.001, max: 0.08, step: 0.001, value: 0.01, fmt: (v) => v.toFixed(3) },
        { key: "gain", label: "细节倍数", min: 1, max: 8, step: 1, value: 5, fmt: (v) => String(v) },
        { key: "sigmaS", label: "双边 σs", min: 2, max: 10, step: 0.5, value: 8, fmt: (v) => v.toFixed(1) },
        { key: "sigmaR", label: "双边 σr", min: 8, max: 60, step: 1, value: 26, fmt: (v) => String(v) },
      ],
      scene: { example: "tulip", r: 8, eps: 0.01, gain: 5, sigmaS: 8, sigmaR: 26 },
      presets: [
        { name: "郁金香 · 细节增强", example: "tulip", r: 8, eps: 0.01, gain: 5, sigmaS: 8, sigmaR: 26 },
        { name: "结构迁移", example: "fur", r: 12, eps: 0.002, gain: 5, sigmaS: 8, sigmaR: 26 },
      ],
    },
  };

  const state = {
    filter: "maxmin",
    params: {},
    noise: "pepper",
    noiseAmt: 0.06,
    compare: false,
    texture: null,
    clean: null,
    noisy: null,
    w: 0,
    h: 0,
    probe: null,
    photos: {},
    lena: null,
    fileImg: null,
    aux: null,
  };

  function defaultParams(id) {
    const params = {};
    FILTERS[id].fields.forEach((f) => { params[f.key] = f.value; });
    return params;
  }

  Object.keys(FILTERS).forEach((id) => { state.params[id] = defaultParams(id); });

  const srcCanvas = document.getElementById("srcCanvas");
  const dstCanvas = document.getElementById("dstCanvas");
  const auxCanvas = document.getElementById("auxCanvas");
  const srcCtx = srcCanvas.getContext("2d");
  const dstCtx = dstCanvas.getContext("2d");
  const auxCtx = auxCanvas.getContext("2d");

  function paintGray(canvas, ctx, data, w, h) {
    canvas.width = w;
    canvas.height = h;
    const img = ctx.createImageData(w, h);
    const px = img.data;
    for (let i = 0; i < data.length; i++) {
      const v = F.clamp(Math.round(data[i]), 0, 255);
      const o = i * 4;
      px[o] = px[o + 1] = px[o + 2] = v;
      px[o + 3] = 255;
    }
    ctx.putImageData(img, 0, 0);
  }

  function imageToGray(img) {
    const scale = Math.min(1, MAX_SIDE / Math.max(img.width, img.height));
    const w = Math.max(32, Math.round(img.width * scale));
    const h = Math.max(32, Math.round(img.height * scale));
    const c = document.createElement("canvas");
    c.width = w;
    c.height = h;
    const ctx = c.getContext("2d");
    ctx.drawImage(img, 0, 0, w, h);
    const px = ctx.getImageData(0, 0, w, h).data;
    const gray = new Float32Array(w * h);
    for (let i = 0; i < gray.length; i++) {
      const o = i * 4;
      gray[i] = 0.299 * px[o] + 0.587 * px[o + 1] + 0.114 * px[o + 2];
    }
    return { gray, w, h };
  }

  function imageToRGB(img, maxSide, size) {
    const scale = size ? 1 : Math.min(1, maxSide / Math.max(img.width, img.height));
    const w = size ? size.w : Math.max(32, Math.round(img.width * scale));
    const h = size ? size.h : Math.max(32, Math.round(img.height * scale));
    const c = document.createElement("canvas");
    c.width = w;
    c.height = h;
    const ctx = c.getContext("2d");
    ctx.drawImage(img, 0, 0, w, h);
    const px = ctx.getImageData(0, 0, w, h).data;
    const n = w * h;
    const rgb = new Float32Array(n * 3);
    for (let i = 0; i < n; i++) {
      const o = i * 4;
      rgb[i] = px[o];
      rgb[n + i] = px[o + 1];
      rgb[2 * n + i] = px[o + 2];
    }
    return { rgb, w, h };
  }

  function mapPlanes(rgb, w, h, fn) {
    const n = w * h;
    const out = new Float32Array(n * 3);
    for (let c = 0; c < 3; c++) out.set(fn(rgb.subarray(c * n, (c + 1) * n)), c * n);
    return out;
  }

  function enhanceRGB(src, base, gain) {
    const out = new Float32Array(src.length);
    for (let i = 0; i < src.length; i++) out[i] = F.clamp(src[i] + gain * (src[i] - base[i]), 0, 255);
    return out;
  }

  function rgbToGray(rgb, w, h) {
    const n = w * h;
    const gray = new Float32Array(n);
    for (let i = 0; i < n; i++) gray[i] = 0.299 * rgb[i] + 0.587 * rgb[n + i] + 0.114 * rgb[2 * n + i];
    return gray;
  }

  function paintRGB(canvas, ctx, rgb, w, h) {
    canvas.width = w;
    canvas.height = h;
    const img = ctx.createImageData(w, h);
    const px = img.data;
    const n = w * h;
    for (let i = 0; i < n; i++) {
      const o = i * 4;
      px[o] = F.clamp(Math.round(rgb[i]), 0, 255);
      px[o + 1] = F.clamp(Math.round(rgb[n + i]), 0, 255);
      px[o + 2] = F.clamp(Math.round(rgb[2 * n + i]), 0, 255);
      px[o + 3] = 255;
    }
    ctx.putImageData(img, 0, 0);
  }

  function makeEdgeImage() {
    const w = 360;
    const h = 280;
    const gray = new Float32Array(w * h);
    for (let y = 0; y < h; y++) {
      for (let x = 0; x < w; x++) {
        let v = x < w * 0.48 ? 48 : 200;
        const dx = x - 230;
        const dy = y - 150;
        if (dx * dx + dy * dy < 46 * 46) v = 230;
        v += 8 * Math.sin(x / 3) * Math.sin(y / 4);
        gray[y * w + x] = v;
      }
    }
    return { gray, w, h };
  }

  function makeTransferPair() {
    const w = 360;
    const h = 280;
    const mask = new Float32Array(w * h);
    const guide = new Float32Array(w * h);
    for (let y = 0; y < h; y++) {
      const boundary = w * 0.46 + 26 * Math.sin(y / 16) + 12 * Math.sin(y / 6);
      for (let x = 0; x < w; x++) {
        const d = x - boundary;
        mask[y * w + x] = d < 0 ? 0 : 255;
        let g = d < -8 ? 36 : 214;
        const hair = Math.exp(-(d * d) / 90) * 78 * Math.sin(y / 2.05) * Math.sin(x / 1.55);
        const shade = 18 * Math.sin(x / 28) + 10 * Math.sin(y / 22);
        guide[y * w + x] = F.clamp(g + hair + shade, 0, 255);
      }
    }
    return { mask, guide, w, h };
  }

  function impulseNoise(clean, p, kind) {
    const out = new Float32Array(clean.length);
    for (let i = 0; i < clean.length; i++) {
      const r = Math.random();
      if (kind === "pepper" && r < p) out[i] = 0;
      else if (kind === "salt" && r < p) out[i] = 255;
      else if (kind === "sp" && r < p / 2) out[i] = 0;
      else if (kind === "sp" && r < p) out[i] = 255;
      else out[i] = clean[i];
    }
    return out;
  }

  function applyNoise(clean) {
    const out = new Float32Array(clean.length);
    if (state.noise === "gauss") {
      const sigma = state.noiseAmt;
      for (let i = 0; i < clean.length; i++) {
        const u = Math.random() || 1e-9;
        const v = Math.random();
        const n = Math.sqrt(-2 * Math.log(u)) * Math.cos(2 * Math.PI * v);
        out[i] = F.clamp(clean[i] + sigma * n, 0, 255);
      }
    } else if (state.noise === "pepper" || state.noise === "salt" || state.noise === "sp") {
      return impulseNoise(clean, state.noiseAmt, state.noise);
    } else {
      out.set(clean);
    }
    return out;
  }

  function currentParams() {
    return state.params[state.filter];
  }

  function kernelFor(p) {
    if (p.kind === "box") {
      const n = 2 * p.r + 1;
      const k = new Float64Array(n);
      const v = 1 / n;
      for (let i = 0; i < n; i++) k[i] = v;
      return k;
    }
    return F.gaussKernel1d(p.sigma);
  }

  function benchKey(k) {
    let s = state.w + "x" + state.h;
    for (let i = 0; i < k.length; i++) s += ":" + k[i].toFixed(5);
    return s;
  }

  function timeCall(fn) {
    const t0 = performance.now();
    fn();
    let dt = performance.now() - t0;
    if (dt < 16) {
      const n = 4;
      const t1 = performance.now();
      for (let i = 0; i < n; i++) fn();
      dt = (performance.now() - t1) / n;
    }
    return dt;
  }

  function runFilter() {
    const p = currentParams();
    const src = state.noisy;
    const w = state.w;
    const h = state.h;
    const t0 = performance.now();
    let image;
    let extra = {};
    if (state.filter === "maxmin") {
      if (p.mode === "minThenMax") {
        image = F.extremum(F.extremum(src, w, h, p.r, "min"), w, h, p.r, "max");
      } else if (p.mode === "maxThenMin") {
        image = F.extremum(F.extremum(src, w, h, p.r, "max"), w, h, p.r, "min");
      } else {
        image = F.extremum(src, w, h, p.r, p.mode);
      }
    } else if (state.filter === "mean") {
      image = F.boxMean(src, w, h, p.r);
    } else if (state.filter === "gauss") {
      const g = F.gaussian(src, w, h, p.sigma);
      image = g.image;
      extra.kernel = g.kernel;
    } else if (state.filter === "separable") {
      const k = kernelFor(p);
      const pass = F.convSeparablePasses(src, w, h, k);
      image = pass.image;
      extra.kernel = k;
      extra.mid = pass.tmp;
    } else if (state.filter === "bilateral") {
      const b = F.bilateral(src, w, h, p.sigmaS, p.sigmaR);
      image = b.image;
      extra.radius = b.radius;
      if (state.compare) extra.gauss = F.gaussian(src, w, h, p.sigmaS).image;
    } else if (state.filter === "joint") {
      const guide = jointGuide(p.guide);
      const b = F.bilateralPair(src, guide, w, h, p.sigmaS, p.sigmaR);
      image = b.image;
      extra.radius = b.radius;
      extra.guide = guide;
    } else if (state.filter === "guided" && p.example === "tulip" && state.photos.tulip) {
      const im = imageToRGB(state.photos.tulip, 220);
      const eps = p.eps * 255 * 255;
      const baseG = mapPlanes(im.rgb, im.w, im.h, (plane) => F.guided(plane, plane, im.w, im.h, p.r, eps).image);
      const baseB = mapPlanes(im.rgb, im.w, im.h, (plane) => F.bilateral(plane, im.w, im.h, p.sigmaS, p.sigmaR, 12).image);
      image = baseG;
      extra.guidedShow = "tulip";
      extra.viewW = im.w;
      extra.viewH = im.h;
      extra.rgbIn = im.rgb;
      extra.rgbBi = enhanceRGB(im.rgb, baseB, p.gain);
      extra.rgbGf = enhanceRGB(im.rgb, baseG, p.gain);
    } else if (state.filter === "guided" && p.example === "fur" && state.photos.mask && state.photos.guide) {
      const mask = imageToGray(state.photos.mask);
      const guide = imageToRGB(state.photos.guide, mask.w, { w: mask.w, h: mask.h });
      const eps = p.eps * 255 * 255;
      const grayI = rgbToGray(guide.rgb, guide.w, guide.h);
      const g = F.guided(mask.gray, grayI, mask.w, mask.h, p.r, eps);
      image = g.image;
      extra.guidedShow = "fur";
      extra.viewW = mask.w;
      extra.viewH = mask.h;
      extra.mask = mask.gray;
      extra.rgbGuide = guide.rgb;
    } else {
      const eps = p.eps * 255 * 255;
      const g = F.guided(src, src, w, h, p.r, eps);
      image = g.image;
      extra.a = g.a;
      extra.guide = src;
    }
    return { image, ms: performance.now() - t0, extra };
  }

  function drawAux(result) {
    const ctx = auxCtx;
    const W = auxCanvas.width;
    const H = auxCanvas.height;
    ctx.clearRect(0, 0, W, H);
    ctx.fillStyle = "#f7fafc";
    ctx.fillRect(0, 0, W, H);
    const cap = document.getElementById("auxCaption");
    if (state.filter === "guided" && result.extra.a) {
      if (currentParams().guide === "texture" && result.extra.guide) {
        cap.textContent = "左：引导图的细纹　　右：系数 a";
        drawGuideAndMap(result.extra.guide, result.extra.a);
      } else {
        cap.textContent = "系数 a（亮处更接近 1，边缘被保留）";
        drawScalarMap(result.extra.a, state.w, state.h);
      }
      return;
    }
    if (state.filter === "joint" && result.extra.guide) {
      cap.textContent = "左：引导图　　右：该点权重（灰度差来自引导图）";
      drawJointAux(result.extra.guide, currentParams());
      return;
    }
    if (state.filter === "gauss" && result.extra.kernel) {
      cap.textContent = "归一化一维高斯核";
      drawKernel1d(result.extra.kernel);
      return;
    }
    if (!state.probe) {
      cap.textContent = "邻域";
      ctx.fillStyle = "#5c6b7a";
      ctx.font = "14px Microsoft YaHei";
      ctx.fillText("点击左侧输入图", 24, H / 2);
      ctx.fillText("查看该点的邻域权重", 24, H / 2 + 24);
      return;
    }
    const x = state.probe.x;
    const y = state.probe.y;
    const p = currentParams();
    if (state.filter === "maxmin" || state.filter === "mean") {
      const r = p.r;
      const view = F.windowAt(state.noisy, state.w, state.h, x, y, r);
      const single = p.mode === "max" || p.mode === "min";
      cap.textContent = state.filter === "mean"
        ? "邻域像素（权重均为 1/N）"
        : p.mode === "max"
          ? "邻域像素（红框为最大值）"
          : p.mode === "min"
            ? "邻域像素（红框为最小值）"
            : "邻域像素（组合滤波看右侧输出）";
      drawWindow(view.win, state.filter === "maxmin" && single ? p.mode : null);
    } else if (state.filter === "bilateral") {
      cap.textContent = "该点的双边权重（越红越大）";
      const bw = F.bilateralWeights(state.noisy, state.w, state.h, x, y, p.sigmaS, p.sigmaR);
      drawWeights(bw.weights, bw.maxW);
    }
  }

  function jointGuide(kind) {
    const clean = state.clean;
    const noisy = state.noisy;
    if (kind === "self" || !clean) return noisy;
    if (kind === "clean") return clean;
    const guide = new Float32Array(clean.length);
    for (let i = 0; i < clean.length; i++) guide[i] = 0.55 * clean[i] + 0.45 * 128;
    return guide;
  }

  function drawJointAux(guide, params) {
    const ctx = auxCtx;
    const c = document.createElement("canvas");
    c.width = state.w;
    c.height = state.h;
    const ictx = c.getContext("2d");
    const img = ictx.createImageData(state.w, state.h);
    for (let i = 0; i < guide.length; i++) {
      const v = F.clamp(Math.round(guide[i]), 0, 255);
      const o = i * 4;
      img.data[o] = img.data[o + 1] = img.data[o + 2] = v;
      img.data[o + 3] = 255;
    }
    ictx.putImageData(img, 0, 0);
    let drawW = 150;
    let drawH = drawW * state.h / state.w;
    if (drawH > 210) {
      drawH = 210;
      drawW = drawH * state.w / state.h;
    }
    ctx.drawImage(c, 12, 36, drawW, drawH);
    ctx.fillStyle = "#5c6b7a";
    ctx.font = "13px Microsoft YaHei";
    ctx.fillText("引导图", 12, 22);
    if (state.probe) {
      const sx = 12 + (state.probe.x / state.w) * drawW;
      const sy = 36 + (state.probe.y / state.h) * drawH;
      ctx.strokeStyle = "#b42024";
      ctx.lineWidth = 1.5;
      ctx.strokeRect(sx - 3, sy - 3, 6, 6);
      const bw = F.bilateralWeights(
        guide, state.w, state.h, state.probe.x, state.probe.y, params.sigmaS, params.sigmaR
      );
      ctx.fillText("权重", drawW + 28, 22);
      drawWeights(bw.weights, bw.maxW, { x: drawW + 28, y: 36, size: 150 });
    } else {
      ctx.fillText("点击输入图查看权重", drawW + 28, 120);
    }
  }

  function drawGuideAndMap(guide, a) {
    const ctx = auxCtx;
    const thumb = document.createElement("canvas");
    thumb.width = state.w;
    thumb.height = state.h;
    const tctx = thumb.getContext("2d");
    const img = tctx.createImageData(state.w, state.h);
    for (let i = 0; i < guide.length; i++) {
      const v = F.clamp(Math.round(guide[i]), 0, 255);
      const o = i * 4;
      img.data[o] = img.data[o + 1] = img.data[o + 2] = v;
      img.data[o + 3] = 255;
    }
    tctx.putImageData(img, 0, 0);
    const gap = 10;
    const boxW = (auxCanvas.width - 28 - gap) / 2;
    const boxH = Math.min(190, boxW * state.h / state.w);
    ctx.drawImage(thumb, 12, 28, boxW, boxH);
    ctx.fillStyle = "#5c6b7a";
    ctx.font = "13px Microsoft YaHei";
    ctx.fillText("引导图", 12, 18);

    let lo = Infinity;
    let hi = -Infinity;
    for (let i = 0; i < a.length; i++) {
      if (a[i] < lo) lo = a[i];
      if (a[i] > hi) hi = a[i];
    }
    const span = hi - lo || 1;
    const map = tctx.createImageData(state.w, state.h);
    for (let i = 0; i < a.length; i++) {
      const t = (a[i] - lo) / span;
      const o = i * 4;
      map.data[o] = Math.round(27 + t * 153);
      map.data[o + 1] = Math.round(78 + t * 20);
      map.data[o + 2] = Math.round(121 - t * 40);
      map.data[o + 3] = 255;
    }
    tctx.putImageData(map, 0, 0);
    const x0 = 12 + boxW + gap;
    ctx.drawImage(thumb, x0, 28, boxW, boxH);
    ctx.fillText("系数 a", x0, 18);
    ctx.fillStyle = "#1c2833";
    ctx.font = "12px Microsoft YaHei";
    ctx.fillText("a  " + lo.toFixed(2) + " … " + hi.toFixed(2), 12, boxH + 48);
  }

  function drawScalarMap(data, w, h) {
    const ctx = auxCtx;
    const img = ctx.createImageData(w, h);
    let lo = Infinity;
    let hi = -Infinity;
    for (let i = 0; i < data.length; i++) {
      if (data[i] < lo) lo = data[i];
      if (data[i] > hi) hi = data[i];
    }
    const span = hi - lo || 1;
    for (let i = 0; i < data.length; i++) {
      const t = (data[i] - lo) / span;
      const o = i * 4;
      img.data[o] = Math.round(27 + t * 153);
      img.data[o + 1] = Math.round(78 + t * 20);
      img.data[o + 2] = Math.round(121 - t * 40);
      img.data[o + 3] = 255;
    }
    const c = document.createElement("canvas");
    c.width = w;
    c.height = h;
    c.getContext("2d").putImageData(img, 0, 0);
    ctx.drawImage(c, 16, 16, auxCanvas.width - 32, auxCanvas.height - 48);
    ctx.fillStyle = "#1c2833";
    ctx.font = "13px Microsoft YaHei";
    ctx.fillText("a 范围  " + lo.toFixed(2) + " … " + hi.toFixed(2), 16, auxCanvas.height - 14);
  }

  function drawKernel1d(k) {
    const ctx = auxCtx;
    const n = k.length;
    const mid = (n - 1) / 2;
    const max = Math.max(...k);
    const left = 28;
    const bottom = 210;
    const width = auxCanvas.width - 56;
    const barW = width / n;
    ctx.strokeStyle = "#d5dee8";
    ctx.beginPath();
    ctx.moveTo(left, bottom);
    ctx.lineTo(left + width, bottom);
    ctx.stroke();
    for (let i = 0; i < n; i++) {
      const bh = (k[i] / max) * 150;
      ctx.fillStyle = i === mid ? "#b42024" : "#1b4e79";
      ctx.fillRect(left + i * barW + 2, bottom - bh, Math.max(2, barW - 4), bh);
      if (n <= 15) {
        ctx.fillStyle = "#1c2833";
        ctx.font = "11px Consolas";
        ctx.textAlign = "center";
        ctx.fillText(k[i].toFixed(3), left + i * barW + barW / 2, bottom + 16);
        ctx.fillText(String(i - mid), left + i * barW + barW / 2, bottom + 32);
      }
    }
    ctx.textAlign = "left";
    ctx.fillStyle = "#5c6b7a";
    ctx.font = "13px Microsoft YaHei";
    ctx.fillText("红柱是中心。二维核 = 该一维核的外积。", 16, 28);
  }

  function drawWindow(win, mode) {
    const ctx = auxCtx;
    const n = win.length;
    let best = mode === "min" ? Infinity : -Infinity;
    if (mode) {
      win.forEach((row) => row.forEach((v) => {
        best = mode === "min" ? Math.min(best, v) : Math.max(best, v);
      }));
    }
    const pad = 16;
    const cell = Math.min(36, Math.floor((auxCanvas.width - pad * 2) / n));
    const ox = (auxCanvas.width - cell * n) / 2;
    const oy = 24;
    ctx.font = (cell > 26 ? 12 : 10) + "px Consolas";
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    for (let y = 0; y < n; y++) {
      for (let x = 0; x < n; x++) {
        const v = win[y][x];
        const mark = mode && v === best;
        ctx.fillStyle = mark ? "#fde8e8" : "#ffffff";
        ctx.strokeStyle = mark ? "#b42024" : "#d5dee8";
        ctx.lineWidth = mark ? 2 : 1;
        ctx.fillRect(ox + x * cell, oy + y * cell, cell - 2, cell - 2);
        ctx.strokeRect(ox + x * cell, oy + y * cell, cell - 2, cell - 2);
        ctx.fillStyle = "#1c2833";
        ctx.fillText(String(Math.round(v)), ox + x * cell + cell / 2 - 1, oy + y * cell + cell / 2 - 1);
      }
    }
    ctx.textAlign = "left";
    ctx.textBaseline = "alphabetic";
  }

  function drawWeights(weights, maxW, box) {
    const ctx = auxCtx;
    const n = weights.length;
    const pad = 20;
    const cell = box
      ? Math.max(4, Math.floor(box.size / n))
      : Math.min(28, Math.floor((auxCanvas.width - pad * 2) / n));
    const ox = box ? box.x : (auxCanvas.width - cell * n) / 2;
    const oy = box ? box.y : 28;
    for (let y = 0; y < n; y++) {
      for (let x = 0; x < n; x++) {
        const t = weights[y][x] / (maxW || 1);
        const R = Math.round(255 - t * 80);
        const G = Math.round(255 - t * 200);
        const B = Math.round(255 - t * 190);
        ctx.fillStyle = "rgb(" + R + "," + G + "," + B + ")";
        ctx.fillRect(ox + x * cell, oy + y * cell, cell - 1, cell - 1);
      }
    }
    const cx = ox + ((n - 1) / 2) * cell;
    const cy = oy + ((n - 1) / 2) * cell;
    ctx.strokeStyle = "#1b4e79";
    ctx.strokeRect(cx, cy, cell - 1, cell - 1);
    if (!box) {
      ctx.fillStyle = "#5c6b7a";
      ctx.font = "13px Microsoft YaHei";
      ctx.fillText("中心格有蓝框。跨边缘的一侧会明显变淡。", 16, auxCanvas.height - 16);
    }
  }

  function comma(n) {
    return Math.round(n).toLocaleString("zh-CN");
  }

  function fillSepBox(k) {
    const box = document.getElementById("sepBox");
    box.hidden = false;
    const K = k.length;
    const perDirect = K * K;
    const perSep = 2 * K;
    const pixels = state.w * state.h;
    const ratio = perDirect / perSep;
    const barSep = Math.max(4, Math.round(100 * perSep / perDirect));
    let grid = "";
    if (K <= 9) {
      const peak = k[0] * k[0];
      let cells = "";
      for (let j = 0; j < K; j++) {
        for (let i = 0; i < K; i++) {
          const wgt = k[j] * k[i];
          const t = peak > 0 ? wgt / peak : 0;
          const ink = Math.round(27 + t * 150);
          cells += "<i style=\"background:rgb(" + ink + "," + Math.round(78 + t * 30) + "," + Math.round(121 - t * 40) + ")\">" + wgt.toFixed(2) + "</i>";
        }
      }
      grid = "<h3>二维核 = 一维核的外积</h3><div class=\"kgrid\" style=\"grid-template-columns:repeat(" + K + ",1fr)\">" + cells + "</div>";
    }
    box.innerHTML =
      "<h3>每个像素的乘法次数</h3>" +
      "<div class=\"cost-row\"><span>直接二维</span><div class=\"bar-track\"><div class=\"bar-fill\" style=\"width:100%\"></div></div><span>" + perDirect + "</span></div>" +
      "<div class=\"cost-row\"><span>可分离</span><div class=\"bar-track\"><div class=\"bar-fill alt\" style=\"width:" + barSep + "%\"></div></div><span>" + perSep + "</span></div>" +
      "<p>整幅 " + state.w + "×" + state.h + "：直接 " + comma(pixels * perDirect) + " 次，可分离 " + comma(pixels * perSep) + " 次。</p>" +
      "<p>理论加速比 " + perDirect + " / " + perSep + " = <b>" + ratio.toFixed(2) + "</b>，也就是 K/2，K = " + K + "。</p>" +
      "<p id=\"sepTime\">正在测量两种写法的耗时…</p>" +
      grid;
  }

  let benchToken = 0;
  let benchTimer = 0;
  let benchCache = null;

  function showBench(b) {
    const el = document.getElementById("sepTime");
    if (!el) return;
    const measured = b.dirT / Math.max(b.sepT, 0.01);
    el.textContent = "实测：直接二维 " + b.dirT.toFixed(1) + " ms，可分离 " + b.sepT.toFixed(1) +
      " ms，加速比 " + measured.toFixed(2) + "。两种结果最大差 " + b.err.toFixed(3) + "。";
  }

  function scheduleBench(k) {
    const key = benchKey(k);
    if (benchCache && benchCache.key === key) {
      showBench(benchCache);
      return;
    }
    const token = ++benchToken;
    const src = state.noisy;
    const w = state.w;
    const h = state.h;
    clearTimeout(benchTimer);
    benchTimer = setTimeout(() => {
      if (token !== benchToken || state.filter !== "separable") return;
      const sepT = timeCall(() => F.convSeparable(src, w, h, k));
      let direct = null;
      const dirT = timeCall(() => { direct = F.conv2d(src, w, h, k); });
      const sep = F.convSeparable(src, w, h, k);
      let err = 0;
      for (let i = 0; i < sep.length; i++) {
        const d = Math.abs(sep[i] - direct[i]);
        if (d > err) err = d;
      }
      benchCache = { key, sepT, dirT, err };
      if (state.filter === "separable") showBench(benchCache);
    }, 280);
  }

  function render() {
    if (!state.noisy) return;
    const result = runFilter();
    const separable = state.filter === "separable" && result.extra.mid;
    const compare = state.filter === "bilateral" && state.compare && result.extra.gauss;
    const show = result.extra.guidedShow;
    const stage = document.querySelector(".stage");
    stage.classList.toggle("sep", !!separable);
    stage.classList.toggle("compare", !!compare || !!show);
    if (show === "tulip") {
      paintRGB(srcCanvas, srcCtx, result.extra.rgbIn, result.extra.viewW, result.extra.viewH);
      paintRGB(dstCanvas, dstCtx, result.extra.rgbBi, result.extra.viewW, result.extra.viewH);
      paintRGB(auxCanvas, auxCtx, result.extra.rgbGf, result.extra.viewW, result.extra.viewH);
    } else if (show === "fur") {
      paintGray(srcCanvas, srcCtx, result.extra.mask, result.extra.viewW, result.extra.viewH);
      paintRGB(dstCanvas, dstCtx, result.extra.rgbGuide, result.extra.viewW, result.extra.viewH);
      paintGray(auxCanvas, auxCtx, result.image, result.extra.viewW, result.extra.viewH);
    } else {
      paintGray(srcCanvas, srcCtx, state.noisy, state.w, state.h);
      if (separable) {
        paintGray(dstCanvas, dstCtx, result.extra.mid, state.w, state.h);
        paintGray(auxCanvas, auxCtx, result.image, state.w, state.h);
      } else if (compare) {
        paintGray(dstCanvas, dstCtx, result.extra.gauss, state.w, state.h);
        paintGray(auxCanvas, auxCtx, result.image, state.w, state.h);
      } else {
        paintGray(dstCanvas, dstCtx, result.image, state.w, state.h);
      }
    }
    if (state.probe && !separable && !show) {
      markProbe(srcCtx, state.probe.x, state.probe.y, state.w, state.h);
      if (compare) {
        markProbe(dstCtx, state.probe.x, state.probe.y, state.w, state.h);
        markProbe(auxCtx, state.probe.x, state.probe.y, state.w, state.h);
      }
    }
    const spec = FILTERS[state.filter];
    const srcKind = document.getElementById("sourceSelect").value;
    if (separable) {
      document.getElementById("srcCaption").textContent = "输入";
      document.getElementById("dstCaption").textContent = "第一步：每一行与一维核卷积";
      document.getElementById("auxCaption").textContent = "第二步：再对每一列卷积";
      fillSepBox(result.extra.kernel);
      scheduleBench(result.extra.kernel);
    } else if (show === "tulip") {
      document.getElementById("sepBox").hidden = true;
      document.getElementById("srcCaption").textContent = "输入（引导图 = 原图）";
      document.getElementById("dstCaption").textContent = "双边滤波后把细节加回去";
      document.getElementById("auxCaption").textContent = "引导滤波后把细节加回去";
    } else if (show === "fur") {
      document.getElementById("sepBox").hidden = true;
      document.getElementById("srcCaption").textContent = "输入 p · 硬掩模";
      document.getElementById("dstCaption").textContent = "引导图 I · 毛发";
      document.getElementById("auxCaption").textContent = "输出 q · 边缘带上细毛";
    } else if (compare) {
      document.getElementById("sepBox").hidden = true;
      const sigma = currentParams().sigmaS.toFixed(1);
      document.getElementById("srcCaption").textContent = "输入 · 人像加高斯噪声";
      document.getElementById("dstCaption").textContent = "普通高斯滤波 · σ = " + sigma;
      document.getElementById("auxCaption").textContent = "双边滤波 · σs = " + sigma;
    } else {
      document.getElementById("sepBox").hidden = true;
      drawAux(result);
      document.getElementById("srcCaption").textContent = srcKind === "mask"
        ? "输入 · 硬掩模（点击查看邻域）"
        : "输入（含噪声） · 点击查看该点邻域";
      document.getElementById("dstCaption").textContent = "输出 · " + spec.name;
    }
    document.getElementById("formula").innerHTML = spec.formula;
    document.getElementById("note").textContent = show === "tulip"
      ? "课件原图上的参数是双边 σs=16、σr=0.1，引导滤波 r=16、ε=0.1²，增强倍数 5。这里的图缩小了，σs 和 r 按宽度大约缩到原来的一半，ε 仍是 0.01。花瓣边缘上，双边滤波会冒出一圈反色；引导滤波只把梯度乘上系数 a，方向与原图一致。"
      : show === "fur"
        ? "课件上的结构迁移：输入是一张硬边界的掩模，引导图是毛发。窗口盖住边界时，输出会跟上引导图的细毛。半径 r 越大，毛越往掩模里长；ε 越大，越接近把掩模涂平。细节倍数和双边的 σ 这一例不用。"
        : compare
          ? "同一幅带高斯噪声的人像，空间尺度都用 σs。普通高斯滤波的核到处一样，帽檐、头发和肩膀会被抹平。双边滤波再乘上灰度差权重，皮肤上的噪声压下去，这些轮廓还留着。拖动 σs、σr 时两张结果一起变。点击输入图，三张图上是同一个点。"
          : spec.note;
    const p = currentParams();
    let sizeText = "";
    if (p.r != null && state.filter !== "separable") sizeText = "窗口 " + (2 * p.r + 1) + "×" + (2 * p.r + 1) + " · ";
    if (state.filter === "gauss") {
      const rad = Math.max(1, Math.ceil(3 * p.sigma));
      sizeText = "核宽 " + (2 * rad + 1) + " · ";
    }
    if (separable) {
      const K = result.extra.kernel.length;
      sizeText = "K=" + K + " · 理论加速比 " + (K / 2).toFixed(2) + " · ";
    }
    if ((state.filter === "bilateral" || state.filter === "joint") && result.extra.radius) {
      const rad = result.extra.radius;
      sizeText = "窗口 " + (2 * rad + 1) + "×" + (2 * rad + 1) + " · ";
    }
    const viewW = show ? result.extra.viewW : state.w;
    const viewH = show ? result.extra.viewH : state.h;
    document.getElementById("stats").textContent =
      viewW + "×" + viewH + " · " + sizeText + (separable ? "行卷积与列卷积已分开显示" : "用时 " + result.ms.toFixed(0) + " ms");
  }

  function markProbe(ctx, x, y, w, h) {
    const s = srcCanvas.width / w;
    ctx.strokeStyle = "#b42024";
    ctx.lineWidth = 1.5;
    ctx.strokeRect(x * s - 3, y * s - 3, 7, 7);
  }

  function refreshNoise() {
    if (!state.clean) return;
    state.noisy = applyNoise(state.clean);
    render();
  }

  function setClean(gray, w, h) {
    state.clean = gray;
    state.w = w;
    state.h = h;
    if (state.filter === "bilateral" && state.compare) {
      state.probe = { x: (w / 2) | 0, y: (h / 2) | 0 };
    } else if (state.filter === "bilateral" || state.filter === "joint") {
      state.probe = { x: Math.round(w * 0.48), y: Math.round(h * 0.32) };
    } else if (state.filter === "guided") {
      state.probe = { x: Math.round(w * 0.46), y: Math.round(h * 0.45) };
    } else {
      state.probe = { x: (w / 2) | 0, y: (h / 2) | 0 };
    }
    refreshNoise();
  }

  function useSource(kind) {
    if (kind === "edge") {
      state.texture = null;
      const im = makeEdgeImage();
      setClean(im.gray, im.w, im.h);
      return;
    }
    if (kind === "mask") {
      const im = makeTransferPair();
      state.texture = im.guide;
      setClean(im.mask, im.w, im.h);
      return;
    }
    state.texture = null;
    const img = kind === "file" ? state.fileImg : state.lena;
    if (!img) return;
    const im = imageToGray(img);
    setClean(im.gray, im.w, im.h);
  }

  function isImpulse(noise) {
    return noise === "pepper" || noise === "salt" || noise === "sp";
  }

  function applyScene(scene) {
    if (!scene) return;
    const params = currentParams();
    state.compare = !!scene.compare;
    if (scene.noise) {
      state.noise = scene.noise;
      document.getElementById("noiseSelect").value = scene.noise;
    }
    if (scene.amt != null) state.noiseAmt = scene.amt;
    Object.keys(scene).forEach((k) => {
      if (k === "name" || k === "source" || k === "noise" || k === "amt" || k === "compare") return;
      params[k] = scene[k];
    });
    if (scene.source) {
      document.getElementById("sourceSelect").value = scene.source;
      useSource(scene.source);
    } else {
      refreshNoise();
    }
  }

  function buildNav() {
    const nav = document.getElementById("filters");
    nav.innerHTML = "";
    Object.keys(FILTERS).forEach((id) => {
      const btn = document.createElement("button");
      btn.type = "button";
      btn.textContent = FILTERS[id].name;
      btn.className = id === state.filter ? "active" : "";
      btn.addEventListener("click", () => {
        state.filter = id;
        applyScene(FILTERS[id].scene);
        buildNav();
        buildControls();
      });
      nav.appendChild(btn);
    });
  }

  function buildControls() {
    const spec = FILTERS[state.filter];
    const params = currentParams();
    document.getElementById("filterTitle").textContent = spec.name;
    const box = document.getElementById("fields");
    box.innerHTML = "";
    spec.fields.forEach((field) => {
      const wrap = document.createElement("div");
      wrap.className = "ctrl";
      if (field.type === "choice") {
        const title = document.createElement("div");
        title.className = "row";
        title.textContent = field.label;
        wrap.appendChild(title);
        const choices = document.createElement("div");
        choices.className = "choices";
        field.options.forEach((opt) => {
          const lab = document.createElement("label");
          const input = document.createElement("input");
          input.type = "radio";
          input.name = field.key;
          input.checked = params[field.key] === opt.value;
          input.addEventListener("change", () => {
            params[field.key] = opt.value;
            render();
          });
          lab.appendChild(input);
          lab.appendChild(document.createTextNode(opt.label));
          choices.appendChild(lab);
        });
        wrap.appendChild(choices);
      } else {
        const row = document.createElement("div");
        row.className = "row";
        const name = document.createElement("span");
        name.textContent = field.label;
        const out = document.createElement("output");
        const show = () => { out.textContent = field.fmt ? field.fmt(params[field.key]) : String(params[field.key]); };
        show();
        row.append(name, out);
        const range = document.createElement("input");
        range.type = "range";
        range.min = field.min;
        range.max = field.max;
        range.step = field.step;
        range.value = params[field.key];
        range.addEventListener("input", () => {
          const n = Number(range.value);
          params[field.key] = field.step >= 1 ? Math.round(n) : n;
          show();
          render();
        });
        wrap.append(row, range);
      }
      box.appendChild(wrap);
    });

    const noiseWrap = document.createElement("div");
    noiseWrap.className = "ctrl";
    const nrow = document.createElement("div");
    nrow.className = "row";
    const nlabel = document.createElement("span");
    const noiseName = { pepper: "黑点比例", salt: "白点比例", sp: "椒盐比例", gauss: "噪声标准差", none: "噪声" };
    nlabel.textContent = noiseName[state.noise] || "噪声";
    const nout = document.createElement("output");
    const showNoise = () => {
      nout.textContent = state.noise === "none"
        ? "关"
        : isImpulse(state.noise)
          ? Math.round(state.noiseAmt * 100) + "%"
          : state.noiseAmt.toFixed(0);
    };
    showNoise();
    nrow.append(nlabel, nout);
    const nrange = document.createElement("input");
    nrange.type = "range";
    if (isImpulse(state.noise)) {
      nrange.min = 0; nrange.max = 0.2; nrange.step = 0.005; nrange.value = state.noiseAmt;
    } else {
      nrange.min = 0; nrange.max = 40; nrange.step = 1; nrange.value = state.noise === "gauss" ? state.noiseAmt : 0;
    }
    nrange.disabled = state.noise === "none";
    nrange.addEventListener("input", () => {
      state.noiseAmt = Number(nrange.value);
      showNoise();
      refreshNoise();
    });
    noiseWrap.append(nrow, nrange);
    box.appendChild(noiseWrap);

    const presets = document.getElementById("presets");
    presets.innerHTML = "";
    (spec.presets || []).forEach((preset) => {
      const btn = document.createElement("button");
      btn.type = "button";
      btn.textContent = preset.name;
      btn.addEventListener("click", () => {
        applyScene(preset);
        buildControls();
      });
      presets.appendChild(btn);
    });
  }

  srcCanvas.addEventListener("click", (ev) => {
    const rect = srcCanvas.getBoundingClientRect();
    const x = Math.round((ev.clientX - rect.left) * (state.w / rect.width));
    const y = Math.round((ev.clientY - rect.top) * (state.h / rect.height));
    state.probe = {
      x: F.clamp(x, 0, state.w - 1),
      y: F.clamp(y, 0, state.h - 1),
    };
    render();
  });

  document.getElementById("sourceSelect").addEventListener("change", (ev) => {
    useSource(ev.target.value);
  });
  document.getElementById("noiseSelect").addEventListener("change", (ev) => {
    const next = ev.target.value;
    const prevImpulse = isImpulse(state.noise);
    state.noise = next;
    if (isImpulse(next) && !prevImpulse) state.noiseAmt = 0.06;
    if (next === "gauss" && state.noiseAmt < 1) state.noiseAmt = 18;
    if (next === "none") state.noiseAmt = 0;
    buildControls();
    refreshNoise();
  });
  document.getElementById("openBtn").addEventListener("click", () => {
    document.getElementById("fileInput").click();
  });
  document.getElementById("fileInput").addEventListener("change", (ev) => {
    const file = ev.target.files && ev.target.files[0];
    if (!file) return;
    const img = new Image();
    img.onload = () => {
      state.fileImg = img;
      document.getElementById("sourceSelect").value = "file";
      useSource("file");
    };
    img.src = URL.createObjectURL(file);
  });

  ["tulip.png", "fur_mask.png", "fur_guide.png"].forEach((src) => {
    const key = src === "tulip.png" ? "tulip" : src === "fur_mask.png" ? "mask" : "guide";
    const img = new Image();
    img.onload = () => {
      state.photos[key] = img;
      if (state.filter === "guided" && state.noisy) render();
    };
    img.src = src;
  });

  buildNav();
  buildControls();
  const boot = new Image();
  boot.onload = () => {
    state.lena = boot;
    useSource("lena");
  };
  boot.onerror = () => useSource("edge");
  boot.src = "lena.png";
})();
