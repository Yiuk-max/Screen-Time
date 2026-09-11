# AGENTS.md

This file provides guidance to Codex (Codex.ai/code) when working with code in this repository.

## Build Commands

This is a Qt 6 application using Qt Charts and SQLite. Build with CMake:

```bash
# Configure and build (Debug)
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Release build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The project uses:
- Qt 6 Core, GUI, Widgets, SQL, Charts, SVG, Network modules
- C++17 standard
- Windows-specific APIs (user32, psapi) for application tracking
- SQLite for data persistence
- CMake 3.16+ as the build system

## Project Architecture

**Screen Time** is a Windows application usage tracking tool built with Qt. The architecture follows a clean separation between tracking, data persistence, and UI.

### Core Components

1. **Tracker** (`core/tracker.h`, `core/tracker.cpp`)
   - Polls foreground window every 5 seconds using Windows APIs
   - Tracks app name, window title, and executable path
   - Accumulates time per application and flushes to database every 30 seconds or on app switch
   - Uses `GetForegroundWindow()`, `QueryFullProcessImageNameW()`, `GetWindowTextW()`

2. **Database** (`core/database.h`, `core/database.cpp`)
   - SQLite database stored in `%APPDATA%/ScreenTime/screen_time.db`
   - Schema: `usage_records` table with date, app_name, window_title, app_path, duration_seconds, tracked_at
   - Provides `queryToday()` and `queryWeekly()` for reporting
   - Auto-migrates old schema versions

3. **MainWindow** (`ui/mainwindow.h`, `ui/mainwindow.cpp`)
   - Tabbed interface with daily/weekly usage views and settings
   - System tray integration with configurable startup behavior
   - Uses `QSplitter` for resizable chart/stats sections
   - Auto-start via Windows registry (`HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run`)

4. **HourlyChartWidget** (`ui/hourlychartwidget.h`, `ui/hourlychartwidget.cpp`)
   - Custom QWidget implementing bar chart visualization
   - Supports both hourly (24 bars) and daily (7 bars) modes
   - Hover tooltips showing top 3 apps per time bucket

5. **Theme system** (`ui/theme/`)
   - `theme.h` — semantic `ColorTokens` (background/sidebar/surface/elevated/text/border/accent/state/chart),
     chart tokens, and `DesignTokens` (radii, border widths, spacing, font sizes, opacity, animation).
     `ThemePalette` is the small set of raw colors a theme author fills; `buildColorTokens()` derives the rest.
   - `thememanager.h/.cpp` — singleton `ThemeManager`: registers themes, holds current theme + accent override,
     persists to QSettings, applies the global QSS/QPalette to `QApplication`, and emits `themeChanged`.
     Built-in themes: `system`, `light`, `dark`, plus six traditional-color themes (`iris`, `amaranth`,
     `mushroom`, `scallion`, `alum`, `plum` — each a paper background + ink accent).
   - `themestyles.h/.cpp` — generates the global stylesheet and QPalette from tokens, plus themed SVG icons.
   - UI code must never hardcode colors: custom-painted widgets read tokens from `ThemeManager::instance().theme()`
     and repaint on `themeChanged`; QSS-styled widgets rely on the global stylesheet (object names / dynamic properties).

6. **i18n system** (`ui/i18n/`)
   - `translationmanager.h/.cpp` — singleton `TranslationManager` with built-in string tables keyed by semantic keys
     (e.g. `settings.theme`). Languages: `zh_CN` (base), `zh_TW`, `en`, `de`, `fr`, `nl`, `uk`, `hi`, `tg`.
     Persists the choice to QSettings (`ui/language`), emits `languageChanged`, and sets `QLocale` default.
   - UI strings are never hardcoded: use `i18n("key")`, or attach `i18nKey` / `i18nTooltipKey` via `i18nSetText()` /
     `i18nSetToolTip()`. `i18nRetranslate(root)` walks a widget tree and re-applies those properties on language change;
     `MainWindow::retranslateUi()` / `AIReportPage::retranslateUi()` also rebuild combos, nav labels and report blocks.
   - The language setting row label is intentionally hardcoded English (`Language`) in every locale.

### Data Flow

1. `Tracker` polls Windows every 5s → accumulates seconds per app
2. Every 30s or app switch → `flushToDatabase()` writes `UsageRecord` to SQLite
3. `MainWindow` queries `Database` on refresh timer or tab switch
4. `HourlyChartWidget` renders aggregated minute data with top apps per bucket

### Key Design Decisions

- `app.setQuitOnLastWindowClosed(false)` - app continues running in tray when window closed
- Tracker starts immediately in `main()`, stops on `aboutToQuit`
- Auto-start detection via `--autostart` command line argument combined with registry setting
- App name derived from executable path when available, falls back to window class name or title
- System tray classes (`Shell_TrayWnd`, `Progman`, `WorkerW`) filtered out from tracking
- **CMake-only build** — no qmake/.pro files; open `CMakeLists.txt` directly in Qt Creator / VS Code

### Windows-Specific Notes

- Requires `PROCESS_QUERY_LIMITED_INFORMATION` and `PROCESS_QUERY_INFORMATION | PROCESS_VM_READ` access rights
- Uses wide-character Windows APIs (`W` suffix functions)
- Database path uses `QStandardPaths::AppDataLocation` which resolves to `%APPDATA%/ScreenTime` on Windows
- Startup launch mode stored in QSettings under `startup/launch_mode` key ("tray" or "window")
- **Theme is decoupled from UI** — a new theme only fills a `ThemePalette`; no widget code changes are needed.
  All colors/visual parameters come from tokens, and theme switches propagate via `ThemeManager::themeChanged`.