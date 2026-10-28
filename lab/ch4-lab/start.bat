@echo off
cd /d %~dp0
echo 第4章 图像滤波课堂演示: http://127.0.0.1:8767/
echo 关闭本窗口即停止服务。
python -m http.server 8767
