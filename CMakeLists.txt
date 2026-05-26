cmake_minimum_required(VERSION 3.12)

project(dde-dock-netspeed-plugin)

set(VERSION 1.2.0)
set(PLUGIN_NAME "netspeed-monitor")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

option(OPT_ENABLE_QT6 "Use Qt6" ON)

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX /usr)
endif ()

include(GNUInstallDirs)

add_compile_definitions(PLUGIN_NAME="${PLUGIN_NAME}")
add_compile_definitions(VERSION="${VERSION}")

set(QT_VERSION_MAJOR 5)
set(DTK_VERSION_MAJOR "")
if(OPT_ENABLE_QT6 MATCHES ON)
    set(QT_VERSION_MAJOR 6)
    set(DTK_VERSION_MAJOR 6)
endif()

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS DBus Network Widgets REQUIRED)
find_package(Dtk${DTK_VERSION_MAJOR} COMPONENTS Core Gui REQUIRED)
find_package(DdeTrayLoader QUIET)

file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS "*.h" "*.hpp" "*.cpp" "*.ui")
add_library(${PROJECT_NAME} MODULE ${SRCS})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PLUGIN_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC
    Qt${QT_VERSION_MAJOR}::DBus
    Qt${QT_VERSION_MAJOR}::Widgets
    Dtk${DTK_VERSION_MAJOR}::Core
    Dtk${DTK_VERSION_MAJOR}::Gui
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::DBus
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Widgets
    Dtk${DTK_VERSION_MAJOR}::Core
    Dtk${DTK_VERSION_MAJOR}::Gui
)



install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/dde-dock/plugins
)
install(FILES ${CMAKE_SOURCE_DIR}/netspeed.json
    DESTINATION ${CMAKE_INSTALL_DATADIR}/dde-dock/plugins
)
install(FILES ${CMAKE_SOURCE_DIR}/resources/gschema/com.deepin.dde.dock.module.netspeed-monitor.gschema.xml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/glib-2.0/schemas
)
install(FILES ${CMAKE_SOURCE_DIR}/resources/icons/netspeed-monitor.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/dde-dock/icons/dcc-setting
)
