@echo off
setlocal
set CODE_ROOT=%~dp0..
set MINGW_BIN=%CODE_ROOT%\lib-3rdparties\mingw-winlibs-16\mingw64\bin
set PATH=%MINGW_BIN%;%PATH%
set OPENCV_ROOT=%CODE_ROOT%\lib-3rdparties\opencv\4.5.2-mingw\OpenCV-MinGW-Build-OpenCV-4.5.2-x64
set OPENCV_BIN=%OPENCV_ROOT%\x64\mingw\bin
set LIB_COMMON=%CODE_ROOT%\lib-common

if not exist bin mkdir bin

g++ -std=c++17 -g -O0 -Wall -o bin\dip_demos.exe ^
  demos\dip_demos.cpp ^
  src\convolution.cpp ^
  src\basicTrans.cpp ^
  %LIB_COMMON%\src\utilities.cpp ^
  %LIB_COMMON%\src\Drawing.cpp ^
  -Iinclude ^
  -I%LIB_COMMON%\include ^
  -I%OPENCV_ROOT%\include ^
  -I%OPENCV_ROOT%\include\opencv2 ^
  -L%OPENCV_ROOT%\x64\mingw\lib ^
  -lopencv_highgui452 -lopencv_imgcodecs452 -lopencv_imgproc452 -lopencv_core452

if errorlevel 1 exit /b 1

copy /Y "%MINGW_BIN%\libgcc_s_seh-1.dll" bin\ >nul
copy /Y "%MINGW_BIN%\libstdc++-6.dll" bin\ >nul
copy /Y "%MINGW_BIN%\libwinpthread-1.dll" bin\ >nul
copy /Y "%OPENCV_BIN%\libopencv_core452.dll" bin\ >nul
copy /Y "%OPENCV_BIN%\libopencv_imgproc452.dll" bin\ >nul
copy /Y "%OPENCV_BIN%\libopencv_imgcodecs452.dll" bin\ >nul
copy /Y "%OPENCV_BIN%\libopencv_highgui452.dll" bin\ >nul

echo Build OK: bin\dip_demos.exe
endlocal
