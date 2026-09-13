# MSIX 打包与签名

## Microsoft Store 身份

本项目对应的商店身份必须精确使用以下值：

| 字段 | 值 |
|---|---|
| `Package/Identity/Name` | `Yiuk.TheScreenTime` |
| `Package/Identity/Publisher` | `CN=6C42CCA0-F9A8-4179-A164-0152ECF29CAD` |
| `Package/Properties/DisplayName` | `The Screen Time` |
| `Package/Properties/PublisherDisplayName` | `Yiuk` |
| Package Family Name | `Yiuk.TheScreenTime_0dwpj9x918enj` |
| Store ID | `9N99N8P4VR3H` |

- Microsoft Store 深层链接：`ms-windows-store://pdp/?productid=9N99N8P4VR3H`
- Web Store URL：<https://apps.microsoft.com/detail/9N99N8P4VR3H>

这些值保存在 `installer/msix/store-identity.json`。其中 `DisplayName` 区分大小写，必须与 Partner Center 中保留的名称 **The Screen Time** 完全一致。

## 生成商店上传包

前置条件：Windows 10/11 SDK、Qt 6 MinGW，以及已经生成的 `release-package\ScreenTime`。

```bat
deploy-scripts\pack_msix.bat
```

默认生成：

```text
release-package\msix\ScreenTime_<版本>.msix
release-package\msix\ScreenTime_<版本>.msixupload
```

提交 Microsoft Store 时无需自行签名；Partner Center 会使用微软证书完成签名。脚本会在打包前校验 Identity、Publisher、DisplayName 和 PublisherDisplayName，避免生成与商店保留身份不一致的软件包。

## 本地签名和旁加载

生成主题与 Store Publisher 相同的开发证书，签名并安装证书：

```powershell
deploy-scripts\pack_msix.bat -Sign -DevSign -InstallCertificate
Add-AppxPackage -Path release-package\msix\ScreenTime_2.0.3.msix
```

也可以使用已有代码签名证书：

```bat
deploy-scripts\pack_msix.bat -Sign -PfxPath certificate.pfx -PfxPassword password
```

已有证书的 Subject 必须精确等于：

```text
CN=6C42CCA0-F9A8-4179-A164-0152ECF29CAD
```

签名只决定本地软件包是否可信，不会修复商店保留名称错误；名称和身份仍由 `store-identity.json` 及 `AppxManifest.xml` 决定。

## 开机自启动

普通安装版使用 `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`；MSIX 使用清单中的 `windows.startupTask`，TaskId 为 `ScreenTimeStartupTask`。

清单不使用受限的 `rescap5:ImmediateRegistration`，因此不需要为该功能申请商店审批。MSIX 的 StartupTask 由 Windows 管理，用户可以在“设置 → 应用 → 启动”中控制它。

使用 `Add-AppxPackage -Register` 注册解包目录时，系统可能不会完整注册 startupTask；验证自启动应使用签名 MSIX 或商店版本。
