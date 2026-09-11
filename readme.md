# Screen Time

一个基于 Qt 开发的 Windows 程序使用时间统计工具，帮助我们更好的使用电脑

可统计当天和过去七天内，电脑使用总时间及各应用程序的使用时间。

## 效果演示

![主界面](./screenshots/mainwindow.png)
![设置](./screenshots/setting.png)
![AI周报](./screenshots/report.png)
## 功能

1.【功能】支持查看当天和近七天两种使用状况

2.【功能】使用情况会以柱状图方式显示

3.【功能】鼠标悬停至柱状图会显示对映时段使用总时间及使用时间最长的三个程序

4.【功能】柱状图下方的"应用统计"会详细统计各程序统计情况

5.【功能】你可以在主界面拖动中间的横条来调整"条形统计图"和"应用统计的大小"

6.【设置】支持开机自启动，可以选择后台启动到托盘 或者 启动后显示程序

7.【设置】支持多套界面主题：跟随系统、浅色、深色，以及六套中国传统色主题
（鸢尾蓝、苋菜红、蕈紫、葱油绿、青矾绿、紫幽兰，均为纸白底 + 主色）。
可另选强调色（跟随主题 / 紫色 / 蓝色 / 橙色）。
主题由统一的语义化颜色 Token + Design Token 驱动，新增主题只需填写一套颜色，
无需修改任何界面代码（详见 `ui/theme/`）。

8.【其它】你可以从设置里面跳转到本项目的github地址，以便下载最新版本

9.【设置】支持界面语言切换（设置项名称固定为英文 "Language"）：简体中文、繁體中文、
English、Deutsch、Français、Nederlands、Українська、हिन्दी、Тоҷикӣ。
翻译由 `ui/i18n/` 统一管理，切换后界面即时刷新，无需重启。

## 运行要求

|系统|Windows 10 及以上|
|----|---------------|
|CPU|能转就行|
|内存|20MB及以上|
|存储|100MB及以上|

>第一次写readme，写的不好请见谅

## 打包

支持三种分发形式，均在 `release-package/` 目录下生成：

| 形式 | 说明 |
|----|----|
| 免安装 zip | `ScreenTime_<版本>.zip`，解压即用 |
| Inno Setup 安装包 | `ScreenTime_Setup_<版本>.exe` |
| MSIX 包 | `msix/ScreenTime_<版本>.msix`（未签名） |
| 商店上传包 | `msix/ScreenTime_<版本>.msixupload`（上传 Partner Center） |

一键打包：

```bat
deploy-scripts\pack_release.bat
```

MSIX 版本的开机自启动通过系统 `startupTask` 实现（MSIX 下注册表 Run 项会被虚拟化而失效），
详见 [installer/msix/README.md](installer/msix/README.md)。