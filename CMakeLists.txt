cmake_minimum_required(VERSION 3.16)
project(ProjectJPO LANGUAGES CXX)

file(TO_CMAKE_PATH "D:/projects/JPO/ProjectJPO" PROJECT_ROOT)

set(CMAKE_PREFIX_PATH "C:/Qt/6.6.0/mingw_64/lib/cmake")

find_package(Qt6 REQUIRED COMPONENTS
    Core Gui Widgets Network Location Positioning Charts Sql Concurrent
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(SOURCES
    "${PROJECT_ROOT}/main.cpp"
    "${PROJECT_ROOT}/ui/mainwindow.cpp"
    "${PROJECT_ROOT}/core/Station.cpp"
    "${PROJECT_ROOT}/core/Sensor.cpp"
    "${PROJECT_ROOT}/core/Measurement.cpp"
    "${PROJECT_ROOT}/core/AirQualityIndex.cpp"
    "${PROJECT_ROOT}/data/ApiHandler.cpp"
    "${PROJECT_ROOT}/data/DatabaseManager.cpp"
)

set(HEADERS
    "${PROJECT_ROOT}/ui/mainwindow.h"
    "${PROJECT_ROOT}/core/Station.h"
    "${PROJECT_ROOT}/core/Sensor.h"
    "${PROJECT_ROOT}/core/Measurement.h"
    "${PROJECT_ROOT}/core/AirQualityIndex.h"
    "${PROJECT_ROOT}/data/ApiHandler.h"
    "${PROJECT_ROOT}/data/DatabaseManager.h"
)

set(FORMS
    "${PROJECT_ROOT}/ui/mainwindow.ui"
)

add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${FORMS}
    data/DatabaseManager.h
    data/DatabaseManager.cpp
    data/JsonbaseManager.h
    data/JsonbaseManager.cpp
)

target_link_libraries(${PROJECT_NAME}
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network
    Qt6::Location Qt6::Positioning Qt6::Charts
    Qt6::Sql Qt6::Concurrent
)

target_include_directories(${PROJECT_NAME} PRIVATE
    "${PROJECT_ROOT}/core"
    "${PROJECT_ROOT}/data"
    "${PROJECT_ROOT}/ui"
)
