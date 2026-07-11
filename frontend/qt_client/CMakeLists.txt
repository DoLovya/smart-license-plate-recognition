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
else()
    find_package(Qt6 REQUIRED COMPONENTS Widgets Network Test)
    set(QT_PACKAGE Qt6)
endif()

set(QT_CLIENT_APP_SOURCES
    src/app/main.cpp
    src/app/MainWindow.cpp
    include/app/MainWindow.h
)

set(QT_CLIENT_API_SOURCES
    src/api/AlgorithmServiceClient.cpp
    include/api/AlgorithmServiceClient.h
)

set(QT_CLIENT_SERVICE_SOURCES
    src/services/ResultExportService.cpp
    include/services/ResultExportService.h
)

set(QT_CLIENT_WIDGET_SOURCES
    src/widgets/ImagePreviewWidget.cpp
    src/widgets/ImageGalleryWidget.cpp
    include/widgets/ImagePreviewWidget.h
    include/widgets/ImageGalleryWidget.h
)

set(QT_CLIENT_SHARED_HEADERS
    include/core/RecognitionTypes.h
)

set(QT_CLIENT_SOURCES
    ${QT_CLIENT_APP_SOURCES}
    ${QT_CLIENT_API_SOURCES}
    ${QT_CLIENT_SERVICE_SOURCES}
    ${QT_CLIENT_WIDGET_SOURCES}
    ${QT_CLIENT_SHARED_HEADERS}
    ui/MainWindow.ui
    resources/resources.qrc
)

add_executable(${PROJECT_NAME} ${QT_CLIENT_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE include)
set(QT_CLIENT_LIBS
    ${QT_PACKAGE}::Widgets
    ${QT_PACKAGE}::Network
)
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
        ${QT_CLIENT_API_SOURCES}
        ${QT_CLIENT_SERVICE_SOURCES}
        ${QT_CLIENT_WIDGET_SOURCES}
        ${QT_CLIENT_SHARED_HEADERS}
        src/app/MainWindow.cpp
        include/app/MainWindow.h
        ui/MainWindow.ui
        resources/resources.qrc
    )

    target_include_directories(${PROJECT_NAME}Tests PRIVATE include)
    target_link_libraries(
        ${PROJECT_NAME}Tests
        PRIVATE
            ${QT_CLIENT_LIBS}
            ${QT_PACKAGE}::Test
    )

    add_test(NAME SmartLicensePlateQtClientTests COMMAND ${PROJECT_NAME}Tests)
endif()
