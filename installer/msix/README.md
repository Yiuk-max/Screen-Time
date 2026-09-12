# MSIX 打包说明

## 前置条件

- Windows 10/11 SDK（提供 `makeappx.exe` 和 `signtool.exe`）
- Qt 6 MinGW 工具链
- 已通过 `deploy-scripts\deploy_release.bat` 生成 `release-package\ScreenTime`

## 生成商店包

```bat
deploy-scripts\pack_msix.bat
```

默认生成：

```text
release-package\msix\ScreenTime_<版本>.msix
release-package\msix\ScreenTime_<版本>.msixupload
```

商店上传包不需要本地签名，Partner Center 会完成最终签名。

## Microsoft Store 身份

将 Partner Center“产品标识”中的 Name 和 Publisher 写入
`installer\msix\store-identity.json`：

```json
{
  "identityName": "12345Publisher.ScreenTime",
  "publisher": "CN=00000000-0000-0000-0000-000000000000"
}
```

也可以通过命令行覆盖：

```bat
deploy-scripts\pack_msix.bat -IdentityName "12345Publisher.ScreenTime" -Publisher "CN=..."
```

## 本地旁加载

生成并安装开发证书，然后构建签名包：

```powershell
deploy-scripts\pack_msix.bat -Sign -DevSign -InstallCertificate
Add-AppxPackage -Path release-package\msix\ScreenTime_2.0.msix
```

也可以使用已有代码签名证书：

```bat
deploy-scripts\pack_msix.bat -Sign -PfxPath certificate.pfx -PfxPassword password
```

证书主题必须与清单中的 Publisher 一致。

## 开机自启动

普通安装版使用 `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`；MSIX 包使用清单中的 `windows.startupTask`。以下 TaskId 必须保持一致：

- `installer/msix/AppxManifest.xml`：`ScreenTimeStartupTask`
- `core/startupmanager.cpp`：`ScreenTimeStartupTask`

使用 `Add-AppxPackage -Register` 注册解包目录时，系统可能不会完整注册 startupTask；验证自启动应使用真实安装的签名 MSIX 或商店版本。
