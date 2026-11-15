// ProductManager.cpp
// ----------------------------------------------------------------------------
// Implementation of the ProductManager class.
// Free of <vector>, <map>, <sstream>, <cassert>, and <cctype>.
// ----------------------------------------------------------------------------

#include "ProductManager.h"
#include <fstream>

namespace ProductBilling {

namespace {
    // Manual character whitespace check without <cctype>
    bool isWhitespaceChar(char c) {
        return c == ' ' || c == '\t' || c == '\r' || c == '\n';
    }

    // Manual string trimming without <cctype>
    std::string trim(const std::string& str) {
        if (str.empty()) return "";
        size_t first = 0;
        while (first < str.length() && isWhitespaceChar(str[first])) {
            ++first;
        }
        if (first == str.length()) return "";

        size_t last = str.length() - 1;
        while (last > first && isWhitespaceChar(str[last])) {
            --last;
        }
        return str.substr(first, (last - first + 1));
    }
}

ProductManager::ProductManager()
    : m_lastLoadedFilePath("")
{
}

int ProductManager::findIndexByBarcode(const std::string& barcode) const {
    std::string cleanBarcode = trim(barcode);
    if (cleanBarcode.empty()) return -1;
    for (size_t i = 0; i < m_products.size(); ++i) {
        if (m_products[i].getBarcode() == cleanBarcode) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool ProductManager::loadFromFile(const std::string& filePath, std::string* statusMessage) {
    std::string cleanPath = trim(filePath);
    if (cleanPath.empty()) {
        if (statusMessage) *statusMessage = "Error: File path cannot be empty.";
        return false;
    }

    std::ifstream inFile(cleanPath);
    if (!inFile.is_open()) {
        if (statusMessage) {
            *statusMessage = "Error: Unable to open file at path: " + cleanPath;
        }
        return false;
    }

    m_products.clear();
    m_lastLoadedFilePath = cleanPath;

    std::string line;
    int lineNumber = 0;
    int loadedCount = 0;
    int skippedCount = 0;

    while (std::getline(inFile, line)) {
        lineNumber++;
        std::string trimmedLine = trim(line);

        // Skip empty lines
        if (trimmedLine.empty()) {
            continue;
        }

        // Skip comments that start with '#' without '#' end marker, or start with '//'
        if ((trimmedLine[0] == '#' && trimmedLine.back() != '#') || trimmedLine.rfind("//", 0) == 0) {
            continue;
        }

        std::string parseError;
        Product p = Product::fromDelimitedString(trimmedLine, ' ', &parseError);
        if (p.isValid()) {
            int existingIdx = findIndexByBarcode(p.getBarcode());
            if (existingIdx >= 0) {
                m_products[existingIdx] = p;
            } else {
                m_products.push_back(p);
            }
            loadedCount++;
        } else {
            skippedCount++;
        }
    }

    inFile.close();

    if (statusMessage) {
        std::string msg = "Loaded " + std::to_string(loadedCount) + " product(s)";
        if (skippedCount > 0) {
            msg += " (" + std::to_string(skippedCount) + " invalid line(s) skipped)";
        }
        msg += " from '" + cleanPath + "'.";
        *statusMessage = msg;
    }

    return (loadedCount > 0);
}

bool ProductManager::saveToFile(const std::string& filePath, std::string* statusMessage) const {
    std::string targetPath = trim(filePath);
    if (targetPath.empty()) {
        targetPath = m_lastLoadedFilePath;
    }

    if (targetPath.empty()) {
        if (statusMessage) *statusMessage = "Error: Destination file path is not specified.";
        return false;
    }

    std::ofstream outFile(targetPath);
    if (!outFile.is_open()) {
        if (statusMessage) *statusMessage = "Error: Failed to open file for writing: " + targetPath;
        return false;
    }

    // Write products in the required specification format (one product per row, ending with #)
    for (size_t i = 0; i < m_products.size(); ++i) {
        outFile << m_products[i].toFileRecord() << "\n";
    }

    outFile.close();

    if (statusMessage) {
        *statusMessage = "Successfully saved " + std::to_string(m_products.size()) +
                         " product records to '" + targetPath + "'.";
    }
    return true;
}

bool ProductManager::addOrUpdateProduct(const Product& product) {
    if (!product.isValid()) {
        return false;
    }
    int idx = findIndexByBarcode(product.getBarcode());
    if (idx >= 0) {
        m_products[idx] = product;
    } else {
        m_products.push_back(product);
    }
    return true;
}

bool ProductManager::removeProduct(const std::string& barcode) {
    int idx = findIndexByBarcode(barcode);
    if (idx < 0) {
        return false;
    }
    m_products.erase(static_cast<size_t>(idx));
    return true;
}

void ProductManager::clear() {
    m_products.clear();
}

bool ProductManager::productExists(const std::string& barcode) const {
    return (findIndexByBarcode(barcode) >= 0);
}

const Product* ProductManager::findProductByBarcode(const std::string& barcode) const {
    int idx = findIndexByBarcode(barcode);
    if (idx >= 0) {
        return &m_products[static_cast<size_t>(idx)];
    }
    return nullptr;
}

Product* ProductManager::getMutableProduct(const std::string& barcode) {
    int idx = findIndexByBarcode(barcode);
    if (idx >= 0) {
        return &m_products[static_cast<size_t>(idx)];
    }
    return nullptr;
}

ProductSearchResult ProductManager::searchByBarcode(const std::string& barcode) const {
    ProductSearchResult result;
    std::string cleanBarcode = trim(barcode);

    if (cleanBarcode.empty()) {
        result.found = false;
        result.errorMessage = "Barcode query cannot be blank.";
        return result;
    }

    const Product* prod = findProductByBarcode(cleanBarcode);
    if (prod != nullptr) {
        result.found = true;
        result.product = *prod;
        result.errorMessage = "";
    } else {
        result.found = false;
        result.errorMessage = "No product found with barcode '" + cleanBarcode + "'.";
    }
    return result;
}

StockValidationResult ProductManager::validateStock(const std::string& barcode, int quantity) const {
    StockValidationResult res;
    res.requestedQuantity = quantity;

    if (quantity <= 0) {
        res.isValid = false;
        res.availableStock = 0;
        res.remainingStock = 0;
        res.message = "Invalid quantity: requested quantity must be greater than zero.";
        return res;
    }

    const Product* prod = findProductByBarcode(barcode);
    if (prod == nullptr) {
        res.isValid = false;
        res.availableStock = 0;
        res.remainingStock = 0;
        res.message = "Product with barcode '" + barcode + "' does not exist in inventory.";
        return res;
    }

    res.availableStock = prod->getAvailableStock();
    if (res.availableStock < quantity) {
        res.isValid = false;
        res.remainingStock = res.availableStock;
        res.message = "Insufficient stock for '" + prod->getName() +
                      "'. Requested: " + std::to_string(quantity) +
                      ", Available: " + std::to_string(res.availableStock) + ".";
        return res;
    }

    res.isValid = true;
    res.remainingStock = res.availableStock - quantity;
    res.message = "Stock validated. Remaining stock will be " +
                  std::to_string(res.remainingStock) + " unit(s).";
    return res;
}

int ProductManager::getStock(const std::string& barcode) const {
    const Product* p = findProductByBarcode(barcode);
    if (p) {
        return p->getAvailableStock();
    }
    return -1;
}

bool ProductManager::reduceStock(const std::string& barcode, int quantity, std::string* reason) {
    if (quantity <= 0) {
        if (reason) *reason = "Quantity to reduce must be positive (received: " + std::to_string(quantity) + ").";
        return false;
    }

    Product* p = getMutableProduct(barcode);
    if (p == nullptr) {
        if (reason) *reason = "Product '" + barcode + "' not found.";
        return false;
    }

    if (!p->hasSufficientStock(quantity)) {
        if (reason) {
            *reason = "Insufficient stock for '" + p->getName() + "'. Required: " +
                      std::to_string(quantity) + ", Available: " + std::to_string(p->getAvailableStock());
        }
        return false;
    }

    p->deductStock(quantity);
    if (reason) {
        *reason = "Stock successfully reduced. New stock: " + std::to_string(p->getAvailableStock());
    }
    return true;
}

bool ProductManager::addStock(const std::string& barcode, int quantity, std::string* reason) {
    if (quantity <= 0) {
        if (reason) *reason = "Quantity to add must be positive (received: " + std::to_string(quantity) + ").";
        return false;
    }

    Product* p = getMutableProduct(barcode);
    if (p == nullptr) {
        if (reason) *reason = "Product '" + barcode + "' not found.";
        return false;
    }

    p->restock(quantity);
    if (reason) {
        *reason = "Stock successfully increased. New stock: " + std::to_string(p->getAvailableStock());
    }
    return true;
}

size_t ProductManager::getProductCount() const {
    return m_products.size();
}

DynamicArray<Product> ProductManager::getAllProducts() const {
    return m_products;
}

DynamicArray<Product> ProductManager::getProductsByCategory(const std::string& category) const {
    DynamicArray<Product> list;
    std::string cleanCat = trim(category);
    for (size_t i = 0; i < m_products.size(); ++i) {
        if (m_products[i].getCategory() == cleanCat) {
            list.push_back(m_products[i]);
        }
    }
    return list;
}

DynamicArray<Product> ProductManager::getLowStockProducts(int threshold) const {
    DynamicArray<Product> list;
    for (size_t i = 0; i < m_products.size(); ++i) {
        if (m_products[i].getAvailableStock() <= threshold) {
            list.push_back(m_products[i]);
        }
    }
    return list;
}

const std::string& ProductManager::getLastLoadedFilePath() const {
    return m_lastLoadedFilePath;
}

} // namespace ProductBilling
