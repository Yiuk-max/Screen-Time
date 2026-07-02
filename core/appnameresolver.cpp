#include "appnameresolver.h"

#include <QFileInfo>
#include <QHash>

namespace {

QString processBaseName(const QString &processName, const QString &appPath)
{
    if (!appPath.isEmpty()) {
        const QFileInfo info(appPath);
        if (!info.fileName().isEmpty()) {
            return info.fileName();
        }
    }
    return processName;
}

QString normalizedKey(const QString &processName, const QString &appPath)
{
    return processBaseName(processName, appPath).toLower();
}

QString lookupKnownName(const QString &key)
{
    static const QHash<QString, QString> known = {
        {QStringLiteral("msedge"), QStringLiteral("微软 Edge 浏览器")},
        {QStringLiteral("msedge.exe"), QStringLiteral("微软 Edge 浏览器")},
        {QStringLiteral("chrome"), QStringLiteral("Google Chrome 浏览器")},
        {QStringLiteral("chrome.exe"), QStringLiteral("Google Chrome 浏览器")},
        {QStringLiteral("firefox"), QStringLiteral("Firefox 浏览器")},
        {QStringLiteral("firefox.exe"), QStringLiteral("Firefox 浏览器")},
        {QStringLiteral("brave"), QStringLiteral("Brave 浏览器")},
        {QStringLiteral("brave.exe"), QStringLiteral("Brave 浏览器")},
        {QStringLiteral("lockapp"), QStringLiteral("Windows 锁屏")},
        {QStringLiteral("lockapp.exe"), QStringLiteral("Windows 锁屏")},
        {QStringLiteral("htgame"), QStringLiteral("异环（游戏）")},
        {QStringLiteral("htgame.exe"), QStringLiteral("异环（游戏）")},
        {QStringLiteral("screentime"), QStringLiteral("Screen Time（本程序）")},
        {QStringLiteral("screentime.exe"), QStringLiteral("Screen Time（本程序）")},
        {QStringLiteral("explorer"), QStringLiteral("文件资源管理器")},
        {QStringLiteral("explorer.exe"), QStringLiteral("文件资源管理器")},
        {QStringLiteral("searchhost"), QStringLiteral("Windows 搜索")},
        {QStringLiteral("searchhost.exe"), QStringLiteral("Windows 搜索")},
        {QStringLiteral("wechat"), QStringLiteral("微信")},
        {QStringLiteral("wechat.exe"), QStringLiteral("微信")},
        {QStringLiteral("weixin"), QStringLiteral("微信")},
        {QStringLiteral("weixin.exe"), QStringLiteral("微信")},
        {QStringLiteral("qq"), QStringLiteral("QQ")},
        {QStringLiteral("qq.exe"), QStringLiteral("QQ")},
        {QStringLiteral("discord"), QStringLiteral("Discord")},
        {QStringLiteral("discord.exe"), QStringLiteral("Discord")},
        {QStringLiteral("code"), QStringLiteral("Visual Studio Code")},
        {QStringLiteral("code.exe"), QStringLiteral("Visual Studio Code")},
        {QStringLiteral("cursor"), QStringLiteral("Cursor 编辑器")},
        {QStringLiteral("cursor.exe"), QStringLiteral("Cursor 编辑器")},
        {QStringLiteral("devenv"), QStringLiteral("Visual Studio")},
        {QStringLiteral("devenv.exe"), QStringLiteral("Visual Studio")},
        {QStringLiteral("idea64"), QStringLiteral("IntelliJ IDEA")},
        {QStringLiteral("idea64.exe"), QStringLiteral("IntelliJ IDEA")},
        {QStringLiteral("pycharm64"), QStringLiteral("PyCharm")},
        {QStringLiteral("pycharm64.exe"), QStringLiteral("PyCharm")},
        {QStringLiteral("winword"), QStringLiteral("Microsoft Word")},
        {QStringLiteral("winword.exe"), QStringLiteral("Microsoft Word")},
        {QStringLiteral("excel"), QStringLiteral("Microsoft Excel")},
        {QStringLiteral("excel.exe"), QStringLiteral("Microsoft Excel")},
        {QStringLiteral("powerpnt"), QStringLiteral("Microsoft PowerPoint")},
        {QStringLiteral("powerpnt.exe"), QStringLiteral("Microsoft PowerPoint")},
        {QStringLiteral("steam"), QStringLiteral("Steam")},
        {QStringLiteral("steam.exe"), QStringLiteral("Steam")},
        {QStringLiteral("spotify"), QStringLiteral("Spotify")},
        {QStringLiteral("spotify.exe"), QStringLiteral("Spotify")},
        {QStringLiteral("notepad"), QStringLiteral("记事本")},
        {QStringLiteral("notepad.exe"), QStringLiteral("记事本")},
        {QStringLiteral("dwm"), QStringLiteral("桌面窗口管理器")},
        {QStringLiteral("dwm.exe"), QStringLiteral("桌面窗口管理器")},
        {QStringLiteral("taskmgr"), QStringLiteral("任务管理器")},
        {QStringLiteral("taskmgr.exe"), QStringLiteral("任务管理器")},
        {QStringLiteral("bilibili"), QStringLiteral("哔哩哔哩")},
        {QStringLiteral("bilibili.exe"), QStringLiteral("哔哩哔哩")},
        {QStringLiteral("cloudmusic"), QStringLiteral("网易云音乐")},
        {QStringLiteral("cloudmusic.exe"), QStringLiteral("网易云音乐")},
        {QStringLiteral("qqmusic"), QStringLiteral("QQ 音乐")},
        {QStringLiteral("qqmusic.exe"), QStringLiteral("QQ 音乐")},
        {QStringLiteral("douyin"), QStringLiteral("抖音")},
        {QStringLiteral("douyin.exe"), QStringLiteral("抖音")},
        {QStringLiteral("feishu"), QStringLiteral("飞书")},
        {QStringLiteral("feishu.exe"), QStringLiteral("飞书")},
        {QStringLiteral("dingtalk"), QStringLiteral("钉钉")},
        {QStringLiteral("dingtalk.exe"), QStringLiteral("钉钉")},
        {QStringLiteral("teams"), QStringLiteral("Microsoft Teams")},
        {QStringLiteral("teams.exe"), QStringLiteral("Microsoft Teams")},
        {QStringLiteral("slack"), QStringLiteral("Slack")},
        {QStringLiteral("slack.exe"), QStringLiteral("Slack")},
        {QStringLiteral("obs64"), QStringLiteral("OBS Studio")},
        {QStringLiteral("obs64.exe"), QStringLiteral("OBS Studio")},
        {QStringLiteral("vlc"), QStringLiteral("VLC 播放器")},
        {QStringLiteral("vlc.exe"), QStringLiteral("VLC 播放器")},
    };

    if (known.contains(key)) {
        return known.value(key);
    }

    const QString withoutExe = key.endsWith(QStringLiteral(".exe"))
        ? key.chopped(4)
        : key;
    if (known.contains(withoutExe)) {
        return known.value(withoutExe);
    }
    return QString();
}

QString formatDurationShort(int seconds)
{
    const int hours = seconds / 3600;
    const int minutes = (seconds % 3600) / 60;
    if (hours > 0) {
        return QStringLiteral("%1小时%2分钟").arg(hours).arg(minutes);
    }
    if (minutes > 0) {
        return QStringLiteral("%1分钟").arg(minutes);
    }
    return QStringLiteral("不足1分钟");
}

} // namespace

QString resolveAppDisplayName(const QString &processName, const QString &appPath)
{
    const QString key = normalizedKey(processName, appPath);
    const QString known = lookupKnownName(key);
    if (!known.isEmpty()) {
        return known;
    }
    return QString();
}

QString formatProcessHintLine(const QString &processName,
                              const QString &appPath,
                              int durationSeconds)
{
    const QString base = processBaseName(processName, appPath);
    const QString known = resolveAppDisplayName(processName, appPath);
    QString line = QStringLiteral("- 进程: %1，时长: %2").arg(base, formatDurationShort(durationSeconds));
    if (!known.isEmpty()) {
        line += QStringLiteral("，本地识别: %1").arg(known);
    }
    if (!appPath.isEmpty()) {
        line += QStringLiteral("，路径: %1").arg(appPath);
    }
    return line;
}
