#pragma once
// Product.h
// ----------------------------------------------------------------------------
// Product Specification and Representation
// Part of the Product Management and Billing Calculation Module.
//
// Encapsulates individual product attributes, validation, stock checks,
// and formatted GUI-ready string generation.
// ----------------------------------------------------------------------------

#include <string>

namespace ProductBilling {

/**
 * @brief Represents a single product entity with complete catalog specifications.
 */
class Product {
private:
    std::string m_barcode;        // Unique barcode string (e.g., 6-digit Code 39)
    std::string m_productId;      // SKU or Product ID (e.g., "PRD-101")
    std::string m_name;           // Product display name
    std::string m_description;    // Detailed description
    std::string m_category;       // Product category (e.g., "Dairy", "Beverages")
    double      m_price;          // Unit price (currency)
    int         m_availableStock; // Current available units in inventory
    double      m_taxRate;        // Tax rate percentage (e.g., 5.0 for 5%)

public:
    // ---- Constructors ----
    Product();

    Product(const std::string& barcode,
            const std::string& productId,
            const std::string& name,
            const std::string& description,
            const std::string& category,
            double price,
            int availableStock,
            double taxRate);

    // ---- Getters ----
    const std::string& getBarcode() const;
    const std::string& getProductId() const;
    const std::string& getName() const;
    const std::string& getDescription() const;
    const std::string& getCategory() const;
    double             getPrice() const;
    int                getAvailableStock() const;
    double             getTaxRate() const;             // As percentage, e.g. 5.0
    double             getTaxRateFraction() const;     // As fraction, e.g. 0.05

    // ---- Setters ----
    void setBarcode(const std::string& barcode);
    void setProductId(const std::string& productId);
    void setName(const std::string& name);
    void setDescription(const std::string& description);
    void setCategory(const std::string& category);
    void setPrice(double price);
    void setAvailableStock(int stock);
    void setTaxRate(double taxRate);

    // ---- Stock Operations ----
    /**
     * @brief Checks if the requested quantity can be fulfilled from current stock.
     * @param quantity Number of items requested (must be > 0).
     * @return true if quantity > 0 and <= availableStock, false otherwise.
     */
    bool hasSufficientStock(int quantity) const;

    /**
     * @brief Deducts requested quantity from available stock if sufficient.
     * @param quantity Number of units to deduct (> 0 and <= availableStock).
     * @return true if stock was successfully reduced, false otherwise.
     */
    bool deductStock(int quantity);

    /**
     * @brief Restocks inventory by adding quantity to available stock.
     * @param quantity Number of units to add (> 0).
     * @return true if stock was added, false if quantity <= 0.
     */
    bool restock(int quantity);

    // ---- Validation ----
    /**
     * @brief Checks whether this product has valid core specifications.
     */
    bool isValid() const;

    // ---- GUI-Ready Formatting ----
    /**
     * @brief Generates a multi-line formatted summary suitable for GUI labels or detail panels.
     */
    std::string getFormattedDetails() const;

    /**
     * @brief Generates a single-line tabulated string suitable for list controls.
     */
    std::string toTableRow() const;

    /**
     * @brief Serializes the product into a delimited record (e.g. for Product.txt).
     */
    std::string toDelimitedString(char delimiter = '|') const;

    // ---- Static Parser ----
    /**
     * @brief Parses a delimited line into a Product object.
     * @param line Text line from Product.txt.
     * @param delimiter Separator character ('|' or ',').
     * @param errorMsg Optional pointer to receive error description on failure.
     * @return Product instance (check isValid() on returned object).
     */
    static Product fromDelimitedString(const std::string& line,
                                       char delimiter = '|',
                                       std::string* errorMsg = nullptr);
};

} // namespace ProductBilling
