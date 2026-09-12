# MSIX 打包 / 上架微软商店说明

## 一、开机自启动在 MSIX 下为什么必须改

旧版本通过写注册表实现开机自启动：

```
HKCU\Software\Microsoft\Windows\CurrentVersion\Run  ->  ScreenTime = "…\ScreenTime.exe" --autostart
```

这对普通 exe（Inno Setup 安装）有效，但**对 MSIX 包无效**：

1. **注册表虚拟化**：MSIX 包内进程运行在容器里，写
   `HKCU\...\Run` 会被重定向到该包的私有 registry hive，系统登录时根本看不到这条记录。
2. **自启动由系统托管**：打包应用的开机自启动必须由系统统一管理
   （“任务管理器 → 启动应用”里的条目），应用不能私自注册。

所以无论用 Inno Setup 先出 exe 再用“MSIX 打包工具”转换，还是直接用 `makeappx`，
**只要清单里没有声明 startupTask，开机自启动就不会生效**。

## 二、正确做法（已在本项目中实现）

### 1. 清单声明 `windows.startupTask`

见 `installer/msix/AppxManifest.xml`：

```xml
<desktop:Extension Category="windows.startupTask"
                   Executable="ScreenTime.exe"
                   EntryPoint="Windows.FullTrustApplication">
  <desktop:StartupTask TaskId="ScreenTimeStartupTask"
                       Enabled="true"
                       DisplayName="Screen Time"
                       rescap5:ImmediateRegistration="true"/>
</desktop:Extension>
```

`rescap5:ImmediateRegistration="true"` 让任务在**安装后立即注册**，
`Enabled="true"` 让任务在安装后**立即启用**，用户不需要先手动启动一次程序。

> 为什么不用 `Enabled="false"` 让程序首次运行再去开：
> `Enabled="true"` 配合 `ImmediateRegistration` 能保证安装后立即注册并启用；
> 即使应用尚未首次运行或设置页查询失败，自启动任务仍然存在。用户之后仍可
> 在程序设置或“任务管理器 → 启动应用”里关闭。

### 2. 程序内读取 / 开关

封装在 `core/startupmanager.h` / `core/startupmanager.cpp`：

- `isAutoStartEnabled()`  → 辅助进程调 `StartupTask.get_State()`；
  查询不可用时按清单默认值返回「已启用」
- `setAutoStartEnabled()` → 辅助进程调 `StartupTask.RequestEnableAsync()` / `Disable()`
- `launchedByAutoStart()` → **父进程判断**（不依赖会崩的 API）：
  - 命令行含 `--autostart`（普通 exe / Inno Setup）→ true
  - MSIX：父进程是 explorer / 终端等 → 用户启动，显示主界面；
    父进程是 svchost 等系统进程 → startupTask 拉起，进托盘
- `runWinRtHelper()` → 隐藏的辅助进程入口（`ScreenTime.exe --winrt-helper <op>`）

### 3. WinRT ABI

`startupmanager.cpp` 会在 CMake 找到 Windows SDK 的 `cppwinrt` 头文件时，
单独使用 C++20 和官方 **C++/WinRT** 调用 `StartupTask`；项目其余代码仍是
C++17。这样避免 MinGW 下手写参数化 `IAsyncOperation<T>` ABI 在部分
Windows 10 机器上于 `GetAsync/GetResults` 附近触发 `0xC0000005`。

构建机若没有 C++/WinRT 头文件，源码仍保留最小 ABI 回退实现，但不建议用
该构建产物发布 MSIX。正式发布前请确认 CMake 输出包含：

```
Screen Time: using C++/WinRT from ...
```

### 4. 崩溃隔离

所有 `Windows.ApplicationModel` 自启动相关调用都在一个独立的辅助进程里执行
（`CreateProcess` 拉起，`WaitForSingleObject` 等 3s）。辅助进程崩溃/卡死不
影响主程序，保证界面一定能打开。

`TaskId` 必须与 `StartupManager::startupTaskId()` 里的字符串完全一致。

未打包（普通 exe / Inno Setup）时，代码自动回退到注册表 Run 方案，两套逻辑共用一个开关。

## 三、打包（Store 版，无需本地证书）

前置：Windows 10/11 SDK（提供 `makeappx.exe`）、Qt MinGW 套件。

```bat
REM 完整流程：构建 -> 部署 Qt 依赖 -> Inno Setup -> MSIX
deploy-scripts\pack_release.bat

REM 或只重新生成 MSIX（假设 release-package\ScreenTime 已是最新）
deploy-scripts\pack_msix.bat
```

产物：

```
release-package\msix\ScreenTime_<版本>.msix        未签名包
release-package\msix\ScreenTime_<版本>.msixupload  上传到 Partner Center 用
```

**上架微软商店时包不需要本地签名**，Partner Center 会用自己的证书重新签名。
因此默认直接输出未签名包。

## 四、上架微软商店

1. 在 Partner Center 创建应用，打开 **产品 → 产品标识 / App identity**，记下：
   - `Package/Identity/Name`，形如 `12345Yiuk-max.ScreenTime`
   - `Package/Identity/Publisher`，形如 `CN=1A2B3C4D-……`
2. 让打包脚本使用这两个值。可以写进 `installer/msix/store-identity.json`：

   ```json
   {
     "identityName": "12345Yiuk-max.ScreenTime",
     "publisher": "CN=1A2B3C4D-1234-5678-9ABC-DEF012345678"
   }
   ```

   也可以每次用参数：

   ```bat
   deploy-scripts\pack_msix.bat -IdentityName "12345Yiuk-max.ScreenTime" -Publisher "CN=1A2B……"
   ```

3. 重新打包，然后在 Partner Center 上传 `release-package\msix\ScreenTime_<版本>.msixupload`。

> 清单里的 `Identity Name` 和 `Publisher` 必须和 Partner Center 完全一致，
> 否则提交时会报身份不匹配。

## 五、本地测试（旁加载）

未签名包无法直接 `Add-AppxPackage`，本地测试二选一：

**A. 用开发证书签名后安装（需要管理员）**

```bat
deploy-scripts\pack_msix.bat -Sign -DevSign -InstallCertificate
Add-AppxPackage -Path release-package\msix\ScreenTime_2.0.msix
```

`-InstallCertificate` 会把生成的 `ScreenTimeDev.cer` 装进
`LocalMachine\TrustedPeople` / `Root`，只需做一次。
（`-DevSign` 生成的是**终端实体**代码签名证书；旧仓库里的
`installer\YiukLabs.pfx` 是 CA 证书，不能用于 MSIX 签名。）

**B. 开发者模式直接注册解包目录（最简单，但不注册 startupTask）**

```powershell
Add-AppxPackage -Register "release-package\msix\layout\AppxManifest.xml"
```

## 六、验证开机自启动

1. 安装完成后打开 **任务管理器 → 启动应用**，应能看到 **Screen Time**，状态“已启用”
   （清单里 `Enabled="true"`，安装即启用，不需要先启动程序）。
2. 重启电脑，程序应直接进托盘（默认“托盘”模式）。
3. 设置 → 开机启动方式 可选“托盘 / 主界面”。

> 调试日志：`%USERPROFILE%\ScreenTime_winrt.log`，正常会看到类似
> `launch: parent=svchost.exe user=0`、`helper: query exit=...`。

## 七、常见问题

- **启动应用里没有条目**：确认清单含 `windows.startupTask` 且
  `rescap5:ImmediateRegistration="true"`；用真实安装（Store 或签名 msix）而非 `-Register`。
- **从旧版本升级后自启动异常**：请先卸载旧包再装新包（安装脚本会自动卸载），
  旧版残留的启动项状态/“WinRT 已禁用”标记会一并清掉。
- **开关打不开 / 关了又打不开**：先检查构建日志是否启用了 C++/WinRT；
  仍失败时可到「任务管理器 → 启动应用」手动操作。用户曾在任务管理器禁用时，
  Windows 可能返回 `DisabledByUser`，应用不能绕过用户选择重新开启。
- **开机启动后停在主界面而不是托盘**：程序用父进程名判断。若日志里是
  `launch: parent=<某个 shell 进程> user=1`，就会被当成用户启动而显示主界面；
  如果是 `user=0`，则会进托盘。必要时把日志发回来。
- **`0x800B0109` / 清单读取失败 `0x87E80034`**：签名证书有问题。
  用于 MSIX 的证书必须是终端实体代码签名证书，不能是 `BasicConstraints CA=TRUE`
  的 CA 证书（仓库里旧的 `installer\YiukLabs.pfx` 就是 CA 证书，已不再使用）。
- **系统版本**：startupTask 需要 Windows 10 1809（17763）及以上，清单里的
  `MinVersion` 已设为 `10.0.17763.0`。
