#ifndef APPNAMERESOLVER_H
#define APPNAMERESOLVER_H

#include <QString>

// 将进程名/路径转为可读中文名；未知则返回空，由 AI 结合路径推断
QString resolveAppDisplayName(const QString &processName, const QString &appPath = QString());

// 生成供 AI 参考的进程识别行（含本地已知映射）
QString formatProcessHintLine(const QString &processName,
                              const QString &appPath,
                              int durationSeconds);

#endif // APPNAMERESOLVER_H
