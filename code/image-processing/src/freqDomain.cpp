#include "freqDomain.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdlib>
#include <vector>

namespace cvrsg
{
namespace kongzi
{
namespace freq
{

namespace
{

const double kPi = 3.14159265358979323846;

// 基 2 一维 FFT，长度必须是 2 的幂。
// 正变换旋转因子是 exp(−j 2π k / N)，不除 N。
// 逆变换使用共轭旋转因子，并在最后除以 N。这样正、逆互为反变换。
void fft1(std::vector<std::complex<double>> &a, bool invert)
{
	const int n = static_cast<int>(a.size());
	// 倒位序：把下标的二进制位反过来，原地重排，后面的蝶形才能按顺序合并。
	for (int i = 1, j = 0; i < n; ++i)
	{
		int bit = n >> 1; // 从最高位开始找 j 里连续的 1
		for (; j & bit; bit >>= 1)
			j ^= bit; // 这些位原来是 1，翻成 0，相当于二进制进位
		j ^= bit;	  // 当前位翻成 1，此时 j 就是 i 的倒位序
		if (i < j)	  // 每对只交换一次
			std::swap(a[i], a[j]);
	}
	// 蝶形。len 从 2 加倍到 n，每一层把两个 len/2 的子变换合成一个 len 点变换。
	// wlen = exp(±j 2π / len)，正变换取负号。
	for (int len = 2; len <= n; len <<= 1)
	{
		const double ang = (2.0 * kPi / len) * (invert ? 1.0 : -1.0); // 正变换角度为负
		const std::complex<double> wlen(std::cos(ang), std::sin(ang)); // 本层的基本旋转因子
		for (int i = 0; i < n; i += len)
		{
			std::complex<double> w(1.0, 0.0); // k = 0 时旋转因子是 1
			const int half = len >> 1;
			for (int k = 0; k < half; ++k)
			{
				const std::complex<double> u = a[i + k];			   // 上半段
				const std::complex<double> v = a[i + k + half] * w; // 下半段乘 w^k
				a[i + k] = u + v;									   // 蝶形的和
				a[i + k + half] = u - v;							   // 蝶形的差
				w *= wlen;											   // 下一个 k 的旋转因子
			}
		}
	}
	if (invert)
	{
		for (int i = 0; i < n; ++i)
			a[i] /= static_cast<double>(n); // 一维逆变换的 1/N，二维会乘两次
	}
}

// 乘 (−1)^{x+y}。x+y 为奇数时变号，偶数时不变。
// 它等于把频谱平移 n/2，所以直流从左上角移到中心。这个因子的平方是 1，再乘一次就移回去。
void modulate(cv::Mat &re, cv::Mat &im)
{
	const int n = re.rows;
	for (int y = 0; y < n; ++y)
	{
		double *pr = re.ptr<double>(y);
		double *pi = im.ptr<double>(y);
		for (int x = 0; x < n; ++x)
		{
			if ((x + y) & 1) // x+y 为奇数时 (−1)^{x+y} = −1
			{
				pr[x] = -pr[x];
				pi[x] = -pi[x]; // 实部、虚部一起变号，等于乘实数 −1
			}
		}
	}
}

// 把下标卷到 [0, n)。C++ 的 % 向 0 取整，负数会得到负余数，所以要再加 n。
int wrapIndex(int i, int n)
{
	int r = i % n;
	if (r < 0)
		r += n; // 例如 −1 % n 得到 −1，加 n 后才是 n−1
	return r;
}

// 不小于 v 的最小 2 的幂。FFT 只能处理这个长度。
int nextPow2(int v)
{
	int p = 1;
	while (p < v)
		p <<= 1;
	return p;
}

// 灰度是实数，虚部先置 0。center 为真时先中心化再正变换。
void dftFromGray(const cv::Mat &gray, cv::Mat &re, cv::Mat &im, bool center)
{
	re = gray.clone();
	im = cv::Mat::zeros(gray.size(), CV_64F);
	if (center)
		modulate(re, im); // 必须在 FFT 之前做
	fft2(re, im, false);
}

// 中心化频谱的逆变换：先 IFFT，再乘一次 (−1)^{x+y}，把直流从中心移回左上角对应的空域原图。
void idftToGray(const cv::Mat &re, const cv::Mat &im, cv::Mat &out)
{
	cv::Mat ore = re.clone();
	cv::Mat oim = im.clone();
	fft2(ore, oim, true); // 逆变换后直流仍在左上角
	modulate(ore, oim);	  // 再乘 (−1)^{x+y}，和正变换前的那一次抵消
	out = ore;			  // 实图像只保留实部，舍入造成的虚部丢掉
}

} // namespace

// 二维 DFT 可分离：F(u,v) = 对每一行做一维 DFT，再对每一列做一维 DFT。
// 逆变换时行、列各除以 n，合计除以 n²。
void fft2(cv::Mat &re, cv::Mat &im, bool invert)
{
	const int n = re.rows;
	std::vector<std::complex<double>> line(n);
	for (int y = 0; y < n; ++y)
	{
		double *pr = re.ptr<double>(y);
		double *pi = im.ptr<double>(y);
		for (int x = 0; x < n; ++x)
			line[x] = std::complex<double>(pr[x], pi[x]);
		fft1(line, invert); // 固定 v，对 u 做一维 DFT
		for (int x = 0; x < n; ++x)
		{
			pr[x] = line[x].real();
			pi[x] = line[x].imag();
		}
	}
	for (int x = 0; x < n; ++x)
	{
		for (int y = 0; y < n; ++y)
			line[y] = std::complex<double>(re.at<double>(y, x), im.at<double>(y, x));
		fft1(line, invert); // 固定 u，对 v 再做一维 DFT
		for (int y = 0; y < n; ++y)
		{
			re.at<double>(y, x) = line[y].real();
			im.at<double>(y, x) = line[y].imag();
		}
	}
}

void centeredDft(const cv::Mat &gray, cv::Mat &re, cv::Mat &im)
{
	dftFromGray(gray, re, im, true);
}

void rawDft(const cv::Mat &gray, cv::Mat &re, cv::Mat &im)
{
	dftFromGray(gray, re, im, false);
}

void synthesize(const cv::Mat &re, const cv::Mat &im, cv::Mat &out, int mode)
{
	const int n = re.rows;
	cv::Mat ore = cv::Mat::zeros(n, n, CV_64F);
	cv::Mat oim = cv::Mat::zeros(n, n, CV_64F);

	if (mode == RandomPhase)
	{
		// 实图像的频谱满足共轭对称：F(n−u, n−v) = F*(u,v)。
		// 随机相位也要成对取相反数，否则逆变换会出现虚部，图像不再是实数。
		// 直流点 (0,0) 的相位保持 0。这里的 (u,v) 仍是未再平移的矩阵下标，
		// 对称关系对中心化后的矩阵同样成立，因为 (−1)^{x+y} 不破坏共轭对称。
		cv::Mat phase = cv::Mat::zeros(n, n, CV_64F);
		for (int v = 0; v < n; ++v)
		{
			for (int u = 0; u < n; ++u)
			{
				const int u2 = (n - u) % n; // 共轭点。u = 0 时对方也是 0，不能写成 n
				const int v2 = (n - v) % n;
				if (v > v2 || (v == v2 && u > u2))
					continue; // 这一对已经由先遇到的那个点写过
				double p = (u == u2 && v == v2) ? 0.0 : (static_cast<double>(std::rand()) / RAND_MAX) * 2.0 * kPi;
				phase.at<double>(v, u) = p;	   // 相位 φ
				phase.at<double>(v2, u2) = -p; // 共轭点必须是 −φ
			}
		}
		for (int v = 0; v < n; ++v)
		{
			for (int u = 0; u < n; ++u)
			{
				const double m = std::hypot(re.at<double>(v, u), im.at<double>(v, u)); // |F|
				const double p = phase.at<double>(v, u);
				ore.at<double>(v, u) = m * std::cos(p); // |F| cos φ
				oim.at<double>(v, u) = m * std::sin(p); // |F| sin φ
			}
		}
	}
	else
	{
		for (int v = 0; v < n; ++v)
		{
			for (int u = 0; u < n; ++u)
			{
				const double rr = re.at<double>(v, u);
				const double ii = im.at<double>(v, u);
				const double m = std::hypot(rr, ii); // |F| = sqrt(Re² + Im²)
				if (mode == MagnitudeOnly)
				{
					// 相位取 0，频谱变成非负实数 |F|。逆变换后通常看不出原图轮廓。
					ore.at<double>(v, u) = m; // 虚部保持 0
				}
				else if (mode == PhaseOnly)
				{
					if (m > 1e-8)
					{
						ore.at<double>(v, u) = rr / m; // cos φ，幅度被归一成 1
						oim.at<double>(v, u) = ii / m; // sin φ
					}
				}
				else
				{
					ore.at<double>(v, u) = rr;
					oim.at<double>(v, u) = ii;
				}
			}
		}
	}
	idftToGray(ore, oim, out);
}

void mixMagPhase(const cv::Mat &magRe, const cv::Mat &magIm,
				 const cv::Mat &phaseRe, const cv::Mat &phaseIm, cv::Mat &out)
{
	const int n = magRe.rows;
	cv::Mat re = cv::Mat::zeros(n, n, CV_64F);
	cv::Mat im = cv::Mat::zeros(n, n, CV_64F);
	for (int v = 0; v < n; ++v)
	{
		for (int u = 0; u < n; ++u)
		{
			// 幅度来自第一幅图，单位复数 e^{jφ} 来自第二幅图的 F / |F|。
			const double m = std::hypot(magRe.at<double>(v, u), magIm.at<double>(v, u));
			const double p = std::hypot(phaseRe.at<double>(v, u), phaseIm.at<double>(v, u));
			if (p < 1e-8)
				continue; // 相位源在该点是 0，单位相位无定义，保持 0
			re.at<double>(v, u) = m * phaseRe.at<double>(v, u) / p; // |F1| · Re(F2)/|F2|
			im.at<double>(v, u) = m * phaseIm.at<double>(v, u) / p; // |F1| · Im(F2)/|F2|
		}
	}
	idftToGray(re, im, out);
}

// H 与中心化频谱对齐，距离从 (n/2, n/2) 量起。
// 理想低通在截止圆上突变，空域对应二维 sinc，卷积后会出现振铃。
// 高斯低通在空域仍是高斯，核没有负值。巴特沃斯介于两者之间，阶数越大越接近理想低通。
cv::Mat lowpassH(int n, double d0, int order, int type)
{
	cv::Mat H(n, n, CV_64F);
	const double c = n / 2.0;			// 中心化后的直流位置
	const double d = std::max(d0, 0.5); // 防止 D0 = 0 时高斯和巴特沃斯除零
	for (int v = 0; v < n; ++v)
	{
		double *row = H.ptr<double>(v);
		for (int u = 0; u < n; ++u)
		{
			const double D = std::hypot(u - c, v - c); // 到直流的欧氏距离
			if (type == Ideal)
				row[u] = D <= d ? 1.0 : 0.0; // 硬截止
			else if (type == Gaussian)
				row[u] = std::exp(-(D * D) / (2.0 * d * d)); // exp(−D² / (2 D0²))
			else
				row[u] = 1.0 / (1.0 + std::pow(D / d, 2.0 * order)); // 阶数是 order
		}
	}
	return H;
}

// 高频强调。keepLow 就是公式里的 a：
//   H = a + (1 − a) (1 − H_low) = a H_low + (1 − H_low)
// a = 0 时 H = 1 − H_low，直流为 0，结果接近边缘图。
// a 取 0.45 时直流增益为 0.45，原图亮度还在，边缘被加强。
cv::Mat emphasis(const cv::Mat &lowpass, double keepLow)
{
	cv::Mat H(lowpass.size(), CV_64F);
	for (int i = 0; i < lowpass.rows * lowpass.cols; ++i)
	{
		const double hl = lowpass.at<double>(i);
		H.at<double>(i) = keepLow + (1.0 - keepLow) * (1.0 - hl); // a + (1−a)(1−H_low)
	}
	return H;
}

// 正弦 sin(2π f x / n) 在中心化频谱的 (中心 ± f, 中心) 形成一对共轭尖峰。
// 另一个方向的正弦落在 (中心, 中心 ± f)。半径 radius 内置 0，其余保持 1。
cv::Mat notchH(int n, int frequency, int radius)
{
	cv::Mat H(n, n, CV_64F, cv::Scalar(1.0));
	const int c = n / 2;
	const int pts[4][2] = {
		{c + frequency, c}, // 竖直条纹：水平频率 +f
		{c - frequency, c}, // 它的共轭：−f
		{c, c + frequency}, // 水平条纹：竖直频率 +f
		{c, c - frequency}};
	for (int v = 0; v < n; ++v)
	{
		double *row = H.ptr<double>(v);
		for (int u = 0; u < n; ++u)
		{
			for (int p = 0; p < 4; ++p)
			{
				if (std::hypot(u - pts[p][0], v - pts[p][1]) <= radius)
				{
					row[u] = 0.0;
					break;
				}
			}
		}
	}
	return H;
}

void applyH(const cv::Mat &re, const cv::Mat &im, const cv::Mat &H, cv::Mat &out)
{
	cv::Mat ore = re.clone();
	cv::Mat oim = im.clone();
	for (int i = 0; i < re.rows * re.cols; ++i)
	{
		ore.at<double>(i) *= H.at<double>(i); // 实部、虚部乘同一个实数 H
		oim.at<double>(i) *= H.at<double>(i); // 幅度变成 |F|H，相位不变
	}
	idftToGray(ore, oim, out);
}

// 帕塞瓦尔意义下的能量比。H 为 0 或 1 时，就是通带内 |F|² 占全部能量的比例。
double energyKept(const cv::Mat &re, const cv::Mat &im, const cv::Mat &H)
{
	double num = 0.0;
	double den = 0.0;
	const int n = re.rows * re.cols;
	for (int i = 0; i < n; ++i)
	{
		const double p = re.at<double>(i) * re.at<double>(i) + im.at<double>(i) * im.at<double>(i); // |F|²
		den += p;
		num += p * H.at<double>(i) * H.at<double>(i); // |F H|²
	}
	return den == 0.0 ? 0.0 : num / den;
}

// 中心化的 H 不能直接逆变换。先把直流从中心卷回 (0,0)，逆变换后原点在左上角，
// 再卷回画面中心，负旁瓣才会出现在原点两侧，振铃才看得清楚。
cv::Mat spatialKernel(const cv::Mat &H)
{
	const int n = H.rows;
	const int c = n >> 1;
	cv::Mat re(n, n, CV_64F);
	cv::Mat im = cv::Mat::zeros(n, n, CV_64F);
	for (int v = 0; v < n; ++v)
		for (int u = 0; u < n; ++u)
			re.at<double>(v, u) = H.at<double>((v + c) % n, (u + c) % n); // ifftshift：直流卷回 (0,0)
	fft2(re, im, true); // H 是实数，逆变换得到空域核
	cv::Mat out(n, n, CV_64F);
	for (int y = 0; y < n; ++y)
		for (int x = 0; x < n; ++x)
			out.at<double>((y + c) % n, (x + c) % n) = re.at<double>(y, x); // fftshift：原点移到画面中心
	const int m = std::min(128, n); // 振铃主要在中心附近，四周不必整幅显示
	const int h = m >> 1;
	return out(cv::Rect(c - h, c - h, m, m)).clone();
}

cv::Mat convolveSpatial(const cv::Mat &src, const cv::Mat &kernel)
{
	// 边界外补 0。核的中心对准当前像素。
	const int n = src.rows;
	const int kh = kernel.rows;
	const int kw = kernel.cols;
	const int cy = kh / 2; // 核的几何中心。奇数核在正中，偶数核偏右下
	const int cx = kw / 2;
	cv::Mat out = cv::Mat::zeros(n, n, CV_64F);
	for (int y = 0; y < n; ++y)
	{
		for (int x = 0; x < n; ++x)
		{
			double s = 0.0;
			for (int ky = 0; ky < kh; ++ky)
			{
				const int yy = y + ky - cy; // 核中心对准当前像素 (x, y)
				if (yy < 0 || yy >= n)
					continue; // 越界当 0，所以这是线性卷积
				for (int kx = 0; kx < kw; ++kx)
				{
					const int xx = x + kx - cx;
					if (xx < 0 || xx >= n)
						continue;
					s += src.at<double>(yy, xx) * kernel.at<double>(ky, kx); // Σ f h
				}
			}
			out.at<double>(y, x) = s;
		}
	}
	return out;
}

// DFT 相乘对应循环卷积。核中心 (cx, cy) 先卷绕到 (0,0)，结果才和空域“中心对齐”的卷积一致。
// 线性卷积的支撑大约是 N+K−1。补零到这个长度以上的 2 的幂后，循环卷绕落在补上的 0 里，
// 裁回左上角 N×N 就等于边界补 0 的线性卷积。
cv::Mat convolveDft(const cv::Mat &src, const cv::Mat &kernel, bool linearPad)
{
	const int n = src.rows;
	const int kh = kernel.rows;
	const int kw = kernel.cols;
	const int cy = kh / 2;
	const int cx = kw / 2;
	const int N = linearPad ? nextPow2(n + std::max(kh, kw) - 1) : n; // 线性卷积至少要 N+K−1
	cv::Mat reA = cv::Mat::zeros(N, N, CV_64F);
	cv::Mat imA = cv::Mat::zeros(N, N, CV_64F);
	cv::Mat reB = cv::Mat::zeros(N, N, CV_64F);
	cv::Mat imB = cv::Mat::zeros(N, N, CV_64F);
	src.copyTo(reA(cv::Rect(0, 0, n, n)));
	for (int y = 0; y < kh; ++y)
	{
		for (int x = 0; x < kw; ++x)
		{
			// 空域滤波是相关：out(y,x)=Σ k(ky,kx) f(y+ky−cy, x+kx−cx)。
			// 循环卷积是 Σ h(i,j) f(y−i, x−j)。要使二者一致，应把 k(ky,kx) 放到
			// (cy−ky, cx−kx)，而不能放到 (ky−cy, kx−cx)。偶对称核两者相同；
			// 梯度等奇对称核若放反，结果会整体反号。
			const int yy = wrapIndex(cy - y, N);
			const int xx = wrapIndex(cx - x, N);
			reB.at<double>(yy, xx) = kernel.at<double>(y, x);
		}
	}
	fft2(reA, imA, false);
	fft2(reB, imB, false);
	for (int i = 0; i < N * N; ++i)
	{
		const double a = reA.at<double>(i); // 图像频谱 F
		const double b = imA.at<double>(i);
		const double c = reB.at<double>(i); // 核的频谱 H
		const double d = imB.at<double>(i);
		reA.at<double>(i) = a * c - b * d; // Re(F H)
		imA.at<double>(i) = a * d + b * c; // Im(F H)
	}
	fft2(reA, imA, true); // 这里没有中心化，逆变换直接得到循环卷积
	return reA(cv::Rect(0, 0, n, n)).clone(); // 丢掉补零区域
}

// 小核嵌进 n×n，中心卷到 (0,0) 后做中心化 DFT。实偶核的虚部应接近 0。
cv::Mat kernelSpectrumRe(const cv::Mat &kernel, int n, cv::Mat &re, cv::Mat &im)
{
	re = cv::Mat::zeros(n, n, CV_64F);
	im = cv::Mat::zeros(n, n, CV_64F);
	const int cy = kernel.rows / 2;
	const int cx = kernel.cols / 2;
	for (int y = 0; y < kernel.rows; ++y)
	{
		for (int x = 0; x < kernel.cols; ++x)
		{
			const int yy = wrapIndex(cy - y, n); // 与 convolveDft 相同，保证奇对称核符号正确
			const int xx = wrapIndex(cx - x, n);
			re.at<double>(yy, xx) = kernel.at<double>(y, x);
		}
	}
	modulate(re, im); // 再乘 (−1)^{x+y}，频谱直流才落到中心，便于和图像频谱对照
	fft2(re, im, false);
	return re;
}

// 一维卷积，核中心在 nh/2。circular 为 false 时越界不加；为 true 时按下标取模。
// 例子 h = [1,2,1]，中心是 2。x 的第一个样本 1 与 h 左端相乘时，
// 线性卷积没有左邻，循环卷积会用到 x 的最后一个样本 5，所以 4 变成 9。
void fillConv1(const double *x, int nx, const double *h, int nh, bool circular, double *y)
{
	const int c = nh / 2;
	for (int i = 0; i < nx; ++i)
	{
		double s = 0.0;
		for (int k = 0; k < nh; ++k)
		{
			int j = i + k - c; // 核中心对准位置 i
			if (circular)
				j = wrapIndex(j, nx); // 越界从另一端卷回来
			if (j >= 0 && j < nx)
				s += x[j] * h[k];
		}
		y[i] = s;
	}
}

void conv1dExample(double linear[5], double circular[5], double padded[5])
{
	const double x[5] = {1, 2, 3, 4, 5};
	const double h[3] = {1, 2, 1};
	fillConv1(x, 5, h, 3, false, linear);
	fillConv1(x, 5, h, 3, true, circular);
	const double xp[7] = {0, 1, 2, 3, 4, 5, 0}; // 两端各补一个 0，长度 5+3−1
	double full[7];
	fillConv1(xp, 7, h, 3, true, full);
	for (int i = 0; i < 5; ++i)
		padded[i] = full[i + 1]; // 核中心落在补零序列的下标 1，有效结果从这里取 5 个
}

// 输出 (x,y) 取自原图 (x−dx, y−dy)，下标对 n 取模，所以移出右边的像素从左边回来。
cv::Mat circShift(const cv::Mat &src, int dx, int dy)
{
	const int n = src.rows;
	cv::Mat out(n, n, CV_64F);
	for (int y = 0; y < n; ++y)
	{
		const int ys = wrapIndex(y - dy, n); // 正的 dy 把内容向下移，移出底边的从顶边进入
		for (int x = 0; x < n; ++x)
			out.at<double>(y, x) = src.at<double>(ys, wrapIndex(x - dx, n));
	}
	return out;
}

double sampleBilinear(const cv::Mat &src, double x, double y)
{
	const int n = src.rows;
	if (x < 0 || y < 0 || x > n - 1 || y > n - 1)
		return 0.0; // 旋转出界填 0
	const int x0 = static_cast<int>(std::floor(x));
	const int y0 = static_cast<int>(std::floor(y));
	const int x1 = std::min(x0 + 1, n - 1);
	const int y1 = std::min(y0 + 1, n - 1);
	const double tx = x - x0;
	const double ty = y - y0;
	const double i00 = src.at<double>(y0, x0);
	const double i10 = src.at<double>(y0, x1);
	const double i01 = src.at<double>(y1, x0);
	const double i11 = src.at<double>(y1, x1);
	return (1 - tx) * (1 - ty) * i00 + tx * (1 - ty) * i10 + (1 - tx) * ty * i01 + tx * ty * i11; // 四个邻点按面积加权
}

// 绕中心逆时针旋转。对输出像素 (x,y) 做逆映射，到原图 (xs,ys) 双线性采样。
// 正映射会留下空洞，逆映射每个输出像素都有值。出界填 0。
cv::Mat rotateImage(const cv::Mat &src, double degrees)
{
	const int n = src.rows;
	const double rad = degrees * kPi / 180.0;
	const double c = std::cos(rad);
	const double s = std::sin(rad);
	const double cx = (n - 1) / 2.0;
	cv::Mat out(n, n, CV_64F);
	for (int y = 0; y < n; ++y)
	{
		const double dy = y - cx;
		for (int x = 0; x < n; ++x)
		{
			const double dx = x - cx;
			const double xs = c * dx - s * dy + cx; // 逆映射：输出点回到原图的位置
			const double ys = s * dx + c * dy + cx; // +90° 时右边的点来自下方，画面上表现为逆时针
			out.at<double>(y, x) = sampleBilinear(src, xs, ys);
		}
	}
	return out;
}

// 在离开直流的一个环带里，沿每条射线累加 |F|²。0° 指向右方，角度增大指向下方。
// 条纹的频谱能量集中在与条纹垂直的方向上。水平条纹的能量在竖直轴，读数接近 90°。
// 图像逆时针转 θ 后，屏幕坐标下的读数减小 θ，所以变化量要用 a0 − a1。
int dominantAngle(const cv::Mat &re, const cv::Mat &im)
{
	const int n = re.rows;
	const double c = n / 2.0;
	const int r0 = std::max(4, static_cast<int>(std::lround(n * 0.08))); // 避开直流附近的大能量
	const int r1 = static_cast<int>(std::lround(n * 0.42));				// 也不用到频谱最外圈
	double bestE = -1.0;
	int bestA = 0;
	for (int a = 0; a < 180; ++a) // 0° 和 180° 是同一条直线，所以只扫半圈
	{
		const double rad = a * kPi / 180.0;
		const double dx = std::cos(rad);
		const double dy = std::sin(rad);
		double e = 0.0;
		for (int r = r0; r < r1; r += 2)
		{
			const int u = static_cast<int>(std::lround(c + r * dx));
			const int v = static_cast<int>(std::lround(c + r * dy));
			if (u < 0 || v < 0 || u >= n || v >= n)
				continue;
			const double rr = re.at<double>(v, u);
			const double ii = im.at<double>(v, u);
			e += rr * rr + ii * ii; // 沿这条射线累加 |F|²
		}
		if (e > bestE)
		{
			bestE = e;
			bestA = a;
		}
	}
	return bestA;
}

// 两个正弦的频率都是 frequency 周 / 整幅图。幅度 amplitude 直接加在灰度上，可能超出 0～255，保存时再截断。
cv::Mat addPeriodicNoise(const cv::Mat &src, int frequency, double amplitude)
{
	const int n = src.rows;
	cv::Mat out(n, n, CV_64F);
	for (int y = 0; y < n; ++y)
	{
		for (int x = 0; x < n; ++x)
		{
			out.at<double>(y, x) = src.at<double>(y, x)
				+ amplitude * std::sin(2.0 * kPi * frequency * x / n) // 沿 x 变化：竖直条纹
				+ amplitude * std::sin(2.0 * kPi * frequency * y / n); // 沿 y 变化：水平条纹
		}
	}
	return out;
}

namespace
{

double cosTable[8][8];
bool cosReady = false;

void ensureCos()
{
	if (cosReady)
		return;
	for (int u = 0; u < 8; ++u)
		for (int x = 0; x < 8; ++x)
			cosTable[u][x] = std::cos((2.0 * x + 1.0) * u * kPi / 16.0); // cos((2x+1) u π / 16)
	cosReady = true;
}

// JPEG 的一维 DCT-II：
//   C(0) = 1/√2，其余 C(u) = 1
//   F(u) = (1/2) C(u) Σ_x f(x) cos((2x+1) u π / 16)
// 先对 8 行、再对 8 列各做一次，二维系数前的因子是 1/4。
void dct8(const double in[8], double out[8])
{
	for (int u = 0; u < 8; ++u)
	{
		const double cu = (u == 0) ? 0.7071067811865476 : 1.0; // C(0) = 1/√2，使 DCT 正交
		double s = 0.0;
		for (int x = 0; x < 8; ++x)
			s += in[x] * cosTable[u][x];
		out[u] = 0.5 * cu * s; // 一维前面的 1/2
	}
}

void idct8(const double in[8], double out[8])
{
	for (int x = 0; x < 8; ++x)
	{
		double s = 0.0;
		for (int u = 0; u < 8; ++u)
		{
			const double cu = (u == 0) ? 0.7071067811865476 : 1.0;
			s += cu * in[u] * cosTable[u][x]; // 反变换把 C(u) 乘回去
		}
		out[x] = 0.5 * s; // 一维反变换的 1/2。二维先列后行各做一次，和正变换的两个 1/2 抵消
	}
}

// JPEG zigzag：从直流 (0,0) 出发，沿反对角线来回折返。
// 序号越小，频率越低。只保留前 K 个，就是丢掉高频细节。
void zigzag(int coord[64][2])
{
	int t = 0;
	for (int s = 0; s <= 14; ++s)
	{
		if (s % 2 == 0) // 偶数对角线从左下扫到右上
		{
			for (int i = 0; i <= s; ++i)
			{
				const int u = i;	 // 列，频率 u
				const int v = s - i; // 行，频率 v
				if (u < 8 && v < 8)
				{
					coord[t][0] = u;
					coord[t][1] = v;
					++t;
				}
			}
		}
		else
		{
			for (int i = 0; i <= s; ++i)
			{
				const int u = s - i;
				const int v = i;
				if (u < 8 && v < 8)
				{
					coord[t][0] = u;
					coord[t][1] = v;
					++t;
				}
			}
		}
	}
}

} // namespace

cv::Mat compressDct(const cv::Mat &gray, int keep, int blockX, int blockY,
					double &psnr, cv::Mat *coeff)
{
	ensureCos();
	int zig[64][2];
	zigzag(zig);
	const int n = gray.rows;
	const int k = std::max(1, std::min(64, keep));
	cv::Mat out(n, n, CV_64F);
	if (coeff)
		*coeff = cv::Mat::zeros(8, 8, CV_64F);
	double mse = 0.0;
	for (int by = 0; by < n; by += 8)
	{
		for (int bx = 0; bx < n; bx += 8)
		{
			double rowT[8][8];
			double coef[8][8];
			double kept[8][8];
			double rec[8][8];
			double tmp[8];
			double col[8];
			for (int y = 0; y < 8; ++y)
			{
				for (int x = 0; x < 8; ++x)
					tmp[x] = gray.at<double>(by + y, bx + x);
				dct8(tmp, rowT[y]); // 先对每一行做 DCT
			}
			for (int u = 0; u < 8; ++u)
			{
				for (int y = 0; y < 8; ++y)
					col[y] = rowT[y][u];
				dct8(col, tmp); // 再对每一列做 DCT，得到二维系数
				for (int v = 0; v < 8; ++v)
					coef[v][u] = tmp[v];
			}
			for (int v = 0; v < 8; ++v)
				for (int u = 0; u < 8; ++u)
					kept[v][u] = 0.0;
			// zigzag 下标是 (u, v) = (列, 行)。只留下前 k 个，其余系数置 0。
			for (int t = 0; t < k; ++t)
				kept[zig[t][1]][zig[t][0]] = coef[zig[t][1]][zig[t][0]];
			if (coeff && bx == blockX && by == blockY)
			{
				for (int v = 0; v < 8; ++v)
					for (int u = 0; u < 8; ++u)
						coeff->at<double>(v, u) = coef[v][u];
			}
			for (int u = 0; u < 8; ++u)
			{
				for (int v = 0; v < 8; ++v)
					col[v] = kept[v][u];
				idct8(col, tmp); // 反变换先走列
				for (int y = 0; y < 8; ++y)
					rec[y][u] = tmp[y];
			}
			for (int y = 0; y < 8; ++y)
			{
				idct8(rec[y], tmp); // 再走行，回到空域
				for (int x = 0; x < 8; ++x)
				{
					const double val = tmp[x];
					const double src = gray.at<double>(by + y, bx + x);
					out.at<double>(by + y, bx + x) = val;
					const double d = val - src;
					mse += d * d;
				}
			}
		}
	}
	mse /= static_cast<double>(n * n);
	// PSNR = 10 log10(255² / MSE)。系数全部保留时误差是舍入误差，记成一个很大的数，作业里显示为 inf。
	psnr = mse < 1e-8 ? 1e9 : 10.0 * std::log10(255.0 * 255.0 / mse);
	return out;
}

cv::Mat makeKernel(int rows, int cols, const double *v)
{
	cv::Mat k(rows, cols, CV_64F);
	for (int i = 0; i < rows * cols; ++i)
		k.at<double>(i) = v[i];
	return k;
}

cv::Mat smoothKernel()
{
	const double v[9] = {0, 1.0 / 6, 0, 1.0 / 6, 1.0 / 3, 1.0 / 6, 0, 1.0 / 6, 0}; // 按行存放，和为 1
	return makeKernel(3, 3, v);
}

cv::Mat sharpenKernel()
{
	const double v[9] = {-1, -1, -1, -1, 9, -1, -1, -1, -1}; // 和为 1，直流增益为 1
	return makeKernel(3, 3, v);
}

cv::Mat laplacianKernel()
{
	const double v[9] = {0, 1, 0, 1, -4, 1, 0, 1, 0}; // 和为 0，直流增益为 0
	return makeKernel(3, 3, v);
}

cv::Mat sobelKernel()
{
	const double v[9] = {-1, 0, 1, -2, 0, 2, -1, 0, 1}; // 水平差分，响应竖直边缘
	return makeKernel(3, 3, v);
}

cv::Mat gaussianKernel(int k, double sigma)
{
	const int odd = (k % 2 == 0) ? (k + 1) : k; // 保证奇数边长，几何中心落在像素上
	const int c = odd / 2;
	const double s2 = 2.0 * sigma * sigma;
	cv::Mat ker(odd, odd, CV_64F);
	double sum = 0.0;
	for (int y = 0; y < odd; ++y)
	{
		for (int x = 0; x < odd; ++x)
		{
			const double dy = y - c;
			const double dx = x - c;
			const double v = std::exp(-(dx * dx + dy * dy) / s2);
			ker.at<double>(y, x) = v;
			sum += v;
		}
	}
	ker /= sum; // 系数和为 1，直流增益为 1
	return ker;
}

cv::Mat horizGradKernel()
{
	const double v[9] = {-1, 0, 1, -2, 0, 2, -1, 0, 1}; // 与 Sobel 相同：水平梯度，突出竖直边
	return makeKernel(3, 3, v);
}

cv::Mat boxKernel(int k)
{
	return cv::Mat(k, k, CV_64F, cv::Scalar(1.0 / (k * k))).clone(); // 必须 clone，否则各像素共用同一块内存
}

cv::Mat motionKernel(int k)
{
	return cv::Mat(1, k, CV_64F, cv::Scalar(1.0 / k)).clone(); // 1 行 k 列，只在水平方向模糊
}

// 一层二维哈尔（Haar）离散小波分解。
// 将输入图像划分为互不重叠的 2×2 像素块，设四个像素依次为左上 a、右上 b、左下 c、右下 d，
// 则低频近似子带 LL = (a+b+c+d)/4，与原图像亮度同一尺度；
// 水平细节 LH = (a+b−c−d)/4，竖直细节 HL = (a−b+c−d)/4，对角细节 HH = (a−b−c+d)/4。
// 逆变换把四个子带直接相加，即可无误差地恢复原图像。这样彩色图像才能直接替换低频。
void haarDwt2(const cv::Mat &src, cv::Mat &LL, cv::Mat &LH, cv::Mat &HL, cv::Mat &HH)
{
	const int h = src.rows / 2;
	const int w = src.cols / 2;
	LL.create(h, w, CV_64F);
	LH.create(h, w, CV_64F);
	HL.create(h, w, CV_64F);
	HH.create(h, w, CV_64F);
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const double a = src.at<double>(2 * y, 2 * x);
			const double b = src.at<double>(2 * y, 2 * x + 1);
			const double c = src.at<double>(2 * y + 1, 2 * x);
			const double d = src.at<double>(2 * y + 1, 2 * x + 1);
			LL.at<double>(y, x) = 0.25 * (a + b + c + d);
			LH.at<double>(y, x) = 0.25 * (a + b - c - d); // 竖直方向差分 → 水平边缘
			HL.at<double>(y, x) = 0.25 * (a - b + c - d); // 水平方向差分 → 竖直边缘
			HH.at<double>(y, x) = 0.25 * (a - b - c + d); // 对角细节
		}
	}
}

void haarIdwt2(const cv::Mat &LL, const cv::Mat &LH, const cv::Mat &HL, const cv::Mat &HH, cv::Mat &out)
{
	const int h = LL.rows;
	const int w = LL.cols;
	out.create(h * 2, w * 2, CV_64F);
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const double ll = LL.at<double>(y, x);
			const double lh = LH.at<double>(y, x);
			const double hl = HL.at<double>(y, x);
			const double hh = HH.at<double>(y, x);
			out.at<double>(2 * y, 2 * x) = ll + lh + hl + hh;
			out.at<double>(2 * y, 2 * x + 1) = ll + lh - hl - hh;
			out.at<double>(2 * y + 1, 2 * x) = ll - lh + hl - hh;
			out.at<double>(2 * y + 1, 2 * x + 1) = ll - lh - hl + hh;
		}
	}
}

cv::Mat makeStripes(int n)
{
	cv::Mat g(n, n, CV_64F, cv::Scalar(235));
	for (int y = 0; y < n; ++y)
	{
		for (int x = 0; x < n; ++x)
		{
			if (x < 16)
				g.at<double>(y, x) = 25; // 左边一条竖带，频谱能量主要在水平方向
			if (y % 16 < 3)
				g.at<double>(y, x) = 15; // 周期水平条纹，频谱能量在竖直轴，主方向约 90°
			if (x > n * 0.78 && x < n * 0.78 + 14 && y > 36 && y < n - 36)
				g.at<double>(y, x) = 40; // 右侧短竖线，旋转后便于肉眼核对方向
		}
	}
	return g;
}

double maxAbsDiff(const cv::Mat &a, const cv::Mat &b)
{
	double m = 0.0;
	for (int i = 0; i < a.rows * a.cols; ++i)
		m = std::max(m, std::abs(a.at<double>(i) - b.at<double>(i)));
	return m;
}

double maxMagnitudeDiff(const cv::Mat &reA, const cv::Mat &imA,
						const cv::Mat &reB, const cv::Mat &imB)
{
	double m = 0.0;
	for (int i = 0; i < reA.rows * reA.cols; ++i)
	{
		const double d = std::abs(std::hypot(reA.at<double>(i), imA.at<double>(i)) -
								  std::hypot(reB.at<double>(i), imB.at<double>(i)));
		m = std::max(m, d);
	}
	return m;
}

cv::Mat toU8Clip(const cv::Mat &src)
{
	cv::Mat u8;
	src.convertTo(u8, CV_8U); // 小于 0 变成 0，大于 255 变成 255
	return u8;
}

cv::Mat toU8Stretch(const cv::Mat &src)
{
	double mn = 0, mx = 0;
	cv::minMaxLoc(src, &mn, &mx);
	cv::Mat u8;
	if (mx - mn < 1e-12)
		return cv::Mat::zeros(src.size(), CV_8U);
	src.convertTo(u8, CV_8U, 255.0 / (mx - mn), -mn * 255.0 / (mx - mn)); // (v − min) / (max − min) × 255
	return u8;
}

cv::Mat toU8Signed(const cv::Mat &src)
{
	double mn = 0, mx = 0;
	cv::minMaxLoc(src, &mn, &mx);
	const double a = std::max(std::abs(mn), std::abs(mx));
	cv::Mat shifted = src * (a < 1e-12 ? 0.0 : 127.0 / a) + 128.0; // 0 映射到 128，正负对称
	cv::Mat u8;
	shifted.convertTo(u8, CV_8U);
	return u8;
}

cv::Mat logMagnitude(const cv::Mat &re, const cv::Mat &im)
{
	cv::Mat mag(re.size(), CV_64F);
	for (int i = 0; i < re.rows * re.cols; ++i)
		mag.at<double>(i) = std::log(1.0 + std::hypot(re.at<double>(i), im.at<double>(i))); // log(1+|F|)，压缩直流
	return toU8Stretch(mag);
}

cv::Mat phaseImage(const cv::Mat &re, const cv::Mat &im)
{
	cv::Mat ph(re.size(), CV_64F);
	for (int i = 0; i < re.rows * re.cols; ++i)
		ph.at<double>(i) = (std::atan2(im.at<double>(i), re.at<double>(i)) + kPi) / (2.0 * kPi); // [−π, π] 映射到 [0, 1]
	cv::Mat u8;
	ph.convertTo(u8, CV_8U, 255.0);
	return u8;
}

cv::Mat showFilter(const cv::Mat &H)
{
	double mx = 0;
	cv::minMaxLoc(H, nullptr, &mx);
	if (mx < 1.0)
		mx = 1.0; // 低通峰值本来就是 1，不要再被拉亮
	cv::Mat u8;
	H.convertTo(u8, CV_8U, 255.0 / mx);
	return u8;
}

} // namespace freq
} // namespace kongzi
} // namespace cvrsg
