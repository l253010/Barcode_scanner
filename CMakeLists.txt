cmake_minimum_required(VERSION 3.10)
project(BarcodeScannerPOS)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories for headers
include_directories(${CMAKE_SOURCE_DIR}/barcode)
include_directories(${CMAKE_SOURCE_DIR}/product_module)
include_directories(${CMAKE_SOURCE_DIR}/order_management)
include_directories(${CMAKE_SOURCE_DIR}/pos)

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
)
target_link_libraries(pos_app BarcodeModule ProductModule OrderModule)
