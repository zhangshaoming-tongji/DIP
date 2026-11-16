#include "freqDomain.h"

#include <algorithm>
#include <cmath>
#include <complex>
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

// 灰度是实数，虚部先置 0。先中心化再正变换，直流落在矩阵中心。
void dftFromGray(const cv::Mat &gray, cv::Mat &re, cv::Mat &im)
{
	re = gray.clone();
	im = cv::Mat::zeros(gray.size(), CV_64F);
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

cv::Mat makeKernel(int rows, int cols, const double *v)
{
	cv::Mat k(rows, cols, CV_64F);
	for (int i = 0; i < rows * cols; ++i)
		k.at<double>(i) = v[i];
	return k;
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
	dftFromGray(gray, re, im);
}

void synthesize(const cv::Mat &re, const cv::Mat &im, cv::Mat &out, int mode)
{
	const int n = re.rows;
	cv::Mat ore = cv::Mat::zeros(n, n, CV_64F);
	cv::Mat oim = cv::Mat::zeros(n, n, CV_64F);

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
	const double v[9] = {-1, 0, 1, -2, 0, 2, -1, 0, 1}; // 水平梯度，突出竖直边
	return makeKernel(3, 3, v);
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

double maxAbsDiff(const cv::Mat &a, const cv::Mat &b)
{
	double m = 0.0;
	for (int i = 0; i < a.rows * a.cols; ++i)
		m = std::max(m, std::abs(a.at<double>(i) - b.at<double>(i)));
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

} // namespace freq
} // namespace kongzi
} // namespace cvrsg
