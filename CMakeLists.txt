cmake_minimum_required(VERSION 3.20)
project(hiocr LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network WebEngineWidgets WebChannel DBus)

# 添加源代码子目录
add_subdirectory(src)

# 安装可执行文件
install(TARGETS hiocr RUNTIME DESTINATION bin)

# 安装桌面文件
install(FILES ${CMAKE_SOURCE_DIR}/hiocr.desktop DESTINATION share/applications)

# 安装图标（假定图标尺寸为 256x256，可按需调整路径）
install(FILES ${CMAKE_SOURCE_DIR}/hiocr.png DESTINATION share/icons/hicolor/512x512/apps)


# 添加 uninstall 目标
if(NOT TARGET uninstall)
    configure_file(
        "${CMAKE_SOURCE_DIR}/cmake_uninstall.cmake.in"
        "${CMAKE_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY
    )

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/cmake_uninstall.cmake
        COMMENT "Uninstalling hiocr"
    )
endif()

