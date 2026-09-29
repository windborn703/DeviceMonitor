# ============ 设备模拟器的施工图纸 ============

# 模拟器是控制台程序，不需要 Widgets 界面库，
# 只需要 Network（TCP 网络）模块（Core 会被自动带上）
find_package(Qt6 REQUIRED COMPONENTS Network)

qt_standard_project_setup()

qt_add_executable(DeviceSimulator
    src/main.cpp
    src/DeviceSimulator.h
    src/DeviceSimulator.cpp
)

# 链接公共代码库（协议 + 帧解析器，头文件路径自动传播过来）
target_link_libraries(DeviceSimulator PRIVATE devicecommon Qt6::Network)
