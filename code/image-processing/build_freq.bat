@echo off
setlocal
set CODE_ROOT=%~dp0..
set MINGW_BIN=%CODE_ROOT%\lib-3rdparties\mingw-winlibs-16\mingw64\bin
set PATH=%MINGW_BIN%;%PATH%
set OPENCV_ROOT=%CODE_ROOT%\lib-3rdparties\opencv\4.5.2-mingw\OpenCV-MinGW-Build-OpenCV-4.5.2-x64
set OPENCV_BIN=%OPENCV_ROOT%\x64\mingw\bin

set CXXFLAGS=-std=c++17 -O2 -Wall -finput-charset=UTF-8 -fexec-charset=UTF-8
set INCLUDES=-Iinclude -I%OPENCV_ROOT%\include
set LIBS=-L%OPENCV_ROOT%\x64\mingw\lib -lopencv_imgcodecs452 -lopencv_imgproc452 -lopencv_core452

if not exist bin mkdir bin
if not exist build mkdir build

echo compile freqDomain.cpp
g++ -c %CXXFLAGS% %INCLUDES% src\freqDomain.cpp -o build\freqDomain.o
if errorlevel 1 exit /b 1

echo compile freq_homework.cpp
g++ -c %CXXFLAGS% %INCLUDES% demos\freq_homework.cpp -o build\freq_homework.o
if errorlevel 1 exit /b 1

echo link bin\freq_homework.exe
g++ -o bin\freq_homework.exe build\freq_homework.o build\freqDomain.o %LIBS%
if errorlevel 1 exit /b 1

copy /Y "%MINGW_BIN%\libgcc_s_seh-1.dll" bin\ >nul
copy /Y "%MINGW_BIN%\libstdc++-6.dll" bin\ >nul
copy /Y "%MINGW_BIN%\libwinpthread-1.dll" bin\ >nul
copy /Y "%OPENCV_BIN%\libopencv_core452.dll" bin\ >nul
copy /Y "%OPENCV_BIN%\libopencv_imgproc452.dll" bin\ >nul
copy /Y "%OPENCV_BIN%\libopencv_imgcodecs452.dll" bin\ >nul
copy /Y "%OPENCV_BIN%\libopencv_highgui452.dll" bin\ >nul

echo Build OK: bin\freq_homework.exe
endlocal
