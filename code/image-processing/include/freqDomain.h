#ifndef _CVRSG_KONGZI_FREQ_DOMAIN_H
#define _CVRSG_KONGZI_FREQ_DOMAIN_H

#include <opencv2/core.hpp>

// 第 8 章频域应用的四例：频谱泄漏、相位与结构、补零后的快速傅里叶变换滤波、哈尔小波遥感融合。
//
// 数据约定
// 1. 灰度图用 CV_64F，边长 n 必须是 2 的幂。演示程序把 Lena 等缩放到 256×256。
// 2. 正变换不除 n。逆变换在行、列上各除一次 n，二维一共除以 n²。
//    因此正变换再逆变换可以回到原图，重建误差应接近 0。
// 3. 中心化：变换前把 f(x,y) 乘 (−1)^{x+y}，直流从左上角 (0,0) 移到 (n/2, n/2)。
//    同一个因子再乘一次，直流又回到左上角。显示幅度谱时使用中心化频谱。
// 4. 卷积核的几何中心对准当前像素。例 3 在补零后把核的中心卷绕到坐标原点，再做快速傅里叶变换。
namespace cvrsg
{
namespace kongzi
{
namespace freq
{

// 用幅度和相位重建图像的方式。输入频谱必须已经中心化。
enum ReconstructMode
{
	Full = 0,          // 幅度和相位都保留，应还原原图
	MagnitudeOnly = 1, // 相位置 0，只剩能量分布，看不出轮廓
	PhaseOnly = 2      // 幅度置 1，轮廓仍然在
};

// 原地二维 FFT。re、im 都是 n×n 的 CV_64F。
// invert 为 false 时做正变换；为 true 时做逆变换，每个方向除以 n。
void fft2(cv::Mat &re, cv::Mat &im, bool invert);

// 中心化 DFT。先乘 (−1)^{x+y}，再做正变换，直流在矩阵中心。
void centeredDft(const cv::Mat &gray, cv::Mat &re, cv::Mat &im);

// 按 mode 改写频谱，再做中心化逆变换。
void synthesize(const cv::Mat &re, const cv::Mat &im, cv::Mat &out, int mode);

// 用第一对频谱的幅度、第二对频谱的相位合成一幅图。
// 相位决定边缘和轮廓，所以 Lena 的幅度配 Cameraman 的相位，结果更像 Cameraman。
void mixMagPhase(const cv::Mat &magRe, const cv::Mat &magIm,
				 const cv::Mat &phaseRe, const cv::Mat &phaseIm, cv::Mat &out);

// 空域线性卷积。边界外补 0，核的几何中心对准当前像素。
// 例 3 用它核对补零后的快速傅里叶变换滤波。
cv::Mat convolveSpatial(const cv::Mat &src, const cv::Mat &kernel);

// 构造边长为 k 的正方形二维高斯平滑卷积核（若传入偶数，则自动加 1 使其有唯一中心像素），
// 参数 sigma 为高斯分布的标准差，最终将所有系数之和归一化为 1，以保持图像平均亮度不变。
cv::Mat gaussianKernel(int k, double sigma);
// 构造 3×3 水平梯度卷积核（即索贝尔水平算子），用于检测图像中的竖直边缘。
cv::Mat horizGradKernel();

// 对输入图像 src 做一层二维哈尔（Haar）离散小波分解，要求图像边长为偶数。
// 输出四个边长为原图一半的子带（数据类型为双精度浮点矩阵 CV_64F）：
// LL 为低频近似子带，LH 为水平边缘细节子带，HL 为竖直边缘细节子带，HH 为对角细节子带。
void haarDwt2(const cv::Mat &src, cv::Mat &LL, cv::Mat &LH, cv::Mat &HL, cv::Mat &HH);
// 由低频近似子带 LL 与三个高频细节子带 LH、HL、HH 做一层二维哈尔小波逆变换，恢复为与分解前同尺寸的图像。
void haarIdwt2(const cv::Mat &LL, const cv::Mat &LH, const cv::Mat &HL, const cv::Mat &HH, cv::Mat &out);

// 两幅同尺寸 CV_64F 图像的最大绝对差。
double maxAbsDiff(const cv::Mat &a, const cv::Mat &b);

// 直接截断转换成 8 位。适合灰度已经在 0～255 的图像。
cv::Mat toU8Clip(const cv::Mat &src);
// 按最小、最大值线性拉伸到 0～255。适合相位重建这类没有固定灰度范围的结果。
cv::Mat toU8Stretch(const cv::Mat &src);
// 以 0 为中灰 128，正负对称显示。适合梯度核的滤波结果。
cv::Mat toU8Signed(const cv::Mat &src);
// 对数幅度 log(1 + |F|)，再拉伸到 8 位，否则直流会把其余频率压暗。
cv::Mat logMagnitude(const cv::Mat &re, const cv::Mat &im);
// 相位 atan2(Im, Re) 从 [−π, π] 映射到 0～255。
cv::Mat phaseImage(const cv::Mat &re, const cv::Mat &im);

} // namespace freq
} // namespace kongzi
} // namespace cvrsg

#endif
