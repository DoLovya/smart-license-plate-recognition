cmake_minimum_required(VERSION 3.16)
project(SmartLicensePlateQtClient VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/ui)

option(BUILD_QT_CLIENT_TESTS "Build Qt client unit tests" ON)

find_package(Qt5 5.15 QUIET COMPONENTS Widgets Network Test)
if(Qt5_FOUND)
    set(QT_PACKAGE Qt5)
    find_package(Qt5 QUIET COMPONENTS Multimedia)
else()
    find_package(Qt6 REQUIRED COMPONENTS Widgets Network Test)
    set(QT_PACKAGE Qt6)
    find_package(Qt6 QUIET COMPONENTS Multimedia)
endif()

set(QT_CLIENT_SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/VideoDisplayWidget.cpp
    src/VideoStreamWorker.cpp
    src/AlgorithmServiceClient.cpp
    src/ResultExportService.cpp
    include/MainWindow.h
    include/RecognitionTypes.h
    include/VideoDisplayWidget.h
    include/VideoStreamWorker.h
    include/AlgorithmServiceClient.h
    include/ResultExportService.h
    ui/MainWindow.ui
    resources/resources.qrc
)

add_executable(${PROJECT_NAME} ${QT_CLIENT_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE include)
set(QT_CLIENT_LIBS
    ${QT_PACKAGE}::Widgets
    ${QT_PACKAGE}::Network
)
if(TARGET ${QT_PACKAGE}::Multimedia)
    list(APPEND QT_CLIENT_LIBS ${QT_PACKAGE}::Multimedia)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_QT_MULTIMEDIA)
endif()
target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
        ${QT_CLIENT_LIBS}
)

if(BUILD_QT_CLIENT_TESTS)
    enable_testing()

    add_executable(
        ${PROJECT_NAME}Tests
        tests/tst_MainWindow.cpp
        src/MainWindow.cpp
        src/VideoDisplayWidget.cpp
        src/VideoStreamWorker.cpp
        src/AlgorithmServiceClient.cpp
        src/ResultExportService.cpp
        include/MainWindow.h
        include/RecognitionTypes.h
        include/VideoDisplayWidget.h
        include/VideoStreamWorker.h
        include/AlgorithmServiceClient.h
        include/ResultExportService.h
        ui/MainWindow.ui
        resources/resources.qrc
    )

    target_include_directories(${PROJECT_NAME}Tests PRIVATE include)
    if(TARGET ${QT_PACKAGE}::Multimedia)
        target_compile_definitions(${PROJECT_NAME}Tests PRIVATE HAVE_QT_MULTIMEDIA)
    endif()
    target_link_libraries(
        ${PROJECT_NAME}Tests
        PRIVATE
            ${QT_CLIENT_LIBS}
            ${QT_PACKAGE}::Test
    )

    add_test(NAME SmartLicensePlateQtClientTests COMMAND ${PROJECT_NAME}Tests)
endif()
