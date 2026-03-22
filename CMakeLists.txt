cmake_minimum_required(VERSION 3.20)
project(hiocr LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network WebEngineWidgets WebChannel DBus)

# 添加源代码子目录
add_subdirectory(src)

