cmake_minimum_required(VERSION 3.16)

project(ScreenTime VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Sql
    Charts
    Svg
    Network
)

qt_add_executable(ScreenTime
    main.cpp
    core/aireporter.cpp
    core/aireporter.h
    core/appnameresolver.cpp
    core/appnameresolver.h
    core/database.cpp
    core/database.h
    core/startupmanager.cpp
    core/startupmanager.h
    core/tracker.cpp
    core/tracker.h
    ui/aireportpage.cpp
    ui/aireportpage.h
    ui/fluenttoggleswitch.cpp
    ui/fluenttoggleswitch.h
    ui/hourlychartwidget.cpp
    ui/hourlychartwidget.h
    ui/i18n/translationmanager.cpp
    ui/i18n/translationmanager.h
    ui/mainwindow.cpp
    ui/mainwindow.h
    ui/theme/theme.cpp
    ui/theme/theme.h
    ui/theme/thememanager.cpp
    ui/theme/thememanager.h
    ui/theme/themestyles.cpp
    ui/theme/themestyles.h
    ui/windowblur.cpp
    ui/windowblur.h
    resources.qrc
)

target_link_libraries(ScreenTime PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
    Qt6::Charts
    Qt6::Svg
    Qt6::Network
)

target_include_directories(ScreenTime PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(ScreenTime PRIVATE SCREENTIME_VERSION="${PROJECT_VERSION}")

if(WIN32)
    target_link_libraries(ScreenTime PRIVATE user32 psapi runtimeobject dwmapi)

    if(MINGW)
        find_program(WINDRES_EXECUTABLE windres)
        if(WINDRES_EXECUTABLE)
            set(APP_ICON_OBJECT "${CMAKE_CURRENT_BINARY_DIR}/app_icon.obj")
            add_custom_command(
                OUTPUT "${APP_ICON_OBJECT}"
                COMMAND "${WINDRES_EXECUTABLE}" -O coff
                        -i "${CMAKE_CURRENT_SOURCE_DIR}/app.rc"
                        -o "${APP_ICON_OBJECT}"
                DEPENDS app.rc icons/app.ico
                VERBATIM
            )
            target_sources(ScreenTime PRIVATE "${APP_ICON_OBJECT}")
        endif()
    else()
        target_sources(ScreenTime PRIVATE app.rc)
    endif()
endif()

set_target_properties(ScreenTime PROPERTIES
    WIN32_EXECUTABLE TRUE
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

install(TARGETS ScreenTime RUNTIME DESTINATION bin)
