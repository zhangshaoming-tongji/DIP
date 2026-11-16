/* 第8次课课堂演示。算法在 fft.js / filters.js，这里只负责交互和显示。 */
(function () {
  const N = 256;
  const F = () => globalThis.Filters;
  const FFT = () => globalThis.Freq;

  const GROUPS = [
    { id: "dft", name: "1 理解 DFT" },
    { id: "kern", name: "2 空域核的频率响应" },
    { id: "conv", name: "3 循环卷积" },
    { id: "design", name: "4 频域滤波器" },
    { id: "apply", name: "5 其他应用" },
  ];

  function lead(text, fn) {
    return () => text + "\n" + fn.toString();
  }

  const CODE = {
    fft: () =>
      "// 正变换不除以 n，逆变换除以 n。二维逆变换合计除以 n²。\n\n"
      + FFT().fft1.toString() + "\n\n" + FFT().fft2.toString(),
    centered: lead("// 先乘 (−1)^{x+y}，直流就出现在矩阵中心。", F().centeredDft),
    raw: lead("// 不中心化时，直流在左上角 (0,0)。", F().rawDft),
    modulate: lead("// 中心化就是这个棋盘调制。", F().modulate),
    synthesize: lead("// 替换幅度或相位后再做逆变换。", F().synthesize),
    mix: lead("// 结构主要来自相位，纹理更多来自幅度。", F().mixMagPhase),
    kernel: lead("// 把核的中心放到原点，再做中心化 DFT，得到频率响应。", F().kernelSpectrum),
    spatial: lead("// 边界外当 0。核越大，这个双重循环越慢。", F().convolveSpatial),
    dftConv: lead("// 不补零 = 循环卷积；补到不少于 N+K−1 再取 2 的幂 = 线性卷积。", F().convolveDFT),
    conv1: lead("// 一维数值例子。原点取核的中心样点。", F().conv1Linear),
    circ1: () => F().conv1Circular.toString() + "\n\n" + F().conv1Padded.toString(),
    low: lead("// D 是到频谱中心的距离。阶数 n 只用于巴特沃斯。", F().lowpassH),
    apply: lead("// 频域相乘代替空域卷积，再逆变换回图像。", F().applyH),
    emphasis: lead("// a = 0 是纯高通；a 增大时把低频轮廓加回来。", F().emphasis),
    notch: lead("// 四个陷波点成对出现，保证逆变换仍是实数。", F().notchH),
    band: lead("// 带阻去掉某一圈频率。理想带阻的空域核会有振铃。", F().bandRejectH),
    noise: lead("// 整数频率 f 会在频谱的 (±f,0) 和 (0,±f) 形成亮点。", F().addPeriodic),
    shift: lead("// 循环平移对应 DFT 的平移性质，幅度应严格不变。", F().circShift),
    zeroShift: lead("// 边缘补 0 的平移会改变图像内容，幅度只是近似不变。", F().zeroShift),
    rotate: lead("// 绕中心逆时针旋转，出界填 0。", F().rotateImage),
    dct: lead("// 8×8 JPEG 型 DCT。K = 64 时应还原原图。", F().compressDCT),
    dct1: () => F().dct8.toString() + "\n\n" + F().idct8.toString(),
  };

  const DEMOS = [
    {
      id: "mag",
      group: "dft",
      name: "幅度与相位",
      question: "对图像做 DFT，分别只用幅度、只用相位重建。哪一个还能看出原来的结构？",
      formula: "F(u,v) = |F| e<sup>jφ</sup>。仅幅度：φ = 0。仅相位：|F| = 1。",
      note: "幅度接近 0 的位置，相位没有意义，所以相位图看起来很乱。仅幅度、仅相位的结果做了线性拉伸。",
      defaults: { mode: "full" },
      fields: [
        {
          key: "mode", label: "重建方式", type: "select",
          options: [
            ["full", "幅度 + 相位（应还原原图）"],
            ["mag", "仅幅度，相位 = 0"],
            ["phase", "仅相位，幅度 = 1"],
            ["random", "真实幅度 + 随机相位"],
          ],
        },
      ],
      codes: [["用幅度相位重建", "synthesize"], ["中心化 DFT", "centered"], ["二维 FFT", "fft"]],
      run(gray) {
        const spec = spectrum(state.source);
        const out = F().synthesize(spec, N, state.p.mag.mode);
        const err = F().maxAbsDiff(out, gray);
        const mode = state.p.mag.mode;
        return {
          stats: mode === "full"
            ? "重建最大误差 " + err.toExponential(2) + "。误差来自浮点，不是公式。"
            : "结果已线性拉伸到 0–255，便于和原图对比结构。",
          panels: [
            img("原图", gray, "clip"),
            specPanel("幅度谱 log(1+|F|)，中心为直流", spec),
            img("相位（−π 黑，+π 白）", F().phaseImage(spec.re, spec.im), "unit"),
            img(mode === "full" ? "重建" : "重建（已拉伸）", out, mode === "full" ? "clip" : "stretch"),
          ],
        };
      },
    },
    {
      id: "swap",
      group: "dft",
      name: "幅度与相位交换",
      question: "取两幅图，交换幅度和相位后重建。轮廓跟着相位走，还是跟着幅度走？",
      formula: "图 3 = |A| ∠B，图 4 = |B| ∠A。",
      note: "建议 A、B 用 Lena 和 Cameraman。交换结果做了线性拉伸。",
      defaults: { other: "camera" },
      fields: [
        {
          key: "other", label: "图 B", type: "select",
          options: [
            ["camera", "Cameraman"],
            ["lena", "Lena"],
            ["stripes", "文档条纹"],
            ["rect", "横向方块"],
            ["zone", "波带片"],
          ],
        },
      ],
      codes: [["交换幅度和相位", "mix"], ["中心化 DFT", "centered"], ["二维 FFT", "fft"]],
      run() {
        const aName = state.source;
        const bName = state.p.swap.other;
        const specA = spectrum(aName);
        const specB = spectrum(bName);
        const ab = F().mixMagPhase(specA, specB, N);
        const ba = F().mixMagPhase(specB, specA, N);
        return {
          stats: "A = " + sourceName(aName) + "，B = " + sourceName(bName) + "。",
          panels: [
            img("A", state.images[aName], "clip"),
            img("B", state.images[bName], "clip"),
            img("|A| + 相位 B", ab, "stretch"),
            img("|B| + 相位 A", ba, "stretch"),
          ],
        };
      },
    },
    {
      id: "center",
      group: "dft",
      name: "频谱中心化",
      question: "为什么课件里的频谱直流在中心，而按定义写出来的 DFT 直流在左上角？",
      formula: "f(x,y) (−1)<sup>x+y</sup> 的 DFT，等于把 F(u,v) 平移 (N/2, N/2)。",
      note: "右下角是调制后的空域图像，不是频谱。它本身像一张棋盘，能量集中在高频。",
      defaults: {},
      fields: [],
      codes: [["棋盘调制", "modulate"], ["未中心化 DFT", "raw"], ["中心化 DFT", "centered"]],
      run(gray) {
        const raw = F().rawDft(gray, N);
        const cen = spectrum(state.source);
        return {
          stats: "两种幅度谱的数值相同，只是原点位置不同。",
          panels: [
            img("原图", gray, "clip"),
            specPanel("未中心化幅度，直流在左上角", raw, "origin"),
            specPanel("中心化幅度，直流在中心", cen, "center"),
            img("乘 (−1)^{x+y} 之后", F().checkerImage(gray, N), "signed"),
          ],
        };
      },
    },
    {
      id: "kernel",
      group: "kern",
      name: "空域核的频率响应",
      question: "把课件里的平滑核、锐化核做 DFT。低频和高频，哪一端系数更大？",
      formula: "频率响应 = 卷积核的 DFT。空域卷积的效果由 |H(u,v)| 决定。",
      note: "均值核的频率响应是二维 sinc，能看到旁瓣。拉普拉斯的直流增益约为 0。",
      defaults: { kernel: "smooth" },
      fields: [
        {
          key: "kernel", label: "卷积核", type: "select",
          options: Object.keys(F().KERNELS).map((id) => [id, F().KERNELS[id].name]),
        },
      ],
      codes: [["核的 DFT", "kernel"], ["空域卷积", "spatial"], ["二维 FFT", "fft"]],
      run(gray) {
        const item = F().KERNELS[state.p.kernel.kernel];
        const t0 = now();
        const filtered = F().convolveSpatial(gray, N, item.k);
        const ms = now() - t0;
        const spec = F().kernelSpectrum(item.k, N);
        const c = N >> 1;
        const dc = Math.hypot(spec.re[c * N + c], spec.im[c * N + c]);
        const corner = Math.hypot(spec.re[0], spec.im[0]);
        const row = centerRowMag(spec.re, spec.im);
        const signed = state.p.kernel.kernel === "lap" || state.p.kernel.kernel === "sobel";
        return {
          stats: "中心增益 " + dc.toFixed(3) + "，最高频角落增益 " + corner.toFixed(3)
            + "。空域卷积 " + ms.toFixed(1) + " ms。" + item.text + "。",
          nums: "h =\n" + item.k.map((rowK) => rowK.map((v) => v.toFixed(3).padStart(8)).join(" ")).join("\n"),
          panels: [
            img("原图", gray, "clip"),
            img(signed ? "卷积结果（0 为中灰）" : "卷积结果", filtered, signed ? "signed" : "clip"),
            specPanel("|H(u,v)|，中心为直流", spec),
            plot("过中心的水平剖面", row.row, row.max, null),
          ],
        };
      },
    },
    {
      id: "conv",
      group: "conv",
      name: "循环卷积与补零",
      question: "同一卷积核，不补零就做 DFT 相乘，和空域线性卷积差在哪里？补多少零才能相等？",
      formula: "循环卷积：把图像和核都看成周期序列。线性卷积：先补零到至少 N+K−1。",
      note: "右下角亮边就是卷绕。运动模糊只有一行，空域乘法很少，所以往往比 DFT 更快；换成方框大核后，频域才划算。一维数字和二维是同一件事。",
      defaults: { kind: "motion", width: 41 },
      fields: [
        {
          key: "kind", label: "核", type: "select",
          options: [["motion", "水平运动模糊"], ["box", "方框均值"]],
        },
        { key: "width", label: "核宽度 K", type: "range", min: 5, max: 61, step: 2 },
      ],
      codes: [["频域卷积", "dftConv"], ["空域线性卷积", "spatial"], ["一维例子", "conv1"], ["补零后再循环", "circ1"]],
      run(gray) {
        const p = state.p.conv;
        let k = p.width | 0;
        if (p.kind === "box") k = Math.min(k, 21);
        const kernel = p.kind === "box" ? F().boxKernel(k) : F().motionKernel(k);
        const t0 = now();
        const spatial = F().convolveSpatial(gray, N, kernel);
        const t1 = now();
        const circular = F().convolveDFT(gray, N, kernel, "circular");
        const t2 = now();
        const linear = F().convolveDFT(gray, N, kernel, "linear");
        const t3 = now();
        const errLin = F().maxAbsDiff(spatial, linear);
        const errCirc = F().maxAbsDiff(spatial, circular);
        const diff = new Float64Array(N * N);
        for (let i = 0; i < diff.length; i++) diff[i] = circular[i] - spatial[i];
        const x = [1, 2, 3, 4, 5];
        const h = [1, 2, 1];
        const pad = F().nextPow2(N + k - 1);
        return {
          stats: "空域 " + (t1 - t0).toFixed(1) + " ms，不补零 DFT " + (t2 - t1).toFixed(1)
            + " ms，补零到 " + pad + " 的 DFT " + (t3 - t2).toFixed(1)
            + " ms。补零结果与空域最大误差 " + errLin.toExponential(2)
            + "，不补零最大误差 " + errCirc.toFixed(2)
            + (p.kind === "box" && (p.width | 0) > 21 ? "。方框核按 21×21 计算，避免页面卡住。" : "。"),
          nums: "一维：x = [" + x + "]，h = [" + h + "]，原点是中间的 2\n"
            + "线性（边界补 0） [" + F().conv1Linear(x, h) + "]\n"
            + "循环（不补零）   [" + F().conv1Circular(x, h) + "]\n"
            + "两侧补 1 个零    [" + F().conv1Padded(x, h) + "]\n"
            + "第 1 个数 4 和 9：循环时把末端的 5 卷了进来。",
          panels: [
            img("空域线性卷积", spatial, "clip"),
            img("DFT 相乘，不补零", circular, "clip"),
            img("补零后的 DFT 相乘", linear, "clip"),
            img("|循环 − 线性|", diff, "stretch"),
          ],
        };
      },
    },
    {
      id: "low",
      group: "design",
      name: "低通：理想 / 高斯 / 巴特沃斯",
      question: "同样的截止频率，为什么理想低通有振铃，高斯没有？把巴特沃斯的阶数 n 加大，会发生什么？",
      formula: "理想：D ≤ D0 为 1。高斯：exp(−D² / 2D0²)。巴特沃斯：1 / (1 + (D/D0)<sup>2n</sup>)。",
      note: "右下角是 H 的逆变换，只取中心 128×128。理想低通对应 sinc 型振荡，高斯对应光滑的正脉冲。波带片上最容易看出截止半径。",
      defaults: { type: "ideal", d0: 28, order: 2 },
      fields: [
        {
          key: "type", label: "类型", type: "select",
          options: [["ideal", "理想"], ["gaussian", "高斯"], ["butter", "巴特沃斯"]],
        },
        { key: "d0", label: "截止 D0", type: "range", min: 2, max: 120, step: 1 },
        { key: "order", label: "阶数 n", type: "range", min: 1, max: 10, step: 1, when: (p) => p.type === "butter" },
      ],
      presets: [
        { name: "理想 · 振铃", type: "ideal", d0: 28 },
        { name: "高斯", type: "gaussian", d0: 28 },
        { name: "巴特沃斯 n=1", type: "butter", order: 1, d0: 28 },
        { name: "巴特沃斯 n=2", type: "butter", order: 2, d0: 28 },
        { name: "巴特沃斯 n=10", type: "butter", order: 10, d0: 28 },
        { name: "换波带片", source: "zone" },
      ],
      codes: [["构造 H", "low"], ["频域相乘", "apply"], ["二维 FFT", "fft"]],
      run(gray) {
        const p = state.p.low;
        const spec = spectrum(state.source);
        const H = F().lowpassH(N, p.d0, p.order, p.type);
        const out = F().applyH(spec, H, N);
        const spatial = F().cropCenter(F().spatialFromCenteredH(H, N), N, 128);
        const energy = F().energyKept(spec.re, spec.im, H);
        const neg = minOf(spatial);
        return {
          stats: "保留能量 " + (100 * energy).toFixed(2) + "%。空域核最小值 "
            + neg.toExponential(2) + (neg < -1e-4 ? "，有负瓣，会振铃。" : "，几乎没有负瓣。"),
          panels: [
            img("滤波结果", out, "clip"),
            hPanel("H(u,v)", H, p.d0),
            plot("径向剖面（过中心的一行）", centerRow(H), 1.2, p.d0),
            img("空域核中心 128×128", spatial, "signed"),
          ],
        };
      },
    },
    {
      id: "high",
      group: "design",
      name: "高通",
      question: "高通写成 Hhp = 1 − Hlp。理想、高斯、巴特沃斯高通的边缘有什么差别？",
      formula: "H<sub>hp</sub> = a + (1 − a)(1 − H<sub>lp</sub>)。a = 0 为纯高通。",
      note: "纯高通的直流约为 0，显示时做了对比拉伸。把 a 调到 0.4 左右，就是高频强调，轮廓和边缘会同时留下来。",
      defaults: { type: "gaussian", d0: 24, order: 2, keep: 0 },
      fields: [
        {
          key: "type", label: "低通原型", type: "select",
          options: [["ideal", "理想"], ["gaussian", "高斯"], ["butter", "巴特沃斯"]],
        },
        { key: "d0", label: "截止 D0", type: "range", min: 2, max: 100, step: 1 },
        { key: "order", label: "阶数 n", type: "range", min: 1, max: 10, step: 1, when: (p) => p.type === "butter" },
        { key: "keep", label: "保留低频 a", type: "range", min: 0, max: 1, step: 0.01 },
      ],
      presets: [
        { name: "纯高通", keep: 0 },
        { name: "高频强调", keep: 0.45 },
        { name: "理想高通", type: "ideal", keep: 0, d0: 24 },
      ],
      codes: [["高通 = 1 − 低通", "emphasis"], ["低通原型", "low"], ["频域相乘", "apply"]],
      run(gray) {
        const p = state.p.high;
        const spec = spectrum(state.source);
        const H = F().emphasis(F().lowpassH(N, p.d0, p.order, p.type), p.keep);
        const out = F().applyH(spec, H, N);
        const dc = H[(N >> 1) * N + (N >> 1)];
        return {
          stats: "直流增益 " + dc.toFixed(3) + "，保留能量 " + (100 * F().energyKept(spec.re, spec.im, H)).toFixed(2) + "%。",
          panels: [
            img(p.keep > 0.05 ? "高频强调结果" : "高通结果（已拉伸）", out, p.keep > 0.05 ? "clip" : "stretch"),
            hPanel("Hhp(u,v)", H, p.d0),
            plot("径向剖面", centerRow(H), Math.max(1.2, maxOf(H)), p.d0),
            img("空域核中心 128×128", F().cropCenter(F().spatialFromCenteredH(H, N), N, 128), "signed"),
          ],
        };
      },
    },
    {
      id: "noise",
      group: "design",
      name: "周期噪声与带阻",
      question: "给图像加上正弦干扰。频谱上的亮点在哪里？怎样只去掉这些频率？",
      formula: "噪声频率 (±f, 0) 与 (0, ±f)。陷波挖掉这四个点；带阻挖掉半径为 f 的一圈。",
      note: "陷波半径或带阻宽度太小，亮点去不干净；太大，会把图像自己的频率一起挖掉。",
      defaults: { freq: 32, amp: 40, width: 6, order: 2, type: "notch" },
      fields: [
        { key: "freq", label: "噪声频率 f", type: "range", min: 6, max: 90, step: 1 },
        { key: "amp", label: "噪声幅度", type: "range", min: 0, max: 90, step: 1 },
        { key: "width", label: "陷波半径 / 带宽", type: "range", min: 1, max: 24, step: 1 },
        { key: "order", label: "巴特沃斯阶数", type: "range", min: 1, max: 6, step: 1, when: (p) => p.type === "butter" },
        {
          key: "type", label: "滤波器", type: "select",
          options: [
            ["notch", "陷波"],
            ["ideal", "理想带阻"],
            ["gaussian", "高斯带阻"],
            ["butter", "巴特沃斯带阻"],
          ],
        },
      ],
      codes: [["加正弦噪声", "noise"], ["陷波", "notch"], ["带阻", "band"], ["频域相乘", "apply"]],
      run(gray) {
        const p = state.p.noise;
        const noisy = F().addPeriodic(gray, N, p.freq, p.amp);
        const spec = F().centeredDft(noisy, N);
        const H = p.type === "notch"
          ? F().notchH(N, p.freq, p.width)
          : F().bandRejectH(N, p.freq, p.width, p.order, p.type);
        const out = F().applyH(spec, H, N);
        const c = N / 2;
        const circles = p.type === "notch"
          ? [[c + p.freq, c], [c - p.freq, c], [c, c + p.freq], [c, c - p.freq]]
            .map((xy) => ({ x: xy[0], y: xy[1], r: p.width }))
          : null;
        return {
          stats: "亮点应落在距离中心 " + p.freq + " 像素的四个轴点上。",
          panels: [
            img("加周期噪声", noisy, "clip"),
            specPanel("加噪后的幅度谱", spec),
            img("滤波结果", out, "clip"),
            hPanel(p.type === "notch" ? "陷波 H" : "带阻 H", H, p.type === "notch" ? null : p.freq, circles),
          ],
        };
      },
    },
    {
      id: "shift",
      group: "apply",
      name: "平移与幅度",
      question: "图像平移之后，幅度谱变不变？循环平移和边缘补零，结论一样吗？",
      formula: "f(x − x0, y − y0) 的 DFT 多一个相位因子 exp(−j2π(ux0+vy0)/N)，幅度不变。",
      note: "这条性质针对循环平移。边缘补 0 会改掉一部分像素，幅度只是大体还在。",
      defaults: { dx: 48, dy: 30, mode: "circ" },
      fields: [
        {
          key: "mode", label: "平移方式", type: "select",
          options: [["circ", "循环平移"], ["zero", "边缘补零"]],
        },
        { key: "dx", label: "水平 dx", type: "range", min: -100, max: 100, step: 1 },
        { key: "dy", label: "垂直 dy", type: "range", min: -100, max: 100, step: 1 },
      ],
      codes: [["循环平移", "shift"], ["补零平移", "zeroShift"], ["中心化 DFT", "centered"]],
      run(gray) {
        const p = state.p.shift;
        const moved = p.mode === "circ"
          ? F().circShift(gray, N, p.dx, p.dy)
          : F().zeroShift(gray, N, p.dx, p.dy);
        const a = spectrum(state.source);
        const b = F().centeredDft(moved, N);
        let max = 0;
        const diff = new Float64Array(N * N);
        for (let i = 0; i < diff.length; i++) {
          const d = Math.abs(Math.hypot(a.re[i], a.im[i]) - Math.hypot(b.re[i], b.im[i]));
          diff[i] = d;
          if (d > max) max = d;
        }
        const same = max < 1e-2;
        return {
          stats: "线性幅度最大差 " + max.toExponential(2) + (same
            ? "。可以当作幅度相同。"
            : "。补零改变了边界，幅度不再严格相等。"),
          panels: [
            img("平移后的图像", moved, "clip"),
            specPanel("原图幅度谱", a),
            specPanel("平移后幅度谱", b),
            same ? img("幅度差（数值上为 0）", diff, "flat") : img("|幅度差|（已拉伸）", diff, "stretch"),
          ],
        };
      },
    },
    {
      id: "rotate",
      group: "apply",
      name: "旋转与频谱方向",
      question: "图像转过 θ 之后，幅度谱是否也转过 θ？用条纹图最容易看。",
      formula: "空域旋转 θ，频谱同样旋转 θ。出界填 0 会额外带来一些频率。",
      note: "红线是幅度能量最集中的方向。水平条纹的能量在竖直方向，所以 0° 时红线接近 90°。",
      defaults: { deg: 24 },
      fields: [
        { key: "deg", label: "逆时针角度 θ", type: "range", min: -180, max: 180, step: 1 },
      ],
      presets: [
        { name: "0°", deg: 0 },
        { name: "20°", deg: 20 },
        { name: "45°", deg: 45 },
        { name: "换文档条纹", source: "stripes" },
      ],
      codes: [["旋转图像", "rotate"], ["中心化 DFT", "centered"], ["二维 FFT", "fft"]],
      run(gray) {
        const deg = state.p.rotate.deg;
        const turned = F().rotateImage(gray, N, deg);
        const a = spectrum(state.source);
        const b = F().centeredDft(turned, N);
        const a0 = F().dominantAngle(a.re, a.im, N);
        const a1 = F().dominantAngle(b.re, b.im, N);
        // 屏幕 y 向下，主方向角沿顺时针增加；图像逆时针转 θ 时，这个读数减少 θ。
        const turnedBy = fold(a0 - a1);
        return {
          stats: "原图主方向 " + a0 + "°，旋转后 " + a1 + "°，逆时针转过 " + turnedBy + "°，滑块 θ = " + deg + "°。",
          panels: [
            img("原图", gray, "clip"),
            img("旋转 " + deg + "°", turned, "clip"),
            specPanel("原图幅度谱", a, "center", a0),
            specPanel("旋转后幅度谱", b, "center", a1),
          ],
        };
      },
    },
    {
      id: "dct",
      group: "apply",
      name: "分块 DCT 与压缩",
      question: "JPEG 把图像切成 8×8，做 DCT 后丢掉高频系数。只留前 K 个，图像会怎样？",
      formula: "每个小块只保留折线顺序的前 K 个系数，其余置 0，再做 IDCT。",
      note: "点击左侧图像，换一块看系数。左上角那一格是直流。K = 1 时每块变成它自己的平均值。",
      defaults: { keep: 6, bx: 16, by: 16 },
      fields: [
        { key: "keep", label: "保留系数 K", type: "range", min: 1, max: 64, step: 1 },
      ],
      presets: [
        { name: "K=1", keep: 1 },
        { name: "K=6", keep: 6 },
        { name: "K=15", keep: 15 },
        { name: "K=64 原图", keep: 64 },
      ],
      codes: [["分块保留低频", "dct"], ["一维 DCT / IDCT", "dct1"]],
      run(gray) {
        const p = state.p.dct;
        const bx = (p.bx | 0) * 8;
        const by = (p.by | 0) * 8;
        const t0 = now();
        const r = F().compressDCT(gray, N, p.keep, bx, by);
        const ms = now() - t0;
        const peak = F().maxAbs(r.diff);
        const psnr = r.psnr === Infinity ? "∞" : r.psnr.toFixed(2) + " dB";
        return {
          stats: "PSNR " + psnr + "，最大绝对误差 " + peak.toFixed(3)
            + "。块 (" + (p.by | 0) + ", " + (p.bx | 0) + ")，用时 " + ms.toFixed(1) + " ms。",
          panels: [
            img("原图（点击选块）", gray, "clip", { x: bx, y: by }),
            img("保留前 " + (p.keep | 0) + " 个系数", r.out, "clip", { x: bx, y: by }),
            peak < 0.05 ? img("差值（几乎为 0）", r.diff, "flat") : img("差值（已拉伸）", r.diff, "signed"),
            block("该块的 DCT 系数", r.coeff, p.keep | 0),
          ],
        };
      },
    },
  ];

  const state = {
    source: "lena",
    demo: "mag",
    images: {},
    spec: {},
    p: {},
    codeTab: 0,
    codeKey: "",
  };

  function now() { return (typeof performance !== "undefined" ? performance.now() : Date.now()); }
  function demoById(id) { return DEMOS.find((d) => d.id === id); }
  function sourceName(id) {
    const opt = document.querySelector('#sourceSelect option[value="' + id + '"]');
    return opt ? opt.textContent : id;
  }
  function fold(d) {
    let x = ((d % 180) + 180) % 180;
    if (x > 90) x -= 180;
    return Math.round(x);
  }
  function minOf(a) {
    let m = Infinity;
    for (let i = 0; i < a.length; i++) if (a[i] < m) m = a[i];
    return m;
  }
  function maxOf(a) {
    let m = -Infinity;
    for (let i = 0; i < a.length; i++) if (a[i] > m) m = a[i];
    return m;
  }
  function centerRow(H) {
    const c = N >> 1;
    const row = new Float64Array(N);
    for (let u = 0; u < N; u++) row[u] = H[c * N + u];
    return row;
  }
  function centerRowMag(re, im) {
    const c = N >> 1;
    const row = new Float64Array(N);
    let max = 1e-6;
    for (let u = 0; u < N; u++) {
      row[u] = Math.hypot(re[c * N + u], im[c * N + u]);
      if (row[u] > max) max = row[u];
    }
    return { row, max };
  }
  function spectrum(name) {
    if (!state.spec[name]) state.spec[name] = F().centeredDft(state.images[name], N);
    return state.spec[name];
  }

  function img(title, data, mode, block) {
    return { title, kind: "img", data, mode, n: data.length === 128 * 128 ? 128 : N, block: block || null };
  }
  function specPanel(title, spec, mark, angle) {
    return {
      title, kind: "img", data: F().logMag(spec.re, spec.im), mode: "stretch", n: N,
      mark: mark || "center", angle: angle == null ? null : angle,
    };
  }
  function hPanel(title, H, d0, circles) {
    return { title, kind: "img", data: H, mode: "h", n: N, circle: d0, circles: circles || null };
  }
  function plot(title, row, yMax, d0) {
    return { title, kind: "plot", row, yMax, d0 };
  }
  function block(title, coeff, keep) {
    return { title, kind: "block", coeff, keep };
  }

  function paintImage(canvas, panel) {
    const n = panel.n;
    const data = panel.data;
    const ctx = canvas.getContext("2d");
    canvas.width = n;
    canvas.height = n;
    canvas.classList.remove("light");
    const image = ctx.createImageData(n, n);
    const px = image.data;
    if (panel.mode === "flat") {
      for (let i = 0; i < n * n; i++) {
        const o = i * 4;
        px[o] = px[o + 1] = px[o + 2] = 0;
        px[o + 3] = 255;
      }
    } else if (panel.mode === "clip") {
      for (let i = 0; i < n * n; i++) {
        let g = data[i];
        if (g < 0) g = 0;
        else if (g > 255) g = 255;
        const o = i * 4;
        px[o] = px[o + 1] = px[o + 2] = g;
        px[o + 3] = 255;
      }
    } else if (panel.mode === "unit") {
      for (let i = 0; i < n * n; i++) {
        const g = data[i] * 255;
        const o = i * 4;
        px[o] = px[o + 1] = px[o + 2] = g;
        px[o + 3] = 255;
      }
    } else if (panel.mode === "signed") {
      let a = 1e-8;
      for (let i = 0; i < data.length; i++) a = Math.max(a, Math.abs(data[i]));
      for (let i = 0; i < data.length; i++) {
        const g = 128 + (127 * data[i]) / a;
        const o = i * 4;
        px[o] = px[o + 1] = px[o + 2] = g;
        px[o + 3] = 255;
      }
    } else if (panel.mode === "h") {
      let max = 1;
      for (let i = 0; i < data.length; i++) if (data[i] > max) max = data[i];
      for (let i = 0; i < data.length; i++) {
        const g = (Math.max(0, data[i]) / max) * 255;
        const o = i * 4;
        px[o] = px[o + 1] = px[o + 2] = g;
        px[o + 3] = 255;
      }
    } else {
      let min = Infinity;
      let max = -Infinity;
      for (let i = 0; i < data.length; i++) {
        if (data[i] < min) min = data[i];
        if (data[i] > max) max = data[i];
      }
      const span = max - min || 1;
      for (let i = 0; i < data.length; i++) {
        const g = ((data[i] - min) / span) * 255;
        const o = i * 4;
        px[o] = px[o + 1] = px[o + 2] = g;
        px[o + 3] = 255;
      }
    }
    ctx.putImageData(image, 0, 0);
    if (panel.mark === "center") {
      ctx.fillStyle = "#b42024";
      ctx.fillRect(n / 2 - 1, n / 2 - 1, 3, 3);
    } else if (panel.mark === "origin") {
      ctx.fillStyle = "#b42024";
      ctx.fillRect(0, 0, 6, 6);
    }
    if (panel.angle != null) {
      const rad = panel.angle * Math.PI / 180;
      const c = n / 2;
      const r = n * 0.46;
      ctx.strokeStyle = "rgba(180,32,36,0.95)";
      ctx.lineWidth = 1.5;
      ctx.beginPath();
      ctx.moveTo(c - r * Math.cos(rad), c - r * Math.sin(rad));
      ctx.lineTo(c + r * Math.cos(rad), c + r * Math.sin(rad));
      ctx.stroke();
    }
    if (panel.circle) {
      ctx.strokeStyle = "rgba(180,32,36,0.95)";
      ctx.lineWidth = 1.5;
      ctx.beginPath();
      ctx.arc(n / 2, n / 2, panel.circle, 0, Math.PI * 2);
      ctx.stroke();
    }
    if (panel.circles) {
      ctx.strokeStyle = "rgba(180,32,36,0.95)";
      ctx.lineWidth = 1.5;
      panel.circles.forEach((c) => {
        ctx.beginPath();
        ctx.arc(c.x, c.y, Math.max(c.r, 1), 0, Math.PI * 2);
        ctx.stroke();
      });
    }
    if (panel.block) {
      ctx.strokeStyle = "#b42024";
      ctx.lineWidth = 2;
      ctx.strokeRect(panel.block.x + 0.5, panel.block.y + 0.5, 8, 8);
    }
  }

  function paintPlot(canvas, panel) {
    const w = canvas.width = 480;
    const h = canvas.height = 250;
    canvas.classList.add("light");
    const ctx = canvas.getContext("2d");
    ctx.fillStyle = "#ffffff";
    ctx.fillRect(0, 0, w, h);
    const pad = 32;
    const yMax = panel.yMax || 1;
    const row = panel.row;
    ctx.strokeStyle = "#1b4e79";
    ctx.lineWidth = 2;
    ctx.beginPath();
    for (let u = 0; u < row.length; u++) {
      const x = pad + (u / (row.length - 1)) * (w - pad * 2);
      const y = h - pad - (Math.max(0, row[u]) / yMax) * (h - pad * 2);
      if (u === 0) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
    }
    ctx.stroke();
    if (panel.d0) {
      ctx.strokeStyle = "#b42024";
      ctx.setLineDash([4, 3]);
      const c = (row.length - 1) / 2;
      [c - panel.d0, c + panel.d0].forEach((u) => {
        const x = pad + (u / (row.length - 1)) * (w - pad * 2);
        ctx.beginPath();
        ctx.moveTo(x, pad);
        ctx.lineTo(x, h - pad);
        ctx.stroke();
      });
      ctx.setLineDash([]);
    }
    ctx.fillStyle = "#5c6b7a";
    ctx.font = "13px Microsoft YaHei";
    ctx.fillText("−Nyquist", pad, h - 8);
    ctx.fillText("直流", w / 2 - 14, h - 8);
    ctx.fillText("+Nyquist", w - pad - 58, h - 8);
  }

  function paintBlock(canvas, panel) {
    const scale = 32;
    canvas.width = 8 * scale;
    canvas.height = 8 * scale;
    canvas.classList.add("light");
    const ctx = canvas.getContext("2d");
    ctx.fillStyle = "#f4f7fb";
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    const coeff = panel.coeff;
    let max = 1;
    for (let v = 0; v < 8; v++) {
      for (let u = 0; u < 8; u++) max = Math.max(max, Math.abs(coeff[v][u]));
    }
    const zig = F().zigzagCoords();
    const keep = new Set();
    for (let i = 0; i < panel.keep; i++) keep.add(zig[i][0] + "," + zig[i][1]);
    for (let v = 0; v < 8; v++) {
      for (let u = 0; u < 8; u++) {
        const on = keep.has(u + "," + v);
        const t = Math.log(1 + Math.abs(coeff[v][u])) / Math.log(1 + max);
        if (!on) ctx.fillStyle = "#9aabbd";
        else {
          const g = Math.round(255 - t * 160);
          ctx.fillStyle = "rgb(" + g + "," + (g + 15) + ",255)";
        }
        ctx.fillRect(u * scale + 1, v * scale + 1, scale - 2, scale - 2);
      }
    }
    ctx.strokeStyle = "#b42024";
    ctx.lineWidth = 3;
    ctx.strokeRect(1.5, 1.5, scale - 3, scale - 3);
  }

  function drawPanel(index, panel) {
    const canvas = document.getElementById("c" + index);
    document.getElementById("cap" + index).textContent = panel.title;
    if (panel.kind === "plot") paintPlot(canvas, panel);
    else if (panel.kind === "block") paintBlock(canvas, panel);
    else paintImage(canvas, panel);
  }

  function highlight(src) {
    const esc = (s) => s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
    const kw = /^(?:function|return|const|let|for|if|else|new|while|true|false)\b/;
    let i = 0;
    let out = "";
    while (i < src.length) {
      if (src.startsWith("//", i)) {
        const j = src.indexOf("\n", i);
        const end = j < 0 ? src.length : j;
        out += '<span class="cm">' + esc(src.slice(i, end)) + "</span>";
        i = end;
        continue;
      }
      const ch = src[i];
      if (ch === '"' || ch === "'") {
        let j = i + 1;
        while (j < src.length && src[j] !== ch) {
          if (src[j] === "\\") j++;
          j++;
        }
        out += '<span class="st">' + esc(src.slice(i, j + 1)) + "</span>";
        i = j + 1;
        continue;
      }
      if (/\d/.test(ch) && (i === 0 || /[^\w.]/.test(src[i - 1]))) {
        const m = src.slice(i).match(/^\d+(?:\.\d+)?(?:e[+-]?\d+)?/);
        out += '<span class="nu">' + m[0] + "</span>";
        i += m[0].length;
        continue;
      }
      if ((i === 0 || /[^\w$]/.test(src[i - 1]))) {
        const m = src.slice(i).match(kw);
        if (m) {
          out += '<span class="kw">' + m[0] + "</span>";
          i += m[0].length;
          continue;
        }
      }
      out += esc(ch);
      i++;
    }
    return out;
  }

  function fillCode() {
    const demo = demoById(state.demo);
    const key = state.demo + ":" + state.codeTab;
    const tabs = document.getElementById("codeTabs");
    tabs.innerHTML = "";
    demo.codes.forEach((item, i) => {
      const btn = document.createElement("button");
      btn.type = "button";
      btn.textContent = item[0];
      btn.className = i === state.codeTab ? "active" : "";
      btn.addEventListener("click", () => {
        state.codeTab = i;
        state.codeKey = "";
        fillCode();
      });
      tabs.appendChild(btn);
    });
    if (state.codeKey === key) return;
    state.codeKey = key;
    const id = demo.codes[state.codeTab][1];
    document.getElementById("code").innerHTML = highlight(CODE[id]());
  }

  function syncFields() {
    const demo = demoById(state.demo);
    const p = state.p[state.demo];
    demo.fields.forEach((f) => {
      const el = document.getElementById("field-" + f.key);
      if (!el) return;
      if (f.when) el.hidden = !f.when(p);
      if (f.type === "range") {
        el.querySelector("input").value = p[f.key];
        el.querySelector("output").textContent = formatVal(p[f.key], f.step);
      } else if (f.type === "select") {
        el.querySelector("select").value = p[f.key];
      }
    });
  }

  function formatVal(v, step) {
    if (step && step < 1) return Number(v).toFixed(2);
    return String(Math.round(Number(v)));
  }

  function buildControls() {
    const demo = demoById(state.demo);
    const box = document.getElementById("controls");
    box.innerHTML = "";
    if (demo.presets) {
      const row = document.createElement("div");
      row.className = "presets";
      demo.presets.forEach((preset) => {
        const btn = document.createElement("button");
        btn.type = "button";
        btn.textContent = preset.name;
        btn.addEventListener("click", () => {
          if (preset.source) {
            setSource(preset.source);
            return;
          }
          Object.keys(preset).forEach((k) => {
            if (k !== "name") state.p[state.demo][k] = preset[k];
          });
          syncFields();
          draw();
        });
        row.appendChild(btn);
      });
      box.appendChild(row);
    }
    demo.fields.forEach((f) => {
      const wrap = document.createElement("div");
      wrap.className = "ctrl";
      wrap.id = "field-" + f.key;
      if (f.type === "range") {
        wrap.innerHTML = '<div class="row"><span>' + f.label + '</span><output></output></div>';
        const input = document.createElement("input");
        input.type = "range";
        input.min = f.min;
        input.max = f.max;
        input.step = f.step;
        input.addEventListener("input", () => {
          const n = Number(input.value);
          state.p[state.demo][f.key] = f.step >= 1 ? Math.round(n) : n;
          wrap.querySelector("output").textContent = formatVal(state.p[state.demo][f.key], f.step);
          draw();
        });
        wrap.appendChild(input);
      } else {
        const label = document.createElement("label");
        label.textContent = f.label;
        const sel = document.createElement("select");
        f.options.forEach((opt) => {
          const o = document.createElement("option");
          o.value = opt[0];
          o.textContent = opt[1];
          sel.appendChild(o);
        });
        sel.addEventListener("change", () => {
          state.p[state.demo][f.key] = sel.value;
          syncFields();
          draw();
        });
        label.appendChild(sel);
        wrap.appendChild(label);
      }
      box.appendChild(wrap);
    });
    syncFields();
  }

  function buildNav() {
    const g = document.getElementById("groups");
    g.innerHTML = "";
    GROUPS.forEach((group) => {
      const btn = document.createElement("button");
      btn.type = "button";
      btn.textContent = group.name;
      btn.dataset.id = group.id;
      btn.addEventListener("click", () => {
        const current = demoById(state.demo);
        if (current.group === group.id) return;
        selectDemo(DEMOS.find((d) => d.group === group.id).id);
      });
      g.appendChild(btn);
    });
    buildDemoNav();
  }

  function buildDemoNav() {
    const demo = demoById(state.demo);
    document.querySelectorAll("#groups button").forEach((btn) => {
      btn.classList.toggle("active", btn.dataset.id === demo.group);
    });
    const nav = document.getElementById("demos");
    nav.innerHTML = "";
    DEMOS.filter((d) => d.group === demo.group).forEach((d) => {
      const btn = document.createElement("button");
      btn.type = "button";
      btn.textContent = d.name;
      btn.classList.toggle("active", d.id === state.demo);
      btn.addEventListener("click", () => selectDemo(d.id));
      nav.appendChild(btn);
    });
  }

  function selectDemo(id) {
    state.demo = id;
    state.codeTab = 0;
    state.codeKey = "";
    if (!state.p[id]) state.p[id] = Object.assign({}, demoById(id).defaults);
    buildDemoNav();
    buildControls();
    draw();
  }

  function setSource(name) {
    if (!state.images[name]) return;
    state.source = name;
    const sel = document.getElementById("sourceSelect");
    if ([...sel.options].some((o) => o.value === name)) sel.value = name;
    draw();
  }

  let drawing = false;
  let pending = false;
  function draw() {
    if (drawing) { pending = true; return; }
    drawing = true;
    const demo = demoById(state.demo);
    const gray = state.images[state.source];
    document.getElementById("demoTitle").textContent = demo.name;
    document.getElementById("question").textContent = demo.question;
    document.getElementById("formula").innerHTML = demo.formula;
    document.getElementById("note").textContent = demo.note;
    let view;
    try {
      view = demo.run(gray);
      document.getElementById("stats").textContent = view.stats || "";
    const nums = document.getElementById("nums");
    if (view.nums) {
      nums.hidden = false;
      nums.textContent = view.nums;
    } else nums.hidden = true;
      view.panels.forEach((panel, i) => drawPanel(i, panel));
      fillCode();
    } catch (err) {
      document.getElementById("stats").textContent = "计算失败：" + err.message;
    } finally {
      drawing = false;
    }
    if (pending) { pending = false; draw(); }
  }

  function fromFile(file) {
    const url = URL.createObjectURL(file);
    const im = new Image();
    im.onload = () => {
      state.images.custom = toGray(im);
      delete state.spec.custom;
      const sel = document.getElementById("sourceSelect");
      if (![...sel.options].some((o) => o.value === "custom")) {
        const opt = document.createElement("option");
        opt.value = "custom";
        opt.textContent = "本地图片";
        sel.appendChild(opt);
      }
      URL.revokeObjectURL(url);
      setSource("custom");
    };
    im.src = url;
  }

  function toGray(im) {
    const c = document.createElement("canvas");
    c.width = N;
    c.height = N;
    const ctx = c.getContext("2d");
    ctx.drawImage(im, 0, 0, N, N);
    const d = ctx.getImageData(0, 0, N, N).data;
    const g = new Float64Array(N * N);
    for (let i = 0, j = 0; i < d.length; i += 4, j++) {
      g[j] = 0.299 * d[i] + 0.587 * d[i + 1] + 0.114 * d[i + 2];
    }
    return g;
  }

  function loadImage(url) {
    return new Promise((resolve, reject) => {
      const im = new Image();
      im.onload = () => resolve(toGray(im));
      im.onerror = () => reject(new Error(url));
      im.src = url;
    });
  }

  function onCanvasClick(event) {
    if (state.demo !== "dct") return;
    const rect = event.target.getBoundingClientRect();
    const x = (event.clientX - rect.left) / rect.width;
    const y = (event.clientY - rect.top) / rect.height;
    state.p.dct.bx = Math.max(0, Math.min(31, Math.floor(x * 32)));
    state.p.dct.by = Math.max(0, Math.min(31, Math.floor(y * 32)));
    draw();
  }

  document.getElementById("c0").addEventListener("click", onCanvasClick);
  document.getElementById("c1").addEventListener("click", onCanvasClick);
  document.getElementById("sourceSelect").addEventListener("change", (e) => setSource(e.target.value));
  document.getElementById("openBtn").addEventListener("click", () => document.getElementById("fileInput").click());
  document.getElementById("fileInput").addEventListener("change", (e) => {
    if (e.target.files[0]) fromFile(e.target.files[0]);
  });
  document.getElementById("codeBtn").addEventListener("click", () => {
    document.body.classList.toggle("focus-code");
  });

  async function boot() {
    state.images.zone = F().makeZone(N);
    state.images.stripes = F().makeStripes(N);
    state.images.rect = F().makeRect(N);
    state.images.geom = F().makeGeom(N);
    const banner = document.getElementById("banner");
    try {
      state.images.lena = await loadImage("images/lena.png");
    } catch (e) {
      state.images.lena = state.images.geom;
      banner.hidden = false;
      banner.textContent = "Lena 没有加载。请双击 start.bat，再用浏览器打开 http://127.0.0.1:8766/ 。";
    }
    try {
      state.images.camera = await loadImage("images/camera.png");
    } catch (e) {
      state.images.camera = state.images.geom;
      const opt = document.querySelector('#sourceSelect option[value="camera"]');
      if (opt) opt.textContent = "几何图";
    }
    buildNav();
    const q = new URLSearchParams(location.search);
    if (q.get("src") && state.images[q.get("src")]) {
      state.source = q.get("src");
      document.getElementById("sourceSelect").value = state.source;
    }
    const wanted = q.get("demo");
    selectDemo(wanted && demoById(wanted) ? wanted : "mag");
    document.body.dataset.ready = "1";
  }

  boot();
})();
