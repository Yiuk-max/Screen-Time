# Screen Time 功能增强计划

**创建日期**: 2026-05-30
**状态**: 待实施

---

## 📋 功能需求总览

### 1. 自动更新系统
- 联网检查 GitHub Releases
- 设置面板添加更新功能
- 自动从 GitHub 下载并解压
- 后台检查 + 弹窗通知用户

### 2. AI 周报功能
- 每周自动分析计算
- 用户填写自己的 API Key
- 带提示词将数据交给 AI 分析生成周报
- 左侧新增按钮入口

### 3. 安装程序和卸载程序
- 从"解压即用"改为正常软件安装/卸载方式

---

## 一、自动更新系统

### 1.1 技术方案
- **网络请求**: 使用 `QNetworkAccessManager` 访问 GitHub Releases API
- **版本检查**: 对比当前版本与 GitHub 最新 release tag
- **下载机制**: 下载 zip 包到临时目录
- **解压替换**: 使用 `QProcess` 调用系统 `tar`（Windows 10+ 自带）或集成 QuaZip 库
- **更新通知**: 使用 `QSystemTrayIcon::showMessage()` 或自定义弹窗

### 1.2 UI 变更
- **设置面板新增**:
  - "检查更新" 按钮
  - 当前版本号显示
  - 自动检查更新开关（后台定时检查）
  - 更新日志显示区域

### 1.3 后台管理
- 应用启动时检查更新（可配置）
- 每 24 小时后台检查一次
- 发现新版本 → 托盘气泡通知 → 点击进入设置页

### 1.4 新增文件
- `core/updater.h` / `core/updater.cpp` - 更新逻辑封装
- `ui/updatewidget.h` / `ui/updatewidget.cpp` - 更新 UI 组件（可选）

---

## 二、AI 周报功能

### 2.1 数据流
```
Database::queryWeekly()
  → 格式化为结构化文本/JSON
  → 拼接用户提示词
  → POST 到 AI API
  → 解析返回结果
  → 显示在周报页面
```

### 2.2 UI 变更
- **左侧边栏新增按钮**: "AI 周报"（图标 + 文字）
  - 位置: 在设置按钮上方
  - 样式与现有按钮一致
  - **收缩时适配**: 左侧菜单收缩时，按钮需像设置按钮一样做相应的大小变化适配（固定大小 44x44，清除文字，居中对齐）
- **新增页面**（`m_contentStack` 索引 2）:
  - 周报展示区：以块状展示历史周报，每块标题为"日期到日期的xx周报"
  - 周报类型：分为"七天周报"和"每月周报"两种
  - 内容格式：相比xx 多了/少了 xx%，使用最多的几个是xx...
  - 手动生成按钮：生成当天报告、生成本周报告
  - 导出按钮（保存为 .md / .txt）
  - 时间到了自动生成周报

### 2.3 设置面板新增
- AI 配置区域（独立栏目，与更新功能分开）:
  - 启用 AI 周报功能开关（默认关闭）
  - API Key 输入（仅 DeepSeek，密码模式，保存到 QSettings）
  - 模型固定为 DeepSeek（用户无需选择，输完 API Key 即完成配置）
  - 让用户选择是否打开 AI 周报功能
  - 不需要其他复杂的模型配置选项

### 2.4 自动化
- 每周日自动生成并通知（需用户在设置中开启）
- 或手动触发（生成当天/本周报告按钮）

### 2.5 新增文件
- `ui/aireportpage.h` / `ui/aireportpage.cpp` - AI 周报页面
- `core/aireporter.h` / `core/aireporter.cpp` - AI API 调用封装

---

## 三、安装程序（Inno Setup）

### 3.1 技术选型
- **Inno Setup**: 免费、轻量、Windows 原生、社区成熟
- 替代方案: Qt Installer Framework（更重，暂不推荐）

### 3.2 安装包功能
- 安装路径选择（默认 `C:\Program Files\ScreenTime`）
- 创建开始菜单快捷方式
- 创建桌面快捷方式（可选）
- 写入注册表实现开机自启动（与现有逻辑兼容）
- 卸载时:
  - 删除程序文件
  - 清理注册表自启动项
  - **可选**: 询问是否保留用户数据（SQLite 数据库）

### 3.3 构建流程
- 新增 `installer/ScreenTime.iss` 脚本
- CI/CD（可选）: GitHub Actions 自动构建 exe + 打包安装程序

### 3.4 新增文件
- `installer/ScreenTime.iss`
- `installer/assets/`（可选: 安装向导图标等）

---

## 四、依赖与配置变更

| 功能 | 新增依赖 | 配置存储 |
|------|---------|---------|
| 自动更新 | `Qt6::Network` | QSettings |
| AI 周报 | `Qt6::Network` | QSettings（API Key） |
| 安装程序 | Inno Setup（外部工具） | - |

**`CMakeLists.txt` 已包含 `Qt6::Network`**，无需修改。

---

> **⚠️ 重要：本项目使用 CMake 构建系统，请勿手动编译。** 在 Qt Creator 或 VS Code 中打开项目根目录的 `CMakeLists.txt`，使用 CMake Preset 或手动配置 CMake 即可自动完成构建。所有依赖（Qt6::Network 等）已在 CMake 中声明，无需额外手动处理。

## 五、实施顺序建议

1. **Phase 1**: 自动更新系统（网络 + 设置面板）
2. **Phase 2**: AI 周报功能（新页面 + AI 调用）
3. **Phase 3**: 安装程序脚本 + 构建流程
4. **Phase 4**: 测试 + 文档更新

---

## 六、风险与注意事项

- **更新替换**: 正在运行的 exe 无法被覆盖，需实现"重启替换"逻辑（常见做法: 启动 updater.exe 进行替换）
- **API Key 安全**: 明文存储在 QSettings，Windows 用户可读，必要时可考虑简单加密
- **AI 费用**: 提示用户 API 调用会产生费用
- **GitHub API 限制**: 未认证请求 60 次/小时，建议添加可选的 GitHub Token 配置

---

## 实施状态

- [ ] Phase 1: 自动更新系统
- [ ] Phase 2: AI 周报功能
- [ ] Phase 3: 安装程序
- [ ] Phase 4: 测试与文档

---

**最后更新**: 2026-05-30
**计划制定者**: Claude Code
