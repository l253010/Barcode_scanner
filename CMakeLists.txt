cmake_minimum_required(VERSION 3.16)

project(BarcodePOSSystem
    VERSION 2.0.0
    DESCRIPTION "Point of Sale (POS) Desktop System with Code 39 Barcode Integration"
    LANGUAGES CXX
)

# Enforce standard C++17 across all platforms
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable Qt-specific compilation tools
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find Qt 6 Widgets using standard CMake package discovery
find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets)

# Portable compiler warning and encoding flags
if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# POS Application Sources
set(POS_SOURCES
    # Application Entry & Shell
    pos/main.cpp
    pos/MainWindow.h
    pos/MainWindow.cpp

    # Shared Team Interfaces & Mock Services
    pos/POSInterfaces.h
    pos/MockDataService.h
    pos/MockDataService.cpp

    # Screens
    pos/DashboardView.h
    pos/DashboardView.cpp
    pos/CashierOrderView.h
    pos/CashierOrderView.cpp
    pos/ProductDescriptionView.h
    pos/ProductDescriptionView.cpp
    pos/BillPreviewView.h
    pos/BillPreviewView.cpp
    pos/PaymentView.h
    pos/PaymentView.cpp
    pos/ManagerView.h
    pos/ManagerView.cpp

    # Reused Increment 1 Core Barcode Logic
    barcode/BarcodeGenerator.h
    barcode/BarcodeGenerator.cpp
)

# Define executable
add_executable(pos_app ${POS_SOURCES})

# Target Include Directories (portable relative paths)
target_include_directories(pos_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/pos
    ${CMAKE_CURRENT_SOURCE_DIR}/barcode
)

# Link against Qt6 Widgets
target_link_libraries(pos_app PRIVATE
    Qt6::Widgets
)

# On Windows Release builds, suppress console window
if(WIN32)
    set_target_properties(pos_app PROPERTIES
        WIN32_EXECUTABLE $<CONFIG:Release>
    )
endif()
