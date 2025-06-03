cmake_minimum_required(VERSION 3.16)
project(NetDebugClient LANGUAGES CXX)

set(CMAKE_PREFIX_PATH "/opt/Qt/6.9.0/gcc_64") # ✅ 用户指定 Qt 安装路径
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)

qt_add_executable(${PROJECT_NAME}
    main.cpp
    TerminalBackend.cpp
    TerminalBackend.hpp
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

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
)

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
