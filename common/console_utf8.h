// ============ 控制台 UTF-8 输出的终极方案 ============
// 背景（面试可聊的 Windows 编码知识点）：
//   Windows 控制台内部原生使用 Unicode（UTF-16）。
//   但 C 运行库（msvcrt）的 printf 输出窄字符串时，会按系统 ANSI 代码页
//   （中文系统是 GBK）先转换一遍——我们的 UTF-8 字节被它当 GBK 转坏，
//   之后设任何控制台代码页都救不回来。
//
// 方案：用 Windows API WriteConsoleW 直接把 UTF-16 写进控制台，
//   绕开 msvcrt 的转换，任何终端（CMD / Windows Terminal / 直连场景）都正确。
//   若 stdout 被重定向（管道/文件），WriteConsoleW 会失败，自动回退 printf。
//
// 注意：本文件依赖 Qt Core（QString），与 protocol.h（纯 C++）不同。
#pragma once

#ifdef Q_OS_WIN
#include <windows.h>
#include <QString>
#include <cstdarg>
#include <cstdio>

// 初始化控制台代码页（对 CMD 直连场景仍有帮助，双保险）
inline void consoleInitUtf8()
{
    SetConsoleOutputCP(CP_UTF8);  // 输出代码页 → UTF-8 (65001)
    SetConsoleCP(CP_UTF8);        // 输入代码页 → UTF-8（键盘输入中文用）
}

// 万能打印：printf 风格的用法，但中文永远不乱码
inline void printUtf8(const char *fmt, ...)
{
    char buf[4096];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);  // 先格式化到 UTF-8 缓冲区
    va_end(args);

    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h != INVALID_HANDLE_VALUE && h != nullptr) {
        QString text = QString::fromUtf8(buf);  // UTF-8 字节 → Qt 的 UTF-16 字符串
        DWORD written = 0;
        // WriteConsoleW 成功 → 控制台正确显示，任务完成
        if (WriteConsoleW(h, text.utf16(), DWORD(text.size()), &written, nullptr))
            return;
    }
    printf("%s", buf);  // 回退：stdout 不是控制台（重定向到文件/管道）时
}

#else  // 非 Windows（Linux/macOS）：终端天然 UTF-8，普通 printf 就够
#include <cstdarg>
#include <cstdio>
inline void consoleInitUtf8() {}
inline void printUtf8(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}
#endif
