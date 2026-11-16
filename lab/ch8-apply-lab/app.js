/* 第8章四例：频谱泄漏、相位结构、FFT 滤波、小波遥感融合。 */
(function () {
  const A = () => globalThis.Algo;
  const N = 256;
  const PAN_N = 512;
  const MS_N = 256;

  const state = {
    demoId: "leak",
    images: {},
    color: {},
    p: {
      leak: { cycles: 8, mode: "1d" },
      phase: { view: "swap_ab" },
      fft: { kernel: "gauss", show: "result" },
      wavelet: { scene: "landsat" },
    },
  };

  const DEMOS = [
    {
      id: "leak",
      name: "1 频谱泄漏",
      question:
        "同一频率的余弦信号，如果在观察窗口内截取整数个周期，或者截取非整数个周期，离散傅里叶变换（DFT）的幅度有什么不同？",
      formula:
        "矩形窗在时域截取有限长信号，相当于在频域将原信号频谱与狄利克雷核（周期 sinc 函数）做卷积。截取整数个周期时，离散傅里叶变换的采样点恰好对准主瓣中心与两侧过零点，能量只集中在一对谱线上；截取非整数个周期时，采样点落到旁瓣上，能量会分散到邻近频率。",
      note:
        "一维视图中，蓝点表示离散序列 x[n]（其中 n 为样本序号），浅色细线表示连续余弦包络；右侧红线表示幅度谱 |X[k]|（其中 k 为频率序号，从 −N/2 到 N/2−1，N 为样本总数），金色曲线为狄利克雷核包络。因为实序列的频谱关于零频对称，正频率与负频率两侧各有一组谱线。可以在「显示方式」里切换为二维图像。",
      fields: [
        {
          key: "mode",
          label: "显示",
          type: "select",
          options: [
            ["1d", "一维离散序列与幅度谱"],
            ["2d", "二维条纹图像与二维频谱"],
          ],
        },
        {
          key: "cycles",
          label: "窗口内的周期数",
          type: "range",
          min: 4,
          max: 20,
          step: 0.5,
        },
        {
          key: "preset",
          label: "常用示例",
          type: "presets",
          options: [
            ["整数 8 个周期", { cycles: 8 }],
            ["8.5 个周期（有泄漏）", { cycles: 8.5 }],
            ["整数 16 个周期", { cycles: 16 }],
            ["16.5 周", { cycles: 16.5 }],
          ],
        },
      ],
      run() {
        const cycles = Number(state.p.leak.cycles);
        const intCycles = Math.round(cycles);
        const isInt = Math.abs(cycles - intCycles) < 1e-9;
        if (state.p.leak.mode === "1d") {
          const n = 128;
          const x = A().makeCos1d(n, cycles);
          const mag = fftshift1d(A().dftMag1d(x));
          const xInt = A().makeCos1d(n, intCycles);
          const magInt = fftshift1d(A().dftMag1d(xInt));
          const ratio = A().sideLobeRatio(A().dftMag1d(x), Math.max(1, Math.round(cycles)));
          return {
            stats:
              "当前窗口内包含 " +
              cycles +
              " 个周期" +
              (isInt ? "（属于整数个周期，没有泄漏）。" : "（属于非整数个周期，发生频谱泄漏）。") +
              "旁瓣能量占主瓣能量的比值为 " +
              ratio.toExponential(2) +
              "。实序列的幅度谱关于零频 k = 0 左右对称。",
            panels: [
              {
                kind: "curve",
                title: "当前一维离散序列（窗口内包含 " + cycles + " 个周期，细线为连续包络）",
                y: x,
                cycles: cycles,
                envelope: true,
                ymin: -1.25,
                ymax: 1.25,
                xlabel: "样本序号 n",
                ylabel: "x[n]",
                color: "#1b4e79",
              },
              {
                kind: "curve",
                title: "当前离散傅里叶变换的幅度谱与狄利克雷核包络（正、负频率对称）",
                y: mag,
                ymin: 0,
                stem: true,
                sincCycles: cycles,
                k0: -(n >> 1),
                xlabel: "频率序号 k（从 −64 到 63）",
                ylabel: "|X[k]|",
                color: "#b42024",
              },
              {
                kind: "curve",
                title: "对照：窗口内恰好包含整数 " + intCycles + " 个周期的离散序列",
                y: xInt,
                cycles: intCycles,
                envelope: true,
                ymin: -1.25,
                ymax: 1.25,
                xlabel: "样本序号 n",
                ylabel: "x[n]",
                color: "#1b4e79",
              },
              {
                kind: "curve",
                title: "对照：整数周期时，采样点只落在正、负 ±" + intCycles + " 处的主瓣峰值和其余过零点上",
                y: magInt,
                ymin: 0,
                stem: true,
                sincCycles: intCycles,
                k0: -(n >> 1),
                xlabel: "频率序号 k（从 −64 到 63）",
                ylabel: "|X[k]|",
                color: "#b42024",
              },
            ],
          };
        }
        const n2 = 512;
        const img = A().makeCos2d(n2, cycles);
        const spec = A().centeredDft(img, n2);
        const imgInt = A().makeCos2d(n2, intCycles);
        const specInt = A().centeredDft(imgInt, n2);
        return {
          stats:
            "当前图像是 512×512 的竖直条纹，沿水平方向周期变化，窗口内包含 " +
            cycles +
            " 个周期。截取非整数个周期时，频谱中的亮点不再集中于单个像素，而是沿水平频率方向扩散。",
          panels: [
            { kind: "gray", title: "当前二维竖直条纹图像（512×512）", data: img, n: n2, map: "clip" },
            {
              kind: "gray",
              title: "当前图像的中心化对数幅度谱（512×512）",
              data: A().logMagnitude(spec.re, spec.im),
              n: n2,
              map: "stretch",
            },
            { kind: "gray", title: "对照：整数个周期的二维竖直条纹（512×512）", data: imgInt, n: n2, map: "clip" },
            {
              kind: "gray",
              title: "对照：整数周期图像的中心化对数幅度谱（512×512）",
              data: A().logMagnitude(specInt.re, specInt.im),
              n: n2,
              map: "stretch",
            },
          ],
        };
      },
    },
    {
      id: "phase",
      name: "2 相位与图像结构",
      question: "分别计算 Lena 与摄影师（Cameraman）两幅图像的离散傅里叶变换，交换它们的幅度谱与相位谱后再做逆变换，重建结果的轮廓跟随哪一幅图？",
      formula: "每个频率点上的复数频谱可以写成幅度乘以单位相位因子的形式。本实验将一幅图像的幅度与另一幅图像的相位组合起来，再做离散傅里叶逆变换（IDFT）。",
      note: "幅度表示各个频率分量的强弱，相位表示这些分量在空间中的对齐位置。因此，图像的边缘与轮廓等结构信息主要由相位决定。只保留幅度重建时几乎看不出原有物体；只保留相位重建时仍然可以辨认出人物轮廓。",
      fields: [
        {
          key: "view",
          label: "观察内容",
          type: "select",
          options: [
            ["swap_ab", "Lena 的幅度 + 摄影师的相位"],
            ["swap_ba", "摄影师的幅度 + Lena 的相位"],
            ["mag_only", "只保留 Lena 的幅度（相位置零）"],
            ["phase_only", "只保留 Lena 的相位（幅度置为 1）"],
            ["spectra", "两幅原图与各自的相位图"],
          ],
        },
      ],
      run() {
        const lena = state.images.lena;
        const cam = state.images.cameraman;
        const sL = A().centeredDft(lena, N);
        const sC = A().centeredDft(cam, N);
        const view = state.p.phase.view;
        if (view === "swap_ab") {
          const out = A().mixMagPhase(sL, sC, N);
          return {
            stats: "使用 Lena 的幅度与摄影师的相位进行逆变换后，画面轮廓与摄影师原图一致。",
            panels: [
              { kind: "gray", title: "原图：Lena", data: lena, n: N, map: "clip" },
              { kind: "gray", title: "原图：摄影师（Cameraman）", data: cam, n: N, map: "clip" },
              {
                kind: "gray",
                title: "用 Lena 的幅度与摄影师的相位重建的结果",
                data: out,
                n: N,
                map: "stretch",
              },
              {
                kind: "gray",
                title: "摄影师原图的相位谱",
                data: A().phaseImage(sC.re, sC.im),
                n: N,
                map: "unit",
              },
            ],
          };
        }
        if (view === "swap_ba") {
          const out = A().mixMagPhase(sC, sL, N);
          return {
            stats: "使用摄影师的幅度与 Lena 的相位进行逆变换后，画面轮廓与 Lena 原图一致。",
            panels: [
              { kind: "gray", title: "原图：Lena", data: lena, n: N, map: "clip" },
              { kind: "gray", title: "原图：摄影师（Cameraman）", data: cam, n: N, map: "clip" },
              {
                kind: "gray",
                title: "用摄影师的幅度与 Lena 的相位重建的结果",
                data: out,
                n: N,
                map: "stretch",
              },
              {
                kind: "gray",
                title: "Lena 原图的相位谱",
                data: A().phaseImage(sL.re, sL.im),
                n: N,
                map: "unit",
              },
            ],
          };
        }
        if (view === "mag_only") {
          const out = A().synthesize(sL, N, "mag");
          return {
            stats: "只保留幅度谱、把相位全部置零后，重建结果只剩能量分布，原有轮廓几乎完全消失。",
            panels: [
              { kind: "gray", title: "原图：Lena", data: lena, n: N, map: "clip" },
              {
                kind: "gray",
                title: "Lena 的对数幅度谱",
                data: A().logMagnitude(sL.re, sL.im),
                n: N,
                map: "stretch",
              },
              { kind: "gray", title: "只保留幅度重建的结果", data: out, n: N, map: "stretch" },
              {
                kind: "gray",
                title: "对照：只保留相位重建的结果",
                data: A().synthesize(sL, N, "phase"),
                n: N,
                map: "stretch",
              },
            ],
          };
        }
        if (view === "phase_only") {
          const out = A().synthesize(sL, N, "phase");
          return {
            stats: "只保留相位谱、把所有频率的幅度都设为 1 后，重建图像依然保持了原图的主要轮廓。",
            panels: [
              { kind: "gray", title: "原图：Lena", data: lena, n: N, map: "clip" },
              {
                kind: "gray",
                title: "Lena 的相位谱",
                data: A().phaseImage(sL.re, sL.im),
                n: N,
                map: "unit",
              },
              { kind: "gray", title: "只保留相位重建的结果", data: out, n: N, map: "stretch" },
              {
                kind: "gray",
                title: "对照：只保留幅度重建的结果",
                data: A().synthesize(sL, N, "mag"),
                n: N,
                map: "stretch",
              },
            ],
          };
        }
        return {
          stats: "左侧为两幅原始图像，右侧为它们各自的相位谱。",
          panels: [
            { kind: "gray", title: "原图：Lena", data: lena, n: N, map: "clip" },
            { kind: "gray", title: "原图：摄影师（Cameraman）", data: cam, n: N, map: "clip" },
            {
              kind: "gray",
              title: "Lena 的相位谱",
              data: A().phaseImage(sL.re, sL.im),
              n: N,
              map: "unit",
            },
            {
              kind: "gray",
              title: "摄影师的相位谱",
              data: A().phaseImage(sC.re, sC.im),
              n: N,
              map: "unit",
            },
          ],
        };
      },
    },
    {
      id: "fft",
      name: "3 快速傅里叶变换图像滤波",
      question: "将图像和滤波器都补零到同一尺寸，再依次做快速傅里叶变换（FFT）、频域逐点相乘、快速傅里叶逆变换（IFFT）并裁剪回原图大小，结果是否与空域滤波一致？",
      formula:
        "先将图像和卷积核补零到不小于“原图边长加上卷积核边长减一”的最小 2 的整数次幂尺寸，再对两者分别做快速傅里叶变换，把得到的两个复数频谱逐点相乘，最后做快速傅里叶逆变换并裁取左上角与原图相同的区域。",
      note: "高斯卷积核用于平滑图像，水平梯度卷积核用于提取竖直方向的边缘。为了和空域以核中心对齐的滤波结果一致，卷积核在补零时需要将中心对齐到坐标原点并做循环卷绕。在「面板内容」中可以切换观察每一步的中间图像。",
      fields: [
        {
          key: "kernel",
          label: "滤波器（卷积核）",
          type: "select",
          options: [
            ["gauss", "9×9 高斯平滑核（标准差为 1.5）"],
            ["grad", "3×3 水平梯度核（索贝尔算子）"],
          ],
        },
        {
          key: "show",
          label: "面板内容",
          type: "select",
          options: [
            ["result", "原图、频域滤波结果、频谱与空域对照"],
            ["pipeline", "补零后的图像、补零后的卷积核、二者频谱逐点相乘后的幅度、滤波结果"],
          ],
        },
      ],
      run() {
        const gray = state.images.lena;
        const ker =
          state.p.fft.kernel === "gauss"
            ? A().gaussianKernel(9, 1.5)
            : A().horizGradKernel();
        const pipe = A().filterByFft(gray, N, ker);
        const spatial = A().convolveSpatial(gray, N, ker);
        const err = A().maxAbsDiff(pipe.out, spatial);
        const mapOut = state.p.fft.kernel === "gauss" ? "clip" : "signed";
        if (state.p.fft.show === "pipeline") {
          return {
            stats:
              "补零后的计算尺寸为 " +
              pipe.N +
              "×" +
              pipe.N +
              " 像素。频域滤波结果与空域直接计算的最大绝对误差为 " +
              err.toExponential(2) +
              "（仅来自浮点舍入）。",
            panels: [
              {
                kind: "gray",
                title: "补零后的图像（原图放在左上角，其余区域补零）",
                data: pipe.padImage,
                n: pipe.N,
                map: "clip",
              },
              {
                kind: "gray",
                title: "补零并将中心卷绕到原点后的卷积核",
                data: pipe.padKernel,
                n: pipe.N,
                map: "signed",
              },
              {
                kind: "gray",
                title: "图像的离散傅里叶变换与卷积核的离散傅里叶变换逐点相乘之后的幅度",
                data: pipe.fftProductMag,
                n: pipe.N,
                map: "stretch",
              },
              { kind: "gray", title: "逆变换并裁剪回原图大小的滤波结果", data: pipe.out, n: N, map: mapOut },
            ],
          };
        }
        const spec = A().centeredDft(gray, N);
        return {
          stats:
            "频域滤波结果与空域直接计算的最大绝对误差为 " +
            err.toExponential(2) +
            "，说明二者在数学上完全等价。",
          panels: [
            { kind: "gray", title: "原始图像：Lena", data: gray, n: N, map: "clip" },
            { kind: "gray", title: "用快速傅里叶变换得到的滤波结果", data: pipe.out, n: N, map: mapOut },
            {
              kind: "gray",
              title: "原始图像的中心化对数幅度谱",
              data: A().logMagnitude(spec.re, spec.im),
              n: N,
              map: "stretch",
            },
            {
              kind: "gray",
              title: "对照：空域直接卷积的滤波结果",
              data: spatial,
              n: N,
              map: mapOut,
            },
          ],
        };
      },
    },
    {
      id: "wavelet",
      name: "4 小波遥感图像融合",
      question:
        "将高分辨率全色图像做一层小波分解得到一个低频子带和三个高频细节子带后，用低分辨率彩色图像替换低频子带，再做二维离散小波逆变换（IDWT），会得到什么结果？",
      formula:
        "先对全色图像做一层二维哈尔（Haar）离散小波变换，得到低频近似分量、竖直细节分量、水平细节分量与对角细节分量；再把低分辨率彩色图像缩放到低频分量大小，逐通道替换低频分量后做小波逆变换。",
      note: "页面准备了两组数据，都可以在上方切换。山地一组是真实的全色图和分辨率减半的彩色图。村镇一组用同一幅彩色遥感图模拟：全色图是它转成的灰度图，分辨率仍为 512×512；彩色图把长和宽都缩小为二分之一，得到 256×256。页面按各自的像素尺寸显示。分解图按四块拼在一起：左上是低频，右上是水平边缘，左下是竖直边缘，右下是对角细节。请对比两张分解图的左上角：替换前是灰色，替换后变成彩色，另外三块细节没有改动。",
      fields: [
        {
          key: "scene",
          label: "数据",
          type: "select",
          options: [
            ["landsat", "山地：真实全色图与半分辨率彩色图"],
            ["town", "村镇：灰度全色图与缩小一半的彩色图"],
          ],
        },
      ],
      run() {
        const scene = state.p.wavelet.scene;
        const town = scene === "town";
        if (!state._fuseCache) state._fuseCache = {};
        if (!state._fuseCache[scene]) {
          const pan = town ? state.images.panTown : state.images.pan;
          const ms = town ? state.color.msTown : state.color.ms;
          state._fuseCache[scene] = A().fuseWavelet(pan, PAN_N, ms.r, ms.g, ms.b, MS_N);
        }
        const f = state._fuseCache[scene];
        const panData = town ? state.images.panTown : state.images.pan;
        const ms = town ? state.color.msTown : state.color.ms;
        return {
          stats: town
            ? "村镇这一组里，全色图是彩色图像转成的灰度图，房屋、道路和树冠的纹理还在；彩色图是长和宽各缩小一半的结果。合成图把这些纹理补回到颜色上。只把彩色图放大时，边缘仍然发糊。"
            : "山地这一组使用真实的全色图和半分辨率彩色图。合成结果既有彩色图像的颜色，也有全色图像里的山脊和道路纹理。只把彩色图放大时，颜色还在，但是边缘发糊。",
          panels: [
            {
              kind: "gray",
              native: true,
              title: town ? "全色图像：由彩色图转成的灰度（512×512）" : "全色图像原图（512×512）",
              data: panData,
              n: PAN_N,
              map: "clip",
            },
            {
              kind: "rgb",
              native: true,
              title: town
                ? "彩色图像：长和宽各缩小为二分之一（256×256）"
                : "彩色图像原图（256×256，保持原始大小）",
              r: ms.r,
              g: ms.g,
              b: ms.b,
              n: MS_N,
              map: "clip",
            },
            {
              kind: "pyramid",
              native: true,
              title: "替换前的小波分解：左上角低频仍是全色图像的灰色近似",
              llGray: f.LL,
              lh: f.LH,
              hl: f.HL,
              hh: f.HH,
              h: f.h,
            },
            {
              kind: "pyramid",
              native: true,
              title: "替换后的小波分解：左上角低频换成彩色图像，三个细节子带仍来自全色图像",
              llR: f.llR,
              llG: f.llG,
              llB: f.llB,
              lh: f.LH,
              hl: f.HL,
              hh: f.HH,
              h: f.h,
            },
            {
              kind: "rgb",
              native: true,
              title: "小波逆变换的合成结果（512×512）：既有纹理，也有颜色",
              r: f.fusedR,
              g: f.fusedG,
              b: f.fusedB,
              n: PAN_N,
              map: "clip",
            },
            {
              kind: "rgb",
              native: true,
              title: "对照：只把彩色图像放大到 512×512，没有加入全色纹理",
              r: f.msUpR,
              g: f.msUpG,
              b: f.msUpB,
              n: PAN_N,
              map: "clip",
            },
          ],
        };
      },
    },
  ];

  function fftshift1d(mag) {
    const n = mag.length;
    const h = n >> 1;
    const out = new Float64Array(n);
    for (let i = 0; i < n; i++) out[i] = mag[(i + h) % n];
    return out;
  }

  function demoById(id) {
    return DEMOS.find((d) => d.id === id);
  }

  function setBanner(msg) {
    const el = document.getElementById("banner");
    if (!msg) {
      el.hidden = true;
      return;
    }
    el.hidden = false;
    el.textContent = msg;
  }

  function buildNav() {
    const nav = document.getElementById("demos");
    nav.innerHTML = "";
    DEMOS.forEach((d) => {
      const btn = document.createElement("button");
      btn.textContent = d.name;
      btn.className = d.id === state.demoId ? "active" : "";
      btn.onclick = () => {
        state.demoId = d.id;
        buildNav();
        buildControls();
        render();
      };
      nav.appendChild(btn);
    });
  }

  function buildControls() {
    const demo = demoById(state.demoId);
    const box = document.getElementById("controls");
    box.innerHTML = "";
    document.getElementById("demoTitle").textContent = demo.name;
    document.getElementById("question").textContent = demo.question;
    document.getElementById("formula").innerHTML = demo.formula;
    document.getElementById("note").textContent = demo.note;

    (demo.fields || []).forEach((f) => {
      const wrap = document.createElement("div");
      wrap.className = "ctrl";
      if (f.type === "select") {
        const lab = document.createElement("label");
        lab.textContent = f.label;
        const sel = document.createElement("select");
        f.options.forEach(([v, t]) => {
          const o = document.createElement("option");
          o.value = v;
          o.textContent = t;
          if (String(state.p[demo.id][f.key]) === String(v)) o.selected = true;
          sel.appendChild(o);
        });
        sel.onchange = () => {
          state.p[demo.id][f.key] = sel.value;
          render();
        };
        lab.appendChild(sel);
        wrap.appendChild(lab);
      } else if (f.type === "range") {
        const row = document.createElement("div");
        row.className = "row";
        const lab = document.createElement("span");
        lab.textContent = f.label;
        const out = document.createElement("output");
        out.textContent = state.p[demo.id][f.key];
        row.appendChild(lab);
        row.appendChild(out);
        const input = document.createElement("input");
        input.type = "range";
        input.min = f.min;
        input.max = f.max;
        input.step = f.step;
        input.value = state.p[demo.id][f.key];
        input.oninput = () => {
          state.p[demo.id][f.key] = Number(input.value);
          out.textContent = input.value;
          render();
        };
        wrap.appendChild(row);
        wrap.appendChild(input);
      } else if (f.type === "presets") {
        const lab = document.createElement("div");
        lab.className = "row";
        lab.innerHTML = "<span>" + f.label + "</span>";
        wrap.appendChild(lab);
        const presets = document.createElement("div");
        presets.className = "presets";
        f.options.forEach(([name, patch]) => {
          const b = document.createElement("button");
          b.type = "button";
          b.textContent = name;
          b.onclick = () => {
            Object.assign(state.p[demo.id], patch);
            buildControls();
            render();
          };
          presets.appendChild(b);
        });
        wrap.appendChild(presets);
      }
      box.appendChild(wrap);
    });
  }

  function mapToRgba(data, n, map) {
    const rgba = new Uint8ClampedArray(n * n * 4);
    let mn = Infinity;
    let mx = -Infinity;
    if (map === "stretch" || map === "signed") {
      for (let i = 0; i < data.length; i++) {
        mn = Math.min(mn, data[i]);
        mx = Math.max(mx, data[i]);
      }
    }
    const absMax = Math.max(Math.abs(mn), Math.abs(mx), 1e-12);
    for (let i = 0; i < n * n; i++) {
      let v = data[i];
      let g = 0;
      if (map === "clip") g = Math.max(0, Math.min(255, v));
      else if (map === "unit") g = Math.max(0, Math.min(255, v * 255));
      else if (map === "stretch") {
        g = mx - mn < 1e-12 ? 0 : ((v - mn) * 255) / (mx - mn);
      } else if (map === "signed") {
        g = (v / absMax) * 127 + 128;
      }
      const o = i * 4;
      rgba[o] = rgba[o + 1] = rgba[o + 2] = g;
      rgba[o + 3] = 255;
    }
    return rgba;
  }

  function rgbToRgba(r, g, b, n, map) {
    const rgba = new Uint8ClampedArray(n * n * 4);
    for (let i = 0; i < n * n; i++) {
      let rr = r[i];
      let gg = g[i];
      let bb = b[i];
      if (map === "clip") {
        rr = Math.max(0, Math.min(255, rr));
        gg = Math.max(0, Math.min(255, gg));
        bb = Math.max(0, Math.min(255, bb));
      }
      const o = i * 4;
      rgba[o] = rr;
      rgba[o + 1] = gg;
      rgba[o + 2] = bb;
      rgba[o + 3] = 255;
    }
    return rgba;
  }

  function blitSquare(canvas, rgba, n, box) {
    const side = box && box !== n ? box : n;
    canvas.width = side;
    canvas.height = side;
    const ctx = canvas.getContext("2d");
    if (side === n) {
      ctx.putImageData(new ImageData(rgba, n, n), 0, 0);
      return;
    }
    const src = document.createElement("canvas");
    src.width = n;
    src.height = n;
    src.getContext("2d").putImageData(new ImageData(rgba, n, n), 0, 0);
    ctx.imageSmoothingEnabled = true;
    ctx.imageSmoothingQuality = "high";
    ctx.drawImage(src, 0, 0, side, side);
  }

  function drawGray(canvas, data, n, map, box) {
    canvas.classList.toggle("light", false);
    blitSquare(canvas, mapToRgba(data, n, map), n, box);
  }

  function drawRgb(canvas, r, g, b, n, map, box) {
    blitSquare(canvas, rgbToRgba(r, g, b, n, map), n, box);
  }

  function drawCurve(canvas, panel) {
    const y = panel.y;
    const n = y.length;
    const W = 720;
    const H = 280;
    canvas.width = W;
    canvas.height = H;
    canvas.classList.add("plot");
    const ctx = canvas.getContext("2d");
    ctx.fillStyle = "#ffffff";
    ctx.fillRect(0, 0, W, H);

    const padL = 52;
    const padR = 16;
    const padT = 14;
    const padB = 36;
    const plotW = W - padL - padR;
    const plotH = H - padT - padB;

    let ymin = panel.ymin;
    let ymax = panel.ymax;
    if (ymin == null || ymax == null) {
      ymin = Infinity;
      ymax = -Infinity;
      for (let i = 0; i < n; i++) {
        ymin = Math.min(ymin, y[i]);
        ymax = Math.max(ymax, y[i]);
      }
      if (panel.ymin != null) ymin = panel.ymin;
      const span = ymax - ymin || 1;
      ymax += span * 0.08;
      if (panel.ymin == null) ymin -= span * 0.08;
    }
    if (ymax <= ymin) ymax = ymin + 1;

    const xOf = (i) => padL + (n === 1 ? plotW / 2 : (i / (n - 1)) * plotW);
    const yOf = (v) => padT + ((ymax - v) / (ymax - ymin)) * plotH;

    ctx.strokeStyle = "#d5dee8";
    ctx.lineWidth = 1;
    ctx.beginPath();
    const y0 = yOf(0);
    if (y0 >= padT && y0 <= padT + plotH) {
      ctx.moveTo(padL, y0);
      ctx.lineTo(padL + plotW, y0);
      ctx.stroke();
    }
    ctx.strokeStyle = "#1c2833";
    ctx.strokeRect(padL, padT, plotW, plotH);

    ctx.fillStyle = "#5c6b7a";
    ctx.font = "12px Microsoft YaHei, sans-serif";
    ctx.textAlign = "center";
    ctx.fillText(panel.xlabel || "n", padL + plotW / 2, H - 8);
    ctx.save();
    ctx.translate(14, padT + plotH / 2);
    ctx.rotate(-Math.PI / 2);
    ctx.fillText(panel.ylabel || "y", 0, 0);
    ctx.restore();
    ctx.textAlign = "right";
    ctx.fillText(ymax.toFixed(ymax >= 10 ? 0 : 2), padL - 6, padT + 10);
    ctx.fillText(ymin.toFixed(ymin >= 10 ? 0 : 2), padL - 6, padT + plotH);
    if (panel.k0 != null) {
      ctx.textAlign = "center";
      ctx.fillText(String(panel.k0), padL, padT + plotH + 14);
      ctx.fillText("0", xOf(n >> 1), padT + plotH + 14);
      ctx.fillText(String(panel.k0 + n - 1), padL + plotW, padT + plotH + 14);
    }

    ctx.strokeStyle = panel.color || "#1b4e79";
    ctx.fillStyle = panel.color || "#1b4e79";
    ctx.lineWidth = 1.6;
    if (panel.envelope) {
      const dense = Math.max(n * 8, 2);
      ctx.strokeStyle = "#9bb0c7";
      ctx.lineWidth = 1.2;
      ctx.beginPath();
      for (let i = 0; i < dense; i++) {
        const t = ((n - 1) * i) / (dense - 1);
        const v = Math.cos((2 * Math.PI * panel.cycles * t) / n);
        const xx = padL + (t / (n - 1)) * plotW;
        const yy = yOf(v);
        if (i === 0) ctx.moveTo(xx, yy);
        else ctx.lineTo(xx, yy);
      }
      ctx.stroke();
      ctx.strokeStyle = panel.color || "#1b4e79";
      ctx.fillStyle = panel.color || "#1b4e79";
      const base = yOf(0);
      for (let i = 0; i < n; i++) {
        const xx = xOf(i);
        const yy = yOf(y[i]);
        ctx.beginPath();
        ctx.moveTo(xx, base);
        ctx.lineTo(xx, yy);
        ctx.stroke();
        ctx.beginPath();
        ctx.arc(xx, yy, 2.2, 0, Math.PI * 2);
        ctx.fill();
      }
    } else if (panel.stem) {
      if (panel.sincCycles != null && panel.k0 != null) {
        const dense = Math.max(n * 8, 2);
        const kLeft = panel.k0;
        const kRight = panel.k0 + n - 1;
        ctx.strokeStyle = "#c48a2a";
        ctx.lineWidth = 1.2;
        ctx.beginPath();
        for (let i = 0; i < dense; i++) {
          const t = i / (dense - 1);
          const k = kLeft + t * (kRight - kLeft);
          const v = A().cosineSpectrumEnvelope(k, panel.sincCycles, n);
          const xx = padL + t * plotW;
          const yy = yOf(v);
          if (i === 0) ctx.moveTo(xx, yy);
          else ctx.lineTo(xx, yy);
        }
        ctx.stroke();
        ctx.strokeStyle = panel.color || "#b42024";
        ctx.fillStyle = panel.color || "#b42024";
      }
      const base = yOf(Math.max(ymin, 0));
      for (let i = 0; i < n; i++) {
        const x = xOf(i);
        const yy = yOf(y[i]);
        ctx.beginPath();
        ctx.moveTo(x, base);
        ctx.lineTo(x, yy);
        ctx.stroke();
        ctx.beginPath();
        ctx.arc(x, yy, 2.2, 0, Math.PI * 2);
        ctx.fill();
      }
    } else {
      ctx.beginPath();
      for (let i = 0; i < n; i++) {
        const x = xOf(i);
        const yy = yOf(y[i]);
        if (i === 0) ctx.moveTo(x, yy);
        else ctx.lineTo(x, yy);
      }
      ctx.stroke();
    }
  }

  function grayToU8(data, map) {
    const n = data.length;
    const out = new Uint8ClampedArray(n);
    let mn = Infinity;
    let mx = -Infinity;
    if (map === "stretch" || map === "signed") {
      for (let i = 0; i < n; i++) {
        mn = Math.min(mn, data[i]);
        mx = Math.max(mx, data[i]);
      }
    }
    const absMax = Math.max(Math.abs(mn), Math.abs(mx), 1e-12);
    for (let i = 0; i < n; i++) {
      const v = data[i];
      let g = 0;
      if (map === "clip") g = Math.max(0, Math.min(255, v));
      else if (map === "stretch") g = mx - mn < 1e-12 ? 0 : ((v - mn) * 255) / (mx - mn);
      else if (map === "signed") g = (v / absMax) * 127 + 128;
      out[i] = g;
    }
    return out;
  }

  function drawPyramid(canvas, panel) {
    const h = panel.h;
    const n = h * 2;
    canvas.width = n;
    canvas.height = n;
    const ctx = canvas.getContext("2d");
    const img = ctx.createImageData(n, n);
    const lh = grayToU8(panel.lh, "signed");
    const hl = grayToU8(panel.hl, "signed");
    const hh = grayToU8(panel.hh, "signed");
    const llGray = panel.llGray ? grayToU8(panel.llGray, "stretch") : null;
    for (let y = 0; y < n; y++) {
      for (let x = 0; x < n; x++) {
        const o = (y * n + x) * 4;
        let r = 0;
        let g = 0;
        let b = 0;
        if (y < h && x < h) {
          const i = y * h + x;
          if (llGray) r = g = b = llGray[i];
          else {
            r = Math.max(0, Math.min(255, panel.llR[i]));
            g = Math.max(0, Math.min(255, panel.llG[i]));
            b = Math.max(0, Math.min(255, panel.llB[i]));
          }
        } else if (y < h) {
          r = g = b = lh[(y) * h + (x - h)];
        } else if (x < h) {
          r = g = b = hl[(y - h) * h + x];
        } else {
          r = g = b = hh[(y - h) * h + (x - h)];
        }
        img.data[o] = r;
        img.data[o + 1] = g;
        img.data[o + 2] = b;
        img.data[o + 3] = 255;
      }
    }
    ctx.putImageData(img, 0, 0);
  }

  function drawPanel(index, panel) {
    const canvas = document.getElementById("c" + index);
    const cap = document.getElementById("cap" + index);
    const fig = canvas.parentElement;
    if (!panel) {
      fig.hidden = true;
      cap.textContent = "";
      return;
    }
    fig.hidden = false;
    cap.textContent = panel.title || "";
    canvas.classList.remove("plot");
    canvas.classList.toggle("native", !!panel.native);
    const box =
      (state.demoId === "leak" || state.demoId === "phase" || state.demoId === "fft") &&
      (panel.kind === "gray" || panel.kind === "rgb")
        ? 512
        : 0;
    canvas.dataset.box = box ? String(box) : "";
    if (panel.kind === "gray") drawGray(canvas, panel.data, panel.n, panel.map, box);
    else if (panel.kind === "rgb")
      drawRgb(canvas, panel.r, panel.g, panel.b, panel.n, panel.map, box);
    else if (panel.kind === "curve") drawCurve(canvas, panel);
    else if (panel.kind === "pyramid") drawPyramid(canvas, panel);
    fitCanvas(canvas);
  }

  function fitCanvas(canvas) {
    if (canvas.classList.contains("native") || canvas.parentElement.hidden) {
      canvas.style.width = "";
      canvas.style.height = "";
      return;
    }
    const fig = canvas.parentElement;
    const avail = Math.max(160, fig.clientWidth - 20);
    const box = Number(canvas.dataset.box || 0);
    const aspect = canvas.height / Math.max(1, canvas.width);
    const w = box > 0 ? Math.min(avail, box) : avail;
    canvas.style.width = w + "px";
    canvas.style.height = Math.round(w * aspect) + "px";
  }

  function fitVisibleCanvases() {
    for (let i = 0; i < 6; i++) fitCanvas(document.getElementById("c" + i));
  }

  function render() {
    const demo = demoById(state.demoId);
    try {
      const view = demo.run();
      document.getElementById("stats").textContent = view.stats || "";
      for (let i = 0; i < 6; i++) drawPanel(i, view.panels[i] || null);
      requestAnimationFrame(fitVisibleCanvases);
      setBanner("");
    } catch (err) {
      console.error(err);
      setBanner("计算失败：" + err.message);
    }
  }

  function loadGray(url, side) {
    return new Promise((resolve, reject) => {
      const img = new Image();
      img.onload = () => {
        const c = document.createElement("canvas");
        c.width = side;
        c.height = side;
        const ctx = c.getContext("2d");
        ctx.drawImage(img, 0, 0, side, side);
        const data = ctx.getImageData(0, 0, side, side).data;
        const gray = new Float64Array(side * side);
        for (let i = 0; i < side * side; i++) {
          const o = i * 4;
          gray[i] = 0.299 * data[o] + 0.587 * data[o + 1] + 0.114 * data[o + 2];
        }
        resolve(gray);
      };
      img.onerror = () => reject(new Error("无法加载 " + url));
      img.src = url;
    });
  }

  function loadRgb(url, side) {
    return new Promise((resolve, reject) => {
      const img = new Image();
      img.onload = () => {
        const c = document.createElement("canvas");
        c.width = side;
        c.height = side;
        const ctx = c.getContext("2d");
        ctx.drawImage(img, 0, 0, side, side);
        const data = ctx.getImageData(0, 0, side, side).data;
        const r = new Float64Array(side * side);
        const g = new Float64Array(side * side);
        const b = new Float64Array(side * side);
        for (let i = 0; i < side * side; i++) {
          const o = i * 4;
          r[i] = data[o];
          g[i] = data[o + 1];
          b[i] = data[o + 2];
        }
        resolve({ r, g, b });
      };
      img.onerror = () => reject(new Error("无法加载 " + url));
      img.src = url;
    });
  }

  async function boot() {
    setBanner("正在加载样例图像…");
    try {
      state.images.lena = await loadGray("images/lena.png", N);
      state.images.cameraman = await loadGray("images/cameraman.png", N);
      state.images.pan = await loadGray("images/pan.png", PAN_N);
      state.color.ms = await loadRgb("images/ms_color.png", MS_N);
      state.images.panTown = await loadGray("images/pan_town.png", PAN_N);
      state.color.msTown = await loadRgb("images/ms_town.png", MS_N);
      buildNav();
      buildControls();
      render();
      setBanner("");
    } catch (err) {
      setBanner(err.message + "。请用 start.bat 启动本地服务后再打开。");
    }
  }

  window.addEventListener("resize", fitVisibleCanvases);
  boot();
})();
