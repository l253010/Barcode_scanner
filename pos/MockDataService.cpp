#include "MockDataService.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>

MockDataService::MockDataService() {
    initSampleCatalog();
}

void MockDataService::initSampleCatalog() {
    // Realistic retail store catalog with PKR pricing (Rs.)
    m_catalog = {
        {101, "100001", "MILK-1L", "Fresh Whole Milk 1L",          "Bottle",  250.00, 0.05, 20.00, 45},
        {102, "100002", "BREAD-W", "Whole Wheat Bread 500g",        "Loaf",    120.00, 0.05, 15.00, 30},
        {103, "100003", "EGGS-12", "Fresh Farm Brown Eggs (12-pk)", "Pack",    380.00, 0.05, 30.00, 60},
        {104, "100004", "RICE-5K", "Premium Basmati Rice 5kg",      "Bag",    1850.00, 0.05, 150.00, 25},
        {105, "100005", "OIL-1L",  "Pure Cooking Oil 1L",           "Bottle",  540.00, 0.05, 40.00, 40},
        {106, "100006", "TEA-500", "Premium Black Tea 500g",        "Box",     480.00, 0.05, 35.00, 35},
        {107, "100007", "SUGAR-2", "Refined White Sugar 2kg",       "Bag",     320.00, 0.05, 25.00, 50},
        {108, "100008", "COF-250", "Classic Roast Coffee 200g",     "Jar",     950.00, 0.08, 75.00, 20}
    };
}

bool MockDataService::findProductByBarcode(const std::string& barcode, ProductInfo& outProduct) {
    for (const auto& item : m_catalog) {
        if (item.barcode == barcode) {
            outProduct = item;
            return true;
        }
    }
    return false;
}

bool MockDataService::findProductById(int id, ProductInfo& outProduct) {
    for (const auto& item : m_catalog) {
        if (item.id == id) {
            outProduct = item;
            return true;
        }
    }
    return false;
}

std::vector<ProductInfo> MockDataService::getAllProducts() {
    return m_catalog;
}

BillInfo MockDataService::generateBill(const std::string& customerName,
                                      const std::string& customerContact,
                                      const std::vector<OrderItem>& items) {
    BillInfo bill;
    std::ostringstream billStream;
    billStream << "BILL-2026-" << std::setw(4) << std::setfill('0') << m_billCounter++;
    bill.billId = billStream.str();

    std::ostringstream orderStream;
    orderStream << "ORD-" << m_orderCounter++;
    bill.orderId = orderStream.str();

    bill.customerName = customerName.empty() ? "Walk-in Customer" : customerName;
    bill.customerContact = customerContact;

    // Portable timestamp using std::chrono
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;
#if defined(_WIN32)
    localtime_s(&tm_buf, &now_c);
#else
    localtime_r(&now_c, &tm_buf);
#endif
    std::ostringstream timeStream;
    timeStream << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    bill.timestamp = timeStream.str();

    bill.items = items;

    double subtotalWithoutTax = 0.0;
    double totalDiscount = 0.0;
    double totalTax = 0.0;

    for (const auto& item : items) {
        subtotalWithoutTax += item.getSubtotal();
        totalDiscount += item.getTotalDiscount();
        totalTax += item.getTotalTax();
    }

    bill.totalWithoutTax = subtotalWithoutTax;
    bill.totalDiscount = totalDiscount;
    bill.totalTax = totalTax;
    bill.totalPayable = (subtotalWithoutTax - totalDiscount) + totalTax;
    if (bill.totalPayable < 0.0) bill.totalPayable = 0.0;

    bill.paymentStatus = "Pending";
    bill.paymentMode = "Cash";
    bill.paymentMerchant = "Main Cash Register #01";

    return bill;
}

bool MockDataService::isCameraAvailable() const {
    return true;
}

bool MockDataService::startCamera() {
    m_cameraActive = true;
    return true;
}

void MockDataService::stopCamera() {
    m_cameraActive = false;
}

bool MockDataService::isScanning() const {
    return m_cameraActive;
}

BarcodeScanResult MockDataService::captureAndScan() {
    BarcodeScanResult result;
    if (!m_cameraActive) {
        result.success = false;
        result.errorMessage = "Scanner module is currently inactive. Please start scanner first.";
        return result;
    }
    // Simulate detecting a sample product barcode
    result.success = true;
    result.barcode = "100001";
    return result;
}

bool MockDataService::saveBill(const BillInfo& bill) {
    for (auto& existing : m_savedOrders) {
        if (existing.orderId == bill.orderId) {
            existing = bill;
            return true;
        }
    }
    m_savedOrders.push_back(bill);
    return true;
}

std::vector<BillInfo> MockDataService::getAllOrders() {
    return m_savedOrders;
}

bool MockDataService::findOrderById(const std::string& orderId, BillInfo& outBill) {
    for (const auto& bill : m_savedOrders) {
        if (bill.orderId == orderId) {
            outBill = bill;
            return true;
        }
    }
    return false;
}
