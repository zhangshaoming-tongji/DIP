// 频域分析作业的演示程序。算法在 freqDomain.h / freqDomain.cpp，这里只负责读图、出图和写报告。
//
// 编译：在 code\image-processing 下运行 build_freq.bat
// 运行：run_freq.bat，或 freq_homework.exe data\freq
// 输出：data\freq\out\ 里的 png，以及同目录的 report.txt
//
// 题1  幅度、相位分别重建。相位里有轮廓，幅度里主要是能量分布。
// 题2  两幅图交换幅度和相位。结果的轮廓跟提供相位的那一幅走。
// 题3  不乘 (−1)^{x+y} 时直流在左上角；乘完后直流在中心。
// 题4  把平滑、锐化、拉普拉斯、Sobel 核变到频域，看直流增益和高频增益。
// 题5  同一核的空域卷积、循环卷积、补零后的 DFT 卷积。不补零会出现卷绕。
// 题6  理想、高斯、二阶和十阶巴特沃斯低通。理想低通的空域核有负值，图像有振铃。
// 题7  纯高通把直流去掉；高频强调保留一部分低频，图像仍有亮度。
// 题8  正弦周期噪声在频谱上是四个亮点，用陷波去掉。
// 题9  循环平移不改变幅度谱。
// 题10 图像逆时针旋转，频谱主方向一起转。屏幕 y 向下，角度读数会减小。
// 题11 8×8 DCT，按 zigzag 只留前 K 个系数。K 越小越模糊，K = 64 时无损。

#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <sstream>
#include <string>

#include "freqDomain.h"

using cvrsg::kongzi::freq::Butterworth;
using cvrsg::kongzi::freq::Full;
using cvrsg::kongzi::freq::Gaussian;
using cvrsg::kongzi::freq::Ideal;
using cvrsg::kongzi::freq::MagnitudeOnly;
using cvrsg::kongzi::freq::PhaseOnly;

namespace
{

const int kN = 256;

cv::Mat loadGray(const std::string &path)
{
	cv::Mat src = cv::imread(path, cv::IMREAD_GRAYSCALE);
	if (src.empty())
	{
		std::cout << "无法读取 " << path << std::endl;
		return cv::Mat();
	}
	cv::Mat resized;
	cv::resize(src, resized, cv::Size(kN, kN));
	cv::Mat gray;
	resized.convertTo(gray, CV_64F);
	return gray;
}

void save(const std::string &path, const cv::Mat &image)
{
	cv::imwrite(path, image);
}

// 把角度差折到 (−90, 90]。频谱能量沿一条直线，0° 和 180° 是同一条线，所以周期是 180°。
int foldAngle(int d)
{
	int x = ((d % 180) + 180) % 180;
	if (x > 90)
		x -= 180;
	return x;
}

std::string num(double v)
{
	std::ostringstream os;
	os.setf(std::ios::scientific);
	os.precision(3);
	os << v;
	return os.str();
}

} // namespace

int main(int argc, char *argv[])
{
	SetConsoleOutputCP(65001);
	const std::string dir = (argc >= 2) ? argv[1] : "data\\freq";
	const std::string outDir = dir + "\\out";
	CreateDirectoryA(dir.c_str(), NULL);
	CreateDirectoryA(outDir.c_str(), NULL);

	cv::Mat lena = loadGray(dir + "\\lena.png");
	cv::Mat camera = loadGray(dir + "\\camera.png");
	if (lena.empty() || camera.empty())
		return 1;

	std::ofstream report(outDir + "\\report.txt");
	report << "频域分析作业结果\n";

	// 题1：完整频谱应还原 Lena。只留幅度时相位置 0，只留相位时幅度置 1。
	cv::Mat re, im, recon, magOnly, phaseOnly;
	cvrsg::kongzi::freq::centeredDft(lena, re, im);
	cvrsg::kongzi::freq::synthesize(re, im, recon, Full);
	cvrsg::kongzi::freq::synthesize(re, im, magOnly, MagnitudeOnly);
	cvrsg::kongzi::freq::synthesize(re, im, phaseOnly, PhaseOnly);
	const double reconErr = cvrsg::kongzi::freq::maxAbsDiff(lena, recon);
	save(outDir + "\\q1_lena.png", cvrsg::kongzi::freq::toU8Clip(lena));
	save(outDir + "\\q1_magnitude.png", cvrsg::kongzi::freq::logMagnitude(re, im));
	save(outDir + "\\q1_phase.png", cvrsg::kongzi::freq::phaseImage(re, im));
	save(outDir + "\\q1_recon.png", cvrsg::kongzi::freq::toU8Clip(recon));
	save(outDir + "\\q1_mag_only.png", cvrsg::kongzi::freq::toU8Stretch(magOnly));
	save(outDir + "\\q1_phase_only.png", cvrsg::kongzi::freq::toU8Stretch(phaseOnly));
	report << "\n题1 幅度与相位\n";
	report << "完整重建最大误差 " << num(reconErr) << "\n";
	report << "只保留幅度或只保留相位时，结构信息在相位里。\n";

	// 题2：ab 用 Lena 的幅度、Cameraman 的相位；ba 相反。轮廓跟着相位走。
	cv::Mat camRe, camIm, ab, ba;
	cvrsg::kongzi::freq::centeredDft(camera, camRe, camIm);
	cvrsg::kongzi::freq::mixMagPhase(re, im, camRe, camIm, ab);
	cvrsg::kongzi::freq::mixMagPhase(camRe, camIm, re, im, ba);
	save(outDir + "\\q2_lena.png", cvrsg::kongzi::freq::toU8Clip(lena));
	save(outDir + "\\q2_camera.png", cvrsg::kongzi::freq::toU8Clip(camera));
	save(outDir + "\\q2_lenaMag_cameraPhase.png", cvrsg::kongzi::freq::toU8Stretch(ab));
	save(outDir + "\\q2_cameraMag_lenaPhase.png", cvrsg::kongzi::freq::toU8Stretch(ba));
	report << "\n题2 幅度相位交换\n";
	report << "q2_lenaMag_cameraPhase 的轮廓应接近 Cameraman。\n";
	report << "q2_cameraMag_lenaPhase 的轮廓应接近 Lena。\n";

	// 题3：raw 是未中心化的频谱。checker 把 (−1)^{x+y} 直接乘在图像上，用来对照中心化的来历。
	cv::Mat rawRe, rawIm;
	cvrsg::kongzi::freq::rawDft(lena, rawRe, rawIm);
	cv::Mat checker(kN, kN, CV_64F);
	for (int y = 0; y < kN; ++y)
		for (int x = 0; x < kN; ++x)
			checker.at<double>(y, x) = ((x + y) & 1) ? -lena.at<double>(y, x) : lena.at<double>(y, x);
	save(outDir + "\\q3_raw_magnitude.png", cvrsg::kongzi::freq::logMagnitude(rawRe, rawIm));
	save(outDir + "\\q3_centered_magnitude.png", cvrsg::kongzi::freq::logMagnitude(re, im));
	save(outDir + "\\q3_checker.png", cvrsg::kongzi::freq::toU8Signed(checker));
	report << "\n题3 频谱中心化\n";
	report << "未中心化时直流在左上角。乘 (−1)^{x+y} 后直流在中心。\n";

	// 题4：中心增益是 H(n/2,n/2)，角落是未中心化时的最高频。
	// 平滑核中心接近 1；拉普拉斯和 Sobel 中心为 0，能量在高频。
	struct KernelCase
	{
		const char *name;
		cv::Mat kernel;
	};
	KernelCase kernels[4] = {
		{"smooth", cvrsg::kongzi::freq::smoothKernel()},
		{"sharpen", cvrsg::kongzi::freq::sharpenKernel()},
		{"laplacian", cvrsg::kongzi::freq::laplacianKernel()},
		{"sobel", cvrsg::kongzi::freq::sobelKernel()}};
	report << "\n题4 空域核的频率响应\n";
	for (int i = 0; i < 4; ++i)
	{
		cv::Mat filtered = cvrsg::kongzi::freq::convolveSpatial(lena, kernels[i].kernel);
		cv::Mat kre, kim;
		cvrsg::kongzi::freq::kernelSpectrumRe(kernels[i].kernel, kN, kre, kim);
		const int c = kN / 2;
		const double dc = std::hypot(kre.at<double>(c, c), kim.at<double>(c, c));
		const double corner = std::hypot(kre.at<double>(0, 0), kim.at<double>(0, 0));
		double peak = 0;
		for (int v = 0; v < kN; ++v)
			for (int u = 0; u < kN; ++u)
				peak = std::max(peak, std::hypot(kre.at<double>(v, u), kim.at<double>(v, u)));
		const bool signedView = (i == 2 || i == 3);
		save(outDir + "\\q4_" + kernels[i].name + ".png",
			 signedView ? cvrsg::kongzi::freq::toU8Signed(filtered) : cvrsg::kongzi::freq::toU8Clip(filtered));
		save(outDir + "\\q4_" + kernels[i].name + "_H.png", cvrsg::kongzi::freq::logMagnitude(kre, kim));
		report << kernels[i].name << " 中心增益 " << dc << "，最高频角落增益 " << corner
			   << "，峰值 " << peak << "\n";
	}

	// 题5：41 点水平运动模糊。核比较长，不补零时左右两边的卷绕很明显。
	// 补零后的 DFT 卷积应与空域线性卷积一致，误差在 1e−6 以下。
	cv::Mat motion = cvrsg::kongzi::freq::motionKernel(41);
	cv::Mat spatial = cvrsg::kongzi::freq::convolveSpatial(lena, motion);
	cv::Mat circular = cvrsg::kongzi::freq::convolveDft(lena, motion, false);
	cv::Mat padded = cvrsg::kongzi::freq::convolveDft(lena, motion, true);
	cv::Mat diff;
	cv::absdiff(circular, spatial, diff);
	const double errPad = cvrsg::kongzi::freq::maxAbsDiff(spatial, padded);
	const double errCirc = cvrsg::kongzi::freq::maxAbsDiff(spatial, circular);
	save(outDir + "\\q5_linear.png", cvrsg::kongzi::freq::toU8Clip(spatial));
	save(outDir + "\\q5_circular.png", cvrsg::kongzi::freq::toU8Clip(circular));
	save(outDir + "\\q5_padded.png", cvrsg::kongzi::freq::toU8Clip(padded));
	save(outDir + "\\q5_circular_minus_linear.png", cvrsg::kongzi::freq::toU8Stretch(diff));
	double linear1[5], circular1[5], padded1[5];
	cvrsg::kongzi::freq::conv1dExample(linear1, circular1, padded1);
	report << "\n题5 循环卷积\n";
	report << "补零后与空域卷积的最大误差 " << num(errPad) << "\n";
	report << "不补零的最大误差 " << errCirc << "\n";
	report << "一维 x=[1,2,3,4,5], h=[1,2,1]，原点在中间的 2\n";
	report << "线性";
	for (int i = 0; i < 5; ++i)
		report << " " << linear1[i];
	report << "\n循环";
	for (int i = 0; i < 5; ++i)
		report << " " << circular1[i];
	report << "\n补零";
	for (int i = 0; i < 5; ++i)
		report << " " << padded1[i];
	report << "\n第 1 个数 4 和 9 的差别来自卷绕。\n";

	// 题6：截止频率 D0 = 28。看三件事：滤波后的图、H 本身、逆变换得到的空域核。
	// 理想低通的核有负值，图上有振铃；高斯核最小值接近 0；巴特沃斯 n = 10 更接近理想低通。
	const double d0 = 28;
	report << "\n题6 低通，D0 = " << d0 << "\n";
	const int types[3] = {Ideal, Gaussian, Butterworth};
	const char *typeName[3] = {"ideal", "gaussian", "butterworth_n2"};
	const int orders[3] = {2, 2, 2};
	for (int i = 0; i < 3; ++i)
	{
		cv::Mat H = cvrsg::kongzi::freq::lowpassH(kN, d0, orders[i], types[i]);
		cv::Mat filtered;
		cvrsg::kongzi::freq::applyH(re, im, H, filtered);
		cv::Mat kernel = cvrsg::kongzi::freq::spatialKernel(H);
		double kmin = 0;
		cv::minMaxLoc(kernel, &kmin, nullptr);
		save(outDir + "\\q6_" + typeName[i] + ".png", cvrsg::kongzi::freq::toU8Clip(filtered));
		save(outDir + "\\q6_" + typeName[i] + "_H.png", cvrsg::kongzi::freq::showFilter(H));
		save(outDir + "\\q6_" + typeName[i] + "_kernel.png", cvrsg::kongzi::freq::toU8Signed(kernel));
		report << typeName[i] << " 保留能量 " << (100.0 * cvrsg::kongzi::freq::energyKept(re, im, H))
			   << "%，空域核最小值 " << kmin << "\n";
	}
	cv::Mat butter10 = cvrsg::kongzi::freq::lowpassH(kN, d0, 10, Butterworth);
	cv::Mat butter10Out;
	cvrsg::kongzi::freq::applyH(re, im, butter10, butter10Out);
	save(outDir + "\\q6_butterworth_n10.png", cvrsg::kongzi::freq::toU8Clip(butter10Out));
	report << "巴特沃斯 n=10 更接近理想低通，振铃更明显。\n";

	// 题7：同一个高斯低通，a = 0 得到纯高通，a = 0.45 得到高频强调。
	// 直流增益应分别是 0 和 0.45。纯高通结果用拉伸显示，否则边缘太暗。
	cv::Mat high = cvrsg::kongzi::freq::emphasis(
		cvrsg::kongzi::freq::lowpassH(kN, 24, 2, Gaussian), 0.0);
	cv::Mat highKeep = cvrsg::kongzi::freq::emphasis(
		cvrsg::kongzi::freq::lowpassH(kN, 24, 2, Gaussian), 0.45);
	cv::Mat highOut, keepOut;
	cvrsg::kongzi::freq::applyH(re, im, high, highOut);
	cvrsg::kongzi::freq::applyH(re, im, highKeep, keepOut);
	save(outDir + "\\q7_highpass.png", cvrsg::kongzi::freq::toU8Stretch(highOut));
	save(outDir + "\\q7_emphasis.png", cvrsg::kongzi::freq::toU8Clip(keepOut));
	report << "\n题7 高通\n";
	report << "纯高通直流增益 " << high.at<double>(kN / 2, kN / 2)
		   << "，高频强调 a=0.45 的直流增益 " << highKeep.at<double>(kN / 2, kN / 2) << "\n";

	// 题8：频率 32 的水平、竖直正弦。频谱亮点在距中心 32 像素的四个位置，陷波半径 6。
	// 陷波也会去掉原图在这四个邻域里的频率，所以和原图的差不会是 0。
	const int noiseF = 32;
	cv::Mat noisy = cvrsg::kongzi::freq::addPeriodicNoise(lena, noiseF, 40);
	cv::Mat nre, nim, denoised;
	cvrsg::kongzi::freq::centeredDft(noisy, nre, nim);
	cv::Mat notch = cvrsg::kongzi::freq::notchH(kN, noiseF, 6);
	cvrsg::kongzi::freq::applyH(nre, nim, notch, denoised);
	save(outDir + "\\q8_noisy.png", cvrsg::kongzi::freq::toU8Clip(noisy));
	save(outDir + "\\q8_spectrum.png", cvrsg::kongzi::freq::logMagnitude(nre, nim));
	save(outDir + "\\q8_notch.png", cvrsg::kongzi::freq::showFilter(notch));
	save(outDir + "\\q8_denoised.png", cvrsg::kongzi::freq::toU8Clip(denoised));
	report << "\n题8 周期噪声\n";
	report << "噪声频率 f=" << noiseF << "，亮点在 (±f,0) 和 (0,±f)。\n";
	report << "陷波后与原图的最大误差 " << cvrsg::kongzi::freq::maxAbsDiff(lena, denoised) << "\n";

	// 题9：循环平移 (48, 30)。幅度谱应与题 1 的 Lena 幅度谱相同，最大差应远小于 1e−4。
	cv::Mat shifted = cvrsg::kongzi::freq::circShift(lena, 48, 30);
	cv::Mat sre, sim;
	cvrsg::kongzi::freq::centeredDft(shifted, sre, sim);
	const double shiftDiff = cvrsg::kongzi::freq::maxMagnitudeDiff(re, im, sre, sim);
	save(outDir + "\\q9_shifted.png", cvrsg::kongzi::freq::toU8Clip(shifted));
	save(outDir + "\\q9_magnitude.png", cvrsg::kongzi::freq::logMagnitude(sre, sim));
	report << "\n题9 循环平移\n";
	report << "平移前后线性幅度最大差 " << num(shiftDiff) << "\n";

	// 题10：条纹图逆时针转 24°。dominantAngle 按 y 向下测量，读数会减小，
	// 所以用 a0 − a1，再折到 (−90, 90]，得到的变化应接近 +24°。
	cv::Mat stripes = cvrsg::kongzi::freq::makeStripes(kN);
	cv::Mat turned = cvrsg::kongzi::freq::rotateImage(stripes, 24);
	cv::Mat strRe, strIm, turnRe, turnIm;
	cvrsg::kongzi::freq::centeredDft(stripes, strRe, strIm);
	cvrsg::kongzi::freq::centeredDft(turned, turnRe, turnIm);
	const int a0 = cvrsg::kongzi::freq::dominantAngle(strRe, strIm);
	const int a1 = cvrsg::kongzi::freq::dominantAngle(turnRe, turnIm);
	save(outDir + "\\q10_stripes.png", cvrsg::kongzi::freq::toU8Clip(stripes));
	save(outDir + "\\q10_rotated.png", cvrsg::kongzi::freq::toU8Clip(turned));
	save(outDir + "\\q10_spectrum.png", cvrsg::kongzi::freq::logMagnitude(strRe, strIm));
	save(outDir + "\\q10_rotated_spectrum.png", cvrsg::kongzi::freq::logMagnitude(turnRe, turnIm));
	report << "\n题10 旋转\n";
	report << "原图主方向 " << a0 << " 度，旋转后 " << a1
		   << " 度，逆时针变化 " << foldAngle(a0 - a1) << " 度，设定角度 24 度。\n";

	// 题11：保留 zigzag 前 1、6、15、64 个系数。块 (128,128) 的系数由 compressDct 算出，这里只比较 PSNR。
	report << "\n题11 分块 DCT\n";
	const int keeps[4] = {1, 6, 15, 64};
	for (int i = 0; i < 4; ++i)
	{
		double psnr = 0;
		cv::Mat coeff;
		cv::Mat rec = cvrsg::kongzi::freq::compressDct(lena, keeps[i], 128, 128, psnr, &coeff);
		save(outDir + "\\q11_k" + std::to_string(keeps[i]) + ".png", cvrsg::kongzi::freq::toU8Clip(rec));
		if (psnr > 1e8)
			report << "K=" << keeps[i] << " PSNR = inf\n";
		else
			report << "K=" << keeps[i] << " PSNR = " << psnr << " dB\n";
	}

	// 自检只覆盖四件可以定量判断的事：重建、补零卷积、循环卷绕确实存在、平移后幅度不变。
	const bool ok = reconErr < 1e-6 && errPad < 1e-6 && errCirc > 1.0 && shiftDiff < 1e-4;
	report << "\n自检 " << (ok ? "通过" : "未通过") << "\n";
	report.close();

	std::cout << "结果已写入 " << outDir << std::endl;
	std::cout << "完整重建最大误差 " << reconErr << std::endl;
	std::cout << "补零卷积误差 " << errPad << "，循环卷积误差 " << errCirc << std::endl;
	std::cout << "平移幅度差 " << shiftDiff << std::endl;
	std::cout << "自检" << (ok ? "通过" : "未通过") << std::endl;
	return ok ? 0 : 2;
}
