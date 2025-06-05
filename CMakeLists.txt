cmake_minimum_required(VERSION 3.16)
project(NetDebugClient LANGUAGES CXX)


if(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
    set(CMAKE_PREFIX_PATH "/opt/Qt/6.9.1/gcc_64")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
    # set(CMAKE_PREFIX_PATH "C:/Qt/6.9.1/mingw_64")
elseif()
    # message(FATAL_ERROR "Unkonw system.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 WebView WebChannel Network WebEngineQuick)

qt_add_executable(${PROJECT_NAME}
    User/qt_main.cpp
    User/app_main.hpp
    User/TerminalBackend.cpp
    User/TerminalBackend.hpp
    User/DeviceManager.hpp
    User/QTTimebase.hpp
)

qt_add_qml_module(${PROJECT_NAME}
    URI MyApp
    VERSION 1.0
    QML_FILES
    qml/Main.qml
    qml/TerminalBackendConnector.qml
    qml/MainTerminalView.qml
    qml/TabButton.qml
    qml/SerialConfigPanel.qml
    qml/StatusIndicators.qml
)

qt6_add_resources(${PROJECT_NAME} "web_resources"
    PREFIX "/web"
    BASE web
    FILES
    web/index.html
    web/favicon.ico
    web/xterm/xterm.js
    web/xterm/xterm.css
    web/xterm/xterm-addon-fit.min.js
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::WebView
    Qt6::WebChannel
    Qt6::Network
    Qt6::WebEngineQuick
    Qt6::WebChannel
)

Set(LIBXR_SYSTEM "None")
Set(LIBXR_PRINTF_BUFFER_SIZE 4096)
Set(XR_LOG_MESSAGE_MAX_LEN 256)
set(LIBXR_LOG_LEVEL 3)

add_subdirectory(libxr)

add_dependencies(${PROJECT_NAME} xr)

target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC xr
)

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC $<TARGET_PROPERTY:xr,INTERFACE_INCLUDE_DIRECTORIES>
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib/Eigen
)
