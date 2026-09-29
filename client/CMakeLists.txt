# ============ 上位机客户端的施工图纸 ============

# 找到 Qt6 的 Widgets 模块（Widgets = 桌面界面控件库）
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# Qt 官方推荐的初始化：自动处理 MOC/UIC/RCC 这些 Qt 特有的编译步骤
qt_standard_project_setup()

# 生成可执行程序 DeviceMonitorClient，由下面这些源文件编译而来
qt_add_executable(DeviceMonitorClient
    src/main.cpp
)

# 链接 Qt6::Widgets 库（没有这行，用了 Qt 的代码就编译不过）
target_link_libraries(DeviceMonitorClient PRIVATE Qt6::Widgets)
