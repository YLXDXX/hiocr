# 核心库源文件
# 注意：这里只放不依赖 Widgets 的基础类
set(LIB_SOURCES
    networkmanager.cpp
    imageprocessor.cpp
    settingsmanager.cpp
    copyprocessor.cpp
)

set(LIB_HEADERS
    networkmanager.h
    imageprocessor.h
    settingsmanager.h
    copyprocessor.h
)

add_library(hiocr_lib STATIC ${LIB_SOURCES} ${LIB_HEADERS})
# 核心库不需要链接 Widgets
target_link_libraries(hiocr_lib Qt6::Core Qt6::Network Qt6::Gui)
target_include_directories(hiocr_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 主程序源文件
# 注意：这里放所有依赖 Widgets 或业务逻辑紧密相关的类
set(APP_SOURCES
    main.cpp
    mainwindow.cpp
    imageviewwidget.cpp
    markdownrenderer.cpp
    markdownsourceeditor.cpp
    promptbar.cpp
    screenshotareaselector.cpp
    screenshotmanager.cpp
    globalshortcutmanager.cpp
    traymanager.cpp
    recognitionmanager.cpp
    appcontroller.cpp
    shortcuthandler.cpp
    settingsdialog.cpp
    markdowncopybar.cpp
    servicemanager.cpp
    shortcutedit.cpp
    singleapplication.cpp
    historymanager.cpp
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
    settingsdialog.h
    constants.h
    markdowncopybar.h
    servicemanager.h
    shortcutedit.h
    singleapplication.h
    historymanager.h
)

# 如果 KF6GlobalAccel 可用，则添加 KGlobalAccel 支持
if(HAVE_KF6)
    list(APPEND APP_SOURCES kdeglobalshortcut.cpp)
    list(APPEND APP_HEADERS kdeglobalshortcut.h)
endif()

add_executable(hiocr ${APP_SOURCES} ${APP_HEADERS})

# 资源文件
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
