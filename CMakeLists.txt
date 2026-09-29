# ============ 顶层施工图纸 ============
# 这个文件管理整个项目：下面两个子项目（client 和 simulator）都归它管

cmake_minimum_required(VERSION 3.21)  # 要求 CMake 版本至少 3.21
project(DeviceMonitor VERSION 0.1 LANGUAGES CXX)  # 项目名 + 版本 + 语言

# 整个项目统一用 C++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 两个子项目：各自的目录里有自己的 CMakeLists.txt
add_subdirectory(client)      # 上位机客户端（简历主角）
add_subdirectory(simulator)   # 设备模拟器（虚拟工业设备）
