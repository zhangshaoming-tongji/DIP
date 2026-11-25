// 第9章 边缘检测课堂演示交互控制
const DEMOS = [
  {
    id: "first",
    tab: "1. 一阶微分算子对比",
    title: "一阶微分算子对比（Roberts / Prewitt / Sobel / Frei-Chen）",
    question: "对比不同一阶模板的平滑加权方式：为什么 Sobel 和 Frei-Chen 对噪声更稳健，且方向响应更均匀？",
    formula: "G = [Gx, Gy]^T,   |G| = sqrt(Gx^2 + Gy^2),   θ = atan2(Gy, Gx)",
    note: "可在右侧切换算子模板，并调节梯度阈值 T 与噪声强度 σ_noise 观察二值化边缘的虚警与漏检。"
  },
  {
    id: "log",
    tab: "2. 拉普拉斯、LoG 与 DoG",
    title: "二阶微分、LoG 与 DoG 过零点检测",
    question: "为什么直接用拉普拉斯算子对噪声极其敏感？先高斯平滑再求拉普拉斯（LoG）以及 DoG 近似如何抑制噪声并保留多尺度边缘？",
    formula: "LoG(x,y) = ((x^2+y^2-2σ^2)/(2πσ^6)) exp(-(x^2+y^2)/(2σ^2))  ≈  c · [G(σ) - G(kσ)]",
    note: "左下角显示 1D 水平剖面（第 128 行）：灰线为原图亮度，蓝线为 LoG 响应，红线为 DoG 响应，零交叉点即边缘位置。"
  },
  {
    id: "canny",
    tab: "3. Canny 全流程拆解",
    title: "Canny 边缘检测五步全流程",
    question: "调节尺度 σ 与双阈值 (T_low, T_high)：非极大值抑制（NMS）如何把粗边缘细化为单像素？滞后双阈值如何连接弱边缘同时抑制孤立噪声？",
    formula: "1. 高斯平滑 → 2. Sobel 梯度(M, θ) → 3. 沿 θ 方向 NMS → 4. 双阈值分类 → 5. 8-邻域滞后连接",
    note: "图5 中红色为强边缘（≥ T_high），蓝色为弱边缘（T_low ≤ M < T_high）；图6 为最终保留的连通边缘。"
  }
];

const state = {
  demo: "first",
  image: "cameraman",
  noiseSigma: 0,
  op: "sobel",
  thresh: 70,
  sigma: 1.4,
  kRatio: 1.6,
  zcSlope: 12,
  cannySigma: 1.4,
  tLow: 30,
  tHigh: 80
};

const imgCache = {};
const canvases = [0, 1, 2, 3, 4, 5].map(i => document.getElementById("c" + i));
const caps = [0, 1, 2, 3, 4, 5].map(i => document.getElementById("cap" + i));
const fig4 = document.getElementById("fig4");
const fig5 = document.getElementById("fig5");

function loadImage(name) {
  if (imgCache[name]) return Promise.resolve(imgCache[name]);
  return new Promise((resolve, reject) => {
    const img = new Image();
    img.onload = () => {
      const c = document.createElement("canvas");
      const size = 256;
      c.width = size;
      c.height = size;
      const ctx = c.getContext("2d");
      ctx.drawImage(img, 0, 0, size, size);
      const data = ctx.getImageData(0, 0, size, size);
      const gray = toGrayFloat(data);
      imgCache[name] = { w: size, h: size, gray };
      resolve(imgCache[name]);
    };
    img.onerror = reject;
    img.src = `images/${name}.png`;
  });
}

function drawGray(canvas, arr, w, h, mode = "clamp") {
  canvas.width = w;
  canvas.height = h;
  const ctx = canvas.getContext("2d");
  const imgData = ctx.createImageData(w, h);
  const d = imgData.data;
  let min = 0, max = 255;
  if (mode === "absmax") {
    let m = 1e-6;
    for (let i = 0; i < arr.length; i++) {
      const a = Math.abs(arr[i]);
      if (a > m) m = a;
    }
    for (let i = 0; i < arr.length; i++) {
      const v = Math.min(255, Math.round((Math.abs(arr[i]) / m) * 255));
      const p = i * 4;
      d[p] = d[p + 1] = d[p + 2] = v;
      d[p + 3] = 255;
    }
  } else if (mode === "signed") {
    let m = 1e-6;
    for (let i = 0; i < arr.length; i++) {
      const a = Math.abs(arr[i]);
      if (a > m) m = a;
    }
    for (let i = 0; i < arr.length; i++) {
      const v = Math.min(255, Math.max(0, Math.round(128 + (arr[i] / m) * 127)));
      const p = i * 4;
      d[p] = d[p + 1] = d[p + 2] = v;
      d[p + 3] = 255;
    }
  } else {
    for (let i = 0; i < arr.length; i++) {
      const v = Math.min(255, Math.max(0, Math.round(arr[i])));
      const p = i * 4;
      d[p] = d[p + 1] = d[p + 2] = v;
      d[p + 3] = 255;
    }
  }
  ctx.putImageData(imgData, 0, 0);
}

function drawHysteresisMap(canvas, st, w, h) {
  canvas.width = w;
  canvas.height = h;
  const ctx = canvas.getContext("2d");
  const imgData = ctx.createImageData(w, h);
  const d = imgData.data;
  for (let i = 0; i < w * h; i++) {
    const p = i * 4;
    if (st[i] === 2) {
      d[p] = 240; d[p + 1] = 60; d[p + 2] = 60; // 强边缘：红
    } else if (st[i] === 1) {
      d[p] = 65; d[p + 1] = 155; d[p + 2] = 255; // 弱边缘：蓝
    } else {
      d[p] = d[p + 1] = d[p + 2] = 12;
    }
    d[p + 3] = 255;
  }
  ctx.putImageData(imgData, 0, 0);
}

function drawProfile(canvas, rowGray, rowLog, rowDog, w) {
  const h = 256;
  canvas.width = w;
  canvas.height = h;
  const ctx = canvas.getContext("2d");
  ctx.fillStyle = "#11161d";
  ctx.fillRect(0, 0, w, h);

  // Zero line
  ctx.strokeStyle = "#394452";
  ctx.setLineDash([4, 4]);
  ctx.beginPath();
  ctx.moveTo(0, h * 0.62);
  ctx.lineTo(w, h * 0.62);
  ctx.stroke();
  ctx.setLineDash([]);

  // Gray profile in top band
  ctx.strokeStyle = "#9aa5b4";
  ctx.lineWidth = 1.4;
  ctx.beginPath();
  for (let x = 0; x < w; x++) {
    const y = 10 + (1 - rowGray[x] / 255) * (h * 0.34);
    if (x === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  }
  ctx.stroke();

  let maxAbs = 1e-3;
  for (let x = 0; x < w; x++) {
    maxAbs = Math.max(maxAbs, Math.abs(rowLog[x]), Math.abs(rowDog[x]));
  }
  const plotCurve = (arr, color) => {
    ctx.strokeStyle = color;
    ctx.lineWidth = 1.6;
    ctx.beginPath();
    for (let x = 0; x < w; x++) {
      const y = h * 0.62 - (arr[x] / maxAbs) * (h * 0.32);
      if (x === 0) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
    }
    ctx.stroke();
  };
  plotCurve(rowLog, "#4da6ff");
  plotCurve(rowDog, "#ff6b6b");

  ctx.fillStyle = "#cfd8e3";
  ctx.font = "11px sans-serif";
  ctx.fillText("灰: I(x)   蓝: LoG   红: DoG (虚线=0)", 8, 16);
}

function makeSelect(label, options, value, onChange) {
  const wrap = document.createElement("div");
  wrap.className = "ctrl";
  const lab = document.createElement("label");
  lab.textContent = label;
  const sel = document.createElement("select");
  for (const [val, txt] of options) {
    const o = document.createElement("option");
    o.value = val;
    o.textContent = txt;
    if (val === value) o.selected = true;
    sel.appendChild(o);
  }
  sel.onchange = () => onChange(sel.value);
  wrap.append(lab, sel);
  return wrap;
}

function makeSlider(label, min, max, step, value, onInput) {
  const wrap = document.createElement("div");
  wrap.className = "ctrl";
  const lab = document.createElement("label");
  const valSpan = document.createElement("span");
  valSpan.textContent = value;
  const textSpan = document.createElement("span");
  textSpan.textContent = label;
  lab.append(textSpan, valSpan);
  const inp = document.createElement("input");
  inp.type = "range";
  inp.min = min;
  inp.max = max;
  inp.step = step;
  inp.value = value;
  inp.oninput = () => {
    valSpan.textContent = inp.value;
    onInput(parseFloat(inp.value));
  };
  wrap.append(lab, inp);
  return wrap;
}

function buildControls() {
  const box = document.getElementById("controls");
  box.innerHTML = "";
  const kBox = document.getElementById("kernelBox");
  kBox.hidden = true;

  box.appendChild(makeSelect("测试图像", [
    ["cameraman", "Cameraman (256×256)"],
    ["lena", "Lena (256×256)"]
  ], state.image, v => { state.image = v; render(); }));

  box.appendChild(makeSlider("高斯噪声 σ_noise", 0, 25, 1, state.noiseSigma, v => {
    state.noiseSigma = v; render();
  }));

  if (state.demo === "first") {
    box.appendChild(makeSelect("一阶微分算子", [
      ["roberts", "Roberts (2×2 对角差分)"],
      ["prewitt", "Prewitt (3×3 均匀平滑)"],
      ["sobel", "Sobel (3×3 高斯加权)"],
      ["freichen", "Frei-Chen (3×3 各向同性)"]
    ], state.op, v => { state.op = v; render(); }));
    box.appendChild(makeSlider("梯度二值化阈值 T", 10, 200, 2, state.thresh, v => {
      state.thresh = v; render();
    }));
    kBox.hidden = false;
  } else if (state.demo === "log") {
    box.appendChild(makeSlider("LoG / DoG 尺度 σ", 0.6, 3.5, 0.1, state.sigma, v => {
      state.sigma = v; render();
    }));
    box.appendChild(makeSlider("DoG 尺度比 k", 1.1, 2.5, 0.05, state.kRatio, v => {
      state.kRatio = v; render();
    }));
    box.appendChild(makeSlider("过零点斜率阈值", 0, 40, 1, state.zcSlope, v => {
      state.zcSlope = v; render();
    }));
  } else if (state.demo === "canny") {
    box.appendChild(makeSlider("高斯平滑尺度 σ", 0.5, 3.5, 0.1, state.cannySigma, v => {
      state.cannySigma = v; render();
    }));
    box.appendChild(makeSlider("低阈值 T_low", 5, 120, 2, state.tLow, v => {
      state.tLow = Math.min(v, state.tHigh); render();
    }));
    box.appendChild(makeSlider("高阈值 T_high", 15, 200, 2, state.tHigh, v => {
      state.tHigh = Math.max(v, state.tLow); render();
    }));
  }
}

async function render() {
  const { w, h, gray } = await loadImage(state.image);
  const noisy = addGaussianNoise(gray, state.noiseSigma);
  const stats = document.getElementById("stats");
  const kBox = document.getElementById("kernelBox");

  if (state.demo === "first") {
    fig4.hidden = true;
    fig5.hidden = true;
    const op = FIRST_ORDER_OPS[state.op];
    kBox.textContent = `${op.name} 卷积核:\n${op.text}`;
    const { gx, gy, mag } = firstOrderDetect(noisy, w, h, state.op);
    const bin = new Uint8Array(w * h);
    let edgeCount = 0;
    for (let i = 0; i < w * h; i++) {
      if (mag[i] >= state.thresh) {
        bin[i] = 255;
        edgeCount++;
      }
    }
    caps[0].textContent = state.noiseSigma > 0 ? `原图 + 噪声 (σ=${state.noiseSigma})` : "原图";
    caps[1].textContent = `水平/垂直分量 Gx`;
    caps[2].textContent = `梯度模值 |G|`;
    caps[3].textContent = `阈值化边缘 (|G| ≥ ${state.thresh})`;
    drawGray(canvases[0], noisy, w, h);
    drawGray(canvases[1], gx, w, h, "signed");
    drawGray(canvases[2], mag, w, h, "absmax");
    drawGray(canvases[3], bin, w, h);
    stats.textContent = `算子: ${op.name}\n边缘像素数: ${edgeCount} / ${w * h} (${((edgeCount / (w * h)) * 100).toFixed(2)}%)`;
  } else if (state.demo === "log") {
    fig4.hidden = false;
    fig5.hidden = false;
    const rawL = laplacianRaw(noisy, w, h);
    const rawZC = zeroCrossings(rawL, w, h, state.zcSlope);
    const logR = logResponse(noisy, w, h, state.sigma);
    const logZC = zeroCrossings(logR, w, h, state.zcSlope);
    const dogR = dogResponse(noisy, w, h, state.sigma, state.kRatio);
    const dogZC = zeroCrossings(dogR, w, h, state.zcSlope * 0.25);

    const row = Math.floor(h / 2);
    const rowGray = noisy.slice(row * w, (row + 1) * w);
    const rowLog = logR.slice(row * w, (row + 1) * w);
    const rowDog = dogR.slice(row * w, (row + 1) * w);

    caps[0].textContent = "原始拉普拉斯过零点 (无平滑)";
    caps[1].textContent = `LoG 响应 (σ=${state.sigma})`;
    caps[2].textContent = `LoG 过零点边缘 (斜率≥${state.zcSlope})`;
    caps[3].textContent = `DoG 近似过零点 (k=${state.kRatio})`;
    caps[4].textContent = `第 ${row} 行 1D 剖面对比`;
    caps[5].textContent = state.noiseSigma > 0 ? `输入图 (σ_noise=${state.noiseSigma})` : "输入原图";

    drawGray(canvases[0], rawZC, w, h);
    drawGray(canvases[1], logR, w, h, "signed");
    drawGray(canvases[2], logZC, w, h);
    drawGray(canvases[3], dogZC, w, h);
    drawProfile(canvases[4], rowGray, rowLog, rowDog, w);
    drawGray(canvases[5], noisy, w, h);

    const cnt = a => a.reduce((s, v) => s + (v ? 1 : 0), 0);
    stats.textContent = `原始 Laplacian 过零点: ${cnt(rawZC)} px\nLoG (σ=${state.sigma}) 过零点: ${cnt(logZC)} px\nDoG (k=${state.kRatio}) 过零点: ${cnt(dogZC)} px`;
  } else if (state.demo === "canny") {
    fig4.hidden = false;
    fig5.hidden = false;
    const res = cannyPipeline(noisy, w, h, state.cannySigma, state.tLow, state.tHigh);
    caps[0].textContent = `1. 高斯平滑 (σ=${state.cannySigma})`;
    caps[1].textContent = "2. Sobel 梯度模值 M";
    caps[2].textContent = "3. 非极大值抑制 (NMS)";
    caps[3].textContent = "4. 双阈值分类 (红=强, 蓝=弱)";
    caps[4].textContent = "5. 滞后连接最终边缘";
    caps[5].textContent = state.noiseSigma > 0 ? `输入图 (σ_noise=${state.noiseSigma})` : "输入原图";

    drawGray(canvases[0], res.smooth, w, h);
    drawGray(canvases[1], res.mag, w, h, "absmax");
    drawGray(canvases[2], res.nms, w, h, "absmax");
    drawHysteresisMap(canvases[3], res.state, w, h);
    drawGray(canvases[4], res.finalEdges, w, h);
    drawGray(canvases[5], noisy, w, h);

    const totalFinal = res.strongCount + res.linkedWeak;
    stats.textContent = `强边缘 (≥${state.tHigh}): ${res.strongCount} px\n弱边缘 (${state.tLow}~${state.tHigh}): ${res.weakCount} px（被连接保留: ${res.linkedWeak} px）\n最终边缘像素: ${totalFinal} px (${((totalFinal / (w * h)) * 100).toFixed(2)}%)`;
  }
}

function selectDemo(id) {
  state.demo = id;
  const d = DEMOS.find(x => x.id === id);
  document.getElementById("demoTitle").textContent = d.title;
  document.getElementById("question").textContent = d.question;
  document.getElementById("formula").textContent = d.formula;
  document.getElementById("note").textContent = d.note;
  for (const btn of document.querySelectorAll("nav.demos button")) {
    btn.classList.toggle("active", btn.dataset.id === id);
  }
  buildControls();
  render();
}

const nav = document.getElementById("demos");
for (const d of DEMOS) {
  const btn = document.createElement("button");
  btn.textContent = d.tab;
  btn.dataset.id = d.id;
  btn.onclick = () => selectDemo(d.id);
  nav.appendChild(btn);
}
selectDemo("first");
