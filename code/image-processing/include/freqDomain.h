#ifndef _CVRSG_KONGZI_FREQ_DOMAIN_H
#define _CVRSG_KONGZI_FREQ_DOMAIN_H

#include <opencv2/core.hpp>
#include <string>

// 第 8 章频域应用（四例：泄漏、相位、FFT 滤波、小波融合）。
//
// 数据约定
// 1. 灰度图用 CV_64F，边长 n 必须是 2 的幂。演示程序把 Lena 等缩放到 256×256。
// 2. 正变换不除 n。逆变换在行、列上各除一次 n，二维一共除以 n²。
//    因此正变换再逆变换可以回到原图，重建误差应接近 0。
// 3. 中心化：变换前把 f(x,y) 乘 (−1)^{x+y}，直流从左上角 (0,0) 移到 (n/2, n/2)。
//    同一个因子再乘一次，直流又回到左上角。滤波和显示都使用中心化频谱。
// 4. 卷积核的几何中心对准当前像素。用 DFT 做滤波前，把 k(ky,kx) 放到 (cy−ky, cx−kx)
//   （中心落到 (0,0)）。这样与空域相关一致；奇对称核若放到 (ky−cy, kx−cx) 会反号。
// 5. 屏幕坐标 y 向下。图像逆时针旋转时，频谱主方向的读数会减小。
namespace cvrsg
{
namespace kongzi
{
namespace freq
{

// 低通类型。D 是到频谱中心的距离，D0 是截止频率。
enum LowpassType
{
	Ideal = 0,      // D ≤ D0 时为 1，否则为 0。空域核是 sinc，有振铃
	Gaussian = 1,   // exp(−D² / (2 D0²))。空域核也是高斯，没有负值
	Butterworth = 2 // 1 / (1 + (D/D0)^{2n})。阶数 n 越大越接近理想低通
};

// 用幅度和相位重建图像的方式。输入频谱必须已经中心化。
enum ReconstructMode
{
	Full = 0,          // 幅度和相位都保留，应还原原图
	MagnitudeOnly = 1, // 相位置 0，只剩能量分布，看不出轮廓
	PhaseOnly = 2,     // 幅度置 1，轮廓仍然在
	RandomPhase = 3    // 保留真实幅度，相位换成共轭对称的随机相位
};

// 原地二维 FFT。re、im 都是 n×n 的 CV_64F。
// invert 为 false 时做正变换；为 true 时做逆变换，每个方向除以 n。
void fft2(cv::Mat &re, cv::Mat &im, bool invert);

// 中心化 DFT。先乘 (−1)^{x+y}，再做正变换，直流在矩阵中心。
void centeredDft(const cv::Mat &gray, cv::Mat &re, cv::Mat &im);

// 不做中心化。直流留在 (0,0)，显示时位于左上角，并在另外三个角周期延拓。
void rawDft(const cv::Mat &gray, cv::Mat &re, cv::Mat &im);

// 按 mode 改写频谱，再做中心化逆变换。
void synthesize(const cv::Mat &re, const cv::Mat &im, cv::Mat &out, int mode);

// 用第一对频谱的幅度、第二对频谱的相位合成一幅图。
// 相位决定边缘和轮廓，所以 Lena 的幅度配 Cameraman 的相位，结果更像 Cameraman。
void mixMagPhase(const cv::Mat &magRe, const cv::Mat &magIm,
				 const cv::Mat &phaseRe, const cv::Mat &phaseIm, cv::Mat &out);

// 构造 n×n 的低通 H(u,v)，直流在中心。
// d0 是截止频率，单位是像素。order 只对巴特沃斯有意义。
cv::Mat lowpassH(int n, double d0, int order, int type);

// 由低通得到高频强调：H = a + (1 − a) (1 − H_low)。
// a = 0 是纯高通，直流增益为 0；a = 0.45 时直流保留 0.45，图像仍有亮度。
cv::Mat emphasis(const cv::Mat &lowpass, double keepLow);

// 陷波。在中心化频谱的 (±frequency, 0) 和 (0, ±frequency) 四个圆内把 H 置 0。
// 竖直条纹对应水平频率，水平条纹对应竖直频率，所以四个点一起去掉。
cv::Mat notchH(int n, int frequency, int radius);

// 中心化频谱乘以 H，再逆变换得到滤波结果。
void applyH(const cv::Mat &re, const cv::Mat &im, const cv::Mat &H, cv::Mat &out);

// 保留能量比例 Σ |F H|² / Σ |F|²。
double energyKept(const cv::Mat &re, const cv::Mat &im, const cv::Mat &H);

// 把中心化的 H 逆变换成空域核，并把原点移到画面中心，方便看振铃。
// 只返回中心最多 128×128 的一块。理想低通的核会出现负值。
cv::Mat spatialKernel(const cv::Mat &H);

// 空域线性卷积。边界外补 0，核的几何中心对准当前像素。
cv::Mat convolveSpatial(const cv::Mat &src, const cv::Mat &kernel);

// 用 DFT 做卷积。
// linearPad = false：在原尺寸上做循环卷积，对边的像素会卷到对面。
// linearPad = true：先补零到不小于 N+K−1 的 2 的幂，循环卷积就等于线性卷积，
// 再裁回原来的 N×N。
cv::Mat convolveDft(const cv::Mat &src, const cv::Mat &kernel, bool linearPad);

// 把核中心卷绕到 (0,0)，乘 (−1)^{x+y} 后做 DFT。
// 得到的频谱与中心化图像频谱对齐。返回实部；虚部通过 re、im 传出。
// 平滑核直流大、角落小；拉普拉斯和 Sobel 的直流为 0。
cv::Mat kernelSpectrumRe(const cv::Mat &kernel, int n, cv::Mat &re, cv::Mat &im);

// 一维数值例子。x = [1,2,3,4,5]，h = [1,2,1]，h 的中心是中间那个 2。
// linear 两端不卷绕；circular 会把另一端卷进来；padded 先两端补 0 再循环卷积。
// 三个数组长度都是 5。
void conv1dExample(double linear[5], double circular[5], double padded[5]);

// 循环平移 f(x − dx, y − dy)，下标对 n 取模。
// 幅度谱不变，相位增加线性相位 −2π(u dx + v dy) / n。
cv::Mat circShift(const cv::Mat &src, int dx, int dy);

// 绕图像中心逆时针旋转 degrees 度。逆映射采样，出界填 0。
// 视觉上的逆时针与数学上的标准旋转一致；屏幕 y 向下不影响这个视觉方向。
cv::Mat rotateImage(const cv::Mat &src, double degrees);

// 在中心化幅度谱上，沿 0°～179° 的射线累加能量，返回能量最大的角度。
// 0° 向右，角度增大时向下转，也就是顺时针。
// 图像逆时针转 θ 后，这个读数大约减小 θ。
int dominantAngle(const cv::Mat &re, const cv::Mat &im);

// 叠加水平、竖直两个正弦：A sin(2π f x / n) + A sin(2π f y / n)。
// 中心化频谱会在 (±f, 0) 和 (0, ±f) 出现四个亮点。
cv::Mat addPeriodicNoise(const cv::Mat &src, int frequency, double amplitude);

// JPEG 风格的可分离 8×8 DCT-II。按 zigzag 只保留前 keep 个系数，再反变换。
// blockX、blockY 是要导出的那一块的左上角，必须是 8 的倍数。coeff 可为 nullptr。
// psnr 按 8 位满幅 255 计算。keep = 64 时系数全部保留，PSNR 应为无穷大。
cv::Mat compressDct(const cv::Mat &gray, int keep, int blockX, int blockY,
					double &psnr, cv::Mat *coeff);

// 十字平滑核：中心 1/3，上下左右各 1/6，四角为 0。系数和为 1，直流增益为 1。
cv::Mat smoothKernel();
// 锐化核：中心 9，其余为 −1。系数和为 1，既保留亮度又加强边缘。
cv::Mat sharpenKernel();
// 四邻域拉普拉斯：中心 −4，上下左右为 1。系数和为 0，直流增益为 0。
cv::Mat laplacianKernel();
// 水平 Sobel：检测竖直边缘。系数和为 0，直流增益为 0。
cv::Mat sobelKernel();
// k×k 均值核，每个系数为 1/k²。
cv::Mat boxKernel(int k);
// 1×k 水平运动模糊核，每个系数为 1/k。
cv::Mat motionKernel(int k);
// 构造边长为 k 的正方形二维高斯平滑卷积核（若传入偶数，则自动加 1 使其有唯一中心像素），
// 参数 sigma 为高斯分布的标准差，最终将所有系数之和归一化为 1，以保持图像平均亮度不变。
cv::Mat gaussianKernel(int k, double sigma);
// 构造 3×3 水平梯度卷积核（即索贝尔水平算子），用于检测图像中的竖直边缘。
cv::Mat horizGradKernel();
// 画一组竖直条纹和一条水平暗带，用来观察频谱方向。
cv::Mat makeStripes(int n);

// 对输入图像 src 做一层二维哈尔（Haar）离散小波分解，要求图像边长为偶数。
// 输出四个边长为原图一半的子带（数据类型为双精度浮点矩阵 CV_64F）：
// LL 为低频近似子带，LH 为水平边缘细节子带，HL 为竖直边缘细节子带，HH 为对角细节子带。
void haarDwt2(const cv::Mat &src, cv::Mat &LL, cv::Mat &LH, cv::Mat &HL, cv::Mat &HH);
// 由低频近似子带 LL 与三个高频细节子带 LH、HL、HH 做一层二维哈尔小波逆变换，恢复为与分解前同尺寸的图像。
void haarIdwt2(const cv::Mat &LL, const cv::Mat &LH, const cv::Mat &HL, const cv::Mat &HH, cv::Mat &out);

// 两幅同尺寸 CV_64F 图像的最大绝对差。
double maxAbsDiff(const cv::Mat &a, const cv::Mat &b);
// 两个频谱幅度 |F| 的最大差。平移不变性用它检查。
double maxMagnitudeDiff(const cv::Mat &reA, const cv::Mat &imA,
						const cv::Mat &reB, const cv::Mat &imB);

// 直接截断转换成 8 位。适合灰度已经在 0～255 的图像。
cv::Mat toU8Clip(const cv::Mat &src);
// 按最小、最大值线性拉伸到 0～255。适合相位重建这类没有固定灰度范围的结果。
cv::Mat toU8Stretch(const cv::Mat &src);
// 以 0 为中灰 128，正负对称显示。适合拉普拉斯、空域 sinc 核。
cv::Mat toU8Signed(const cv::Mat &src);
// 对数幅度 log(1 + |F|)，再拉伸到 8 位，否则直流会把其余频率压暗。
cv::Mat logMagnitude(const cv::Mat &re, const cv::Mat &im);
// 相位 atan2(Im, Re) 从 [−π, π] 映射到 0～255。
cv::Mat phaseImage(const cv::Mat &re, const cv::Mat &im);
// 把 H 按峰值显示成 8 位。峰值小于 1 时仍按 1 归一化，避免低通被拉得过亮。
cv::Mat showFilter(const cv::Mat &H);

} // namespace freq
} // namespace kongzi
} // namespace cvrsg

#endif
