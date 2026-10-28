# DIP

数字图像处理课程资料（2025 秋季）。目前公开第 2 讲到第 5 讲：基础知识、灰度与几何变换、卷积滤波，以及这次内容的作业辅导。后面的课会陆续放出。课件在根目录，课堂演示在 `lab/`，插图在 `canvas/`，上机代码在 `code/`。

作者：[张绍明](https://celiang.tongji.edu.cn/info/1301/2420.htm)，同济大学。

## 课件

根目录 `数字图像处理202509-*.pptx`，文件名用两位课次编号，按文件名排序即上课顺序。

| 文件 | 内容 |
| --- | --- |
| `202509-02` | 图像处理基础知识 |
| `202509-03` | 基本变换：灰度变换与几何变换 |
| `202509-04` | 邻域变换：卷积计算与图像滤波 |
| `202509-05` | 作业辅导 1：基本变换与卷积 |

## 课堂演示

浏览器页面，不需要安装依赖。带 `start.bat` 的目录双击即可，它会在本机起一个静态服务；关掉窗口服务就停。

| 目录 | 内容 | 打开方式 |
| --- | --- | --- |
| `lab/ch2-lab-hsi-hsv-demo` | 用狒狒图对比 HSI 与 HSV | `start.bat`，<http://127.0.0.1:8765/> |
| `lab/ch3-lab` | 灰度变换、直方图、代数运算与几何变换 | 浏览器直接打开 `index.html` |
| `lab/ch4-lab` | 最大最小、均值、高斯、可分离卷积、双边、联合双边、引导滤波 | `start.bat`，<http://127.0.0.1:8767/> |

第 4 章演示里，引导滤波用课件上的两例：郁金香细节增强，以及硬掩模配毛发引导图的结构迁移。

## 插图

`canvas/ch2` 到 `canvas/ch4` 放对应课件用的图和生成脚本，包括采样、量化、几何变换，以及一维、二维卷积的示意图和动图。

## 上机代码

`code/image-processing` 是 MinGW 下的课堂程序，源码在 `src/`，示例入口在 `demos/`。这一版只含灰度变换、几何变换和卷积滤波。OpenCV 4.5.2 在 `code/lib-3rdparties/opencv`。编译器不放进仓库：下载 WinLibs `gcc-16.1.0-mingw-w64msvcrt-14.0.0-r4` 的 Win64 压缩包，解压到 `code/lib-3rdparties/mingw-winlibs-16/`，使 `mingw64/bin/g++.exe` 位于其下。下载地址和解压步骤见 `code/lib-3rdparties/README.md`。`build.bat` 按这个路径找编译器。灰度与卷积用到的公共代码在 `code/lib-common`（`utilities`、`Drawing`）。

在 `code/image-processing` 下，`build.bat` 编译 `demos/dip_demos.cpp`、`src/basicTrans.cpp`、`src/convolution.cpp`，得到 `bin/dip_demos.exe`。`run_conv.bat` 用 `data/conv_src.bmp` 跑一维、二维平滑。

## 开源许可

课件（根目录 `*.pptx`）、`lab/` 里的页面和图片、`canvas/` 里的图，使用知识共享署名-非商业性使用-相同方式共享 4.0（CC BY-NC-SA 4.0）。法律文本在 `LICENSE-CC-BY-NC-SA-4.0`，与 <https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.txt> 一致。

`code/image-processing`、`code/lib-common`，以及 `canvas/`、`lab/` 里的脚本（`.py`、`.js`、`.bat`）使用 MIT 许可，文本在 `LICENSE`。版权人是[张绍明](https://celiang.tongji.edu.cn/info/1301/2420.htm)（Shaoming Zhang），2025–2026。

署名请写：[张绍明](https://celiang.tongji.edu.cn/info/1301/2420.htm)，同济大学，<https://github.com/zhangshaoming-tongji/DIP> 。分享改过的课件时，保留署名、标明改动，并继续以 CC BY-NC-SA 4.0 提供。按该许可，这些课件和演示材料只用于非商业目的。

`code/lib-3rdparties/opencv` 里的 OpenCV 4.5.2 是 Apache License 2.0。课堂程序不用视频读写，因此 LGPL 的 `opencv_videoio_ffmpeg452_64.dll` 不放进仓库；需要 `cv::VideoCapture` 或 `cv::VideoWriter` 时，从 OpenCV 4.5.2 官方包取出该文件再放到原来的 `bin` 目录，见 `code/lib-3rdparties/README.md`。WinLibs 工具链也不随仓库分发，解压到本机后仍用它自己的上游许可（GCC、Binutils、GDB、GNU Make 等为 GPL-3.0，MinGW-w64 为 ZPL-2.1 及文件中标明的例外）。

## 贡献

欢迎提 issue 和 pull request。仓库写权限在维护者张绍明这里。
