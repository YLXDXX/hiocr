cmake_minimum_required(VERSION 3.20)
project(hiocr LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network WebEngineWidgets WebChannel DBus Sql)

# 可选：查找 KF6GlobalAccel（KDE 原生全局快捷键）
find_package(KF6GlobalAccel QUIET)
if(KF6GlobalAccel_FOUND)
    set(HAVE_KF6 TRUE)
    message(STATUS "KF6GlobalAccel found, will use KGlobalAccel")
else()
    message(STATUS "KF6GlobalAccel not found, will fallback to xdg-desktop-portal")
endif()

add_subdirectory(src)

install(TARGETS hiocr RUNTIME DESTINATION bin)
install(FILES ${CMAKE_SOURCE_DIR}/hiocr.desktop DESTINATION share/applications)
install(FILES ${CMAKE_SOURCE_DIR}/hiocr.svg DESTINATION share/icons/hicolor/scalable/apps)

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
