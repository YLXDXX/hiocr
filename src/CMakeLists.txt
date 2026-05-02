# 核心库头文件 (已合并为 header-only)
set(LIB_HEADERS
    networkmanager.h
    imageprocessor.h
    settingsmanager.h
    copyprocessor.h
)

add_library(hiocr_lib STATIC ${LIB_HEADERS})
target_link_libraries(hiocr_lib Qt6::Core Qt6::Network Qt6::Gui)
target_include_directories(hiocr_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 主程序源文件
set(APP_SOURCES
    main.cpp
)

set(APP_HEADERS
    mainwindow.h
    markdownbridge.h
    imageviewwidget.h
    markdownrenderer.h
    markdownsourceeditor.h
    promptbar.h
    screenshotareaselector.h
    screenshotmanager.h
    globalshortcutmanager.h
    traymanager.h
    recognitionmanager.h
    appcontroller.h
    shortcuthandler.h
    settingsdialog.h
    constants.h
    markdowncopybar.h
    servicemanager.h
    shortcutedit.h
    singleapplication.h
    historymanager.h
    floatingball.h
)

if(HAVE_KF6)
    list(APPEND APP_HEADERS kdeglobalshortcut.h)
endif()

add_executable(hiocr ${APP_SOURCES} ${APP_HEADERS})

qt6_add_resources(APP_RESOURCES resources.qrc)
target_sources(hiocr PRIVATE ${APP_RESOURCES})

target_link_libraries(hiocr
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::WebChannel
    Qt6::DBus
    Qt6::Sql
    hiocr_lib
)

if(HAVE_KF6)
    target_link_libraries(hiocr KF6::GlobalAccel)
    target_compile_definitions(hiocr PRIVATE HAVE_KF6)
endif()

if(HAVE_KF6_WINDOWSYSTEM)
    target_link_libraries(hiocr KF6::WindowSystem)
    target_compile_definitions(hiocr PRIVATE HAVE_KF6_WINDOWSYSTEM)
endif()
