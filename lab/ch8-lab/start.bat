@echo off
cd /d %~dp0
echo 频域分析课堂演示: http://127.0.0.1:8766/
echo 关闭本窗口即停止服务。
python -m http.server 8766
