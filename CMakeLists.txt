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

# Standard cross-platform discovery for OpenCV (optional – app works with clear fallback if absent)
if(APPLE)
    list(APPEND CMAKE_PREFIX_PATH
        "/opt/homebrew"
        "/opt/homebrew/opt/opencv"
        "/opt/homebrew/opt/opencv/lib/cmake/opencv5"
        "/opt/homebrew/opt/opencv/lib/cmake/opencv4"
        "/opt/homebrew/lib/cmake/opencv5"
        "/opt/homebrew/lib/cmake/opencv4"
        "/usr/local"
        "/usr/local/opt/opencv"
    )
elseif(WIN32)
    if(DEFINED ENV{OpenCV_DIR})
        list(APPEND CMAKE_PREFIX_PATH "$ENV{OpenCV_DIR}")
    endif()
    if(DEFINED ENV{OPENCV_DIR})
        list(APPEND CMAKE_PREFIX_PATH "$ENV{OPENCV_DIR}")
    endif()
    # Common portable Windows installation paths
    list(APPEND CMAKE_PREFIX_PATH
        "C:/opencv/build"
        "C:/opencv"
        "C:/tools/opencv/build"
        "C:/tools/opencv"
        "C:/vcpkg/installed/x64-windows"
    )
endif()

# Try finding required camera scanning components first
find_package(OpenCV QUIET COMPONENTS core videoio highgui imgproc)
if(NOT OpenCV_FOUND)
    # General fallback search across standard system paths
    find_package(OpenCV QUIET)
endif()

if(OpenCV_FOUND)
    message(STATUS "OpenCV found: ${OpenCV_VERSION} (${OpenCV_DIR}) — live camera scanner ENABLED")
    add_compile_definitions(OPENCV_AVAILABLE)
else()
    message(STATUS "OpenCV NOT found — live camera scanner will be safely disabled with runtime notice.")
    message(STATUS "  To enable live camera scanning:")
    message(STATUS "  - On macOS: brew install opencv")
    message(STATUS "  - On Windows: Extract OpenCV and set OpenCV_DIR environment variable (e.g. C:\\opencv\\build)")
    add_compile_definitions(OPENCV_NOT_AVAILABLE)
endif()

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

    # Product & Billing Module Integration
    pos/ProductBillingService.h
    pos/ProductBillingService.cpp
    product_module/DynamicArray.h
    product_module/Product.h
    product_module/Product.cpp
    product_module/ProductManager.h
    product_module/ProductManager.cpp
    product_module/Billing.h
    product_module/Billing.cpp

    # Camera Scanner & Real Barcode Decoding Integration
    barcode/BarcodeDecoder.h
    barcode/BarcodeDecoder.cpp
    barcode/camera_scanner.h
    barcode/camera_scanner.cpp
    pos/CameraScannerService.h
    pos/CameraScannerService.cpp

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

    # Order Management Module (Jav)
    order_management/OrderModule.h
    order_management/OrderModule.cpp

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
    ${CMAKE_CURRENT_SOURCE_DIR}/product_module
    ${CMAKE_CURRENT_SOURCE_DIR}/order_management
)

# Link against Qt6 Widgets
target_link_libraries(pos_app PRIVATE
    Qt6::Widgets
)

# Link OpenCV if available
if(OpenCV_FOUND)
    target_include_directories(pos_app PRIVATE ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(pos_app PRIVATE ${OpenCV_LIBS})
endif()

# On Windows Release builds, suppress console window
if(WIN32)
    set_target_properties(pos_app PROPERTIES
        WIN32_EXECUTABLE $<CONFIG:Release>
    )
endif()

# Copy Product.txt alongside the executable so the application finds it at runtime
add_custom_command(TARGET pos_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/product_module/Product.txt"
    "$<TARGET_FILE_DIR:pos_app>/Product.txt"
    COMMENT "Copying Product.txt to output directory"
)
