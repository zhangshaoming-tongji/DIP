# MinGW build for kongzi DIP demos.
# Reuses lib-common sources and OpenCV 4.5.2 MinGW binaries under PATH_CVRSG.

PATH_CVRSG ?= D:/works/Projects/cvrsg-codes
CXX       ?= $(PATH_CVRSG)/lib-3rdparties/mingw-winlibs-16/mingw64/bin/g++
CXXFLAGS  ?= -std=c++17 -g -O0 -Wall

OPENCV_ROOT := $(PATH_CVRSG)/lib-3rdparties/opencv/4.5.2-mingw/OpenCV-MinGW-Build-OpenCV-4.5.2-x64
COMMON_INC  := $(PATH_CVRSG)/lib-common/include
COMMON_SRC  := $(PATH_CVRSG)/lib-common/src

INCLUDES := \
	-Iinclude \
	-I$(COMMON_INC) \
	-I$(OPENCV_ROOT)/include \
	-I$(OPENCV_ROOT)/include/opencv2

LIBS := \
	-L$(OPENCV_ROOT)/x64/mingw/lib \
	-lopencv_highgui452 \
	-lopencv_imgcodecs452 \
	-lopencv_imgproc452 \
	-lopencv_core452

SRCS := \
	demos/dip_demos.cpp \
	src/convolution.cpp \
	src/basicTrans.cpp \
	$(COMMON_SRC)/utilities.cpp \
	$(COMMON_SRC)/Drawing.cpp

TARGET := bin/dip_demos.exe

.PHONY: all clean run-conv

all: $(TARGET)

$(TARGET): $(SRCS)
	@if not exist bin mkdir bin
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(SRCS) $(LIBS)

clean:
	@if exist bin\dip_demos.exe del /q bin\dip_demos.exe

run-conv: $(TARGET)
	$(TARGET) data/ conv_src.bmp conv_k_blur_5_5.txt
