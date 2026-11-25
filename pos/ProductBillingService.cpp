#include "ProductBillingService.h"
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>

// New billing system includes
#include "billing/BillIdGenerator.h"
#include "billing/Bill.h"
#include "billing/DiscountEngine.h"
#include "billing/PdfGenerator.h"

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

ProductInfo ProductBillingService::toProductInfo(const ProductBilling::Product& product) {
    ProductInfo info;
    info.id = extractNumericId(product.getProductId());
    info.barcode = product.getBarcode();
    info.acronym = product.getAcronym();
    info.name = product.getName();
    info.category = product.getCategory().empty() ? (product.getMeasurementUnit() == "weight" ? "Bulk & Staples" : "Packaged Goods") : product.getCategory();
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

    // ----- New unified billing flow -----
    // 1️⃣ Create our own billing::Bill and fill basic info
    billing::Bill myBill;
    myBill.billId = billInfo.billId;
    myBill.customerName = billInfo.customerName;
    myBill.orderId = billInfo.orderId;
    myBill.billTimestamp = std::time(nullptr);

    // 2️⃣ Populate line items from the POS order
    for (const auto& item : items) {
        std::string prodCode = item.product.acronym.empty() ? item.product.barcode : item.product.acronym;
        double unitPrice = item.product.unitPriceWithoutTax; // fallback to 0 if member missing
        double lineTotal = unitPrice * static_cast<double>(item.quantity);
        double discountAmt = item.getTotalDiscount();
        double discountPct = (lineTotal > 0.0) ? (discountAmt / lineTotal) * 100.0 : 0.0;
        billing::BillItem bi{prodCode, unitPrice, item.quantity, discountPct};
        myBill.items.push_back(bi);
    }

    // 3️⃣ Base totals (product‑level discounts & tax)
    myBill.computeTotals();

    // 4️⃣ Apply manager‑controlled special discount (currently 0 %)
    billing::DiscountEngine::applySpecialDiscount(myBill, 0.0);

    // 5️⃣ Apply seasonal discounts from Seasons.txt
    auto seasons = billing::DiscountEngine::loadSeasons();
    billing::DiscountEngine::applySeasonalDiscount(myBill, seasons);

    // 6️⃣ Re‑compute after discounts
    myBill.computeTotals();

    // 7️⃣ Persist bill records
    {
        std::ofstream out("Bill.txt", std::ios::app);
        if (out) out << myBill.serialize() << '\n';
    }
    {
        std::ofstream out("BillDetail.txt", std::ios::app);
        if (out) {
            for (const auto& bi : myBill.items) {
                out << myBill.billId << '|' << bi.productCode << '|' << bi.unitPriceWithoutTax << '|' << bi.quantity << '|' << bi.discountPercent << '\n';
            }
        }
    }

    // 8️⃣ Generate PDF receipt in an "output" folder
    std::filesystem::create_directory("output");
    std::string pdfPath = std::string("output/Bill_") + myBill.billId + ".pdf";
    billing::PdfGenerator::createPdf(myBill, QString::fromStdString(pdfPath));

    // 9️⃣ Populate BillInfo for the existing UI flow
    billInfo.totalWithoutTax = myBill.totalWithoutTax;
    billInfo.totalDiscount   = myBill.totalProductDiscounts + myBill.seasonalDiscount + myBill.specialDiscount;
    billInfo.totalTax        = myBill.totalTax;
    billInfo.totalPayable    = myBill.totalPayable;
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
        primaryPath = "product_module/Product.txt";
    }

    std::string status;
    if (!m_productManager.saveToFile(primaryPath, &status)) {
        if (errorMsg) *errorMsg = status;
        return false;
    }

    // Also sync to other known relative locations if they exist and are distinct
    const char* secondaryPaths[] = {
        "product_module/Product.txt",
        "../../product_module/Product.txt",
        "../product_module/Product.txt",
        "Product.txt"
    };

    for (const char* secPath : secondaryPaths) {
        if (primaryPath != secPath) {
            std::ifstream test(secPath);
            if (test.good()) {
                test.close();
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

    return true;
}
