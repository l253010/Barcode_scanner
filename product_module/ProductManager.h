#pragma once
// ProductManager.h
// ----------------------------------------------------------------------------
// Catalog Inventory and File Handling
// Part of the Product Management and Billing Calculation Module.
//
// Manages loading, caching, querying, and updating product records.
// Free of <vector>, <map>, <sstream>, <cassert>, and <cctype>.
// ----------------------------------------------------------------------------

#include "Product.h"
#include "DynamicArray.h"
#include <string>

namespace ProductBilling {

/**
 * @brief Structured result for GUI-friendly product lookups.
 */
struct ProductSearchResult {
    bool        found = false;
    Product     product;
    std::string errorMessage;
};

/**
 * @brief Structured outcome of stock validation requests.
 */
struct StockValidationResult {
    bool        isValid           = false;
    int         requestedQuantity = 0;
    int         availableStock    = 0;
    int         remainingStock    = 0;
    std::string message;
};

/**
 * @brief Manages inventory records, file parsing, barcode searching, and stock verification.
 *        Uses a manually managed DynamicArray structure instead of std::map / std::vector.
 */
class ProductManager {
private:
    DynamicArray<Product> m_products;           // Dynamically managed contiguous catalog storage
    std::string           m_lastLoadedFilePath;

    // Internal helper to find index by barcode; returns -1 if not found
    int findIndexByBarcode(const std::string& barcode) const;

public:
    ProductManager();

    // ---- File Handling ----
    /**
     * @brief Loads and parses product records from a text file (e.g. Product.txt).
     * @param filePath Path to the product records file.
     * @param statusMessage Optional pointer to receive a human-readable status or error summary.
     * @return true if file was opened and at least one valid record was parsed, false otherwise.
     */
    bool loadFromFile(const std::string& filePath, std::string* statusMessage = nullptr);

    /**
     * @brief Explicitly saves current product inventory state to file.
     *        Does NOT run automatically; must be called explicitly when persistence is required.
     * @param filePath Destination file path. If empty, uses m_lastLoadedFilePath.
     * @param statusMessage Optional pointer to receive operation outcome.
     * @return true if successfully written, false on I/O error.
     */
    bool saveToFile(const std::string& filePath = "", std::string* statusMessage = nullptr) const;

    // ---- Product Registration & Modification ----
    /**
     * @brief Adds or updates a product in memory.
     * @param product The product to add.
     * @return true if added/updated, false if product was invalid.
     */
    bool addOrUpdateProduct(const Product& product);

    /**
     * @brief Removes a product by barcode.
     */
    bool removeProduct(const std::string& barcode);

    /**
     * @brief Clears all cached products in memory.
     */
    void clear();

    // ---- Searching & Retrieval ----
    /**
     * @brief Checks if a product exists by exact barcode.
     */
    bool productExists(const std::string& barcode) const;

    /**
     * @brief Retrieves a direct pointer to a product by barcode.
     * @return Pointer to const Product if found; nullptr if not found.
     */
    const Product* findProductByBarcode(const std::string& barcode) const;

    /**
     * @brief Mutable pointer to a product (for internal updates like stock deduction).
     */
    Product* getMutableProduct(const std::string& barcode);

    /**
     * @brief GUI-ready search function returning a structured ProductSearchResult.
     */
    ProductSearchResult searchByBarcode(const std::string& barcode) const;

    // ---- Stock Management ----
    /**
     * @brief Validates whether a requested purchase quantity can be fulfilled.
     * @param barcode The product barcode.
     * @param quantity The requested quantity (> 0).
     * @return StockValidationResult with detailed status and remaining stock projection.
     */
    StockValidationResult validateStock(const std::string& barcode, int quantity) const;

    /**
     * @brief Queries the current stock level for a barcode. Returns -1 if not found.
     */
    int getStock(const std::string& barcode) const;

    /**
     * @brief Deducts quantity from stock for a product.
     * @param barcode Product barcode.
     * @param quantity Number of units to deduct (> 0).
     * @param reason Optional pointer to receive detailed outcome reason.
     * @return true on success, false if unavailable or insufficient stock.
     */
    bool reduceStock(const std::string& barcode, int quantity, std::string* reason = nullptr);

    /**
     * @brief Restocks/adds inventory for a product.
     */
    bool addStock(const std::string& barcode, int quantity, std::string* reason = nullptr);

    /**
     * @brief Directly sets the inventory stock level for a product.
     * @param barcode Product barcode.
     * @param newQuantity Non-negative new stock level.
     * @param reason Optional pointer to receive status or error outcome.
     * @return true on success, false if product not found or invalid quantity.
     */
    bool setStock(const std::string& barcode, int newQuantity, std::string* reason = nullptr);

    // ---- Catalog Statistics & Queries ----
    size_t getProductCount() const;
    DynamicArray<Product> getAllProducts() const;
    DynamicArray<Product> getProductsByCategory(const std::string& category) const;
    DynamicArray<Product> getLowStockProducts(int threshold = 5) const;
    const std::string& getLastLoadedFilePath() const;
};

} // namespace ProductBilling
