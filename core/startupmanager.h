#ifndef STARTUPMANAGER_H
#define STARTUPMANAGER_H

#include <QString>

// 统一封装“开机自启动”的实现。
//
// - 传统 exe / Inno Setup 安装：写入 HKCU\...\Run 注册表项。
// - MSIX / AppX 打包（runFullTrust 桌面应用）：注册表 Run 项会被容器虚拟化，
//   永远不会生效，必须改用清单里声明的 startupTask 扩展 + WinRT
//   Windows.ApplicationModel.StartupTask API。
//
// 本类会自动判断当前进程是否运行在 MSIX 包内并选择正确的实现方式。
class StartupManager
{
public:
    // 当前进程是否运行在 MSIX/AppX 包上下文中。
    static bool isPackaged();

    // 当前系统是否支持在应用内切换开机自启动。
    static bool isAutoStartSupported();

    // 是否已经开启开机自启动。
    static bool isAutoStartEnabled();

    // 开启 / 关闭开机自启动，成功返回 true。
    static bool setAutoStartEnabled(bool enabled);

    // 本次进程是否由“开机自启动”拉起（MSIX 下为 startupTask 激活）。
    static bool launchedByAutoStart();

    // 清单中声明的 startupTask TaskId，必须与 AppxManifest.xml 保持一致。
    static const char *startupTaskId();

    // 隐藏的辅助进程入口：执行一次 WinRT 操作并返回退出码。
    // 由主程序通过 CreateProcess 拉起，崩溃/卡死都不会影响主程序。
    static int runWinRtHelper(const QString &operation);

private:
    StartupManager() = delete;
};

#endif // STARTUPMANAGER_H
