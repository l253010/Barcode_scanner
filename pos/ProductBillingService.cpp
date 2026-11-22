#include "ProductBillingService.h"
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <filesystem>

#if defined(QT_CORE_LIB) || defined(QT_WIDGETS_LIB)
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#endif

ProductBillingService::ProductBillingService() {
    m_productManager.setCategoryManager(&m_categoryManager);
    loadCategories();
    loadCatalog();
}

ProductBillingService::ProductBillingService(const std::string& catalogFilePath) {
    m_productManager.setCategoryManager(&m_categoryManager);
    loadCategories();
    loadCatalog(catalogFilePath);
}

std::string ProductBillingService::resolveProductFilePath(const std::string& explicitPath) {
    std::vector<std::string> candidates;

    // 1. Explicit path if specified
    if (!explicitPath.empty()) {
        candidates.push_back(explicitPath);
    }

    // 2. Discover relative to Qt Application Executable Directory
#if defined(QT_CORE_LIB) || defined(QT_WIDGETS_LIB)
    if (QCoreApplication::instance()) {
        QString appDir = QCoreApplication::applicationDirPath();
        candidates.push_back(QDir(appDir).filePath("product_module/Product.txt").toStdString());
        candidates.push_back(QDir(appDir).filePath("../product_module/Product.txt").toStdString());
        candidates.push_back(QDir(appDir).filePath("../../product_module/Product.txt").toStdString());
        candidates.push_back(QDir(appDir).filePath("Product.txt").toStdString());
        candidates.push_back(QDir(appDir).filePath("../Product.txt").toStdString());
    }
#endif

    // 3. Discover relative to Current Working Directory
    candidates.push_back("product_module/Product.txt");
    candidates.push_back("../product_module/Product.txt");
    candidates.push_back("../../product_module/Product.txt");
    candidates.push_back("Product.txt");
    candidates.push_back("../Product.txt");

    // First, find an existing Product.txt file and resolve to canonical absolute path
    for (const auto& cand : candidates) {
        std::error_code ec;
        if (std::filesystem::exists(cand, ec) && !std::filesystem::is_directory(cand, ec)) {
            auto can = std::filesystem::canonical(cand, ec);
            if (!ec) {
                return can.string();
            }
            auto abs = std::filesystem::absolute(cand, ec);
            if (!ec) {
                return abs.string();
            }
            return cand;
        }
    }

    // Fallback if no file exists yet: locate parent repository directory to target product_module/Product.txt
#if defined(QT_CORE_LIB) || defined(QT_WIDGETS_LIB)
    if (QCoreApplication::instance()) {
        QString appDir = QCoreApplication::applicationDirPath();
        QDir parentDir(appDir);
        parentDir.cdUp();
        if (parentDir.exists("product_module")) {
            return parentDir.filePath("product_module/Product.txt").toStdString();
        }
    }
#endif

    return "product_module/Product.txt";
}

std::string ProductBillingService::resolveCategoryFilePath(const std::string& explicitPath) {
    std::vector<std::string> candidates;

    if (!explicitPath.empty()) {
        candidates.push_back(explicitPath);
    }

#if defined(QT_CORE_LIB) || defined(QT_WIDGETS_LIB)
    if (QCoreApplication::instance()) {
        QString appDir = QCoreApplication::applicationDirPath();
        candidates.push_back(QDir(appDir).filePath("product_module/PCategory.txt").toStdString());
        candidates.push_back(QDir(appDir).filePath("../product_module/PCategory.txt").toStdString());
        candidates.push_back(QDir(appDir).filePath("../../product_module/PCategory.txt").toStdString());
        candidates.push_back(QDir(appDir).filePath("PCategory.txt").toStdString());
        candidates.push_back(QDir(appDir).filePath("../PCategory.txt").toStdString());
    }
#endif

    candidates.push_back("product_module/PCategory.txt");
    candidates.push_back("../product_module/PCategory.txt");
    candidates.push_back("../../product_module/PCategory.txt");
    candidates.push_back("PCategory.txt");
    candidates.push_back("../PCategory.txt");

    for (const auto& cand : candidates) {
        std::error_code ec;
        if (std::filesystem::exists(cand, ec) && !std::filesystem::is_directory(cand, ec)) {
            auto can = std::filesystem::canonical(cand, ec);
            if (!ec) {
                return can.string();
            }
            auto abs = std::filesystem::absolute(cand, ec);
            if (!ec) {
                return abs.string();
            }
            return cand;
        }
    }

#if defined(QT_CORE_LIB) || defined(QT_WIDGETS_LIB)
    if (QCoreApplication::instance()) {
        QString appDir = QCoreApplication::applicationDirPath();
        QDir parentDir(appDir);
        parentDir.cdUp();
        if (parentDir.exists("product_module")) {
            return parentDir.filePath("product_module/PCategory.txt").toStdString();
        }
    }
#endif

    return "product_module/PCategory.txt";
}

bool ProductBillingService::loadCategories(const std::string& filePath) {
    std::string canonicalPath = resolveCategoryFilePath(filePath);
    std::string status;
    bool loaded = m_categoryManager.loadFromFile(canonicalPath, &status);

    if (!loaded || m_categoryManager.getCategoryCount() == 0) {
        initFallbackCategories();
        return false;
    }

    return true;
}

void ProductBillingService::initFallbackCategories() {
    m_categoryManager.clear();
    m_categoryManager.addCategory(ProductBilling::Category("10", "Dairy & Bakery", "DAIRY"));
    m_categoryManager.addCategory(ProductBilling::Category("12", "Coffee & Beverages", "COF"));
    m_categoryManager.addCategory(ProductBilling::Category("20", "Beverages & Spreads", "BEV"));
    m_categoryManager.addCategory(ProductBilling::Category("30", "Confectionery & Oils", "CONF"));
    m_categoryManager.addCategory(ProductBilling::Category("40", "Grains & Staples", "GRAIN"));
    m_categoryManager.addCategory(ProductBilling::Category("50", "Soft Drinks & Juices", "DRINK"));
}

bool ProductBillingService::saveCategories(std::string* errorMsg) {
    std::string primaryPath = m_categoryManager.getLastLoadedFilePath();
    if (primaryPath.empty()) {
        primaryPath = resolveCategoryFilePath();
    }

    std::string status;
    if (!m_categoryManager.saveToFile(primaryPath, &status)) {
        if (errorMsg) *errorMsg = status;
        return false;
    }

    std::vector<std::string> secondaryCandidates = {
        "PCategory.txt",
        "../PCategory.txt",
        "product_module/PCategory.txt",
        "../product_module/PCategory.txt"
    };

    std::error_code ecPrim;
    auto primCan = std::filesystem::canonical(primaryPath, ecPrim);

    for (const auto& secPath : secondaryCandidates) {
        std::error_code ecSec;
        if (std::filesystem::exists(secPath, ecSec) && !std::filesystem::is_directory(secPath, ecSec)) {
            auto secCan = std::filesystem::canonical(secPath, ecSec);
            if (!ecPrim && !ecSec && primCan != secCan) {
                m_categoryManager.saveToFile(secPath, nullptr);
            }
        }
    }

    if (errorMsg) *errorMsg = status;
    return true;
}

std::vector<CategoryInfo> ProductBillingService::getAllCategories() {
    std::vector<CategoryInfo> result;
    auto all = m_categoryManager.getAllCategories();
    result.reserve(all.size());
    for (size_t i = 0; i < all.size(); ++i) {
        result.push_back({all[i].id, all[i].name, all[i].acronym});
    }
    return result;
}

bool ProductBillingService::addCategory(const CategoryInfo& category, std::string* errorMsg) {
    ProductBilling::Category c(category.id, category.name, category.acronym);
    if (!m_categoryManager.addCategory(c, errorMsg)) {
        return false;
    }
    return saveCategories(errorMsg);
}

bool ProductBillingService::updateCategory(const CategoryInfo& category, std::string* errorMsg) {
    ProductBilling::Category c(category.id, category.name, category.acronym);
    if (!m_categoryManager.updateCategory(c, errorMsg)) {
        return false;
    }
    return saveCategories(errorMsg);
}

bool ProductBillingService::findCategoryById(const std::string& id, CategoryInfo& outCategory) {
    const ProductBilling::Category* c = m_categoryManager.findCategoryById(id);
    if (c) {
        outCategory.id = c->id;
        outCategory.name = c->name;
        outCategory.acronym = c->acronym;
        return true;
    }
    return false;
}

std::string ProductBillingService::resolveCategoryId(const ProductInfo& product) const {
    if (!product.categoryId.empty()) {
        return product.categoryId;
    }

    if (product.category.length() == 2 &&
        product.category[0] >= '0' && product.category[0] <= '9' &&
        product.category[1] >= '0' && product.category[1] <= '9') {
        return product.category;
    }

    if (product.category.length() >= 4 && product.category[0] == '[') {
        size_t closeBracket = product.category.find(']');
        if (closeBracket != std::string::npos && closeBracket > 1) {
            std::string inside = product.category.substr(1, closeBracket - 1);
            if (inside.length() == 2 && inside[0] >= '0' && inside[0] <= '9' && inside[1] >= '0' && inside[1] <= '9') {
                return inside;
            }
        }
    }

    if (!product.category.empty()) {
        auto allCats = m_categoryManager.getAllCategories();
        for (size_t i = 0; i < allCats.size(); ++i) {
            if (allCats[i].name == product.category || allCats[i].acronym == product.category) {
                return allCats[i].id;
            }
        }
        return "";
    }

    if (product.categoryId.empty() && product.category.empty()) {
        if (product.barcode.length() >= 2 &&
            product.barcode[0] >= '0' && product.barcode[0] <= '9' &&
            product.barcode[1] >= '0' && product.barcode[1] <= '9') {
            return product.barcode.substr(0, 2);
        }
    }

    return "";
}

bool ProductBillingService::validateBarcodeCategory(const ProductInfo& product, std::string* errorMsg) const {
    // 1. Check that the barcode contains at least two characters
    if (product.barcode.length() < 2) {
        if (errorMsg) *errorMsg = "Invalid barcode: Barcode must contain at least 2 characters.";
        return false;
    }

    // 2. Check that its first two characters are numeric digits
    if (product.barcode[0] < '0' || product.barcode[0] > '9' ||
        product.barcode[1] < '0' || product.barcode[1] > '9') {
        if (errorMsg) *errorMsg = "Invalid barcode: The first two characters must be numeric digits.";
        return false;
    }

    // 3. Resolve the selected Category ID
    std::string selectedCatId = resolveCategoryId(product);
    if (selectedCatId.empty()) {
        if (errorMsg) *errorMsg = "Invalid product: A valid Category ID must be selected or specified.";
        return false;
    }

    // 4. Verify that the selected Category ID exists in PCategory.txt
    if (!m_categoryManager.categoryExists(selectedCatId)) {
        if (errorMsg) {
            *errorMsg = "Invalid Category ID: Category ID '" + selectedCatId +
                        "' does not exist in PCategory.txt.";
        }
        return false;
    }

    // 5. Compare those exact two digits with the selected Category ID
    std::string barcodePrefix = product.barcode.substr(0, 2);
    if (barcodePrefix != selectedCatId) {
        if (errorMsg) {
            *errorMsg = "Invalid barcode: The first two digits must match the selected Category ID (" +
                        selectedCatId + ").";
        }
        return false;
    }

    // 6. Full 6-digit Code 39 format validation
    if (product.barcode.length() != 6) {
        if (errorMsg) *errorMsg = "Invalid barcode: Barcode must be exactly 6 digits.";
        return false;
    }
    for (char c : product.barcode) {
        if (c < '0' || c > '9') {
            if (errorMsg) *errorMsg = "Invalid barcode: Barcode must consist of numeric digits only.";
            return false;
        }
    }
    if (product.barcode[0] == '0') {
        if (errorMsg) *errorMsg = "Invalid barcode: Barcode must start with a non-zero digit (1-9).";
        return false;
    }

    // 7. Validate other required product fields
    if (product.name.empty()) {
        if (errorMsg) *errorMsg = "Product name cannot be empty.";
        return false;
    }
    if (product.acronym.empty() || product.acronym.length() > 8) {
        if (errorMsg) *errorMsg = "Product acronym must be between 1 and 8 characters.";
        return false;
    }
    if (product.unitPrice < 0.0) {
        if (errorMsg) *errorMsg = "Product price cannot be negative.";
        return false;
    }
    if (product.discountPerUnit < 0.0) {
        if (errorMsg) *errorMsg = "Discount cannot be negative.";
        return false;
    }
    if (product.discountPerUnit > product.unitPrice) {
        if (errorMsg) *errorMsg = "Discount cannot exceed unit price.";
        return false;
    }
    if (product.availableQuantity < 0) {
        if (errorMsg) *errorMsg = "Stock quantity cannot be negative.";
        return false;
    }

    return true;
}

bool ProductBillingService::addProduct(const ProductInfo& product, std::string* errorMsg) {
    if (!validateBarcodeCategory(product, errorMsg)) {
        return false;
    }

    std::string catId = product.barcode.substr(0, 2);
    ProductInfo toAdd = product;
    toAdd.categoryId = catId;
    const ProductBilling::Category* catObj = m_categoryManager.findCategoryById(catId);
    if (catObj && (toAdd.category.empty() || toAdd.category == catId)) {
        toAdd.category = catObj->name;
    }

    ProductBilling::Product p = toProduct(toAdd);
    if (!m_productManager.addProduct(p, errorMsg)) {
        return false;
    }

    if (!saveCatalog(errorMsg)) {
        m_productManager.removeProduct(product.barcode);
        return false;
    }

    return true;
}

bool ProductBillingService::updateProduct(const ProductInfo& product, std::string* errorMsg) {
    if (!validateBarcodeCategory(product, errorMsg)) {
        return false;
    }

    std::string catId = product.barcode.substr(0, 2);
    const ProductBilling::Product* existing = m_productManager.findProductByBarcode(product.barcode);
    if (!existing && product.id > 0) {
        ProductInfo info;
        if (findProductById(product.id, info)) {
            existing = m_productManager.findProductByBarcode(info.barcode);
        }
    }
    if (!existing) {
        if (errorMsg) *errorMsg = "Product with barcode '" + product.barcode + "' not found.";
        return false;
    }
    std::string oldBarcode = existing->getBarcode();
    ProductBilling::Product backup = *existing;

    ProductInfo toUpdate = product;
    toUpdate.categoryId = catId;
    const ProductBilling::Category* catObj = m_categoryManager.findCategoryById(catId);
    if (catObj && (toUpdate.category.empty() || toUpdate.category == catId)) {
        toUpdate.category = catObj->name;
    }

    ProductBilling::Product p = toProduct(toUpdate);
    if (oldBarcode != product.barcode) {
        m_productManager.removeProduct(oldBarcode);
        if (!m_productManager.addProduct(p, errorMsg)) {
            m_productManager.addProduct(backup, nullptr);
            return false;
        }
    } else {
        if (!m_productManager.updateProduct(p, errorMsg)) {
            return false;
        }
    }

    if (!saveCatalog(errorMsg)) {
        if (oldBarcode != product.barcode) {
            m_productManager.removeProduct(product.barcode);
            m_productManager.addProduct(backup, nullptr);
        } else {
            m_productManager.updateProduct(backup, nullptr);
        }
        return false;
    }

    return true;
}

bool ProductBillingService::loadCatalog(const std::string& filePath) {
    std::string canonicalPath = resolveProductFilePath(filePath);
    std::string status;
    bool loaded = m_productManager.loadFromFile(canonicalPath, &status);

    if (!loaded || m_productManager.getProductCount() == 0) {
        initFallbackCatalog();
        return false;
    }

    return true;
}

void ProductBillingService::initFallbackCatalog() {
    m_productManager.clear();
    // Default supermarket items conforming to the 10-field space-delimited specification
    m_productManager.addOrUpdateProduct(ProductBilling::Product(
        "PRD-101", "100001", "MILK-1L", "Organic Whole Milk 1L", "count", 3.49, 0.175, 0.00, 50, "barcodes/barcode_100001.png"));
    m_productManager.addOrUpdateProduct(ProductBilling::Product(
        "PRD-102", "100002", "BREAD-W", "Whole Grain Wheat Bread", "count", 2.89, 0.00, 0.00, 40, "barcodes/barcode_100002.png"));
    m_productManager.addOrUpdateProduct(ProductBilling::Product(
        "PRD-103", "100003", "EGGS-12", "Farm Fresh Large Eggs 12pk", "count", 4.25, 0.2125, 0.00, 30, "barcodes/barcode_100003.png"));
    m_productManager.addOrUpdateProduct(ProductBilling::Product(
        "PRD-104", "123456", "COF-250", "Arabica Dark Roast Coffee 250g", "weight", 7.99, 0.6392, 0.00, 25, "barcodes/barcode_123456.png"));
    m_productManager.addOrUpdateProduct(ProductBilling::Product(
        "PRD-105", "200001", "WAT-500", "Natural Spring Water 500ml", "count", 1.20, 0.00, 0.00, 100, "barcodes/barcode_200001.png"));
    m_productManager.addOrUpdateProduct(ProductBilling::Product(
        "PRD-106", "200002", "PB-500G", "Crunchy Peanut Butter 500g", "count", 4.50, 0.225, 0.00, 20, "barcodes/barcode_200002.png"));
    m_productManager.addOrUpdateProduct(ProductBilling::Product(
        "PRD-107", "300001", "CHOC-100", "Dark Chocolate Bar 100g", "count", 2.50, 0.20, 0.00, 60, "barcodes/barcode_300001.png"));
    m_productManager.addOrUpdateProduct(ProductBilling::Product(
        "PRD-108", "300002", "OIL-750", "Extra Virgin Olive Oil 750ml", "count", 11.99, 0.5995, 0.00, 15, "barcodes/barcode_300002.png"));
    m_productManager.addOrUpdateProduct(ProductBilling::Product(
        "PRD-109", "400001", "RICE-2K", "Basmati Long Grain Rice 2kg", "weight", 8.75, 0.00, 0.00, 35, "barcodes/barcode_400001.png"));
    m_productManager.addOrUpdateProduct(ProductBilling::Product(
        "PRD-110", "500001", "LEMON-33", "Sparkling Lemonade 330ml", "count", 1.75, 0.14, 0.00, 8, "barcodes/barcode_500001.png"));
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

ProductInfo ProductBillingService::toProductInfo(const ProductBilling::Product& product) const {
    ProductInfo info;
    info.id = extractNumericId(product.getProductId());
    info.barcode = product.getBarcode();
    info.acronym = product.getAcronym();
    info.name = product.getName();
    std::string catId = product.getCategoryId();
    info.categoryId = catId;
    const ProductBilling::Category* catObj = m_categoryManager.findCategoryById(catId);
    if (catObj) {
        info.category = catObj->name;
    } else if (!product.getCategory().empty()) {
        info.category = product.getCategory();
    } else {
        info.category = (product.getMeasurementUnit() == "weight" ? "Bulk & Staples" : "Packaged Goods");
    }
    info.unit = product.getMeasurementUnit();
    info.unitPrice = product.getPrice();
    info.taxRate = product.getTaxRateFraction(); // e.g. 0.05 for 5%
    info.discountPerUnit = product.getDiscountPerUnit();
    info.availableQuantity = product.getAvailableStock();
    return info;
}

ProductBilling::Product ProductBillingService::toProduct(const ProductInfo& info) {
    std::string pid = info.acronym.empty() ? ("PRD-" + std::to_string(info.id)) : info.acronym;
    std::string path = "barcodes/barcode_" + info.barcode + ".png";
    return ProductBilling::Product(
        pid,
        info.barcode,
        info.acronym,
        info.name,
        info.unit.empty() ? "count" : info.unit,
        info.unitPrice,
        info.unitPrice * info.taxRate,
        info.discountPerUnit,
        info.availableQuantity,
        path,
        info.category,
        info.name
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

bool ProductBillingService::saveCatalog(std::string* errorMsg) {
    std::string primaryPath = m_productManager.getLastLoadedFilePath();
    if (primaryPath.empty()) {
        primaryPath = resolveProductFilePath();
    }

    std::string status;
    if (!m_productManager.saveToFile(primaryPath, &status)) {
        if (errorMsg) *errorMsg = status;
        return false;
    }

    // Also sync secondary copies if they exist and are distinct files
    std::vector<std::string> secondaryCandidates;
#if defined(QT_CORE_LIB) || defined(QT_WIDGETS_LIB)
    if (QCoreApplication::instance()) {
        QString appDir = QCoreApplication::applicationDirPath();
        secondaryCandidates.push_back(QDir(appDir).filePath("Product.txt").toStdString());
        secondaryCandidates.push_back(QDir(appDir).filePath("../Product.txt").toStdString());
    }
#endif
    secondaryCandidates.push_back("Product.txt");
    secondaryCandidates.push_back("product_module/Product.txt");

    std::error_code ecPrim;
    auto primCan = std::filesystem::canonical(primaryPath, ecPrim);

    for (const auto& secPath : secondaryCandidates) {
        std::error_code ecSec;
        if (std::filesystem::exists(secPath, ecSec) && !std::filesystem::is_directory(secPath, ecSec)) {
            auto secCan = std::filesystem::canonical(secPath, ecSec);
            if (!ecPrim && !ecSec && primCan != secCan) {
                // Secondary file exists and is distinct from primary; sync to keep working trees aligned
                m_productManager.saveToFile(secPath, nullptr);
            }
        }
    }

    if (errorMsg) *errorMsg = status;
    return true;
}

bool ProductBillingService::updateProductStock(const std::string& barcode, int newStock, std::string* errorMsg) {
    if (newStock < 0) {
        if (errorMsg) *errorMsg = "Stock quantity cannot be negative.";
        return false;
    }

    const ProductBilling::Product* prod = m_productManager.findProductByBarcode(barcode);
    if (!prod) {
        if (errorMsg) *errorMsg = "Product with barcode '" + barcode + "' not found in catalog.";
        return false;
    }

    int oldStock = prod->getAvailableStock();
    if (oldStock == newStock) {
        return true;
    }

    // 1. Update in-memory stock
    if (!m_productManager.setStock(barcode, newStock, errorMsg)) {
        return false;
    }

    // 2. Persist to disk
    std::string saveErr;
    if (!saveCatalog(&saveErr)) {
        // Rollback in memory to keep RAM and disk consistent
        m_productManager.setStock(barcode, oldStock);
        if (errorMsg) {
            *errorMsg = "Failed to persist stock update to disk: " + saveErr + " (In-memory update rolled back).";
        }
        return false;
    }

    return true;
}

bool ProductBillingService::commitOrderStock(const BillInfo& bill, std::string* errorMsg) {
    if (bill.items.empty()) {
        return true;
    }

    // Prevent double stock deduction if this bill is already marked Paid or already committed
    if (bill.paymentStatus == "Paid") {
        return true;
    }
    if (!bill.billId.empty() && m_committedOrderIds.find(bill.billId) != m_committedOrderIds.end()) {
        return true;
    }
    if (!bill.orderId.empty() && m_committedOrderIds.find(bill.orderId) != m_committedOrderIds.end()) {
        return true;
    }

    // 1. Pre-validate stock availability for all items in the bill
    for (const auto& item : bill.items) {
        const ProductBilling::Product* prod = m_productManager.findProductByBarcode(item.product.barcode);
        if (!prod) {
            if (errorMsg) *errorMsg = "Product with barcode '" + item.product.barcode + "' not found in catalog.";
            return false;
        }
        if (!prod->hasSufficientStock(item.quantity)) {
            if (errorMsg) {
                *errorMsg = "Insufficient stock for '" + prod->getName() +
                            "'. Requested: " + std::to_string(item.quantity) +
                            ", Available: " + std::to_string(prod->getAvailableStock()) + ".";
            }
            return false;
        }
    }

    // 2. Record previous stock states for atomic rollback if disk persistence fails
    struct RollbackItem {
        std::string barcode;
        int previousStock;
    };
    std::vector<RollbackItem> history;
    history.reserve(bill.items.size());

    for (const auto& item : bill.items) {
        const ProductBilling::Product* prod = m_productManager.findProductByBarcode(item.product.barcode);
        if (prod) {
            history.push_back({item.product.barcode, prod->getAvailableStock()});
        }
    }

    // 3. Deduct stock in memory
    for (const auto& item : bill.items) {
        if (!m_productManager.reduceStock(item.product.barcode, item.quantity, errorMsg)) {
            // Rollback in-memory state
            for (const auto& entry : history) {
                m_productManager.setStock(entry.barcode, entry.previousStock);
            }
            return false;
        }
    }

    // 4. Persist updated inventory to Product.txt
    std::string saveErr;
    if (!saveCatalog(&saveErr)) {
        // Rollback in-memory stock to prevent state inconsistency
        for (const auto& entry : history) {
            m_productManager.setStock(entry.barcode, entry.previousStock);
        }
        if (errorMsg) {
            *errorMsg = "Persistence failed: " + saveErr + " (In-memory stock was rolled back).";
        }
        return false;
    }

    if (!bill.billId.empty()) {
        m_committedOrderIds.insert(bill.billId);
    }
    if (!bill.orderId.empty()) {
        m_committedOrderIds.insert(bill.orderId);
    }

    return true;
}
