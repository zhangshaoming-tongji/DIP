# 第三方组件

OpenCV 随仓库提供。WinLibs 编译器不放进仓库，按下面的地址自行下载，解压到原来的路径后，`build.bat` 不用改。

## OpenCV 4.5.2

路径：`opencv/4.5.2-mingw/OpenCV-MinGW-Build-OpenCV-4.5.2-x64/`。

`include/opencv2/core/version.hpp` 为 4.5.2。根上的 `LICENSE` 是 Apache License 2.0 全文。依赖库的许可文本在 `etc/licenses/`（ade、libjpeg-turbo、libopenjp2、libpng、libtiff、opencl-headers、openexr、protobuf、quirc、SoftFloat、zlib）。

课堂程序不调用 `cv::VideoCapture` 或 `cv::VideoWriter`。`build.bat`、`build_no_obj.bat` 和 `run_conv.bat` 都不引用 `opencv_videoio_ffmpeg452_64.dll`。这份 DLL 是 LGPL-2.1 的 FFmpeg 插件，`libopencv_videoio452.dll` 只在运行时按文件名加载它，所以没有放进仓库，并已写入 `.gitignore`。只为它准备的 `etc/licenses/ffmpeg-license.txt` 和 `ffmpeg-readme.txt` 一并去掉。

需要视频读写时，从 OpenCV 4.5.2 官方 Windows 包取出同名文件：

<https://github.com/opencv/opencv/releases/tag/4.5.2>

安装包：<https://github.com/opencv/opencv/releases/download/4.5.2/opencv-4.5.2-vc14_vc15.exe>

包内路径是 `opencv/build/bin/opencv_videoio_ffmpeg452_64.dll`。放到本仓库原来的位置：

```text
code/lib-3rdparties/opencv/4.5.2-mingw/OpenCV-MinGW-Build-OpenCV-4.5.2-x64/x64/mingw/bin/opencv_videoio_ffmpeg452_64.dll
```

与 `libopencv_videoio452.dll` 放在一起即可。

## WinLibs 编译器

版本与本课程原来带的那份相同：`gcc-16.1.0-mingw-w64msvcrt-14.0.0-r4`（64 位，POSIX 线程，MSVCRT，2026-07-30 打包）。里面有 GCC 16.1.0、GDB 17.2、MinGW-w64 14.0.0、GNU Binutils 2.47.20260726、GNU Make 4.4.1、dos2unix 7.5.6、PExports 0.47、Yasm 1.3.0、NASM 3.01、JWasm 2.12pre、ccache 4.13.6、CMake 4.4.1、ninja 1.13.2、Doxygen 1.17.0、pedeps 0.1.15、Universal Ctags 6.2.1、Cppcheck 2.21.0、Premake 5.0.0-beta8、ntldd 0.2、pkgconf 3.0.3、winlibs-tools 1.0.24。

这些程序沿用各自的上游许可。GCC、Binutils、GDB、GNU Make 等是 GPL-3.0；libstdc++ 另有 GCC Runtime Library Exception 3.1；MinGW-w64 是 Zope Public License 2.1，标明为公有领域、BSD 或 LGPL 的部分除外。PExports 0.47 的可执行文件只写了 “GNU General Public License”，没有版本号。winlibs-tools 1.0.24 没有随包许可声明。

下载 Win64 压缩包（与 `.7z` 内容相同，Windows 可直接解压 zip）：

<https://github.com/brechtsanders/winlibs_mingw/releases/download/16.1.0posix-14.0.0-msvcrt-r4/winlibs-x86_64-posix-seh-gcc-16.1.0-mingw-w64msvcrt-14.0.0-r4.zip>

发布页：<https://github.com/brechtsanders/winlibs_mingw/releases/tag/16.1.0posix-14.0.0-msvcrt-r4>

压缩包顶层是 `mingw64/`。在 `code/lib-3rdparties/` 下解压，使目录名为 `mingw-winlibs-16`：

```text
code/lib-3rdparties/mingw-winlibs-16/mingw64/bin/g++.exe
```

`code/image-processing/build.bat` 和 `build_freq.bat` 使用的就是这个路径。该目录已写入 `.gitignore`。
