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
用户不需要先手动启动一次程序。

### 2. 程序内用 WinRT API 读取 / 开关

封装在 `core/startupmanager.h` / `core/startupmanager.cpp`：

- `isAutoStartEnabled()`  → `StartupTask.get_State()`
- `setAutoStartEnabled()` → `StartupTask.RequestEnableAsync()` / `Disable()`
- `launchedByAutoStart()` → `AppInstance.GetActivatedEventArgs().Kind == ActivationKind_StartupTask`
  （startupTask 启动时没有命令行参数，必须靠它判断，才能决定是否启动到托盘）

`TaskId` 必须与 `StartupManager::startupTaskId()` 里的字符串完全一致。

未打包（普通 exe / Inno Setup）时，代码自动回退到原来的注册表方案，两套逻辑共用一个开关。

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

1. 安装后打开 **任务管理器 → 启动应用**，应能看到 **Screen Time**，状态“已启用”。
2. 在程序“设置”里切换“开机自启动”，任务管理器里的状态会同步变化。
3. 重启电脑，程序按“开机启动方式”在托盘或主界面启动。

## 七、常见问题

- **启动应用里没有条目**：确认清单含 `windows.startupTask` 且
  `rescap5:ImmediateRegistration="true"`；用真实安装（Store 或签名 msix）而非 `-Register`。
- **开关打不开 / 关了又打不开**：若用户曾在任务管理器里手动禁用，状态会变成
  `DisabledByUser`，只能由用户在任务管理器重新启用，程序无权修改（会弹窗提示）。
- **`0x800B0109` / 清单读取失败 `0x87E80034`**：签名证书有问题。
  用于 MSIX 的证书必须是终端实体代码签名证书，不能是 `BasicConstraints CA=TRUE`
  的 CA 证书（仓库里旧的 `installer\YiukLabs.pfx` 就是 CA 证书，已不再使用）。
- **系统版本**：startupTask 需要 Windows 10 1809（17763）及以上，清单里的
  `MinVersion` 已设为 `10.0.17763.0`。
