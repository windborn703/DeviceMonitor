# ============ 公共代码库：协议 + 帧解析器 ============
# 做成静态库（.a）：编译一次，模拟器/客户端/测试三个程序共用。
# 面试可讲：为什么抽静态库？—— 避免三处重复编译同一份代码，
# 且协议逻辑改一处、全体生效；将来还能给其他项目复用。

find_package(Qt6 REQUIRED COMPONENTS Core)

# 关键：启用 Qt 特有的自动处理（AUTOMOC 等）。
# 没有这行，Q_OBJECT 类的 moc 代码不会生成 → 链接报
# "undefined reference to vtable/staticMetaObject"（我们刚踩的坑）
qt_standard_project_setup()

qt_add_library(devicecommon STATIC
    FrameParser.h
    FrameParser.cpp
)

# PUBLIC 关键字 = "传染性"传播：
# 谁链接 devicecommon，谁就自动获得 common 目录的 include 路径
# （protocol.h、console_utf8.h 都在这个目录下）
target_include_directories(devicecommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# FrameParser 用了 QObject/信号槽，需要 Qt Core
target_link_libraries(devicecommon PUBLIC Qt6::Core)
