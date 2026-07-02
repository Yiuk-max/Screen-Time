cmake_minimum_required(VERSION 3.16)

project(ScreenTime VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Qt's meta-object compiler, resource compiler, and UI compiler
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6 packages
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Sql
    Charts
    Svg
    Network
)

# Define the executable using qt_add_executable (Qt 6 recommended)
qt_add_executable(ScreenTime
    main.cpp
    core/database.cpp

    core/database.h
    core/tracker.cpp
    core/tracker.h
	    core/aireporter.cpp
	    core/aireporter.h
	    core/appnameresolver.cpp
	    core/appnameresolver.h
    ui/hourlychartwidget.cpp
    ui/hourlychartwidget.h
	    ui/aireportpage.cpp
	    ui/aireportpage.h
	    ui/apptheme.cpp
	    ui/apptheme.h
    ui/mainwindow.cpp
    ui/mainwindow.h
    resources.qrc
)

# Link Qt libraries
target_link_libraries(ScreenTime PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
    Qt6::Charts
    Qt6::Svg
    Qt6::Network
)

# Windows-specific libraries for application tracking
if(WIN32)
    target_link_libraries(ScreenTime PRIVATE
        user32
        psapi
    )

    if(MINGW)
        find_program(WINDRES_EXECUTABLE windres)
        if(WINDRES_EXECUTABLE)
            set(APP_ICON_OBJECT "${CMAKE_CURRENT_BINARY_DIR}/app_icon.obj")
            add_custom_command(
                OUTPUT "${APP_ICON_OBJECT}"
                COMMAND "${WINDRES_EXECUTABLE}" -O coff -i "${CMAKE_CURRENT_SOURCE_DIR}/app.rc" -o "${APP_ICON_OBJECT}"
                DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/app.rc" "${CMAKE_CURRENT_SOURCE_DIR}/icons/app.ico"
                VERBATIM
            )
            target_sources(ScreenTime PRIVATE "${APP_ICON_OBJECT}")
        endif()
    else()
        target_sources(ScreenTime PRIVATE app.rc)
    endif()
endif()

# Include project root directory so headers in core/ and ui/ can be found
target_include_directories(ScreenTime PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(ScreenTime PRIVATE
    SCREENTIME_VERSION="${PROJECT_VERSION}"
)

# Set target properties for Windows GUI application
set_target_properties(ScreenTime PROPERTIES
    WIN32_EXECUTABLE TRUE
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Installation rules (optional)
install(TARGETS ScreenTime
    RUNTIME DESTINATION bin
)
