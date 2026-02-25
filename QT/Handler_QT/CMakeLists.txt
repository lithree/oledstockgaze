cmake_minimum_required(VERSION 3.5)
project(OLED_StockGaze VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network WebSockets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network WebSockets)

# Set the include directory to the current project folder where libusb.h is located
set(LIBUSB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

# Set the exact path to the library file in the current folder.
# Change ".a" to ".lib" if you are using the MSVC compiler instead of MinGW.
set(LIBUSB_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/libusb-1.0.a")

add_executable(OLED_StockGaze
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
)

target_include_directories(OLED_StockGaze PRIVATE ${LIBUSB_INCLUDE_DIR})

target_link_libraries(OLED_StockGaze PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::WebSockets
    ${LIBUSB_LIBRARY}
)
