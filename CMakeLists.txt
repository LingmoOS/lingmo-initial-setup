cmake_minimum_required(VERSION 3.22)

project(lingmo-oobe VERSION 1.2.8 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)
include(CTest)

find_package(LingmoSDK REQUIRED)
find_package(LingmoClients REQUIRED COMPONENTS appearance timedate)

set(OOBE_DATA_DIR ${CMAKE_INSTALL_FULL_DATADIR}/lingmo-oobe CACHE PATH
    "OOBE installed data directory")

add_executable(lingmo-oobe
    src/main.cpp
    src/oobewindow.cpp
)
target_include_directories(lingmo-oobe PRIVATE src)
target_compile_definitions(lingmo-oobe PRIVATE OOBE_DATA_DIR="${OOBE_DATA_DIR}")
target_link_libraries(lingmo-oobe PRIVATE Lingmo::SDK Lingmo::AppearanceClient Lingmo::TimeDateClient)

install(TARGETS lingmo-oobe RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES data/lingmo-oobe.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/wayland-sessions)
install(FILES data/lingmo-oobe-app.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(DIRECTORY data/images/
        DESTINATION ${OOBE_DATA_DIR}/images)

if(BUILD_TESTING)
    add_test(NAME oobe-state-test
        COMMAND ${CMAKE_COMMAND} -E env
            PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/tests
            python3 ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_state.py)
endif()
