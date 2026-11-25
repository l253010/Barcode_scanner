cmake_minimum_required(VERSION 3.10)
project(BarcodeScannerPOS)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories for headers
include_directories(${CMAKE_SOURCE_DIR}/barcode)
include_directories(${CMAKE_SOURCE_DIR}/product_module)
include_directories(${CMAKE_SOURCE_DIR}/order_management)
include_directories(${CMAKE_SOURCE_DIR}/pos)

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

# -----------------------------------------------------------------
# 1. Barcode Module Library
# -----------------------------------------------------------------
add_library(BarcodeModule STATIC
    barcode/BarcodeDecoder.cpp
    barcode/BarcodeDecoder.h
    barcode/BarcodeGenerator.cpp
    barcode/BarcodeGenerator.h
    barcode/camera_scanner.cpp
    barcode/camera_scanner.h
)

# -----------------------------------------------------------------
# 2. Product Module Library
# -----------------------------------------------------------------
add_library(ProductModule STATIC
    product_module/Product.cpp
    product_module/Product.h
    product_module/ProductManager.cpp
    product_module/ProductManager.h
    product_module/Billing.cpp
    product_module/Billing.h
    product_module/DynamicArray.h
)

# -----------------------------------------------------------------
# 3. Order Management Module Library (Jav's Module)
# -----------------------------------------------------------------
add_library(OrderModule STATIC
    order_management/OrderModule.cpp
    order_management/OrderModule.h
)

# -----------------------------------------------------------------
# 4. Executables and Unit Tests
# -----------------------------------------------------------------

# Order Module Unit Test Executable
add_executable(test_order 
    order_management/test_order.cpp
)
target_link_libraries(test_order OrderModule)

# Existing Product Module Unit Test
add_executable(test_product_module
    product_module/test_product_module.cpp
)
target_link_libraries(test_product_module ProductModule)

# Existing Barcode/Decoder Test
add_executable(test_decoder
    barcode/test_decoder.cpp
)
target_link_libraries(test_decoder BarcodeModule)

# Main POS Application Executable (linking all components together)
add_executable(pos_app
    pos/main.cpp
    pos/MainWindow.cpp
    pos/DashboardView.cpp
    pos/CashierOrderView.cpp
    pos/ManagerView.cpp
    pos/PaymentView.cpp
    pos/BillPreviewView.cpp
    pos/ProductDescriptionView.cpp
    pos/ProductBillingService.cpp
    pos/MockDataService.cpp
    pos/CameraScannerService.cpp
    barcode/BarcodeDecoder.cpp
    barcode/BarcodeGenerator.cpp
    barcode/camera_scanner.cpp
)
target_link_libraries(pos_app BarcodeModule ProductModule OrderModule)

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