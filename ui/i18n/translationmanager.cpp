#include "translationmanager.h"

#include <QAbstractButton>
#include <QLabel>
#include <QLocale>
#include <QSettings>
#include <QWidget>

namespace {

// 每条记录：key, zh_CN(基准), en, zh_TW, tg, hi, uk, de, fr, nl
struct TranslationEntry {
    const char *key;
    const char *zh;
    const char *en;
    const char *zhTW;
    const char *tg;
    const char *hi;
    const char *uk;
    const char *de;
    const char *fr;
    const char *nl;
};

const TranslationEntry kEntries[] = {
    {"nav.home", "主页", "Home", "主頁", "Асосӣ", "होम", "Головна", "Start", "Accueil", "Start"},
    {"nav.report", "分析报告", "Reports", "分析報告", "Ҳисоботҳо", "रिपोर्ट", "Звіти", "Berichte", "Rapports", "Rapporten"},
    {"nav.settings", "设置和帮助", "Settings", "設定與說明", "Танзимот", "सेटिंग", "Налаштування", "Einstellungen", "Paramètres", "Instellingen"},

    {"tooltip.home", "返回主页", "Back to Home", "返回主頁", "Бозгашт ба асосӣ", "होम पर वापस जाएँ", "Повернутися на головну", "Zurück zur Startseite", "Retour à l'accueil", "Terug naar start"},
    {"tooltip.report", "查看使用分析报告", "View usage reports", "檢視使用分析報告", "Дидани ҳисоботҳои истифода", "उपयोग रिपोर्ट देखें", "Переглянути звіти про використання", "Nutzungsberichte anzeigen", "Voir les rapports d'utilisation", "Gebruiksrapporten bekijken"},
    {"tooltip.settings", "设置和帮助", "Settings & Help", "設定與說明", "Танзимот ва кӯмак", "सेटिंग और सहायता", "Налаштування та довідка", "Einstellungen & Hilfe", "Paramètres et aide", "Instellingen en help"},
    {"tooltip.expand", "展开菜单", "Expand menu", "展開選單", "Кушодани меню", "मेनू फैलाएँ", "Розгорнути меню", "Menü erweitern", "Développer le menu", "Menu uitklappen"},
    {"tooltip.splitter", "拖动调整柱状图和应用统计区域大小", "Drag to resize chart and app stats", "拖曳調整柱狀圖與應用統計區域大小", "Барои тағйири андозаи диаграмма ва омор кашед", "चार्ट और ऐप आँकड़ों का आकार बदलने के लिए खींचें", "Перетягніть, щоб змінити розмір діаграми та статистики", "Ziehen, um Diagramm und App-Statistik zu skalieren", "Glisser pour redimensionner le graphique et les statistiques", "Slepen om grafiek en app-statistieken te schalen"},
    {"tooltip.open_link", "打开项目链接", "Open project link", "開啟專案連結", "Кушодани пайванди лоиҳа", "प्रोजेक्ट लिंक खोलें", "Відкрити посилання на проєкт", "Projektlink öffnen", "Ouvrir le lien du projet", "Projectlink openen"},

    {"period.daily", "每日", "Daily", "每日", "Ҳаррӯза", "प्रतिदिन", "Щодня", "Täglich", "Quotidien", "Dagelijks"},
    {"period.weekly", "近7天", "Last 7 days", "近 7 天", "7 рӯзи охир", "पिछले 7 दिन", "Останні 7 днів", "Letzte 7 Tage", "7 derniers jours", "Laatste 7 dagen"},
    {"stats.title", "应用统计", "App usage", "應用統計", "Истифодаи барномаҳо", "ऐप उपयोग", "Використання програм", "App-Nutzung", "Utilisation des applications", "App-gebruik"},

    {"stat.today_total", "今日总时长 %1", "Total today %1", "今日總時長 %1", "Ҳамагӣ имрӯз %1", "आज कुल %1", "Усього сьогодні %1", "Gesamt heute %1", "Total aujourd'hui %1", "Totaal vandaag %1"},
    {"stat.week_total", "近7天总时长 %1        日均时长 %2", "Last 7 days %1        Daily avg %2", "近 7 天總時長 %1        日均時長 %2", "7 рӯзи охир %1        Миёна дар рӯз %2", "पिछले 7 दिन %1        प्रतिदिन औसत %2", "Останні 7 днів %1        Сер. за день %2", "Letzte 7 Tage %1        Ø pro Tag %2", "7 derniers jours %1        Moyenne/jour %2", "Laatste 7 dagen %1        Gem. per dag %2"},
    {"duration.hm", "%1小时%2分钟", "%1 h %2 min", "%1 小時 %2 分鐘", "%1 соат %2 дақ", "%1 घं %2 मि", "%1 год %2 хв", "%1 Std. %2 Min.", "%1 h %2 min", "%1 u %2 min"},
    {"duration.m", "%1分钟", "%1 min", "%1 分鐘", "%1 дақ", "%1 मि", "%1 хв", "%1 Min.", "%1 min", "%1 min"},
    {"hour.label", "%1时", "%1h", "%1時", "%1 соат", "%1 बजे", "%1 год", "%1 Uhr", "%1 h", "%1 u"},
    {"chart.minutes", "%1 分钟", "%1 min", "%1 分鐘", "%1 дақ", "%1 मि", "%1 хв", "%1 Min.", "%1 min", "%1 min"},
    {"chart.total", "总时长 %1", "Total %1", "總時長 %1", "Ҳамагӣ %1", "कुल %1", "Усього %1", "Gesamt %1", "Total %1", "Totaal %1"},
    {"chart.hour_range", "  (%1时-%2时)", "  (%1:00-%2:00)", "  (%1時-%2時)", "  (%1-%2 соат)", "  (%1-%2 बजे)", "  (%1-%2 год)", "  (%1-%2 Uhr)", "  (%1h-%2h)", "  (%1-%2 u)"},

    {"settings.title", "设置", "Settings", "設定", "Танзимот", "सेटिंग", "Налаштування", "Einstellungen", "Paramètres", "Instellingen"},
    {"settings.theme", "界面主题", "Theme", "介面主題", "Мавзӯъ", "थीम", "Тема", "Design", "Thème", "Thema"},
    {"settings.accent", "强调色", "Accent color", "強調色", "Ранги асосӣ", "एक्सेंट रंग", "Акцентний колір", "Akzentfarbe", "Couleur d'accent", "Accentkleur"},
    {"settings.notifications", "允许通知", "Notifications", "允許通知", "Огоҳиномаҳо", "सूचनाएँ", "Сповіщення", "Benachrichtigungen", "Notifications", "Meldingen"},
    {"settings.autostart", "开机自启动", "Launch at startup", "開機自動啟動", "Ҳангоми оғоз иҷро шавад", "स्टार्टअप पर चलाएँ", "Запускати при старті", "Beim Start ausführen", "Lancer au démarrage", "Starten bij opstarten"},
    {"settings.autostart_mode", "开机启动方式", "Startup mode", "開機啟動方式", "Ҳолати оғоз", "स्टार्टअप मोड", "Режим запуску", "Startmodus", "Mode de démarrage", "Opstartmodus"},
    {"settings.repo", "项目地址", "Project", "專案位址", "Лоиҳа", "प्रोजेक्ट", "Проєкт", "Projekt", "Projet", "Project"},
    {"settings.ai_section", "AI 分析报告", "AI Reports", "AI 分析報告", "Ҳисоботҳои AI", "AI रिपोर्ट", "ШІ-звіти", "KI-Berichte", "Rapports IA", "AI-rapporten"},
    {"settings.ai_enable", "启用分析报告", "Enable AI reports", "啟用分析報告", "Фаъол кардани ҳисоботҳои AI", "AI रिपोर्ट सक्षम करें", "Увімкнути ШІ-звіти", "KI-Berichte aktivieren", "Activer les rapports IA", "AI-rapporten inschakelen"},
    {"settings.api_key", "DeepSeek API Key", "DeepSeek API Key", "DeepSeek API Key", "Калиди API-и DeepSeek", "DeepSeek API कुंजी", "Ключ API DeepSeek", "DeepSeek API Key", "Clé API DeepSeek", "DeepSeek API-sleutel"},
    {"settings.help", "帮助", "Help", "說明", "Кӯмак", "सहायता", "Довідка", "Hilfe", "Aide", "Help"},
    {"settings.version", "当前版本", "Version", "目前版本", "Версия", "संस्करण", "Версія", "Version", "Version", "Versie"},
    {"settings.about", "关于", "About", "關於", "Дар бораи", "परिचय", "Про програму", "Über", "À propos", "Over"},
    {"settings.privacy", "隐私政策", "Privacy policy", "隱私政策", "Махфият", "गोपनीयता नीति", "Конфіденційність", "Datenschutz", "Confidentialité", "Privacybeleid"},

    {"startup.tray", "系统托盘启动", "Start in system tray", "系統匣啟動", "Оғоз дар трейи система", "सिस्टम ट्रे में प्रारंभ करें", "Запуск у системному лотку", "Im Infobereich starten", "Démarrer dans la zone de notification", "Starten in systeemvak"},
    {"startup.window", "弹出主界面", "Show main window", "顯示主視窗", "Нишон додани равзанаи асосӣ", "मुख्य विंडो दिखाएँ", "Показати головне вікно", "Hauptfenster anzeigen", "Afficher la fenêtre principale", "Hoofdvenster tonen"},

    {"help.open_privacy", "打开隐私政策", "Open privacy policy", "開啟隱私政策", "Кушодани сиёсати махфият", "गोपनीयता नीति खोलें", "Відкрити політику конфіденційності", "Datenschutzerklärung öffnen", "Ouvrir la politique de confidentialité", "Privacybeleid openen"},

    {"tray.show", "显示主窗口", "Show main window", "顯示主視窗", "Нишон додани равзанаи асосӣ", "मुख्य विंडो दिखाएँ", "Показати головне вікно", "Hauptfenster anzeigen", "Afficher la fenêtre principale", "Hoofdvenster tonen"},
    {"tray.quit", "退出", "Quit", "結束", "Баромад", "बंद करें", "Вийти", "Beenden", "Quitter", "Afsluiten"},
    {"close.minimized", "程序已最小化到托盘，仍在后台记录使用时间", "Minimized to tray; still tracking in the background", "程式已最小化至系統匣，仍在背景記錄使用時間", "Ба трей хурд карда шуд; дар замина сабт идома дорад", "ट्रे में छोटा किया गया; पृष्ठभूमि में रिकॉर्डिंग जारी है", "Згорнуто в лоток; запис триває у фоновому режимі", "In den Infobereich minimiert; zeichnet weiter im Hintergrund auf", "Réduit dans la zone de notification ; l'enregistrement continue en arrière-plan", "Geminimaliseerd naar systeemvak; wordt op de achtergrond bijgehouden"},
    {"autostart.failed_title", "开机自启动", "Launch at startup", "開機自動啟動", "Ҳангоми оғоз иҷро шавад", "स्टार्टअप पर चलाएँ", "Запускати при старті", "Beim Start ausführen", "Lancer au démarrage", "Starten bij opstarten"},
    {"autostart.failed_body", "无法开启开机自启动。\n\n可能是被“任务管理器 → 启动应用”或系统策略禁用了，请在那里手动开启。", "Could not enable launch at startup.\n\nIt may be disabled in Task Manager → Startup apps or by system policy. Please enable it there.", "無法開啟開機自動啟動。\n\n可能被「工作管理員 → 啟動應用程式」或系統原則停用，請在該處手動開啟。", "Фаъол кардани иҷро ҳангоми оғоз муяссар нашуд.\n\nЭҳтимол он дар Менеҷери вазифаҳо → Барномаҳои оғоз ё аз ҷониби сиёсати система ғайрифаъол шудааст. Лутфан он ҷо фаъол кунед.", "स्टार्टअप पर चलाना सक्षम नहीं किया जा सका।\n\nयह टास्क मैनेजर → स्टार्टअप ऐप्स या सिस्टम नीति से अक्षम हो सकता है। कृपया वहाँ सक्षम करें।", "Не вдалося увімкнути запуск при старті.\n\nМожливо, його вимкнено в Диспетчері завдань → Програми автозапуску або системною політикою. Увімкніть його там.", "Autostart konnte nicht aktiviert werden.\n\nEr ist möglicherweise im Task-Manager → Autostart oder durch eine Richtlinie deaktiviert. Bitte dort aktivieren.", "Impossible d'activer le lancement au démarrage.\n\nIl est peut-être désactivé dans le Gestionnaire des tâches → Applications de démarrage ou par une stratégie système. Veuillez l'activer là-bas.", "Kan starten bij opstarten niet inschakelen.\n\nHet is mogelijk uitgeschakeld in Taakbeheer → Opstartapps of door systeembeleid. Schakel het daar in."},

    {"report.title", "使用分析报告", "Usage report", "使用分析報告", "Ҳисоботи истифода", "उपयोग रिपोर्ट", "Звіт про використання", "Nutzungsbericht", "Rapport d'utilisation", "Gebruiksrapport"},
    {"report.subtitle", "基于屏幕使用数据生成分析报告", "Generate reports from your screen time data", "依螢幕使用資料產生分析報告", "Аз маълумоти вақти экран ҳисобот созед", "स्क्रीन टाइम डेटा से रिपोर्ट बनाएँ", "Створюйте звіти на основі даних екранного часу", "Berichte aus Ihren Bildschirmzeitdaten erstellen", "Générer des rapports à partir de vos données de temps d'écran", "Rapporten genereren op basis van uw schermtijdgegevens"},
    {"report.generate_today", "生成今日分析", "Generate today", "產生今日分析", "Сохтани имрӯз", "आज की बनाएँ", "Створити за сьогодні", "Heute erstellen", "Générer aujourd'hui", "Vandaag genereren"},
    {"report.generate_week", "生成本周分析", "Generate this week", "產生本週分析", "Сохтани ин ҳафта", "इस सप्ताह की बनाएँ", "Створити за тиждень", "Diese Woche erstellen", "Générer cette semaine", "Deze week genereren"},
    {"report.export", "导出全部", "Export all", "匯出全部", "Ҳамаро содир кунед", "सभी निर्यात करें", "Експортувати все", "Alle exportieren", "Tout exporter", "Alles exporteren"},
    {"report.disabled_hint", "请在设置中启用「分析报告」功能", "Enable \"AI Reports\" in Settings first", "請先在設定中啟用「分析報告」功能", "Аввал дар Танзимот «Ҳисоботҳои AI»-ро фаъол кунед", "पहले सेटिंग में \"AI रिपोर्ट\" सक्षम करें", "Спершу увімкніть «ШІ-звіти» в налаштуваннях", "Bitte zuerst „KI-Berichte“ in den Einstellungen aktivieren", "Activez d'abord « Rapports IA » dans les paramètres", "Schakel eerst 'AI-rapporten' in bij Instellingen"},
    {"report.generating_today", "正在生成今日分析...", "Generating today's report...", "正在產生今日分析...", "Ҳисоботи имрӯз сохта мешавад...", "आज की रिपोर्ट बन रही है...", "Створення звіту за сьогодні...", "Bericht für heute wird erstellt...", "Génération du rapport du jour...", "Rapport van vandaag wordt gegenereerd..."},
    {"report.generating_week", "正在生成本周分析...", "Generating this week's report...", "正在產生本週分析...", "Ҳисоботи ин ҳафта сохта мешавад...", "इस सप्ताह की रिपोर्ट बन रही है...", "Створення звіту за тиждень...", "Bericht für diese Woche wird erstellt...", "Génération du rapport de la semaine...", "Rapport van deze week wordt gegenereerd..."},
    {"report.done", "生成完成", "Done", "產生完成", "Тайёр", "पूर्ण", "Готово", "Fertig", "Terminé", "Klaar"},
    {"report.failed", "生成失败: %1", "Failed: %1", "產生失敗：%1", "Хатогӣ: %1", "विफल: %1", "Помилка: %1", "Fehlgeschlagen: %1", "Échec : %1", "Mislukt: %1"},
    {"report.receiving", "接收中 %1%", "Receiving %1%", "接收中 %1%", "Қабул %1%", "प्राप्त हो रहा है %1%", "Отримання %1%", "Empfange %1%", "Réception %1%", "Ontvangen %1%"},
    {"report.need_enable", "请先在设置中启用「分析报告」功能", "Please enable \"AI Reports\" in Settings first", "請先在設定中啟用「分析報告」功能", "Аввал дар Танзимот «Ҳисоботҳои AI»-ро фаъол кунед", "पहले सेटिंग में \"AI रिपोर्ट\" सक्षम करें", "Спершу увімкніть «ШІ-звіти» в налаштуваннях", "Bitte zuerst „KI-Berichte“ in den Einstellungen aktivieren", "Activez d'abord « Rapports IA » dans les paramètres", "Schakel eerst 'AI-rapporten' in bij Instellingen"},
    {"report.need_api_key", "请先在设置中填写 DeepSeek API Key", "Please enter your DeepSeek API Key in Settings first", "請先在設定中填寫 DeepSeek API Key", "Аввал калиди API-и DeepSeek-ро дар Танзимот ворид кунед", "पहले सेटिंग में अपनी DeepSeek API कुंजी दर्ज करें", "Спершу введіть ключ API DeepSeek у налаштуваннях", "Bitte zuerst den DeepSeek API Key in den Einstellungen eingeben", "Saisissez d'abord votre clé API DeepSeek dans les paramètres", "Voer eerst uw DeepSeek API-sleutel in bij Instellingen"},
    {"report.consent_title", "AI 分析数据传输提示", "AI data transfer notice", "AI 分析資料傳輸提示", "Огоҳии интиқоли маълумот ба AI", "AI डेटा हस्तांतरण सूचना", "Повідомлення про передачу даних ШІ", "Hinweis zur KI-Datenübertragung", "Avis de transfert de données IA", "Kennisgeving gegevensoverdracht AI"},
    {"report.consent_body", "启用 AI 分析功能后，应用会将统计数据发送给第三方 AI 服务（如 OpenAI、DeepSeek）进行处理。继续即表示您同意相关数据传输。", "With AI reports enabled, usage statistics are sent to a third-party AI service (e.g. OpenAI, DeepSeek) for processing. Continuing means you agree to this data transfer.", "啟用 AI 分析功能後，應用程式會將統計資料傳送給第三方 AI 服務（如 OpenAI、DeepSeek）處理。繼續即表示您同意相關資料傳輸。", "Ҳангоми фаъол будани ҳисоботҳои AI, омори истифода ба хидмати AI-и сеюм (масалан OpenAI, DeepSeek) барои коркард фиристода мешавад. Идома додан маънои розӣ будан бо ин интиқолро дорад.", "AI रिपोर्ट सक्षम होने पर उपयोग आँकड़े प्रसंस्करण के लिए किसी तृतीय-पक्ष AI सेवा (जैसे OpenAI, DeepSeek) को भेजे जाते हैं। जारी रखने का अर्थ है कि आप इस हस्तांतरण से सहमत हैं।", "Коли ШІ-звіти увімкнено, статистика використання надсилається сторонній службі ШІ (наприклад, OpenAI, DeepSeek) для обробки. Продовжуючи, ви погоджуєтеся на це.", "Wenn KI-Berichte aktiviert sind, werden Nutzungsdaten zur Verarbeitung an einen externen KI-Dienst (z. B. OpenAI, DeepSeek) gesendet. Mit dem Fortfahren stimmen Sie dieser Datenübertragung zu.", "Lorsque les rapports IA sont activés, les statistiques d'utilisation sont envoyées à un service d'IA tiers (par ex. OpenAI, DeepSeek) pour traitement. Continuer signifie que vous acceptez ce transfert.", "Als AI-rapporten zijn ingeschakeld, worden gebruiksstatistieken naar een externe AI-dienst (bijv. OpenAI, DeepSeek) gestuurd voor verwerking. Doorgaan betekent dat u hiermee instemt."},
    {"report.agree", "同意并继续", "Agree and continue", "同意並繼續", "Розӣ шуда идома диҳед", "सहमत हों और जारी रखें", "Погодитися й продовжити", "Zustimmen und fortfahren", "Accepter et continuer", "Akkoord en doorgaan"},
    {"report.cancel", "取消", "Cancel", "取消", "Бекор кардан", "रद्द करें", "Скасувати", "Abbrechen", "Annuler", "Annuleren"},
    {"report.cancelled", "已取消 AI 分析", "AI report cancelled", "已取消 AI 分析", "Ҳисоботи AI бекор карда шуд", "AI रिपोर्ट रद्द की गई", "ШІ-звіт скасовано", "KI-Bericht abgebrochen", "Rapport IA annulé", "AI-rapport geannuleerd"},
    {"report.export_dialog", "导出分析报告", "Export reports", "匯出分析報告", "Содироти ҳисоботҳо", "रिपोर्ट निर्यात करें", "Експорт звітів", "Berichte exportieren", "Exporter les rapports", "Rapporten exporteren"},
    {"report.export_default_name", "使用分析报告_%1.md", "usage_report_%1.md", "使用分析報告_%1.md", "ҳисоботи_истифода_%1.md", "उपयोग_रिपोर्ट_%1.md", "звіт_використання_%1.md", "nutzungsbericht_%1.md", "rapport_utilisation_%1.md", "gebruiksrapport_%1.md"},
    {"report.export_filter", "Markdown 文件 (*.md)", "Markdown files (*.md)", "Markdown 檔案 (*.md)", "Файлҳои Markdown (*.md)", "Markdown फ़ाइलें (*.md)", "Файли Markdown (*.md)", "Markdown-Dateien (*.md)", "Fichiers Markdown (*.md)", "Markdown-bestanden (*.md)"},
    {"report.error", "错误", "Error", "錯誤", "Хатогӣ", "त्रुटि", "Помилка", "Fehler", "Erreur", "Fout"},
    {"report.cannot_create", "无法创建文件", "Could not create the file", "無法建立檔案", "Файл сохта нашуд", "फ़ाइल नहीं बनाई जा सकी", "Не вдалося створити файл", "Datei konnte nicht erstellt werden", "Impossible de créer le fichier", "Kan het bestand niet aanmaken"},
    {"report.success", "成功", "Success", "成功", "Муваффақ", "सफल", "Успіх", "Erfolg", "Succès", "Gelukt"},
    {"report.exported", "分析报告已导出", "Reports exported", "分析報告已匯出", "Ҳисоботҳо содир карда шуданд", "रिपोर्ट निर्यात की गईं", "Звіти експортовано", "Berichte exportiert", "Rapports exportés", "Rapporten geëxporteerd"},
    {"report.delete_title", "删除报告", "Delete report", "刪除報告", "Нест кардани ҳисобот", "रिपोर्ट हटाएँ", "Видалити звіт", "Bericht löschen", "Supprimer le rapport", "Rapport verwijderen"},
    {"report.delete_body", "确定删除这份分析报告吗？此操作不可恢复。", "Delete this report? This cannot be undone.", "確定要刪除這份分析報告嗎？此操作無法復原。", "Ин ҳисобот нест карда шавад? Ин амал барқарорнашаванда аст.", "यह रिपोर्ट हटाएँ? इसे पूर्ववत नहीं किया जा सकता।", "Видалити цей звіт? Цю дію не можна скасувати.", "Diesen Bericht löschen? Dies kann nicht rückgängig gemacht werden.", "Supprimer ce rapport ? Cette action est irréversible.", "Dit rapport verwijderen? Dit kan niet ongedaan worden gemaakt."},
    {"report.auto_generated", "自动生成", "Auto-generated", "自動產生", "Худкор сохта шудааст", "स्वतः निर्मित", "Створено автоматично", "Automatisch erstellt", "Généré automatiquement", "Automatisch gegenereerd"},
    {"report.delete_tooltip", "删除此报告", "Delete this report", "刪除此報告", "Нест кардани ин ҳисобот", "यह रिपोर्ट हटाएँ", "Видалити цей звіт", "Diesen Bericht löschen", "Supprimer ce rapport", "Dit rapport verwijderen"},
    {"report.tip", "提示", "Notice", "提示", "Огоҳӣ", "सूचना", "Підказка", "Hinweis", "Info", "Info"},
    {"report.entry_today", "%1 · 今日使用分析", "%1 · Today's usage", "%1 · 今日使用分析", "%1 · Истифодаи имрӯз", "%1 · आज का उपयोग", "%1 · Використання сьогодні", "%1 · Heutige Nutzung", "%1 · Utilisation du jour", "%1 · Gebruik vandaag"},
    {"report.entry_week_range", "%1 · 七天使用分析（%2 至 %3）", "%1 · 7-day usage (%2 to %3)", "%1 · 七天使用分析（%2 至 %3）", "%1 · Истифодаи 7 рӯз (%2 то %3)", "%1 · 7-दिन का उपयोग (%2 से %3)", "%1 · Використання за 7 днів (%2 – %3)", "%1 · 7-Tage-Nutzung (%2 bis %3)", "%1 · Utilisation sur 7 jours (%2 à %3)", "%1 · Gebruik over 7 dagen (%2 tot %3)"},
    {"report.entry_week", "%1 · 七天使用分析", "%1 · 7-day usage", "%1 · 七天使用分析", "%1 · Истифодаи 7 рӯз", "%1 · 7-दिन का उपयोग", "%1 · Використання за 7 днів", "%1 · 7-Tage-Nutzung", "%1 · Utilisation sur 7 jours", "%1 · Gebruik over 7 dagen"},

    {"error.api_key_missing", "API Key 未设置", "API Key is not set", "API Key 未設定", "Калиди API гузошта нашудааст", "API कुंजी सेट नहीं है", "Ключ API не встановлено", "API Key ist nicht festgelegt", "Clé API non définie", "API-sleutel is niet ingesteld"},
    {"error.no_records", "没有可用的使用记录数据", "No usage records available", "沒有可用的使用記錄資料", "Маълумоти истифода дастрас нест", "कोई उपयोग रिकॉर्ड उपलब्ध नहीं", "Немає даних про використання", "Keine Nutzungsdaten verfügbar", "Aucune donnée d'utilisation disponible", "Geen gebruiksgegevens beschikbaar"},
    {"error.parse_failed", "解析响应失败: %1", "Failed to parse response: %1", "解析回應失敗：%1", "Таҳлили посух ноком шуд: %1", "प्रतिक्रिया पार्स करने में विफल: %1", "Не вдалося обробити відповідь: %1", "Antwort konnte nicht verarbeitet werden: %1", "Échec de l'analyse de la réponse : %1", "Kan antwoord niet verwerken: %1"},
    {"error.request_failed", "API 请求失败", "API request failed", "API 請求失敗", "Дархости API ноком шуд", "API अनुरोध विफल", "Помилка запиту API", "API-Anfrage fehlgeschlagen", "Échec de la requête API", "API-verzoek mislukt"},
    {"error.empty_result", "AI 返回结果为空", "AI returned an empty result", "AI 傳回結果為空", "AI натиҷаи холӣ баргардонд", "AI ने खाली परिणाम लौटाया", "AI повернув порожній результат", "KI hat ein leeres Ergebnis zurückgegeben", "L'IA a renvoyé un résultat vide", "AI gaf een leeg resultaat terug"},
    {"error.empty_content", "AI 返回内容为空", "AI returned empty content", "AI 傳回內容為空", "AI мундариҷаи холӣ баргардонд", "AI ने खाली सामग्री लौटाई", "AI повернув порожній вміст", "KI hat leeren Inhalt zurückgegeben", "L'IA a renvoyé un contenu vide", "AI gaf lege inhoud terug"},

    {"about.title", "关于 Screen Time", "About Screen Time", "關於 Screen Time", "Дар бораи Screen Time", "Screen Time के बारे में", "Про Screen Time", "Über Screen Time", "À propos de Screen Time", "Over Screen Time"},
    {"about.body", "Screen Time\n版本：%1", "Screen Time\nVersion: %1", "Screen Time\n版本：%1", "Screen Time\nВерсия: %1", "Screen Time\nसंस्करण: %1", "Screen Time\nВерсія: %1", "Screen Time\nVersion: %1", "Screen Time\nVersion : %1", "Screen Time\nVersie: %1"},

    {"theme.system", "跟随系统", "Follow system", "跟隨系統", "Тобеи система", "सिस्टम के अनुसार", "За системою", "System folgen", "Suivre le système", "Systeem volgen"},
    {"theme.light", "浅色", "Light", "淺色", "Равшан", "हल्का", "Світла", "Hell", "Clair", "Licht"},
    {"theme.dark", "深色", "Dark", "深色", "Торик", "गहरा", "Темна", "Dunkel", "Sombre", "Donker"},

    {"theme.iris", "鸢尾蓝", "Iris Blue", "鳶尾藍", "Ириси кабуд", "आइरिस नीला", "Ірисовий синій", "Irisblau", "Bleu iris", "Irisblauw"},
    {"theme.iris.desc", "宣纸白 · 鸢尾蓝", "Xuan Paper · Iris Blue", "宣紙白 · 鳶尾藍", "Коғази Сюан · Ириси кабуд", "शुआन कागज़ · आइरिस नीला", "Папір сюань · Ірисовий синій", "Xuan-Papier · Irisblau", "Papier Xuan · Bleu iris", "Xuan-papier · Irisblauw"},
    {"theme.amaranth", "苋菜红", "Amaranth Red", "莧菜紅", "Амаранти сурх", "एमरैंथ लाल", "Амарантовий червоний", "Amarantrot", "Rouge amarante", "Amarantrood"},
    {"theme.amaranth.desc", "石蕊红 · 苋菜红", "Litmus · Amaranth Red", "石蕊紅 · 莧菜紅", "Лакмус · Амаранти сурх", "लिटमस · एमरैंथ लाल", "Лакмус · Амарантовий червоний", "Lackmus · Amarantrot", "Tournesol · Rouge amarante", "Lakmoes · Amarantrood"},
    {"theme.mushroom", "蕈紫", "Mushroom Purple", "蕈紫", "Занбӯруғи бунафш", "मशरूम बैंगनी", "Грибний фіолетовий", "Pilzviolett", "Violet champignon", "Paddenstoelpaars"},
    {"theme.mushroom.desc", "豆汁黄 · 蕈紫", "Soybean · Mushroom Purple", "豆汁黃 · 蕈紫", "Лӯбиё · Занбӯруғи бунафш", "सोयाबीन · मशरूम बैंगनी", "Соєвий · Грибний фіолетовий", "Soja · Pilzviolett", "Soja · Violet champignon", "Soja · Paddenstoelpaars"},
    {"theme.scallion", "葱油绿", "Scallion Green", "蔥油綠", "Пиёзи сабз", "हरा प्याज़", "Цибулевий зелений", "Frühlingszwiebelgrün", "Vert ciboule", "Lente-ui groen"},
    {"theme.scallion.desc", "海天蓝 · 葱油绿", "Sea & Sky · Scallion Green", "海天藍 · 蔥油綠", "Баҳру осмон · Пиёзи сабз", "समुद्र-आकाश · हरा प्याज़", "Море й небо · Цибулевий зелений", "Meer & Himmel · Frühlingszwiebelgrün", "Mer et ciel · Vert ciboule", "Zee & lucht · Lente-ui groen"},
    {"theme.alum", "青矾绿", "Alum Green", "青礬綠", "Зағи сабз", "फिटकरी हरा", "Галуновий зелений", "Alaungrün", "Vert alun", "Aluingroen"},
    {"theme.alum.desc", "芡食白 · 青矾绿", "Gorgon · Alum Green", "芡食白 · 青礬綠", "Гургон · Зағи сабз", "गोर्गन · फिटकरी हरा", "Горгон · Галуновий зелений", "Gorgone · Alaungrün", "Gorgone · Vert alun", "Gorgon · Aluingroen"},
    {"theme.plum", "紫幽兰", "Violet Orchid", "紫幽蘭", "Орхидеяи бунафш", "बैंगनी ऑर्किड", "Фіолетова орхідея", "Violette Orchidee", "Orchidée violette", "Violette orchidee"},
    {"theme.plum.desc", "烟雨白 · 紫幽兰", "Misty Rain · Violet Orchid", "煙雨白 · 紫幽蘭", "Борони туман · Орхидеяи бунафш", "धुंधली वर्षा · बैंगनी ऑर्किड", "Туманний дощ · Фіолетова орхідея", "Nieselregen · Violette Orchidee", "Pluie brumeuse · Orchidée violette", "Motregen · Violette orchidee"},

    {"accent.theme", "跟随主题", "Theme default", "跟隨主題", "Пешфарзи мавзӯъ", "थीम डिफ़ॉल्ट", "За темою", "Designstandard", "Par défaut", "Standaard"},
    {"accent.purple", "紫色", "Purple", "紫色", "Бунафш", "बैंगनी", "Фіолетовий", "Lila", "Violet", "Paars"},
    {"accent.blue", "蓝色", "Blue", "藍色", "Кабуд", "नीला", "Синій", "Blau", "Bleu", "Blauw"},
    {"accent.orange", "橙色", "Orange", "橙色", "Норанҷӣ", "नारंगी", "Оранжевий", "Orange", "Orange", "Oranje"},
};

const LanguageInfo kLanguages[] = {
    {QStringLiteral("zh_CN"), QStringLiteral("简体中文"), QStringLiteral("Chinese (Simplified)")},
    {QStringLiteral("zh_TW"), QStringLiteral("繁體中文"), QStringLiteral("Chinese (Traditional)")},
    {QStringLiteral("en"), QStringLiteral("English"), QStringLiteral("English")},
    {QStringLiteral("de"), QStringLiteral("Deutsch"), QStringLiteral("German")},
    {QStringLiteral("fr"), QStringLiteral("Français"), QStringLiteral("French")},
    {QStringLiteral("nl"), QStringLiteral("Nederlands"), QStringLiteral("Dutch")},
    {QStringLiteral("uk"), QStringLiteral("Українська"), QStringLiteral("Ukrainian")},
    {QStringLiteral("hi"), QStringLiteral("हिन्दी"), QStringLiteral("Hindi")},
    {QStringLiteral("tg"), QStringLiteral("Тоҷикӣ"), QStringLiteral("Tajik")},
};

} // namespace

TranslationManager &TranslationManager::instance()
{
    static TranslationManager manager;
    return manager;
}

TranslationManager::TranslationManager()
{
    const auto addLanguage = [this](const char *id, int column) {
        QHash<QString, QString> table;
        const QString locale = QString::fromLatin1(id);
        for (const TranslationEntry &entry : kEntries) {
            const char *value = nullptr;
            switch (column) {
            case 0: value = entry.en;   break;
            case 1: value = entry.zhTW; break;
            case 2: value = entry.tg;   break;
            case 3: value = entry.hi;   break;
            case 4: value = entry.uk;   break;
            case 5: value = entry.de;   break;
            case 6: value = entry.fr;   break;
            case 7: value = entry.nl;   break;
            default: break;
            }
            if (value && *value) {
                table.insert(QString::fromLatin1(entry.key), QString::fromUtf8(value));
            }
        }
        m_tables.insert(locale, table);
    };

    for (const TranslationEntry &entry : kEntries) {
        m_base.insert(QString::fromLatin1(entry.key), QString::fromUtf8(entry.zh));
    }
    addLanguage("en", 0);
    addLanguage("zh_TW", 1);
    addLanguage("tg", 2);
    addLanguage("hi", 3);
    addLanguage("uk", 4);
    addLanguage("de", 5);
    addLanguage("fr", 6);
    addLanguage("nl", 7);

    QSettings settings(QStringLiteral("ScreenTime"), QStringLiteral("ScreenTime"));
    QString saved = settings.value(QStringLiteral("ui/language")).toString();
    if (saved.isEmpty() || !m_tables.contains(saved)) {
        if (saved.isEmpty()) {
            // 首次启动：跟随系统区域设置。
            const QString locale = QLocale::system().name();
            if (m_tables.contains(locale)) {
                saved = locale;
            } else if (locale.startsWith(QStringLiteral("zh"))) {
                saved = locale.contains(QStringLiteral("TW")) || locale.contains(QStringLiteral("HK"))
                            ? QStringLiteral("zh_TW")
                            : QStringLiteral("zh_CN");
            } else {
                saved = QStringLiteral("zh_CN");
            }
        } else {
            saved = QStringLiteral("zh_CN");
        }
    }
    m_languageId = saved;
    QLocale::setDefault(QLocale(m_languageId));
}

QVector<LanguageInfo> TranslationManager::languages() const
{
    QVector<LanguageInfo> list;
    for (const LanguageInfo &info : kLanguages) {
        list.append(info);
    }
    return list;
}

bool TranslationManager::setLanguage(const QString &id)
{
    if (id == m_languageId) {
        return false;
    }
    if (id != QStringLiteral("zh_CN") && !m_tables.contains(id)) {
        return false;
    }

    m_languageId = id;
    QLocale::setDefault(QLocale(id));
    QSettings settings(QStringLiteral("ScreenTime"), QStringLiteral("ScreenTime"));
    settings.setValue(QStringLiteral("ui/language"), id);
    emit languageChanged();
    return true;
}

bool TranslationManager::has(const QString &key) const
{
    return m_base.contains(key);
}

QStringList TranslationManager::missingKeys(const QString &languageId) const
{
    QStringList missing;
    if (languageId == QStringLiteral("zh_CN")) {
        return missing;
    }
    const auto tableIt = m_tables.constFind(languageId);
    if (tableIt == m_tables.constEnd()) {
        return m_base.keys();
    }
    for (auto it = m_base.constBegin(); it != m_base.constEnd(); ++it) {
        if (!tableIt->contains(it.key())) {
            missing.append(it.key());
        }
    }
    return missing;
}

QString TranslationManager::text(const QString &key) const
{
    if (m_languageId != QStringLiteral("zh_CN")) {
        const auto tableIt = m_tables.constFind(m_languageId);
        if (tableIt != m_tables.constEnd()) {
            const auto valueIt = tableIt->constFind(key);
            if (valueIt != tableIt->constEnd()) {
                return valueIt.value();
            }
        }
    }
    const auto baseIt = m_base.constFind(key);
    if (baseIt != m_base.constEnd()) {
        return baseIt.value();
    }
    return key;
}

// ── 控件辅助 ──────────────────────────────────────────────────────
void i18nSetText(QLabel *label, const char *key)
{
    if (!label) {
        return;
    }
    label->setProperty("i18nKey", QString::fromLatin1(key));
    label->setText(i18n(key));
}

void i18nSetText(QAbstractButton *button, const char *key)
{
    if (!button) {
        return;
    }
    button->setProperty("i18nKey", QString::fromLatin1(key));
    button->setText(i18n(key));
}

void i18nSetToolTip(QWidget *widget, const char *key)
{
    if (!widget) {
        return;
    }
    widget->setProperty("i18nTooltipKey", QString::fromLatin1(key));
    widget->setToolTip(i18n(key));
}

void i18nRetranslate(QWidget *root)
{
    if (!root) {
        return;
    }
    for (QLabel *label : root->findChildren<QLabel *>()) {
        const QVariant key = label->property("i18nKey");
        if (key.isValid()) {
            label->setText(TranslationManager::instance().text(key.toString()));
        }
    }
    for (QAbstractButton *button : root->findChildren<QAbstractButton *>()) {
        const QVariant key = button->property("i18nKey");
        if (key.isValid()) {
            button->setText(TranslationManager::instance().text(key.toString()));
        }
    }
    for (QWidget *widget : root->findChildren<QWidget *>()) {
        const QVariant key = widget->property("i18nTooltipKey");
        if (key.isValid()) {
            widget->setToolTip(TranslationManager::instance().text(key.toString()));
        }
    }
}
