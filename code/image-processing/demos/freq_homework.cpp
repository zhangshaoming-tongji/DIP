// 第 8 章频域应用实验：随堂演示与四道练习。
//
// 例1 频谱泄漏：分别观察一维离散余弦序列和二维正弦条纹图像，在观察窗口包含整数个周期与非整数个周期时，离散傅里叶变换（DFT）的幅度分布。
// 例2 相位携带图像结构：对 Lena 与摄影师（Cameraman）两幅图像做离散傅里叶变换，交换它们的幅度谱与相位谱之后再做离散傅里叶逆变换（IDFT），观察重建结果的轮廓。
// 例3 用快速傅里叶变换（FFT）计算图像滤波：将原图像与卷积核都补零到同一尺寸，分别做快速傅里叶变换，在频域逐点相乘，再做快速傅里叶逆变换（IFFT），最后裁取左上角与原图相同的区域。
// 例4 基于小波分解与重构的遥感图像融合：对高分辨率全色图像做一层二维哈尔（Haar）小波分解，得到一个低频近似子带与三个高频细节子带；用低分辨率彩色图像替换低频近似子带，再做二维离散小波逆变换（IDWT），得到融合结果。
//
// 编译方法：在当前目录运行 build_freq.bat。
// 运行方法：运行 run_freq.bat，或执行 bin\freq_homework.exe data\ch8。
// 输出位置：结果图像和实验报告保存在 data/ch8/out/ 目录中。

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
const int kN = 256;

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

std::string num(double v)
{
	std::ostringstream os;
	os.setf(std::ios::scientific);
	os.precision(3);
	os << v;
	return os.str();
}

// 矩形窗对应的狄利克雷（Dirichlet）核函数，即周期化的 sinc 函数。
// 参数 delta 表示当前频率位置相对整数频率点的偏移量，n 为矩形窗的样本长度。
// 当偏移量为 n 的整数倍时函数值为 n，在其余整数偏移位置处函数值为 0。
std::complex<double> dirichletKernel(double delta, int n)
{
	const double wrapped = delta - n * std::round(delta / n);
	if (std::abs(wrapped) < 1e-9)
		return std::complex<double>(static_cast<double>(n), 0.0);
	const double mag = std::sin(kPi * delta) / std::sin(kPi * delta / n);
	const double phase = -kPi * delta * (n - 1) / n;
	return std::complex<double>(mag * std::cos(phase), mag * std::sin(phase));
}

// 计算实余弦序列的离散傅里叶变换幅度包络。
// 实余弦信号由正频率和负频率两支复指数相加而成，因此它的频谱包络是两支狄利克雷核复数叠加后再取模。
double cosineSpectrumEnvelope(double k, double cycles, int n)
{
	const auto pos = dirichletKernel(k - cycles, n);
	const auto neg = dirichletKernel(k + cycles, n);
	return 0.5 * std::abs(pos + neg);
}

// 绘制一维图形的辅助函数。
// 当 stem 为真时，从纵轴零点向每个数据点绘制竖直谱线；当 centeredK 为真时，横轴按频率序号 −N/2 到 N/2−1 标注。
// 当 samplesEnvelope 为真时，先用浅色细线绘制连续余弦包络，再把离散样点 x[n] 画成蓝色的竖线和圆点。
// 当同时满足 stem、centeredK 为真且 cycles 大于 0 时，会在谱线图上叠加绘制狄利克雷核的理论包络曲线。
// 由于 OpenCV 自带的字体不支持中文，图中的坐标轴标签仍使用数学符号。
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

// 长度必须是 2 的幂的一维 FFT（与库内算法一致的教学副本，仅用于例 1 的 1D）。
void fft1Local(std::vector<std::complex<double>> &a, bool invert)
{
	const int n = static_cast<int>(a.size());
	for (int i = 1, j = 0; i < n; ++i)
	{
		int bit = n >> 1;
		for (; j & bit; bit >>= 1)
			j ^= bit;
		j ^= bit;
		if (i < j)
			std::swap(a[i], a[j]);
	}
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
		for (int i = 0; i < n; ++i)
			a[i] /= static_cast<double>(n);
	}
}

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

// 生成长度 N 的余弦：cycles 为窗口内包含的周期数。整数周期 ⇒ 谱线集中；非整数 ⇒ 泄漏。
std::vector<double> makeCos1d(int n, double cycles)
{
	std::vector<double> x(n);
	for (int i = 0; i < n; ++i)
		x[i] = std::cos(2.0 * kPi * cycles * i / n);
	return x;
}

// 二维竖直条纹：f(x,y)=128+127 cos(2π cycles x / N)。cycles 非整数时频谱沿频率轴铺开。
cv::Mat makeCos2d(int n, double cycles)
{
	cv::Mat g(n, n, CV_64F);
	for (int y = 0; y < n; ++y)
		for (int x = 0; x < n; ++x)
			g.at<double>(y, x) = 128.0 + 127.0 * std::cos(2.0 * kPi * cycles * x / n);
	return g;
}

// 把 DFT 下标 0…N−1 重排成 k=−N/2…N/2−1，实序列的正、负频率对称地出现在 0 的两侧。
std::vector<double> fftshiftMag(const std::vector<double> &mag)
{
	const int n = static_cast<int>(mag.size());
	const int half = n / 2;
	std::vector<double> out(n);
	for (int i = 0; i < n; ++i)
		out[i] = mag[(i + half) % n];
	return out;
}

int nextPow2(int v)
{
	int p = 1;
	while (p < v)
		p <<= 1;
	return p;
}

int wrapIndex(int i, int n)
{
	int r = i % n;
	if (r < 0)
		r += n;
	return r;
}

// 显式写出补零 → FFT → 相乘 → IFFT → 裁剪，并把中间结果写出，便于课堂逐步讲解。
cv::Mat filterByFftPipeline(const cv::Mat &src, const cv::Mat &kernel, const std::string &tag,
							const std::string &outDir, double &errVsSpatial)
{
	const int n = src.rows;
	const int kh = kernel.rows;
	const int kw = kernel.cols;
	const int cy = kh / 2;
	const int cx = kw / 2;
	const int N = nextPow2(n + std::max(kh, kw) - 1);

	cv::Mat reA = cv::Mat::zeros(N, N, CV_64F);
	cv::Mat imA = cv::Mat::zeros(N, N, CV_64F);
	cv::Mat reB = cv::Mat::zeros(N, N, CV_64F);
	cv::Mat imB = cv::Mat::zeros(N, N, CV_64F);

	src.copyTo(reA(cv::Rect(0, 0, n, n))); // 图像右下补零
	for (int y = 0; y < kh; ++y)
	{
		for (int x = 0; x < kw; ++x)
		{
			// 与空域“中心对齐”的相关一致：k(ky,kx) 放到 (cy−ky, cx−kx)
			const int yy = wrapIndex(cy - y, N);
			const int xx = wrapIndex(cx - x, N);
			reB.at<double>(yy, xx) = kernel.at<double>(y, x);
		}
	}

	save(outDir + "\\" + tag + "_padded_image.png", cvrsg::kongzi::freq::toU8Clip(reA));
	save(outDir + "\\" + tag + "_padded_kernel.png", cvrsg::kongzi::freq::toU8Signed(reB));

	cvrsg::kongzi::freq::fft2(reA, imA, false);
	cvrsg::kongzi::freq::fft2(reB, imB, false);
	save(outDir + "\\" + tag + "_fft_image.png", cvrsg::kongzi::freq::logMagnitude(reA, imA));
	save(outDir + "\\" + tag + "_fft_kernel.png", cvrsg::kongzi::freq::logMagnitude(reB, imB));

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

	cvrsg::kongzi::freq::fft2(reA, imA, true);
	cv::Mat out = reA(cv::Rect(0, 0, n, n)).clone();

	cv::Mat spatial = cvrsg::kongzi::freq::convolveSpatial(src, kernel);
	errVsSpatial = cvrsg::kongzi::freq::maxAbsDiff(out, spatial);
	return out;
}

cv::Mat tile2x2(const cv::Mat &a, const cv::Mat &b, const cv::Mat &c, const cv::Mat &d);

// 用彩色图像替换全色图像的低频子带，写出这一组融合结果。返回全色图像分解后再还原的最大绝对误差。
double saveWaveletFusion(const cv::Mat &pan, const cv::Mat &msBgr, const std::string &outDir, const std::string &prefix)
{
	cv::Mat msUp;
	cv::resize(msBgr, msUp, cv::Size(pan.cols, pan.rows), 0, 0, cv::INTER_LINEAR);

	cv::Mat panLL, panLH, panHL, panHH;
	cvrsg::kongzi::freq::haarDwt2(pan, panLL, panLH, panHL, panHH);

	std::vector<cv::Mat> msCh(3), fusedCh(3);
	cv::split(msUp, msCh);
	for (int c = 0; c < 3; ++c)
	{
		cv::Mat ch64;
		msCh[c].convertTo(ch64, CV_64F);
		cv::Mat colorLL;
		cv::resize(ch64, colorLL, panLL.size(), 0, 0, cv::INTER_AREA);
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
	cv::Mat pan = loadGray(dir + "\\pan.png", 512);
	cv::Mat msBgr = loadColor(dir + "\\ms_color.png", 256);
	cv::Mat panTown = loadGray(dir + "\\pan_town.png", 512);
	cv::Mat msTown = loadColor(dir + "\\ms_town.png", 256);
	if (lena.empty() || camera.empty() || pan.empty() || msBgr.empty() || panTown.empty() || msTown.empty())
		return 1;

	std::ofstream report(outDir + "\\report.txt");
	report << "第 8 章频域应用：四例实验结果\n";

	// ------------------------------------------------------------------
	// 例1 频谱泄漏
	// ------------------------------------------------------------------
	const int n1 = 128;
	const auto cosInt = makeCos1d(n1, 8.0);	  // 整 8 个周期
	const auto cosLeak = makeCos1d(n1, 8.5); // 8.5 个周期：边界相位不连续
	const auto magInt = dftMag1d(cosInt);
	const auto magLeak = dftMag1d(cosLeak);

	save(outDir + "\\ex1_1d_integer_signal.png",
		 plotCurve(cosInt, false, -1.25, 1.25, "n", "x[n]", false, true, 8.0));
	save(outDir + "\\ex1_1d_leak_signal.png",
		 plotCurve(cosLeak, false, -1.25, 1.25, "n", "x[n]", false, true, 8.5));
	save(outDir + "\\ex1_1d_integer_mag.png",
		 plotCurve(fftshiftMag(magInt), true, 0.0, 0.0, "k", "|X[k]|", true, false, 8.0));
	save(outDir + "\\ex1_1d_leak_mag.png",
		 plotCurve(fftshiftMag(magLeak), true, 0.0, 0.0, "k", "|X[k]|", true, false, 8.5));
	double envErr = 0.0;
	for (int k = 0; k < n1; ++k)
	{
		envErr = std::max(envErr, std::abs(magLeak[k] - cosineSpectrumEnvelope(k, 8.5, n1)));
		envErr = std::max(envErr, std::abs(magInt[k] - cosineSpectrumEnvelope(k, 8.0, n1)));
	}
	const double sincErr = envErr;

	// 只统计正频率半边的“旁瓣能量比”，用来定量说明泄漏
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
	// ------------------------------------------------------------------
	cv::Mat reLena, imLena, reCam, imCam, ab, ba;
	cvrsg::kongzi::freq::centeredDft(lena, reLena, imLena);
	cvrsg::kongzi::freq::centeredDft(camera, reCam, imCam);
	cvrsg::kongzi::freq::mixMagPhase(reLena, imLena, reCam, imCam, ab); // |Lena| + ∠Cameraman
	cvrsg::kongzi::freq::mixMagPhase(reCam, imCam, reLena, imLena, ba); // |Cameraman| + ∠Lena

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
	// 例3 FFT 图像滤波（高斯 + 水平梯度）
	// ------------------------------------------------------------------
	cv::Mat gauss = cvrsg::kongzi::freq::gaussianKernel(9, 1.5);
	cv::Mat grad = cvrsg::kongzi::freq::horizGradKernel();
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
	// 例4 小波分解重构的遥感融合。两组数据使用同一套分解、替换和逆变换。
	// ------------------------------------------------------------------
	const double haarErr = saveWaveletFusion(pan, msBgr, outDir, "ex4");
	const double haarErrTown = saveWaveletFusion(panTown, msTown, outDir, "ex4_town");

	report << "\n例4 基于小波分解与重构的遥感图像融合\n";
	report << "第一组是 Landsat 山地场景。对全色图像做一层二维哈尔小波分解后直接重构，最大绝对误差为 " << num(haarErr) << "。\n";
	report << "第二组是村镇场景。全色图由彩色图像转成灰度并保持 512×512，彩色图是同一幅图像在长和宽上各缩小为二分之一后的 256×256。分解后再直接重构的最大绝对误差为 " << num(haarErrTown) << "。\n";
	report << "两组的融合步骤相同：对高分辨率全色图像做小波分解得到低频近似子带与三个高频细节子带；将低分辨率彩色图像上采样并按通道缩放到低频子带尺寸，用它替换全色图像的低频子带，同时保留全色图像的三个高频细节子带，最后做小波逆变换。\n";
	report << "融合后的图像既保留了彩色图像的颜色，又具有全色图像里的边缘和纹理。村镇一组可以同时看到房屋、道路和植被。\n";

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
