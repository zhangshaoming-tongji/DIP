@echo off
setlocal
set CODE_ROOT=%~dp0..
set MINGW_BIN=%CODE_ROOT%\lib-3rdparties\mingw-winlibs-16\mingw64\bin
set PATH=%MINGW_BIN%;%PATH%
set OPENCV_ROOT=%CODE_ROOT%\lib-3rdparties\opencv\4.5.2-mingw\OpenCV-MinGW-Build-OpenCV-4.5.2-x64
set OPENCV_BIN=%OPENCV_ROOT%\x64\mingw\bin
set LIB_COMMON=%CODE_ROOT%\lib-common

set CXXFLAGS=-std=c++17 -g -O0 -Wall
set INCLUDES=-Iinclude -I%LIB_COMMON%\include -I%OPENCV_ROOT%\include -I%OPENCV_ROOT%\include\opencv2
set LIBS=-L%OPENCV_ROOT%\x64\mingw\lib -lopencv_highgui452 -lopencv_imgcodecs452 -lopencv_imgproc452 -lopencv_core452

if not exist bin mkdir bin
if not exist build mkdir build

echo [1/6] compile dip_demos.cpp
g++ -c %CXXFLAGS% %INCLUDES% demos\dip_demos.cpp -o build\dip_demos.o
if errorlevel 1 exit /b 1

echo [2/6] compile convolution.cpp
g++ -c %CXXFLAGS% %INCLUDES% src\convolution.cpp -o build\convolution.o
if errorlevel 1 exit /b 1

echo [3/6] compile basicTrans.cpp
g++ -c %CXXFLAGS% %INCLUDES% src\basicTrans.cpp -o build\basicTrans.o
if errorlevel 1 exit /b 1

echo [4/6] compile utilities.cpp
g++ -c %CXXFLAGS% %INCLUDES% %LIB_COMMON%\src\utilities.cpp -o build\utilities.o
if errorlevel 1 exit /b 1

echo [5/6] compile Drawing.cpp
g++ -c %CXXFLAGS% %INCLUDES% %LIB_COMMON%\src\Drawing.cpp -o build\Drawing.o
if errorlevel 1 exit /b 1

echo [6/6] link dip_demos.exe
g++ -o bin\dip_demos.exe ^
  build\dip_demos.o ^
  build\convolution.o ^
  build\basicTrans.o ^
  build\utilities.o ^
  build\Drawing.o ^
  %LIBS%
if errorlevel 1 exit /b 1

copy /Y "%MINGW_BIN%\libgcc_s_seh-1.dll" bin\ >nul
copy /Y "%MINGW_BIN%\libstdc++-6.dll" bin\ >nul
copy /Y "%MINGW_BIN%\libwinpthread-1.dll" bin\ >nul
copy /Y "%OPENCV_BIN%\libopencv_core452.dll" bin\ >nul
copy /Y "%OPENCV_BIN%\libopencv_imgproc452.dll" bin\ >nul
copy /Y "%OPENCV_BIN%\libopencv_imgcodecs452.dll" bin\ >nul
copy /Y "%OPENCV_BIN%\libopencv_highgui452.dll" bin\ >nul

echo Build OK: bin\dip_demos.exe
echo Objects:  build\*.o
dir /b build\*.o
endlocal
