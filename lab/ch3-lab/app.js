/* Chapter 3 classroom demos. Point ops follow the slide formulas. */
(function () {
  const GROUPS = [
    { id: "gray", name: "灰度级变换" },
    { id: "hist", name: "直方图" },
    { id: "alg", name: "代数与阈值" },
    { id: "geo", name: "几何变换" },
  ];

  const DEMOS = {
    linear: {
      group: "gray",
      name: "线性拉伸",
      formula: "M(v) = αv + β<br>α = (q′−p′)/(q−p)，β = p′−αp",
      note: "α < 1 时动态范围变窄；β > 0 时整体变亮。结果截断到 0–255。",
      fields: [
        { key: "alpha", label: "α 斜率", min: -1, max: 3, step: 0.01, value: 1 },
        { key: "beta", label: "β 截距", min: -150, max: 200, step: 1, value: 0 },
      ],
      presets: [
        { name: "原图", alpha: 1, beta: 0 },
        { name: "变亮", alpha: 1, beta: 40 },
        { name: "压缩", alpha: 0.45, beta: 60 },
        { name: "拉伸对比", alpha: 1.8, beta: -80 },
        { name: "反相斜率", alpha: -1, beta: 255 },
      ],
    },
    piece: {
      group: "gray",
      name: "分片线性",
      formula: "v < μ1 → 0；μ1 ≤ v ≤ μ2 → 255(v−μ1)/(μ2−μ1)；v > μ2 → 255",
      note: "把聚集在 [μ1, μ2] 的灰度拉开到 [0, 255]。低对比度图上最明显。",
      fields: [
        { key: "u1", label: "μ1", min: 0, max: 250, step: 1, value: 70 },
        { key: "u2", label: "μ2", min: 5, max: 255, step: 1, value: 170 },
      ],
    },
    log: {
      group: "gray",
      name: "对数 · 压高光",
      formula: "M(v) = (c / σ) ln[1 + (e^σ − 1) v]，v ∈ [0, 1]",
      note: "σ 越大，暗部扩张越强、亮部越被压缩。c 把输出缩放到灰度范围。",
      fields: [
        { key: "sigma", label: "σ", min: 0.4, max: 6, step: 0.05, value: 2 },
        { key: "c", label: "c", min: 40, max: 400, step: 1, value: 255 },
      ],
      presets: [
        { name: "弱", sigma: 0.8, c: 255 },
        { name: "中", sigma: 2, c: 255 },
        { name: "强", sigma: 5, c: 255 },
      ],
    },
    sar: {
      group: "gray",
      name: "对数 · 提亮区",
      formula: "M(v) = 255 α log2(1 + v) / log2(256)",
      note: "课件中 SAR 写成 255α log2(v)，输入可达 16 位。这里用 8 位图，并写成 log2(1+v)，避免 log(0)。α > 1 会让高亮更快饱和。",
      fields: [
        { key: "alpha", label: "α", min: 0.3, max: 2.5, step: 0.01, value: 1 },
      ],
    },
    exp: {
      group: "gray",
      name: "指数变换",
      formula: "M(v) = c [ (1+α)^v − 1 ] / α，v ∈ [0, 1]",
      note: "α 增大时，暗部更暗、亮部细节被拉开。减 1 保证 v = 0 时输出为 0。",
      fields: [
        { key: "alpha", label: "α", min: 0.2, max: 12, step: 0.1, value: 3 },
        { key: "c", label: "c", min: 40, max: 400, step: 1, value: 255 },
      ],
      presets: [
        { name: "弱", alpha: 0.8, c: 255 },
        { name: "中", alpha: 3, c: 255 },
        { name: "强", alpha: 8, c: 255 },
      ],
    },
    gamma: {
      group: "gray",
      name: "幂律变换",
      formula: "M(v) = c · v<sup>γ</sup>，v ∈ [0, 1]",
      note: "γ > 1 增强亮部对比（偏指数）；γ < 1 增强暗部（偏对数）。c 做灰度伸缩。",
      fields: [
        { key: "gamma", label: "γ", min: 0.1, max: 4, step: 0.01, value: 1 },
        { key: "c", label: "c", min: 40, max: 400, step: 1, value: 255 },
      ],
      presets: [
        { name: "γ=0.4", gamma: 0.4, c: 255 },
        { name: "γ=1", gamma: 1, c: 255 },
        { name: "γ=1.5", gamma: 1.5, c: 255 },
        { name: "γ=3", gamma: 3, c: 255 },
      ],
    },
    invert: {
      group: "gray",
      name: "黑白反转",
      formula: "M(v) = (1 − t) v + t (255 − v)",
      note: "t = 1 为完全反转，可突出暗背景里的亮细节。",
      fields: [
        { key: "t", label: "反转强度 t", min: 0, max: 1, step: 0.01, value: 1 },
      ],
    },
    equalize: {
      group: "hist",
      name: "直方图均衡",
      formula: "s(v) = 255 · (累计像素数 − cdf<sub>min</sub>) / (总像素 − cdf<sub>min</sub>)<br>M(v) = (1 − t) v + t s(v)",
      note: "t 不是均衡算法本身的参数，而是原图与均衡结果的混合系数。先由累计直方图得到均衡灰度 s(v)，再按上式混合。t = 0 时图像不变；t = 1 时为完全均衡；t 越大，对比度拉开得越明显。离散均衡通常得不到完全平坦的直方图。",
      fields: [
        { key: "t", label: "均衡强度 t", min: 0, max: 1, step: 0.01, value: 1 },
      ],
    },
    specify: {
      group: "hist",
      name: "直方图规定化",
      formula: "令输入与目标的累计直方图对齐，再取最近的目标灰度",
      note: "目标分布取高斯：均值 μ、标准差 σ。规定化只能近似目标直方图。",
      fields: [
        { key: "mu", label: "目标均值 μ", min: 20, max: 235, step: 1, value: 140 },
        { key: "sigma", label: "目标标准差 σ", min: 8, max: 80, step: 1, value: 40 },
      ],
    },
    add: {
      group: "alg",
      name: "加常量",
      formula: "g(x, y) = f(x, y) + b",
      note: "整幅图加同一个常数，用来调亮度。",
      fields: [
        { key: "b", label: "常量 b", min: -120, max: 120, step: 1, value: 30 },
      ],
    },
    diff: {
      group: "alg",
      name: "帧差",
      formula: "g(x, y) = |f(x, y) − f(x − dx, y − dy)|",
      note: "用平移后的自身模拟下一帧，位移越大，边缘响应越强。",
      fields: [
        { key: "dx", label: "水平位移 dx", min: 0, max: 40, step: 1, value: 6 },
        { key: "dy", label: "垂直位移 dy", min: -20, max: 20, step: 1, value: 0 },
      ],
    },
    thresh: {
      group: "alg",
      name: "阈值化",
      formula: "g = 255，若 v ≥ T；否则 g = 0",
      note: "直方图上的红线是当前阈值。可对照灰度分布选择 T。",
      fields: [
        { key: "T", label: "阈值 T", min: 0, max: 255, step: 1, value: 120 },
      ],
    },
    geom: {
      group: "geo",
      name: "几何变换与插值",
      formula: "p′ = H p，输出像素用 H⁻¹ 回到输入图再插值<br>只看尺度时，第三列为 [c<sub>x</sub>(1−s)+t<sub>x</sub>,&nbsp; c<sub>y</sub>(1−s)+t<sub>y</sub>,&nbsp; 1]",
      note: "统一尺度 s 对 x、y 乘同一个数，形状不变。缩放绕图像中心，不绕左上角：先把中心移到原点，乘 s，再移回去，最后加上滑块平移。所以 tx、ty 不动时，矩阵第三列前两行仍随 s 变化；s = 1 时这两项为 0，第三列才正好是 tx、ty。左上 2×2 才是尺度本身。欧式只含旋转和平移；相似再加 s；仿射再改 sx、sy 和错切；射影用透视系数 py。",
      fields: [
        { key: "theta", label: "旋转 θ °", min: -180, max: 180, step: 1, value: 20 },
        { key: "tx", label: "平移 tx", min: -180, max: 180, step: 1, value: 30 },
        { key: "ty", label: "平移 ty", min: -180, max: 180, step: 1, value: 16 },
        { key: "scale", label: "统一尺度 s", min: 0.4, max: 1.8, step: 0.01, value: 1 },
        { key: "sx", label: "sx", min: 0.5, max: 1.6, step: 0.01, value: 1 },
        { key: "sy", label: "sy", min: 0.5, max: 1.6, step: 0.01, value: 1 },
        { key: "shx", label: "错切 shx", min: -0.6, max: 0.6, step: 0.01, value: 0 },
        { key: "py", label: "透视 py", min: -0.0015, max: 0.0015, step: 0.00005, value: 0 },
      ],
      choice: {
        key: "interp",
        label: "灰度插值",
        options: [
          { value: "nearest", name: "最近邻" },
          { value: "bilinear", name: "双线性" },
          { value: "bicubic", name: "双三次" },
        ],
        value: "bilinear",
      },
      presets: [
        { name: "欧式", theta: 25, tx: 40, ty: 20, scale: 1, sx: 1, sy: 1, shx: 0, py: 0 },
        { name: "相似", theta: 20, tx: 50, ty: 30, scale: 0.75, sx: 1, sy: 1, shx: 0, py: 0 },
        { name: "仿射", theta: 12, tx: 20, ty: 10, scale: 1, sx: 0.95, sy: 0.8, shx: 0.35, py: 0 },
        { name: "射影", theta: 0, tx: 0, ty: 0, scale: 1, sx: 1, sy: 1, shx: 0, py: 0.0008 },
      ],
    },
  };

  const state = {
    group: "gray",
    demo: "linear",
    params: {},
    rgba: null,
    gray: null,
    w: 0,
    h: 0,
    lena: null,
  };

  const srcCanvas = document.getElementById("srcCanvas");
  const dstCanvas = document.getElementById("dstCanvas");
  const auxCanvas = document.getElementById("auxCanvas");
  const auxCtx = auxCanvas.getContext("2d");
  const formulaEl = document.getElementById("formula");
  const noteEl = document.getElementById("note");
  const matrixBox = document.getElementById("matrixBox");
  const auxCaption = document.getElementById("auxCaption");
  const controls = document.getElementById("controls");

  function clamp(v, a, b) { return v < a ? a : v > b ? b : v; }
  function clamp8(v) { return v < 0 ? 0 : v > 255 ? 255 : v; }

  function defaultParams(demo) {
    const p = {};
    (demo.fields || []).forEach((f) => { p[f.key] = f.value; });
    if (demo.choice) p[demo.choice.key] = demo.choice.value;
    return p;
  }

  function buildNav() {
    const g = document.getElementById("groups");
    const d = document.getElementById("demos");
    g.innerHTML = GROUPS.map((x) =>
      `<button type="button" data-group="${x.id}" class="${x.id === state.group ? "active" : ""}">${x.name}</button>`
    ).join("");
    const items = Object.entries(DEMOS).filter(([, v]) => v.group === state.group);
    d.innerHTML = items.map(([id, v]) =>
      `<button type="button" data-demo="${id}" class="${id === state.demo ? "active" : ""}">${v.name}</button>`
    ).join("");
    g.onclick = (e) => {
      const id = e.target.dataset.group;
      if (!id) return;
      state.group = id;
      state.demo = Object.keys(DEMOS).find((k) => DEMOS[k].group === id);
      state.params = defaultParams(DEMOS[state.demo]);
      buildNav();
      buildControls();
      render();
    };
    d.onclick = (e) => {
      const id = e.target.dataset.demo;
      if (!id) return;
      state.demo = id;
      state.params = defaultParams(DEMOS[id]);
      buildNav();
      buildControls();
      render();
    };
  }

  function buildControls() {
    const demo = DEMOS[state.demo];
    const p = state.params;
    let html = `<h2>${demo.name}</h2>`;
    (demo.fields || []).forEach((f) => {
      const val = formatVal(p[f.key], f.step);
      html += `<div class="ctrl">
        <div class="row"><span>${f.label}</span><output id="out-${f.key}">${val}</output></div>
        <input type="range" data-key="${f.key}" min="${f.min}" max="${f.max}" step="${f.step}" value="${p[f.key]}" />
      </div>`;
    });
    if (demo.choice) {
      html += `<div class="ctrl"><div class="row"><span>${demo.choice.label}</span></div><div class="choices">`;
      demo.choice.options.forEach((op) => {
        html += `<label><input type="radio" name="choice" value="${op.value}" ${p[demo.choice.key] === op.value ? "checked" : ""}/> ${op.name}</label>`;
      });
      html += `</div></div>`;
    }
    if (demo.presets) {
      html += `<div class="presets">` + demo.presets.map((pr, i) =>
        `<button type="button" class="ghost" data-preset="${i}">${pr.name}</button>`
      ).join("") + `</div>`;
    }
    controls.innerHTML = html;
    controls.querySelectorAll('input[type="range"]').forEach((el) => {
      el.addEventListener("input", () => {
        const key = el.dataset.key;
        state.params[key] = Number(el.value);
        if (state.demo === "piece") fixPiece();
        document.getElementById("out-" + key).textContent = formatVal(state.params[key], Number(el.step));
        render();
      });
    });
    controls.querySelectorAll('input[type="radio"]').forEach((el) => {
      el.addEventListener("change", () => {
        state.params[DEMOS[state.demo].choice.key] = el.value;
        render();
      });
    });
    controls.querySelectorAll("[data-preset]").forEach((el) => {
      el.addEventListener("click", () => {
        const pr = DEMOS[state.demo].presets[Number(el.dataset.preset)];
        Object.keys(pr).forEach((k) => { if (k !== "name") state.params[k] = pr[k]; });
        buildControls();
        render();
      });
    });
  }

  function formatVal(v, step) {
    if (step && step < 0.001) return Number(v).toFixed(5);
    if (step && step < 1) return Number(v).toFixed(2);
    return String(Math.round(Number(v)));
  }

  function fixPiece() {
    if (state.params.u1 >= state.params.u2) state.params.u2 = Math.min(255, state.params.u1 + 1);
    const u2 = controls.querySelector('[data-key="u2"]');
    if (u2) {
      u2.value = state.params.u2;
      document.getElementById("out-u2").textContent = String(state.params.u2);
    }
  }

  function lutOf() {
    const p = state.params;
    const lut = new Uint8Array(256);
    const id = state.demo;
    for (let v = 0; v < 256; v++) {
      let y = v;
      const u = v / 255;
      if (id === "linear") y = p.alpha * v + p.beta;
      else if (id === "piece") {
        if (v < p.u1) y = 0;
        else if (v > p.u2) y = 255;
        else y = 255 * (v - p.u1) / (p.u2 - p.u1);
      } else if (id === "log") {
        const s = Math.max(p.sigma, 0.05);
        y = (p.c / s) * Math.log(1 + (Math.exp(s) - 1) * u);
      } else if (id === "sar") {
        y = 255 * p.alpha * Math.log2(1 + v) / Math.log2(256);
      } else if (id === "exp") {
        const a = Math.max(p.alpha, 0.05);
        y = p.c * (Math.pow(1 + a, u) - 1) / a;
      } else if (id === "gamma") y = p.c * Math.pow(u, p.gamma);
      else if (id === "invert") y = (1 - p.t) * v + p.t * (255 - v);
      else if (id === "add") y = v + p.b;
      lut[v] = clamp8(Math.round(y));
    }
    return lut;
  }

  function histogram(gray) {
    const h = new Uint32Array(256);
    for (let i = 0; i < gray.length; i++) h[gray[i]]++;
    return h;
  }

  function cdfOf(h) {
    const c = new Float64Array(256);
    let s = 0;
    for (let i = 0; i < 256; i++) { s += h[i]; c[i] = s; }
    return c;
  }

  function equalizeLut(gray) {
    const h = histogram(gray);
    const c = cdfOf(h);
    let cdfmin = 0;
    for (let i = 0; i < 256; i++) if (c[i] > 0) { cdfmin = c[i]; break; }
    const n = gray.length;
    const lut = new Uint8Array(256);
    const denom = Math.max(1, n - cdfmin);
    for (let i = 0; i < 256; i++) lut[i] = clamp8(Math.round((c[i] - cdfmin) / denom * 255));
    return lut;
  }

  function specifyLut(gray, mu, sigma) {
    const n = gray.length;
    const src = cdfOf(histogram(gray));
    for (let i = 0; i < 256; i++) src[i] /= n;
    const pdf = new Float64Array(256);
    let z = 0;
    for (let i = 0; i < 256; i++) {
      const d = (i - mu) / sigma;
      pdf[i] = Math.exp(-0.5 * d * d);
      z += pdf[i];
    }
    const tgt = new Float64Array(256);
    let acc = 0;
    for (let i = 0; i < 256; i++) { acc += pdf[i] / z; tgt[i] = acc; }
    const lut = new Uint8Array(256);
    let j = 0;
    for (let i = 0; i < 256; i++) {
      while (j < 255 && tgt[j] < src[i]) j++;
      lut[i] = j;
    }
    return { lut, pdf };
  }

  function applyGrayLut(lut) {
    const w = state.w, h = state.h;
    const out = dstCanvas.getContext("2d").createImageData(w, h);
    const g = state.gray;
    const d = out.data;
    for (let i = 0, p = 0; i < g.length; i++, p += 4) {
      const y = lut[g[i]];
      d[p] = d[p + 1] = d[p + 2] = y;
      d[p + 3] = 255;
    }
    paint(dstCanvas, out);
  }

  function frameDiff() {
    const w = state.w, h = state.h;
    const dx = state.params.dx | 0;
    const dy = state.params.dy | 0;
    const out = dstCanvas.getContext("2d").createImageData(w, h);
    const g = state.gray;
    const d = out.data;
    for (let y = 0; y < h; y++) {
      for (let x = 0; x < w; x++) {
        const xs = x - dx, ys = y - dy;
        const a = g[y * w + x];
        const b = (xs < 0 || ys < 0 || xs >= w || ys >= h) ? 0 : g[ys * w + xs];
        const v = Math.abs(a - b);
        const p = (y * w + x) * 4;
        d[p] = d[p + 1] = d[p + 2] = v;
        d[p + 3] = 255;
      }
    }
    paint(dstCanvas, out);
  }

  function threshold() {
    const T = state.params.T;
    const lut = new Uint8Array(256);
    for (let i = 0; i < 256; i++) lut[i] = i >= T ? 255 : 0;
    applyGrayLut(lut);
  }

  function mul3(A, B) {
    const C = new Array(9);
    for (let r = 0; r < 3; r++) {
      for (let c = 0; c < 3; c++) {
        C[r * 3 + c] = A[r * 3] * B[c] + A[r * 3 + 1] * B[3 + c] + A[r * 3 + 2] * B[6 + c];
      }
    }
    return C;
  }

  function invert3(M) {
    const [a, b, c, d, e, f, g, h, i] = M;
    const A = e * i - f * h, B = f * g - d * i, C = d * h - e * g;
    const D = c * h - b * i, E = a * i - c * g, F = b * g - a * h;
    const G = b * f - c * e, H = c * d - a * f, I = a * e - b * d;
    const det = a * A + b * B + c * C;
    return [A / det, D / det, G / det, B / det, E / det, H / det, C / det, F / det, I / det];
  }

  function geometryMatrix() {
    const p = state.params;
    const cx = (state.w - 1) / 2, cy = (state.h - 1) / 2;
    const t = p.theta * Math.PI / 180;
    const c = Math.cos(t), s = Math.sin(t);
    const T1 = [1, 0, cx, 0, 1, cy, 0, 0, 1];
    const T0 = [1, 0, -cx, 0, 1, -cy, 0, 0, 1];
    const R = [c, -s, 0, s, c, 0, 0, 0, 1];
    const S = [p.scale * p.sx, 0, 0, 0, p.scale * p.sy, 0, 0, 0, 1];
    const Sh = [1, p.shx, 0, 0, 1, 0, 0, 0, 1];
    const P = [1, 0, 0, 0, 1, 0, 0, p.py, 1];
    const Tr = [1, 0, p.tx, 0, 1, p.ty, 0, 0, 1];
    return mul3(Tr, mul3(T1, mul3(P, mul3(R, mul3(Sh, mul3(S, T0))))));
  }

  function cubicWeight(t) {
    const a = -0.5;
    const x = Math.abs(t);
    if (x <= 1) return (a + 2) * x * x * x - (a + 3) * x * x + 1;
    if (x < 2) return a * x * x * x - 5 * a * x * x + 8 * a * x - 4 * a;
    return 0;
  }

  function sample(data, w, h, x, y, ch, mode) {
    if (mode === "nearest") {
      const ix = Math.round(x), iy = Math.round(y);
      if (ix < 0 || iy < 0 || ix >= w || iy >= h) return 0;
      return data[(iy * w + ix) * 4 + ch];
    }
    if (mode === "bilinear") {
      const x0 = Math.floor(x), y0 = Math.floor(y);
      const dx = x - x0, dy = y - y0;
      const at = (xx, yy) => {
        if (xx < 0 || yy < 0 || xx >= w || yy >= h) return 0;
        return data[(yy * w + xx) * 4 + ch];
      };
      return at(x0, y0) * (1 - dx) * (1 - dy)
        + at(x0 + 1, y0) * dx * (1 - dy)
        + at(x0, y0 + 1) * (1 - dx) * dy
        + at(x0 + 1, y0 + 1) * dx * dy;
    }
    const x0 = Math.floor(x), y0 = Math.floor(y);
    let acc = 0;
    for (let m = -1; m <= 2; m++) {
      const wy = cubicWeight(y - (y0 + m));
      const yy = y0 + m;
      for (let n = -1; n <= 2; n++) {
        const xx = x0 + n;
        const wx = cubicWeight(x - xx);
        if (xx < 0 || yy < 0 || xx >= w || yy >= h) continue;
        acc += data[(yy * w + xx) * 4 + ch] * wx * wy;
      }
    }
    return acc;
  }

  function warp() {
    const w = state.w, h = state.h;
    const H = geometryMatrix();
    const inv = invert3(H);
    const src = state.rgba;
    const out = dstCanvas.getContext("2d").createImageData(w, h);
    const d = out.data;
    const mode = state.params.interp;
    for (let y = 0; y < h; y++) {
      for (let x = 0; x < w; x++) {
        const X = inv[0] * x + inv[1] * y + inv[2];
        const Y = inv[3] * x + inv[4] * y + inv[5];
        const Z = inv[6] * x + inv[7] * y + inv[8];
        const xs = X / Z, ys = Y / Z;
        const p = (y * w + x) * 4;
        d[p] = clamp8(sample(src, w, h, xs, ys, 0, mode));
        d[p + 1] = clamp8(sample(src, w, h, xs, ys, 1, mode));
        d[p + 2] = clamp8(sample(src, w, h, xs, ys, 2, mode));
        d[p + 3] = 255;
      }
    }
    paint(dstCanvas, out);
    drawMatrix(H);
    drawZoom(out);
  }

  function drawMatrix(H) {
    const row = (i) => [0, 1, 2].map((k) => {
      const v = H[i * 3 + k];
      const s = (v >= 0 ? " " : "") + v.toFixed(4);
      return s.padStart(9);
    }).join(" ");
    matrixBox.hidden = false;
    matrixBox.textContent = "H =\n" + [0, 1, 2].map(row).join("\n");
  }

  function drawZoom(imageData) {
    auxCaption.textContent = "局部放大（插值差异）";
    const ctx = auxCtx;
    ctx.clearRect(0, 0, auxCanvas.width, auxCanvas.height);
    const sw = 64, sh = 64;
    const sx = Math.round(state.w * 0.42);
    const sy = Math.round(state.h * 0.38);
    const dstCtx = dstCanvas.getContext("2d");
    dstCtx.strokeStyle = "#ff3b30";
    dstCtx.lineWidth = 2;
    dstCtx.strokeRect(sx + 0.5, sy + 0.5, sw, sh);
    const tmp = document.createElement("canvas");
    tmp.width = state.w;
    tmp.height = state.h;
    tmp.getContext("2d").putImageData(imageData, 0, 0);
    ctx.imageSmoothingEnabled = false;
    ctx.drawImage(tmp, sx, sy, sw, sh, 16, 16, 328, 248);
    ctx.strokeStyle = "#b42024";
    ctx.lineWidth = 2;
    ctx.strokeRect(16, 16, 328, 248);
  }

  function paint(canvas, imageData) {
    canvas.width = imageData.width;
    canvas.height = imageData.height;
    canvas.getContext("2d").putImageData(imageData, 0, 0);
  }

  function showSource() {
    const img = srcCanvas.getContext("2d").createImageData(state.w, state.h);
    if (state.demo === "geom") {
      img.data.set(state.rgba);
    } else {
      const g = state.gray;
      for (let i = 0, p = 0; i < g.length; i++, p += 4) {
        img.data[p] = img.data[p + 1] = img.data[p + 2] = g[i];
        img.data[p + 3] = 255;
      }
    }
    paint(srcCanvas, img);
  }

  function drawCurve(lut, extra) {
    auxCaption.textContent = extra && extra.hist ? "直方图" : "映射曲线 M(v)";
    matrixBox.hidden = true;
    const ctx = auxCtx;
    const W = auxCanvas.width, H = auxCanvas.height;
    ctx.clearRect(0, 0, W, H);
    ctx.fillStyle = "#fff";
    ctx.fillRect(0, 0, W, H);
    const L = 36, R = 16, T = 16, B = 28;
    const pw = W - L - R, ph = H - T - B;
    ctx.strokeStyle = "#c5d0db";
    ctx.strokeRect(L, T, pw, ph);
    if (extra && extra.hist) {
      const h = extra.hist;
      let m = 1;
      for (let i = 0; i < 256; i++) if (h[i] > m) m = h[i];
      ctx.beginPath();
      ctx.strokeStyle = "#1b4e79";
      for (let i = 0; i < 256; i++) {
        const x = L + (i / 255) * pw;
        const y = T + ph - (h[i] / m) * ph;
        if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
      }
      ctx.stroke();
      if (extra.T != null) {
        const x = L + (extra.T / 255) * pw;
        ctx.strokeStyle = "#b42024";
        ctx.beginPath();
        ctx.moveTo(x, T);
        ctx.lineTo(x, T + ph);
        ctx.stroke();
      }
      if (extra.pdf) {
        let pm = 0;
        extra.pdf.forEach((v) => { if (v > pm) pm = v; });
        ctx.beginPath();
        ctx.strokeStyle = "#b42024";
        for (let i = 0; i < 256; i++) {
          const x = L + (i / 255) * pw;
          const y = T + ph - (extra.pdf[i] / pm) * ph * 0.85;
          if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
        }
        ctx.stroke();
      }
    } else if (lut) {
      ctx.beginPath();
      ctx.strokeStyle = "#b42024";
      ctx.lineWidth = 2;
      for (let i = 0; i < 256; i++) {
        const x = L + (i / 255) * pw;
        const y = T + ph - (lut[i] / 255) * ph;
        if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
      }
      ctx.stroke();
      ctx.lineWidth = 1;
    }
    ctx.fillStyle = "#5c6b7a";
    ctx.font = "12px Microsoft YaHei";
    ctx.fillText("0", L - 4, T + ph + 16);
    ctx.fillText("255", L + pw - 18, T + ph + 16);
    ctx.fillText("v", L + pw / 2, T + ph + 16);
  }

  function render() {
    if (!state.gray) return;
    const demo = DEMOS[state.demo];
    formulaEl.innerHTML = demo.formula;
    noteEl.textContent = demo.note;
    showSource();
    if (state.demo === "geom") {
      warp();
      return;
    }
    if (state.demo === "diff") {
      frameDiff();
      drawCurve(null, { hist: histogram(state.gray) });
      return;
    }
    if (state.demo === "thresh") {
      threshold();
      drawCurve(null, { hist: histogram(state.gray), T: state.params.T });
      return;
    }
    let lut;
    let extra = null;
    if (state.demo === "equalize") {
      const eq = equalizeLut(state.gray);
      lut = new Uint8Array(256);
      const t = state.params.t;
      for (let i = 0; i < 256; i++) lut[i] = clamp8(Math.round((1 - t) * i + t * eq[i]));
    } else if (state.demo === "specify") {
      const spec = specifyLut(state.gray, state.params.mu, state.params.sigma);
      lut = spec.lut;
      extra = { pdf: spec.pdf };
    } else {
      lut = lutOf();
    }
    applyGrayLut(lut);
    const hist = histogram(mappedGray(lut));
    if (state.demo === "equalize" || state.demo === "specify") {
      drawCurve(null, { hist, pdf: extra && extra.pdf });
    } else {
      drawCurve(lut);
    }
  }

  function mappedGray(lut) {
    const g = state.gray;
    const o = new Uint8Array(g.length);
    for (let i = 0; i < g.length; i++) o[i] = lut[g[i]];
    return o;
  }

  function setFromImageData(imageData, asGray) {
    state.w = imageData.width;
    state.h = imageData.height;
    state.rgba = new Uint8ClampedArray(imageData.data);
    const n = state.w * state.h;
    state.gray = new Uint8Array(n);
    const d = state.rgba;
    for (let i = 0, p = 0; i < n; i++, p += 4) {
      let y = (0.299 * d[p] + 0.587 * d[p + 1] + 0.114 * d[p + 2]) | 0;
      if (asGray === "low") y = clamp8(0.35 * y + 88);
      state.gray[i] = y;
      if (asGray) {
        d[p] = d[p + 1] = d[p + 2] = state.gray[i];
      }
    }
    render();
  }

  function loadImage(img, mode) {
    const c = document.createElement("canvas");
    const maxSide = 480;
    const scale = Math.min(1, maxSide / Math.max(img.width, img.height));
    c.width = Math.round(img.width * scale);
    c.height = Math.round(img.height * scale);
    const ctx = c.getContext("2d");
    ctx.drawImage(img, 0, 0, c.width, c.height);
    setFromImageData(ctx.getImageData(0, 0, c.width, c.height), mode === "low" ? "low" : null);
  }

  function makeRamp() {
    const w = 480, h = 360;
    const c = document.createElement("canvas");
    c.width = w; c.height = h;
    const ctx = c.getContext("2d");
    const img = ctx.createImageData(w, h);
    for (let y = 0; y < h; y++) {
      for (let x = 0; x < w; x++) {
        const v = Math.round(255 * x / (w - 1));
        const p = (y * w + x) * 4;
        img.data[p] = img.data[p + 1] = img.data[p + 2] = v;
        img.data[p + 3] = 255;
      }
    }
    setFromImageData(img, null);
  }

  function useSource(kind) {
    if (kind === "ramp") { makeRamp(); return; }
    if (!state.lena) return;
    loadImage(state.lena, kind);
  }

  document.getElementById("sourceSelect").addEventListener("change", (e) => useSource(e.target.value));
  document.getElementById("openBtn").onclick = () => document.getElementById("fileInput").click();
  document.getElementById("fileInput").addEventListener("change", (e) => {
    const file = e.target.files && e.target.files[0];
    if (!file) return;
    const img = new Image();
    img.onload = () => {
      state.lena = img;
      document.getElementById("sourceSelect").value = "lena";
      loadImage(img, "lena");
    };
    img.src = URL.createObjectURL(file);
  });

  const query = new URLSearchParams(location.search);
  if (query.get("demo") && DEMOS[query.get("demo")]) {
    state.demo = query.get("demo");
    state.group = DEMOS[state.demo].group;
  }
  state.params = defaultParams(DEMOS[state.demo]);
  const choiceKey = DEMOS[state.demo].choice && DEMOS[state.demo].choice.key;
  query.forEach((val, key) => {
    if (key === "demo" || !(key in state.params) || val === "") return;
    state.params[key] = key === choiceKey ? val : Number(val);
  });
  buildNav();
  buildControls();

  const boot = new Image();
  boot.onload = () => { state.lena = boot; useSource("lena"); };
  boot.src = "lena.png";
})();
