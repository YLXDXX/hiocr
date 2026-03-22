# 核心库源文件
set(LIB_SOURCES
    networkmanager.cpp
    imageprocessor.cpp
)

set(LIB_HEADERS
    networkmanager.h
    imageprocessor.h
)

# 创建静态库
add_library(hiocr_lib STATIC ${LIB_SOURCES} ${LIB_HEADERS})

target_link_libraries(hiocr_lib
    Qt6::Core
    Qt6::Network
    Qt6::Gui
)

target_include_directories(hiocr_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 主程序
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
)

add_executable(hiocr ${APP_SOURCES} ${APP_HEADERS})

# 添加资源文件
qt6_add_resources(APP_RESOURCES resources.qrc)
target_sources(hiocr PRIVATE ${APP_RESOURCES})

target_link_libraries(hiocr
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::WebChannel
    Qt6::DBus
    hiocr_lib
)

install(TARGETS hiocr RUNTIME DESTINATION bin)
