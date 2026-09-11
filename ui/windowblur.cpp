#include "windowblur.h"

#include <QtGlobal>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>

#include <QWidget>

namespace {

// 老版本 Windows SDK 的 dwmapi.h 可能没有收录这些新枚举，用数字常量兜底。
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif

constexpr DWORD kDwmwaLegacyDarkMode = 19;
constexpr int kDwmcpRound = 2; // DWMWCP_ROUND

} // namespace
#endif // Q_OS_WIN

namespace WindowBlur {

void applyFrame(QWidget *window, bool dark)
{
#ifdef Q_OS_WIN
    if (!window) {
        return;
    }
    HWND hwnd = reinterpret_cast<HWND>(window->winId());
    if (!hwnd) {
        return;
    }

    const BOOL darkFlag = dark ? TRUE : FALSE;
    if (FAILED(DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkFlag, sizeof(darkFlag)))) {
        DwmSetWindowAttribute(hwnd, kDwmwaLegacyDarkMode, &darkFlag, sizeof(darkFlag));
    }

    const int corner = kDwmcpRound;
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));
#else
    Q_UNUSED(window);
    Q_UNUSED(dark);
#endif
}

} // namespace WindowBlur
