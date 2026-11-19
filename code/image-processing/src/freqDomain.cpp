#include "freqDomain.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <vector>

// 这一份是四个例子共用的算法。
// 例1 的二维条纹、例2 的相位交换，调用 centeredDft、mixMagPhase、synthesize。
// 例3 的空域对照和两种卷积核，调用 convolveSpatial、gaussianKernel、horizGradKernel；二维 FFT 也在这里。
// 例4 的分解和重构，调用 haarDwt2、haarIdwt2。
// 逐步实验流程写在 demos/freq_homework.cpp，读例子时两边对照即可。

namespace cvrsg
{
namespace kongzi
{
namespace freq
{

namespace
{

const double kPi = 3.14159265358979323846;

// 基 2 一维 FFT。数组长度必须是 2 的整数次幂，计算才能不断对半拆开。
// 正变换的旋转因子是 exp(−j 2π k / N)，算完不除以 N。
// 逆变换用相反的旋转方向，并在最后把每个数除以 N。这样正变换和逆变换互为反变换。
void fft1(std::vector<std::complex<double>> &a, bool invert)
{
	const int n = static_cast<int>(a.size());
	// 倒位序：把下标写成二进制后左右翻转，再按新下标重排。
	// 例如长度 8 时，下标 1（001）和 4（100）交换。重排之后，蝶形可以按「先短后长」原地合并。
	for (int i = 1, j = 0; i < n; ++i)
	{
		int bit = n >> 1; // 从最高位开始，找 j 末尾有多少个连续的 1
		for (; j & bit; bit >>= 1)
			j ^= bit; // 这些位原来是 1，翻成 0，相当于二进制进位
		j ^= bit;	  // 当前这一位翻成 1，此时 j 就是 i 的倒位序
		if (i < j)	  // 每一对只交换一次，避免换回去
			std::swap(a[i], a[j]);
	}
	// 蝶形。len 从 2 加倍到 n。每一层把两个长度为 len/2 的变换，合成一个长度为 len 的变换。
	// 上半段记为 u，下半段先乘旋转因子再记为 v，输出就是 u+v 和 u−v。
	for (int len = 2; len <= n; len <<= 1)
	{
		const double ang = (2.0 * kPi / len) * (invert ? 1.0 : -1.0); // 正变换取负角度
		const std::complex<double> wlen(std::cos(ang), std::sin(ang)); // 这一层的基本旋转因子 W
		for (int i = 0; i < n; i += len)
		{
			std::complex<double> w(1.0, 0.0); // k = 0 时 W^0 = 1，不必旋转
			const int half = len >> 1;
			for (int k = 0; k < half; ++k)
			{
				const std::complex<double> u = a[i + k];			   // 上半段
				const std::complex<double> v = a[i + k + half] * w; // 下半段乘 W^k
				a[i + k] = u + v;
				a[i + k + half] = u - v;
				w *= wlen; // 下一次循环用 W^{k+1}
			}
		}
	}
	if (invert)
	{
		for (int i = 0; i < n; ++i)
			a[i] /= static_cast<double>(n); // 一维逆变换的 1/N。二维会对行、列各做一次，合计除以 N²
	}
}

// 把每个像素乘上 (−1)^{x+y}。x+y 为偶数时这个因子是 +1，为奇数时是 −1。
// 它的作用等价于把频谱平移 N/2，直流分量从左上角 (0,0) 移到矩阵中心。
// 这个因子再乘一次就变回 1，所以逆变换之后再乘一次，可以把直流移回左上角。
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

// 灰度图每个像素都是实数，所以虚部先全部写成 0。
// 必须先做 (−1)^{x+y}，再做正变换，直流才会落在矩阵中心，幅度谱看起来才是「低频在中间」。
void dftFromGray(const cv::Mat &gray, cv::Mat &re, cv::Mat &im)
{
	re = gray.clone();
	im = cv::Mat::zeros(gray.size(), CV_64F);
	modulate(re, im); // 必须在 FFT 之前做
	fft2(re, im, false);
}

// 中心化频谱的逆变换。先做 IFFT，此时直流仍在左上角；
// 再乘一次 (−1)^{x+y}，抵消正变换之前乘的那一次，得到和原来对齐的空域图像。
// 实图像只保留实部。虚部来自舍入误差，直接丢掉。
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

// 二维 DFT 可以拆开做：先对每一行做一维 DFT，再对每一列做一维 DFT。
// 逆变换时行和列各除以 n 一次，合起来除以 n²，正变换再逆变换才能回到原图。
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
		fft1(line, invert); // 这一行上，列下标是频率的水平序号
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
		fft1(line, invert); // 这一列上，行下标是频率的竖直序号
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

	// 每个频率点都是一个复数 F = |F| · (cos φ + j sin φ)。
	// 例2 用三种改写方式看「轮廓到底留在幅度里还是相位里」。
	for (int v = 0; v < n; ++v)
	{
		for (int u = 0; u < n; ++u)
		{
			const double rr = re.at<double>(v, u);
			const double ii = im.at<double>(v, u);
			const double m = std::hypot(rr, ii); // 幅度 |F| = sqrt(实部² + 虚部²)
			if (mode == MagnitudeOnly)
			{
				// 相位全部写成 0，频谱变成非负实数 |F|。各频率成分都从同一相位起步，对不齐边缘。
				ore.at<double>(v, u) = m;
			}
			else if (mode == PhaseOnly)
			{
				// 幅度全部改成 1，只保留方向 cos φ、sin φ。m 太小的点相位没有意义，保持 0。
				if (m > 1e-8)
				{
					ore.at<double>(v, u) = rr / m;
					oim.at<double>(v, u) = ii / m;
				}
			}
			else
			{
				// 幅度和相位都保留，逆变换应回到原图。
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
	// 新频谱 = 第一幅图的幅度 × 第二幅图的单位相位。
	// 单位相位是 F2 / |F2|，它的模长是 1，方向仍是第二幅图的相位。
	for (int v = 0; v < n; ++v)
	{
		for (int u = 0; u < n; ++u)
		{
			const double m = std::hypot(magRe.at<double>(v, u), magIm.at<double>(v, u));
			const double p = std::hypot(phaseRe.at<double>(v, u), phaseIm.at<double>(v, u));
			if (p < 1e-8)
				continue; // 第二幅图在这个频率上是 0，相位没有定义，结果保持 0
			re.at<double>(v, u) = m * phaseRe.at<double>(v, u) / p;
			im.at<double>(v, u) = m * phaseIm.at<double>(v, u) / p;
		}
	}
	idftToGray(re, im, out);
}

cv::Mat convolveSpatial(const cv::Mat &src, const cv::Mat &kernel)
{
	// 这是例3 用来对照的空域算法。对每个像素，把核的中心对准它，核覆盖到的灰度乘上核的系数再相加。
	// 图像外面的像素当作 0，所以这是线性卷积，不是把图像首尾接起来的循环卷积。
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
	const int odd = (k % 2 == 0) ? (k + 1) : k; // 偶数边长没有唯一的中心像素，这里改成相邻的奇数
	const int c = odd / 2;                      // 中心下标。权重离这个像素越远越小
	const double s2 = 2.0 * sigma * sigma;      // 高斯函数分母里的 2σ²。σ 越大，平滑范围越宽
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
	ker /= sum; // 系数加起来等于 1，整幅图的平均亮度不会被整体抬高或压低
	return ker;
}

cv::Mat horizGradKernel()
{
	// 同一行里，右边像素减左边像素，中间列权重更大。竖直方向的边缘左右灰度差大，响应就强。
	const double v[9] = {-1, 0, 1, -2, 0, 2, -1, 0, 1};
	return makeKernel(3, 3, v);
}

// 一层二维哈尔小波分解。图像边长必须是偶数。
// 每四个互不重叠的像素组成一块，按下面的位置命名：
//   a b
//   c d
// LL = (a+b+c+d)/4，是这一小块的平均亮度，和原图用同一套灰度单位，后面才能用彩色图直接替换。
// LH = (a+b−c−d)/4，上下两行相减，响应水平方向的边缘。
// HL = (a−b+c−d)/4，左右两列相减，响应竖直方向的边缘。
// HH = (a−b−c+d)/4，对角方向的细节。
// 四个式子都除以 4，逆变换就可以把四个子带直接相加，不用再乘别的系数。
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
	// 把分解公式反过来解 a、b、c、d。四个子带都没有被替换时，这里应精确回到原图。
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
			out.at<double>(2 * y, 2 * x) = ll + lh + hl + hh;             // 还原左上 a
			out.at<double>(2 * y, 2 * x + 1) = ll + lh - hl - hh;       // 还原右上 b
			out.at<double>(2 * y + 1, 2 * x) = ll - lh + hl - hh;       // 还原左下 c
			out.at<double>(2 * y + 1, 2 * x + 1) = ll - lh - hl + hh;   // 还原右下 d
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
	src.convertTo(u8, CV_8U); // 小于 0 的写成 0，大于 255 的写成 255。适合本来就在 0～255 的灰度
	return u8;
}

cv::Mat toU8Stretch(const cv::Mat &src)
{
	double mn = 0, mx = 0;
	cv::minMaxLoc(src, &mn, &mx);
	cv::Mat u8;
	if (mx - mn < 1e-12)
		return cv::Mat::zeros(src.size(), CV_8U);
	src.convertTo(u8, CV_8U, 255.0 / (mx - mn), -mn * 255.0 / (mx - mn)); // 最小值映到 0，最大值映到 255
	return u8;
}

cv::Mat toU8Signed(const cv::Mat &src)
{
	double mn = 0, mx = 0;
	cv::minMaxLoc(src, &mn, &mx);
	const double a = std::max(std::abs(mn), std::abs(mx));
	cv::Mat shifted = src * (a < 1e-12 ? 0.0 : 127.0 / a) + 128.0; // 0 写成中灰 128，正值更亮，负值更暗
	cv::Mat u8;
	shifted.convertTo(u8, CV_8U);
	return u8;
}

cv::Mat logMagnitude(const cv::Mat &re, const cv::Mat &im)
{
	cv::Mat mag(re.size(), CV_64F);
	for (int i = 0; i < re.rows * re.cols; ++i)
		mag.at<double>(i) = std::log(1.0 + std::hypot(re.at<double>(i), im.at<double>(i))); // log(1+|F|) 把特别亮的直流压下来，旁边的频率才看得见
	return toU8Stretch(mag);
}

cv::Mat phaseImage(const cv::Mat &re, const cv::Mat &im)
{
	cv::Mat ph(re.size(), CV_64F);
	for (int i = 0; i < re.rows * re.cols; ++i)
		ph.at<double>(i) = (std::atan2(im.at<double>(i), re.at<double>(i)) + kPi) / (2.0 * kPi); // 相位从 −π～π 改写成 0～1，再存成灰度
	cv::Mat u8;
	ph.convertTo(u8, CV_8U, 255.0);
	return u8;
}

} // namespace freq
} // namespace kongzi
} // namespace cvrsg
