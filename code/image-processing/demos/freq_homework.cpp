// 第 8 章频域应用实验：随堂演示与四道练习共用这一份程序。
// 读代码时可以先看 main 里标了「例1」到「例4」的四段，再按函数名回到上面的定义。
//
// 例1 频谱泄漏。
//     先做一条余弦序列，再做一幅竖直条纹图。观察窗口里如果刚好放进整数个周期，
//     离散傅里叶变换（DFT）的能量集中在一对谱线上；如果放进的不是整数个周期，
//     把窗口首尾接成一个周期时会在接缝处出现跳变，能量就会散到邻近频率，这叫做频谱泄漏。
// 例2 相位携带图像结构。
//     对 Lena 和摄影师两幅图分别做 DFT。交换幅度谱和相位谱后再做离散傅里叶逆变换（IDFT）。
//     重建出来的轮廓跟着提供相位的那一幅走。
// 例3 用快速傅里叶变换（FFT）做图像滤波。
//     空域卷积等价于频域相乘，但 DFT 默认做的是循环卷积。所以先把图像和卷积核补零，
//     让相邻周期不再重叠，再做 FFT、逐点相乘、快速傅里叶逆变换（IFFT），最后裁回原图大小。
// 例4 用小波分解和重构做遥感图像融合。
//     高分辨率全色图细节清楚但没有颜色，低分辨率彩色图有颜色但比较糊。
//     对全色图做一层二维哈尔（Haar）离散小波变换（DWT），用彩色图替换低频近似子带，
//     保留全色图的三个高频细节子带，再做离散小波逆变换（IDWT）。
//
// 编译：在当前目录运行 build_freq.bat。
// 运行：运行 run_freq.bat，或执行 bin\freq_homework.exe data\ch8。
// 输出：结果图像和 report.txt 写在 data/ch8/out/。

#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <fstream>
#include <iostream>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <sstream>
#include <string>
#include <vector>

#include "freqDomain.h"

namespace
{

const double kPi = 3.14159265358979323846;
const int kN = 256; // 例1 的条纹图、例2 和例3 的 Lena 都统一缩放到这个边长

// 读一张灰度图，缩放到 side×side，再转成双精度浮点。
// 后面的 DFT 和卷积都用浮点做，避免 0～255 的整数在中间步骤被截断。
cv::Mat loadGray(const std::string &path, int side)
{
	cv::Mat src = cv::imread(path, cv::IMREAD_GRAYSCALE);
	if (src.empty())
	{
		std::cout << "无法读取 " << path << std::endl;
		return cv::Mat();
	}
	cv::Mat resized;
	cv::resize(src, resized, cv::Size(side, side));
	cv::Mat gray;
	resized.convertTo(gray, CV_64F);
	return gray;
}

// 读一张彩色图并缩放到 side×side。OpenCV 默认按蓝、绿、红三个通道存放。
cv::Mat loadColor(const std::string &path, int side)
{
	cv::Mat src = cv::imread(path, cv::IMREAD_COLOR);
	if (src.empty())
	{
		std::cout << "无法读取 " << path << std::endl;
		return cv::Mat();
	}
	cv::Mat resized;
	cv::resize(src, resized, cv::Size(side, side));
	return resized;
}

void save(const std::string &path, const cv::Mat &image)
{
	cv::imwrite(path, image);
}

// 把很小或很大的误差写成科学计数法，方便写进 report.txt。
std::string num(double v)
{
	std::ostringstream os;
	os.setf(std::ios::scientific);
	os.precision(3);
	os << v;
	return os.str();
}

// 有限长矩形窗的频谱是狄利克雷（Dirichlet）核，可以看成周期化的 sinc 函数。
// delta 是「当前频率」减去「余弦真正的频率」之后的偏移，单位是 DFT 的频率序号；n 是窗口里的样本个数。
// 偏移正好是 n 的整数倍时，所有样本同相相加，函数值等于 n；偏移是其他整数时，正负样本抵消，函数值为 0。
// 例1 用这条曲线解释：整数周期时，DFT 的采样点刚好落在主瓣顶点和这些零点上。
std::complex<double> dirichletKernel(double delta, int n)
{
	// DFT 的频谱本身以 n 为周期。先把偏移折回最靠近 0 的那一份，避免在周期边界上把峰值误判成 0。
	const double wrapped = delta - n * std::round(delta / n);
	if (std::abs(wrapped) < 1e-9)
		return std::complex<double>(static_cast<double>(n), 0.0);
	// 幅度是 sin(πδ) / sin(πδ/n)。相位来自窗口没有以 0 为中心，而是从 n = 0 开始截取。
	const double mag = std::sin(kPi * delta) / std::sin(kPi * delta / n);
	const double phase = -kPi * delta * (n - 1) / n;
	return std::complex<double>(mag * std::cos(phase), mag * std::sin(phase));
}

// 实余弦可以写成两支复指数之和：一半在正频率，一半在负频率。
// 截断之后，每一支都变成一个狄利克雷核。把两支复数加起来再取模，就是余弦频谱的理论包络。
// k 是 DFT 频率序号，cycles 是窗口内包含的周期个数。
double cosineSpectrumEnvelope(double k, double cycles, int n)
{
	const auto pos = dirichletKernel(k - cycles, n);
	const auto neg = dirichletKernel(k + cycles, n);
	return 0.5 * std::abs(pos + neg);
}

// 把一维序列画成一张小图，只负责显示，不参与后面的计算。
// stem 为真时画频谱常用的竖线；centeredK 为真时，横轴标成 −N/2 到 N/2−1，零频率在正中间。
// samplesEnvelope 为真时，先画连续余弦，再把离散样点 x[n] 画成竖线和圆点，用来看「窗口截到了多少个周期」。
// 画频谱并且给出了 cycles 时，再叠一条狄利克雷核的理论包络，用来对照离散幅度有没有落在零点上。
// OpenCV 自带字体不能写中文，坐标轴上仍用 n、k、x[n]、|X[k]| 这些符号。
cv::Mat plotCurve(const std::vector<double> &y, bool stem, double ymin, double ymax,
				  const std::string &xlabel, const std::string &ylabel,
				  bool centeredK = false, bool samplesEnvelope = false, double cycles = 0.0)
{
	const int W = 720;
	const int H = 280;
	cv::Mat canvas(H, W, CV_8UC3, cv::Scalar(255, 255, 255));
	const int padL = 56;
	const int padR = 16;
	const int padT = 16;
	const int padB = 36;
	const int plotW = W - padL - padR;
	const int plotH = H - padT - padB;
	const int n = static_cast<int>(y.size());
	if (ymax <= ymin)
	{
		ymax = 1e-12;
		for (double v : y)
			ymax = std::max(ymax, v);
		ymax *= 1.08;
		if (ymax <= ymin)
			ymax = ymin + 1.0;
	}
	auto xOf = [&](int i) {
		if (n <= 1)
			return padL + plotW / 2;
		return padL + static_cast<int>(std::lround(static_cast<double>(i) / (n - 1) * plotW));
	};
	auto yOf = [&](double v) {
		return padT + static_cast<int>(std::lround((ymax - v) / (ymax - ymin) * plotH));
	};
	const int yZero = yOf(0.0);
	if (yZero >= padT && yZero <= padT + plotH)
		cv::line(canvas, cv::Point(padL, yZero), cv::Point(padL + plotW, yZero), cv::Scalar(220, 226, 232), 1);
	cv::rectangle(canvas, cv::Rect(padL, padT, plotW, plotH), cv::Scalar(40, 48, 56), 1);
	if (centeredK)
	{
		const int half = n / 2;
		cv::putText(canvas, std::to_string(-half), cv::Point(padL - 8, padT + plotH + 16),
					cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(90, 100, 110), 1, cv::LINE_AA);
		cv::putText(canvas, "0", cv::Point(xOf(half) - 4, padT + plotH + 16),
					cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(90, 100, 110), 1, cv::LINE_AA);
		cv::putText(canvas, std::to_string(half - 1), cv::Point(padL + plotW - 18, padT + plotH + 16),
					cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(90, 100, 110), 1, cv::LINE_AA);
	}
	const cv::Scalar color = stem || samplesEnvelope ? cv::Scalar(121, 78, 27) : cv::Scalar(121, 78, 27);
	const cv::Scalar stemColor = cv::Scalar(36, 32, 180);
	if (samplesEnvelope)
	{
		const int dense = std::max(n * 8, 2);
		std::vector<cv::Point> env;
		env.reserve(dense);
		for (int i = 0; i < dense; ++i)
		{
			const double t = (n - 1) * static_cast<double>(i) / (dense - 1);
			const double v = std::cos(2.0 * kPi * cycles * t / n);
			const int xx = padL + static_cast<int>(std::lround(t / (n - 1) * plotW));
			env.emplace_back(xx, yOf(v));
		}
		for (int i = 1; i < dense; ++i)
			cv::line(canvas, env[i - 1], env[i], cv::Scalar(190, 176, 155), 1, cv::LINE_AA);
		const int base = yOf(0.0);
		for (int i = 0; i < n; ++i)
		{
			const int xx = xOf(i);
			const int yy = yOf(y[i]);
			cv::line(canvas, cv::Point(xx, base), cv::Point(xx, yy), color, 1, cv::LINE_AA);
			cv::circle(canvas, cv::Point(xx, yy), 2, color, cv::FILLED, cv::LINE_AA);
		}
	}
	else if (stem)
	{
		if (centeredK && cycles > 0.0)
		{
			const int dense = std::max(n * 8, 2);
			const double kLeft = -n / 2.0;
			const double kRight = n / 2.0 - 1.0;
			cv::Point prev;
			for (int i = 0; i < dense; ++i)
			{
				const double t = static_cast<double>(i) / (dense - 1);
				const double k = kLeft + t * (kRight - kLeft);
				const double v = cosineSpectrumEnvelope(k, cycles, n);
				const int xx = padL + static_cast<int>(std::lround(t * plotW));
				const cv::Point cur(xx, yOf(v));
				if (i > 0)
					cv::line(canvas, prev, cur, cv::Scalar(180, 140, 60), 1, cv::LINE_AA);
				prev = cur;
			}
		}
		const int base = yOf(std::max(ymin, 0.0));
		for (int i = 0; i < n; ++i)
		{
			const int x = xOf(i);
			const int yy = yOf(y[i]);
			cv::line(canvas, cv::Point(x, base), cv::Point(x, yy), stemColor, 1, cv::LINE_AA);
			cv::circle(canvas, cv::Point(x, yy), 2, stemColor, cv::FILLED, cv::LINE_AA);
		}
	}
	else
	{
		for (int i = 1; i < n; ++i)
			cv::line(canvas, cv::Point(xOf(i - 1), yOf(y[i - 1])), cv::Point(xOf(i), yOf(y[i])),
					 color, 2, cv::LINE_AA);
	}
	cv::putText(canvas, xlabel, cv::Point(padL + plotW / 2 - 10, H - 8),
				cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(90, 100, 110), 1, cv::LINE_AA);
	cv::putText(canvas, ylabel, cv::Point(6, padT + 16),
				cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(90, 100, 110), 1, cv::LINE_AA);
	return canvas;
}

// 一维基 2 快速傅里叶变换。长度必须是 2 的整数次幂。例1 的一维序列用这份副本，二维图像用 freqDomain.cpp 里同一套算法。
// invert 为 false 做正变换，旋转因子是 exp(−j 2π / N)，结果不除以 N。
// invert 为 true 做逆变换，旋转方向相反，最后把每个数除以 N，这样正变换再逆变换能回到原序列。
void fft1Local(std::vector<std::complex<double>> &a, bool invert)
{
	const int n = static_cast<int>(a.size());
	// 倒位序：下标的二进制左右翻转后再重排。例如长度 8 时，1（二进制 001）和 4（二进制 100）交换。
	// 重排之后，下面的蝶形才能从长度为 2 的小组一直合并到整段序列。
	for (int i = 1, j = 0; i < n; ++i)
	{
		int bit = n >> 1;
		for (; j & bit; bit >>= 1)
			j ^= bit;
		j ^= bit;
		if (i < j)
			std::swap(a[i], a[j]);
	}
	// 每一层把序列分成若干段，每段前一半记为 u，后一半乘旋转因子后记为 v。
	// 这一层的两个输出是 u+v 和 u−v。len 从 2 加倍，直到覆盖整段长度 n。
	for (int len = 2; len <= n; len <<= 1)
	{
		const double ang = (2.0 * kPi / len) * (invert ? 1.0 : -1.0);
		const std::complex<double> wlen(std::cos(ang), std::sin(ang));
		for (int i = 0; i < n; i += len)
		{
			std::complex<double> w(1.0, 0.0);
			const int half = len >> 1;
			for (int k = 0; k < half; ++k)
			{
				const auto u = a[i + k];
				const auto v = a[i + k + half] * w;
				a[i + k] = u + v;
				a[i + k + half] = u - v;
				w *= wlen;
			}
		}
	}
	if (invert)
	{
		// 逆变换要除以长度。例1 只用正变换看幅度，这段不会执行。
		for (int i = 0; i < n; ++i)
			a[i] /= static_cast<double>(n);
	}
}

// 对一条实序列做 DFT，返回每个频率序号上的幅度 |X[k]|。
// 输入是实数，虚部先写成 0。幅度是实部和虚部平方和的平方根，它不区分正负相位。
std::vector<double> dftMag1d(const std::vector<double> &x)
{
	std::vector<std::complex<double>> a(x.size());
	for (size_t i = 0; i < x.size(); ++i)
		a[i] = std::complex<double>(x[i], 0.0);
	fft1Local(a, false);
	std::vector<double> mag(a.size());
	for (size_t i = 0; i < a.size(); ++i)
		mag[i] = std::abs(a[i]);
	return mag;
}

// 在长度 n 的窗口里生成一条余弦。cycles 是这个窗口里一共包含多少个周期。
// 采样点写成 cos(2π · cycles · i / n)：cycles 为整数时，第 0 个点和「第 n 个点」相位相同，
// 首尾相接没有跳变；cycles 不是整数时，首尾相位对不上，周期延拓就会产生跳变。
std::vector<double> makeCos1d(int n, double cycles)
{
	std::vector<double> x(n);
	for (int i = 0; i < n; ++i)
		x[i] = std::cos(2.0 * kPi * cycles * i / n);
	return x;
}

// 生成一幅只沿水平方向变化的竖直条纹。每一列灰度相同，沿 x 做余弦。
// 128 把余弦从 −1～1 抬到大约 1～255，才能存成普通灰度图。
// cycles 不是整数时，二维频谱里的亮点会沿水平频率方向散开，和一维泄漏是同一件事。
cv::Mat makeCos2d(int n, double cycles)
{
	cv::Mat g(n, n, CV_64F);
	for (int y = 0; y < n; ++y)
		for (int x = 0; x < n; ++x)
			g.at<double>(y, x) = 128.0 + 127.0 * std::cos(2.0 * kPi * cycles * x / n);
	return g;
}

// DFT 默认把 k = 0 放在数组开头，负频率排在数组后半段。
// 显示时把后半段挪到前面，横轴变成 −N/2 … N/2−1，正频率和负频率就对称地出现在 0 的两侧。
std::vector<double> fftshiftMag(const std::vector<double> &mag)
{
	const int n = static_cast<int>(mag.size());
	const int half = n / 2;
	std::vector<double> out(n);
	for (int i = 0; i < n; ++i)
		out[i] = mag[(i + half) % n];
	return out;
}

// 从 1 开始不断乘 2，直到不小于 v。FFT 要求边长是 2 的整数次幂。
int nextPow2(int v)
{
	int p = 1;
	while (p < v)
		p <<= 1;
	return p;
}

// 把可能为负的下标折回 0 … n−1。补零后的卷积核有一部分会卷到矩阵右下角，要用这个下标。
int wrapIndex(int i, int n)
{
	int r = i % n;
	if (r < 0)
		r += n;
	return r;
}

// 例3 的五步都写在这个函数里，每一步的中间图像都会存盘。
// 空域直接卷积用来核对：两边的最大绝对差应接近 0，说明补零之后的循环卷积等于线性卷积。
cv::Mat filterByFftPipeline(const cv::Mat &src, const cv::Mat &kernel, const std::string &tag,
							const std::string &outDir, double &errVsSpatial)
{
	const int n = src.rows;
	const int kh = kernel.rows;
	const int kw = kernel.cols;
	const int cy = kh / 2; // 核的中心行。奇数边长时正好落在正中间那个像素
	const int cx = kw / 2;
	// 两个长度分别为 n 和 k 的序列，线性卷积结果长度是 n+k−1。
	// 这里取不小于「原图边长 + 核边长 − 1」的最小 2 的整数次幂，图像和核都补到这个正方形上。
	const int N = nextPow2(n + std::max(kh, kw) - 1);

	cv::Mat reA = cv::Mat::zeros(N, N, CV_64F);
	cv::Mat imA = cv::Mat::zeros(N, N, CV_64F);
	cv::Mat reB = cv::Mat::zeros(N, N, CV_64F);
	cv::Mat imB = cv::Mat::zeros(N, N, CV_64F);

	// 第 1 步，图像：原图放在左上角，右边和下边都是 0。这些 0 把相邻周期隔开。
	src.copyTo(reA(cv::Rect(0, 0, n, n)));
	// 第 1 步，卷积核：频域相乘对应的是循环卷积，卷积会把核翻转一次。
	// 空域代码是「核的中心对准当前像素」再相乘相加，并没有先翻转。
	// 所以这里按中心把核翻转，再把中心卷到左上角原点；翻转到负坐标的那几行、几列会出现在矩阵右下角。
	for (int y = 0; y < kh; ++y)
	{
		for (int x = 0; x < kw; ++x)
		{
			const int yy = wrapIndex(cy - y, N);
			const int xx = wrapIndex(cx - x, N);
			reB.at<double>(yy, xx) = kernel.at<double>(y, x);
		}
	}

	save(outDir + "\\" + tag + "_padded_image.png", cvrsg::kongzi::freq::toU8Clip(reA));
	save(outDir + "\\" + tag + "_padded_kernel.png", cvrsg::kongzi::freq::toU8Signed(reB));

	// 第 2 步：补零后的图像和卷积核各自做二维 FFT。false 表示正变换。
	cvrsg::kongzi::freq::fft2(reA, imA, false);
	cvrsg::kongzi::freq::fft2(reB, imB, false);
	save(outDir + "\\" + tag + "_fft_image.png", cvrsg::kongzi::freq::logMagnitude(reA, imA));
	save(outDir + "\\" + tag + "_fft_kernel.png", cvrsg::kongzi::freq::logMagnitude(reB, imB));

	// 第 3 步：同一频率上的两个复数相乘。(a+jb)(c+jd) 的实部是 ac−bd，虚部是 ad+bc。
	// 这就是该频率上「图像的频谱系数 × 卷积核的频率响应」。
	for (int i = 0; i < N * N; ++i)
	{
		const double a = reA.at<double>(i);
		const double b = imA.at<double>(i);
		const double c = reB.at<double>(i);
		const double d = imB.at<double>(i);
		reA.at<double>(i) = a * c - b * d;
		imA.at<double>(i) = a * d + b * c;
	}
	save(outDir + "\\" + tag + "_fft_product.png", cvrsg::kongzi::freq::logMagnitude(reA, imA));

	// 第 4 步：true 表示逆变换，乘积从频域回到空域。
	// 第 5 步：线性卷积的有效结果在左上角 n×n。其余位置是补零留出来的边界，这里裁掉。
	cvrsg::kongzi::freq::fft2(reA, imA, true);
	cv::Mat out = reA(cv::Rect(0, 0, n, n)).clone();

	cv::Mat spatial = cvrsg::kongzi::freq::convolveSpatial(src, kernel);
	errVsSpatial = cvrsg::kongzi::freq::maxAbsDiff(out, spatial);
	return out;
}

cv::Mat tile2x2(const cv::Mat &a, const cv::Mat &b, const cv::Mat &c, const cv::Mat &d);

// 例4 的一组融合。pan 是高分辨率全色灰度图，msBgr 是边长只有一半的彩色图。
// 返回值是「全色图分解后再原样重构」的最大绝对误差，用来确认小波正变换和逆变换互为反变换。
double saveWaveletFusion(const cv::Mat &pan, const cv::Mat &msBgr, const std::string &outDir, const std::string &prefix)
{
	// 彩色图长宽都只有全色图的一半。先线性插值放大到同一尺寸，后面才能和全色图的子带对齐比较。
	cv::Mat msUp;
	cv::resize(msBgr, msUp, cv::Size(pan.cols, pan.rows), 0, 0, cv::INTER_LINEAR);

	// 一层二维哈尔小波把全色图拆成四个边长减半的子带：
	// LL 是低频近似，看起来像缩小后的原图；LH、HL、HH 分别是水平、竖直和对角方向的细节。
	cv::Mat panLL, panLH, panHL, panHH;
	cvrsg::kongzi::freq::haarDwt2(pan, panLL, panLH, panHL, panHH);

	// 彩色图有蓝、绿、红三个通道，每个通道单独替换一次低频，高频仍用全色图的细节。
	std::vector<cv::Mat> msCh(3), fusedCh(3);
	cv::split(msUp, msCh);
	for (int c = 0; c < 3; ++c)
	{
		cv::Mat ch64;
		msCh[c].convertTo(ch64, CV_64F);
		// 小波的 LL 边长已经是原图的一半。把该颜色通道缩到和 LL 一样大，数值范围仍是灰度，可以直接替换。
		cv::Mat colorLL;
		cv::resize(ch64, colorLL, panLL.size(), 0, 0, cv::INTER_AREA);
		// 逆变换：低频来自彩色图，三个高频子带来自全色图。
		cvrsg::kongzi::freq::haarIdwt2(colorLL, panLH, panHL, panHH, fusedCh[c]);
	}
	cv::Mat fused64;
	cv::merge(fusedCh, fused64);
	cv::Mat fused8;
	fused64.convertTo(fused8, CV_8U);
	cv::Mat msOnly8;
	msUp.convertTo(msOnly8, CV_8U);

	save(outDir + "\\" + prefix + "_pan.png", cvrsg::kongzi::freq::toU8Clip(pan));
	save(outDir + "\\" + prefix + "_ms_color.png", msBgr);
	save(outDir + "\\" + prefix + "_ms_upsampled.png", msOnly8);
	save(outDir + "\\" + prefix + "_pan_LL.png", cvrsg::kongzi::freq::toU8Stretch(panLL));
	save(outDir + "\\" + prefix + "_pan_LH.png", cvrsg::kongzi::freq::toU8Signed(panLH));
	save(outDir + "\\" + prefix + "_pan_HL.png", cvrsg::kongzi::freq::toU8Signed(panHL));
	save(outDir + "\\" + prefix + "_pan_HH.png", cvrsg::kongzi::freq::toU8Signed(panHH));
	save(outDir + "\\" + prefix + "_subbands.png",
		 tile2x2(cvrsg::kongzi::freq::toU8Stretch(panLL), cvrsg::kongzi::freq::toU8Signed(panLH),
				 cvrsg::kongzi::freq::toU8Signed(panHL), cvrsg::kongzi::freq::toU8Signed(panHH)));
	save(outDir + "\\" + prefix + "_fused.png", fused8);

	cv::Mat panRec;
	cvrsg::kongzi::freq::haarIdwt2(panLL, panLH, panHL, panHH, panRec);
	return cvrsg::kongzi::freq::maxAbsDiff(pan, panRec);
}

cv::Mat tile2x2(const cv::Mat &a, const cv::Mat &b, const cv::Mat &c, const cv::Mat &d)
{
	// 按小波子带的习惯排成一张图：左上 LL，右上 LH，左下 HL，右下 HH。
	cv::Mat top, bottom, all;
	cv::hconcat(a, b, top);
	cv::hconcat(c, d, bottom);
	cv::vconcat(top, bottom, all);
	return all;
}

} // namespace

int main(int argc, char *argv[])
{
	SetConsoleOutputCP(65001);
	const std::string dir = (argc >= 2) ? argv[1] : "data\\ch8";
	const std::string outDir = dir + "\\out";
	CreateDirectoryA(dir.c_str(), NULL);
	CreateDirectoryA(outDir.c_str(), NULL);

	cv::Mat lena = loadGray(dir + "\\lena.png", kN);
	cv::Mat camera = loadGray(dir + "\\cameraman.png", kN);
	cv::Mat pan = loadGray(dir + "\\pan.png", 512);          // 山地全色图，512×512
	cv::Mat msBgr = loadColor(dir + "\\ms_color.png", 256); // 山地彩色图，边长是全色图的一半
	cv::Mat panTown = loadGray(dir + "\\pan_town.png", 512);
	cv::Mat msTown = loadColor(dir + "\\ms_town.png", 256);
	if (lena.empty() || camera.empty() || pan.empty() || msBgr.empty() || panTown.empty() || msTown.empty())
		return 1;

	std::ofstream report(outDir + "\\report.txt");
	report << "第 8 章频域应用：四例实验结果\n";

	// ------------------------------------------------------------------
	// 例1 频谱泄漏
	// 同一条余弦，只改变窗口里包含的周期个数，比较 DFT 幅度是集中还是散开。
	// ------------------------------------------------------------------
	const int n1 = 128; // 窗口长度。必须是 2 的整数次幂，才能用上面的基 2 FFT
	// 8.0 个周期：第 0 点和下一个周期起点相位相同，首尾相接没有跳变。
	const auto cosInt = makeCos1d(n1, 8.0);
	// 8.5 个周期：窗口结束时停在半个周期上，首尾接成一圈时会出现相位跳变。
	const auto cosLeak = makeCos1d(n1, 8.5);
	const auto magInt = dftMag1d(cosInt);
	const auto magLeak = dftMag1d(cosLeak);

	// 先看时域波形，再看把零频率挪到中间之后的幅度谱。整数周期那张谱应只有一对高谱线。
	save(outDir + "\\ex1_1d_integer_signal.png",
		 plotCurve(cosInt, false, -1.25, 1.25, "n", "x[n]", false, true, 8.0));
	save(outDir + "\\ex1_1d_leak_signal.png",
		 plotCurve(cosLeak, false, -1.25, 1.25, "n", "x[n]", false, true, 8.5));
	save(outDir + "\\ex1_1d_integer_mag.png",
		 plotCurve(fftshiftMag(magInt), true, 0.0, 0.0, "k", "|X[k]|", true, false, 8.0));
	save(outDir + "\\ex1_1d_leak_mag.png",
		 plotCurve(fftshiftMag(magLeak), true, 0.0, 0.0, "k", "|X[k]|", true, false, 8.5));
	// 用理论包络核对数值：每个频率序号上，算出来的幅度和狄利克雷核包络应几乎相同。
	double envErr = 0.0;
	for (int k = 0; k < n1; ++k)
	{
		envErr = std::max(envErr, std::abs(magLeak[k] - cosineSpectrumEnvelope(k, 8.5, n1)));
		envErr = std::max(envErr, std::abs(magInt[k] - cosineSpectrumEnvelope(k, 8.0, n1)));
	}
	const double sincErr = envErr;

	// 旁瓣能量比：主瓣附近（峰值频率及其左右各 1 个点）以外的能量，除以主瓣能量。
	// 整数周期时这个比值接近 0；泄漏时能量散到旁边，比值明显变大。只统计 k = 0 到 N/2 的正频率一侧。
	auto sideLobeRatio = [](const std::vector<double> &mag, int peakBin) {
		double peak = 0.0;
		double side = 0.0;
		const int half = static_cast<int>(mag.size()) / 2;
		for (int k = 0; k <= half; ++k)
		{
			if (std::abs(k - peakBin) <= 1)
				peak += mag[k] * mag[k];
			else
				side += mag[k] * mag[k];
		}
		return side / std::max(peak, 1e-18);
	};
	const double rInt = sideLobeRatio(magInt, 8);
	const double rLeak = sideLobeRatio(magLeak, 8);

	// 二维用同一组对比。16 个周期的竖直条纹，频谱应是中心两侧各一个亮点；
	// 16.5 个周期时，这两个亮点沿水平频率方向被拉长。
	cv::Mat imgInt = makeCos2d(kN, 16.0);
	cv::Mat imgLeak = makeCos2d(kN, 16.5);
	cv::Mat reI, imI, reL, imL;
	cvrsg::kongzi::freq::centeredDft(imgInt, reI, imI);
	cvrsg::kongzi::freq::centeredDft(imgLeak, reL, imL);
	save(outDir + "\\ex1_2d_integer.png", cvrsg::kongzi::freq::toU8Clip(imgInt));
	save(outDir + "\\ex1_2d_leak.png", cvrsg::kongzi::freq::toU8Clip(imgLeak));
	save(outDir + "\\ex1_2d_integer_spectrum.png", cvrsg::kongzi::freq::logMagnitude(reI, imI));
	save(outDir + "\\ex1_2d_leak_spectrum.png", cvrsg::kongzi::freq::logMagnitude(reL, imL));

	report << "\n例1 频谱泄漏\n";
	report << "理论狄利克雷核包络与离散傅里叶变换幅度之间的最大偏差为 " << num(sincErr) << "。\n";
	report << "一维序列中，截取整数个周期时旁瓣能量与主瓣能量之比为 " << num(rInt)
		   << "，截取非整数个周期时该比值为 " << num(rLeak) << "。\n";
	report << "在窗口内截取整数个周期时，离散傅里叶变换的采样点恰好落在狄利克雷核的主瓣峰值和过零点上，因此谱线集中在单个频率位置。\n";
	report << "当截取非整数个周期时，窗口两端做周期延拓会出现相位跳变，采样点会落在旁瓣上，从而使能量扩散到邻近频率。二维图像同样满足这一规律。\n";

	// ------------------------------------------------------------------
	// 例2 相位代表结构
	// 一幅图的 DFT 是复数。幅度表示该频率有多强，相位表示这些频率成分怎么对齐。
	// 边缘和轮廓靠的是不同频率在空间上的对齐，所以主要留在相位里。
	// ------------------------------------------------------------------
	cv::Mat reLena, imLena, reCam, imCam, ab, ba;
	cvrsg::kongzi::freq::centeredDft(lena, reLena, imLena);
	cvrsg::kongzi::freq::centeredDft(camera, reCam, imCam);
	// ab：幅度用 Lena，相位用摄影师。重建轮廓应更像摄影师。
	cvrsg::kongzi::freq::mixMagPhase(reLena, imLena, reCam, imCam, ab);
	// ba：幅度用摄影师，相位用 Lena。重建轮廓应更像 Lena。
	cvrsg::kongzi::freq::mixMagPhase(reCam, imCam, reLena, imLena, ba);

	// 对照实验仍用 Lena：一种把相位全部写成 0，只留幅度；一种把每个频率的幅度都改成 1，只留相位。
	cv::Mat magOnly, phaseOnly;
	cvrsg::kongzi::freq::synthesize(reLena, imLena, magOnly, cvrsg::kongzi::freq::MagnitudeOnly);
	cvrsg::kongzi::freq::synthesize(reLena, imLena, phaseOnly, cvrsg::kongzi::freq::PhaseOnly);

	save(outDir + "\\ex2_lena.png", cvrsg::kongzi::freq::toU8Clip(lena));
	save(outDir + "\\ex2_cameraman.png", cvrsg::kongzi::freq::toU8Clip(camera));
	save(outDir + "\\ex2_lena_mag.png", cvrsg::kongzi::freq::logMagnitude(reLena, imLena));
	save(outDir + "\\ex2_lena_phase.png", cvrsg::kongzi::freq::phaseImage(reLena, imLena));
	save(outDir + "\\ex2_mag_only.png", cvrsg::kongzi::freq::toU8Stretch(magOnly));
	save(outDir + "\\ex2_phase_only.png", cvrsg::kongzi::freq::toU8Stretch(phaseOnly));
	save(outDir + "\\ex2_lenaMag_cameraPhase.png", cvrsg::kongzi::freq::toU8Stretch(ab));
	save(outDir + "\\ex2_cameraMag_lenaPhase.png", cvrsg::kongzi::freq::toU8Stretch(ba));

	report << "\n例2 相位与图像结构\n";
	report << "使用 Lena 的幅度谱和摄影师的相位谱重建得到的图像（ex2_lenaMag_cameraPhase.png），其轮廓与摄影师一致。\n";
	report << "使用摄影师的幅度谱和 Lena 的相位谱重建得到的图像（ex2_cameraMag_lenaPhase.png），其轮廓与 Lena 一致。\n";
	report << "只保留相位谱（把所有频率的幅度设为 1）时，依然能辨认出原图的人物轮廓；只保留幅度谱（把相位全部置零）时，几乎看不出原图结构。\n";

	// ------------------------------------------------------------------
	// 例3 用 FFT 计算图像滤波
	// 高斯核把邻域加权平均，图像变平滑；水平梯度核比较左右邻域，竖直边缘会变亮。
	// 两种核走同一条「补零、FFT、相乘、IFFT、裁剪」流程。
	// ------------------------------------------------------------------
	cv::Mat gauss = cvrsg::kongzi::freq::gaussianKernel(9, 1.5); // 9×9，标准差 1.5
	cv::Mat grad = cvrsg::kongzi::freq::horizGradKernel();       // 3×3 水平梯度核
	double errG = 0.0, errH = 0.0;
	cv::Mat outG = filterByFftPipeline(lena, gauss, "ex3_gauss", outDir, errG);
	cv::Mat outH = filterByFftPipeline(lena, grad, "ex3_grad", outDir, errH);

	save(outDir + "\\ex3_src.png", cvrsg::kongzi::freq::toU8Clip(lena));
	save(outDir + "\\ex3_gauss_result.png", cvrsg::kongzi::freq::toU8Clip(outG));
	save(outDir + "\\ex3_grad_result.png", cvrsg::kongzi::freq::toU8Signed(outH));
	save(outDir + "\\ex3_gauss_kernel.png", cvrsg::kongzi::freq::toU8Stretch(gauss));
	save(outDir + "\\ex3_grad_kernel.png", cvrsg::kongzi::freq::toU8Signed(grad));

	report << "\n例3 用快速傅里叶变换计算图像滤波\n";
	report << "使用高斯平滑核时，频域滤波结果与空域直接卷积的最大绝对误差为 " << num(errG) << "。\n";
	report << "使用水平梯度核时，频域滤波结果与空域直接卷积的最大绝对误差为 " << num(errH) << "。\n";
	report << "计算步骤为：将原图像和卷积核都补零到同一 2 的整数次幂尺寸，分别做二维快速傅里叶变换，在频域逐点相乘，做二维快速傅里叶逆变换，最后裁取左上角与原图相同的区域。\n";

	// ------------------------------------------------------------------
	// 例4 小波分解与重构做遥感融合
	// 两组图的尺寸关系相同，都调用上面的 saveWaveletFusion，只是文件名前缀不同。
	// ------------------------------------------------------------------
	const double haarErr = saveWaveletFusion(pan, msBgr, outDir, "ex4");
	const double haarErrTown = saveWaveletFusion(panTown, msTown, outDir, "ex4_town");

	report << "\n例4 基于小波分解与重构的遥感图像融合\n";
	report << "第一组是 Landsat 山地场景。对全色图像做一层二维哈尔小波分解后直接重构，最大绝对误差为 " << num(haarErr) << "。\n";
	report << "第二组是村镇场景。全色图由彩色图像转成灰度并保持 512×512，彩色图是同一幅图像在长和宽上各缩小为二分之一后的 256×256。分解后再直接重构的最大绝对误差为 " << num(haarErrTown) << "。\n";
	report << "两组的融合步骤相同：对高分辨率全色图像做小波分解得到低频近似子带与三个高频细节子带；将低分辨率彩色图像上采样并按通道缩放到低频子带尺寸，用它替换全色图像的低频子带，同时保留全色图像的三个高频细节子带，最后做小波逆变换。\n";
	report << "融合后的图像既保留了彩色图像的颜色，又具有全色图像里的边缘和纹理。村镇一组可以同时看到房屋、道路和植被。\n";

	// 四条都成立才算通过：泄漏时旁瓣比更大；两种滤波和空域卷积的误差小于 1e−6；
	// 两组小波分解后再原样重构的误差小于 1e−9。
	const bool ok = (rLeak > rInt) && (errG < 1e-6) && (errH < 1e-6) && (haarErr < 1e-9) && (haarErrTown < 1e-9);
	report << "\n自动检查结果：" << (ok ? "通过" : "未通过") << "\n";
	report.close();

	std::cout << "所有结果图像和报告已写入 " << outDir << "。" << std::endl;
	std::cout << "例1 旁瓣与主瓣能量比：整数周期为 " << rInt << "，非整数周期为 " << rLeak << "。" << std::endl;
	std::cout << "例3 与空域直接卷积的最大绝对误差：高斯核为 " << errG << "，水平梯度核为 " << errH << "。" << std::endl;
	std::cout << "例4 全色图像小波分解并直接重构的最大绝对误差：山地为 " << haarErr << "，村镇为 " << haarErrTown << "。" << std::endl;
	std::cout << "自动检查：" << (ok ? "通过" : "未通过") << std::endl;
	return ok ? 0 : 2;
}
