@echo off
setlocal
set CODE_ROOT=%~dp0..
set MINGW_BIN=%CODE_ROOT%\lib-3rdparties\mingw-winlibs-16\mingw64\bin
set OPENCV_BIN=%CODE_ROOT%\lib-3rdparties\opencv\4.5.2-mingw\OpenCV-MinGW-Build-OpenCV-4.5.2-x64\x64\mingw\bin
set PATH=%OPENCV_BIN%;%MINGW_BIN%;%PATH%
cd /d "%~dp0"
bin\dip_demos.exe "%~dp0data/" conv_src.bmp conv_k_blur_5_5.txt
endlocal
