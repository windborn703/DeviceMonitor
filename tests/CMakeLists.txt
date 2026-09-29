# ============ 测试程序的施工图纸 ============
# 纯 C++ 控制台程序，故意不链接 Qt —— 验证协议层与 Qt 解耦：
# 协议代码不依赖任何 Qt 类，将来可以直接拿到别的环境用

add_executable(test_protocol test_protocol.cpp)

# 告诉编译器去哪找 protocol.h（common 是共享目录）
target_include_directories(test_protocol PRIVATE ${CMAKE_SOURCE_DIR}/common)
