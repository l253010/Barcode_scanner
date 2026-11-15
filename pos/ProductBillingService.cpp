#include "ProductBillingService.h"
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>

ProductBillingService::ProductBillingService() {
    loadCatalog();
}

ProductBillingService::ProductBillingService(const std::string& catalogFilePath) {
    loadCatalog(catalogFilePath);
}

bool ProductBillingService::loadCatalog(const std::string& filePath) {
    std::vector<std::string> candidatePaths;
    if (!filePath.empty()) {
        candidatePaths.push_back(filePath);
    }
    candidatePaths.push_back("product_module/Product.txt");
    candidatePaths.push_back("../product_module/Product.txt");
    candidatePaths.push_back("../../product_module/Product.txt");
    candidatePaths.push_back("Product.txt");
    candidatePaths.push_back("../Product.txt");

    bool loaded = false;
    for (const auto& path : candidatePaths) {
        std::ifstream testFile(path);
        if (testFile.good()) {
            testFile.close();
            std::string status;
            if (m_productManager.loadFromFile(path, &status)) {
                loaded = true;
                break;
            }
        }
    }

    if (!loaded || m_productManager.getProductCount() == 0) {
        initFallbackCatalog();
        return false;
    }

    return true;
}

void ProductBillingService::initFallbackCatalog() {
    m_productManager.clear();
    // Default supermarket items ensuring POS always operates
    m_productManager.addOrUpdateProduct(ProductBilling::Product(
        "100001", "PRD-101", "Organic Whole Milk 1L", "Fresh pasteurized whole dairy milk", "Dairy", 3.49, 50, 5.0));
    m_productManager.addOrUpdateProduct(ProductBilling::Product(
        "100002", "PRD-102", "Whole Grain Wheat Bread", "Nutritious sliced whole wheat bread", "Bakery", 2.89, 40, 0.0));
    m_productManager.addOrUpdateProduct(ProductBilling::Product(
        "100003", "PRD-103", "Farm Fresh Large Eggs 12pk", "Grade A brown free-range eggs", "Dairy", 4.25, 30, 5.0));
    m_productManager.addOrUpdateProduct(ProductBilling::Product(
        "123456", "PRD-104", "Arabica Dark Roast Coffee 250g", "Premium ground dark roast coffee beans", "Beverages", 7.99, 25, 8.0));
    m_productManager.addOrUpdateProduct(ProductBilling::Product(
        "200001", "PRD-105", "Natural Spring Water 500ml", "Pure bottled mineral spring water", "Beverages", 1.20, 100, 0.0));
    m_productManager.addOrUpdateProduct(ProductBilling::Product(
        "200002", "PRD-106", "Crunchy Peanut Butter 500g", "All-natural roasted peanut butter", "Pantry", 4.50, 20, 5.0));
    m_productManager.addOrUpdateProduct(ProductBilling::Product(
        "300001", "PRD-107", "Dark Chocolate Bar 100g", "70 percent cocoa rich dark chocolate", "Confectionery", 2.50, 60, 8.0));
    m_productManager.addOrUpdateProduct(ProductBilling::Product(
        "300002", "PRD-108", "Extra Virgin Olive Oil 750ml", "Cold pressed Mediterranean olive oil", "Pantry", 11.99, 15, 5.0));
    m_productManager.addOrUpdateProduct(ProductBilling::Product(
        "400001", "PRD-109", "Basmati Long Grain Rice 2kg", "Aromatic Himalayan aged basmati rice", "Pantry", 8.75, 35, 0.0));
    m_productManager.addOrUpdateProduct(ProductBilling::Product(
        "500001", "PRD-110", "Sparkling Lemonade 330ml", "Refreshing carbonated real lemon beverage", "Beverages", 1.75, 8, 8.0));
}

int ProductBillingService::extractNumericId(const std::string& productIdStr) {
    int id = 0;
    for (char c : productIdStr) {
        if (c >= '0' && c <= '9') {
            id = id * 10 + (c - '0');
        }
    }
    return (id > 0) ? id : 100;
}

ProductInfo ProductBillingService::toProductInfo(const ProductBilling::Product& product) {
    ProductInfo info;
    info.id = extractNumericId(product.getProductId());
    info.barcode = product.getBarcode();
    info.acronym = product.getProductId();
    info.name = product.getName();
    info.category = product.getCategory();

    // Map units based on category
    if (info.category == "Dairy" || info.category == "Beverages") {
        info.unit = "Bottle";
    } else if (info.category == "Bakery") {
        info.unit = "Loaf";
    } else if (info.category == "Pantry") {
        info.unit = "Pack";
    } else {
        info.unit = "Unit";
    }

    info.unitPrice = product.getPrice();
    info.taxRate = product.getTaxRateFraction(); // e.g. 0.05 for 5%
    info.discountPerUnit = 0.0;
    info.availableQuantity = product.getAvailableStock();
    return info;
}

ProductBilling::Product ProductBillingService::toProduct(const ProductInfo& info) {
    std::string pid = info.acronym.empty() ? ("PRD-" + std::to_string(info.id)) : info.acronym;
    return ProductBilling::Product(
        info.barcode,
        pid,
        info.name,
        info.name,
        info.category,
        info.unitPrice,
        info.availableQuantity,
        info.taxRate * 100.0 // percentage e.g. 5.0
    );
}

bool ProductBillingService::findProductByBarcode(const std::string& barcode, ProductInfo& outProduct) {
    const ProductBilling::Product* p = m_productManager.findProductByBarcode(barcode);
    if (p) {
        outProduct = toProductInfo(*p);
        return true;
    }
    return false;
}

bool ProductBillingService::findProductById(int id, ProductInfo& outProduct) {
    ProductBilling::DynamicArray<ProductBilling::Product> all = m_productManager.getAllProducts();
    for (int i = 0; i < all.size(); ++i) {
        if (extractNumericId(all[i].getProductId()) == id) {
            outProduct = toProductInfo(all[i]);
            return true;
        }
    }
    return false;
}

std::vector<ProductInfo> ProductBillingService::getAllProducts() {
    std::vector<ProductInfo> result;
    ProductBilling::DynamicArray<ProductBilling::Product> all = m_productManager.getAllProducts();
    result.reserve(all.size());
    for (int i = 0; i < all.size(); ++i) {
        result.push_back(toProductInfo(all[i]));
    }
    return result;
}

BillInfo ProductBillingService::generateBill(const std::string& customerName,
                                            const std::string& customerContact,
                                            const std::vector<OrderItem>& items) {
    BillInfo billInfo;
    std::ostringstream billStream;
    billStream << "BILL-2026-" << std::setw(4) << std::setfill('0') << m_billCounter++;
    billInfo.billId = billStream.str();

    std::ostringstream orderStream;
    orderStream << "ORD-" << m_orderCounter++;
    billInfo.orderId = orderStream.str();

    billInfo.customerName = customerName.empty() ? "Walk-in Customer" : customerName;
    billInfo.customerContact = customerContact.empty() ? "N/A" : customerContact;

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
    billInfo.timestamp = timeStream.str();

    billInfo.items = items;

    // Use pure ProductBilling::Bill engine to calculate financial totals
    ProductBilling::Bill bill(billInfo.billId);
    for (const auto& item : items) {
        const ProductBilling::Product* catalogProduct = m_productManager.findProductByBarcode(item.product.barcode);
        ProductBilling::Product itemProduct = catalogProduct ? *catalogProduct : toProduct(item.product);
        double itemDiscount = item.getTotalDiscount();
        bill.addItem(itemProduct, item.quantity, itemDiscount);
    }

    billInfo.totalWithoutTax = bill.calculateSubtotal();
    billInfo.totalDiscount   = bill.calculateTotalDiscount();
    billInfo.totalTax        = bill.calculateTotalTax();
    billInfo.totalPayable    = bill.calculateFinalPayable();
    if (billInfo.totalPayable < 0.0) {
        billInfo.totalPayable = 0.0;
    }

    billInfo.paymentStatus = "Pending";
    billInfo.paymentMode = "Cash";
    billInfo.paymentMerchant = "Cash Register Drawer #01";

    return billInfo;
}

bool ProductBillingService::commitOrderStock(const BillInfo& bill, std::string* errorMsg) {
    for (const auto& item : bill.items) {
        if (!m_productManager.reduceStock(item.product.barcode, item.quantity, errorMsg)) {
            return false;
        }
    }
    return true;
}
