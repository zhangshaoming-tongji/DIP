/* 基-2 FFT。课堂演示用，长度必须是 2 的幂。 */
(function (root, factory) {
  const api = factory();
  if (typeof module === "object" && module.exports) module.exports = api;
  else root.Freq = api;
})(typeof globalThis !== "undefined" ? globalThis : this, function () {
  function fft1(re, im, invert) {
    const n = re.length;
    // 倒位序：下标按二进制翻转，对应蝶形运算的输入顺序。
    for (let i = 1, j = 0; i < n; i++) {
      let bit = n >> 1;
      for (; j & bit; bit >>= 1) j ^= bit;
      j ^= bit;
      if (i < j) {
        let tmp = re[i]; re[i] = re[j]; re[j] = tmp;
        tmp = im[i]; im[i] = im[j]; im[j] = tmp;
      }
    }
    // 蝶形：长度从 2 开始倍增。正变换旋转因子 exp(-j 2π / len)。
    for (let len = 2; len <= n; len <<= 1) {
      const ang = (2 * Math.PI / len) * (invert ? 1 : -1);
      const wRe = Math.cos(ang);
      const wIm = Math.sin(ang);
      for (let i = 0; i < n; i += len) {
        let curRe = 1;
        let curIm = 0;
        const half = len >> 1;
        for (let k = 0; k < half; k++) {
          const evenRe = re[i + k];
          const evenIm = im[i + k];
          const oddRe = re[i + k + half];
          const oddIm = im[i + k + half];
          const vRe = oddRe * curRe - oddIm * curIm;
          const vIm = oddRe * curIm + oddIm * curRe;
          re[i + k] = evenRe + vRe;
          im[i + k] = evenIm + vIm;
          re[i + k + half] = evenRe - vRe;
          im[i + k + half] = evenIm - vIm;
          const nextRe = curRe * wRe - curIm * wIm;
          curIm = curRe * wIm + curIm * wRe;
          curRe = nextRe;
        }
      }
    }
    if (invert) {
      for (let i = 0; i < n; i++) {
        re[i] /= n;
        im[i] /= n;
      }
    }
  }

  function fft2(re, im, n, invert) {
    // 可分离：先逐行，再逐列。逆变换每一维除以 n，合计除以 n²。
    const rowRe = new Float64Array(n);
    const rowIm = new Float64Array(n);
    for (let y = 0; y < n; y++) {
      for (let x = 0; x < n; x++) {
        rowRe[x] = re[y * n + x];
        rowIm[x] = im[y * n + x];
      }
      fft1(rowRe, rowIm, invert);
      for (let x = 0; x < n; x++) {
        re[y * n + x] = rowRe[x];
        im[y * n + x] = rowIm[x];
      }
    }
    for (let x = 0; x < n; x++) {
      for (let y = 0; y < n; y++) {
        rowRe[y] = re[y * n + x];
        rowIm[y] = im[y * n + x];
      }
      fft1(rowRe, rowIm, invert);
      for (let y = 0; y < n; y++) {
        re[y * n + x] = rowRe[y];
        im[y * n + x] = rowIm[y];
      }
    }
  }

  return { fft1, fft2 };
});
