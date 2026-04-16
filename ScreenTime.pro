QT += core gui widgets sql charts svg

CONFIG += c++17 windows
CONFIG -= app_bundle

TEMPLATE = app
TARGET = ScreenTime

SOURCES += \
    main.cpp \
    core/database.cpp \
    core/tracker.cpp \
    ui/hourlychartwidget.cpp \
    ui/mainwindow.cpp

HEADERS += \
    core/database.h \
    core/tracker.h \
    ui/hourlychartwidget.h \
    ui/mainwindow.h

win32 {
    LIBS += -luser32 -lpsapi
}

DISTFILES += \
    icons/setting.ico
