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

target_link_libraries(DeviceSimulator PRIVATE Qt6::Network)

# 让它能找到共享协议头文件 protocol.h
target_include_directories(DeviceSimulator PRIVATE ${CMAKE_SOURCE_DIR}/common)
