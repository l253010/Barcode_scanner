#pragma once
// Product.h
// ----------------------------------------------------------------------------
// Product Specification and Representation
// Part of the Product Management and Billing Calculation Module.
//
// Compliant with assignment specification:
// Fields: Product ID, Barcode, Acronym (max 8), Name, Measurement Unit,
//         Unit Price, Tax Amount (PKR), Discount per Unit, Available Qty,
//         Barcode file path, terminated by '#'.
// ----------------------------------------------------------------------------

#include <string>

namespace ProductBilling {

/**
 * @brief Represents a single product entity with complete catalog specifications.
 */
class Product {
private:
    std::string m_productId;       // 1. Unique product ID (e.g., "PRD-101")
    std::string m_barcode;         // 2. 6-digit Code 39 barcode (e.g., "100001")
    std::string m_acronym;         // 3. Short acronym (max 8 characters, e.g., "MILK-1L")
    std::string m_name;            // 4. Product display name (e.g., "Organic Whole Milk 1L")
    std::string m_measurementUnit; // 5. Measurement unit ("count", "weight")
    double      m_price;           // 6. Unit price
    double      m_taxAmount;       // 7. Tax on product (amount in PKR)
    double      m_discountPerUnit; // 8. Discount on one unit of product (PKR)
    int         m_availableStock;  // 9. Available quantity in stock
    std::string m_barcodeFilePath; // 10. Barcode image file name and path

    // Retained for POS GUI compatibility
    std::string m_category;
    std::string m_description;

public:
    // ---- Constructors ----
    Product();

    Product(const std::string& productId,
            const std::string& barcode,
            const std::string& acronym,
            const std::string& name,
            const std::string& measurementUnit,
            double price,
            double taxAmount,
            double discountPerUnit,
            int availableStock,
            const std::string& barcodeFilePath,
            const std::string& category = "",
            const std::string& description = "");

    // Backward-compatible constructor
    Product(const std::string& barcode,
            const std::string& productId,
            const std::string& name,
            const std::string& description,
            const std::string& category,
            double price,
            int availableStock,
            double taxRate);

    // ---- Getters ----
    const std::string& getProductId() const;
    const std::string& getBarcode() const;
    const std::string& getAcronym() const;
    const std::string& getName() const;
    const std::string& getMeasurementUnit() const;
    double             getPrice() const;
    double             getTaxAmount() const;       // Amount in PKR
    double             getDiscountPerUnit() const; // Discount on one unit in PKR
    int                getAvailableStock() const;
    const std::string& getBarcodeFilePath() const;
    const std::string& getCategory() const;
    const std::string& getDescription() const;

    // Derived compatibility getters
    double getTaxRate() const;             // As percentage, e.g. 5.0 for 5%
    double getTaxRateFraction() const;     // As fraction, e.g. 0.05 for 5%

    // ---- Setters ----
    void setProductId(const std::string& productId);
    void setBarcode(const std::string& barcode);
    void setAcronym(const std::string& acronym);
    void setName(const std::string& name);
    void setMeasurementUnit(const std::string& unit);
    void setPrice(double price);
    void setTaxAmount(double taxAmount);
    void setDiscountPerUnit(double discount);
    void setAvailableStock(int stock);
    void setBarcodeFilePath(const std::string& path);
    void setCategory(const std::string& category);
    void setDescription(const std::string& description);
    void setTaxRate(double taxRate);       // Sets percentage and recalculates taxAmount

    // ---- Stock Operations ----
    bool hasSufficientStock(int quantity) const;
    bool deductStock(int quantity);
    bool restock(int quantity);

    // ---- Validation ----
    bool isValid() const;

    // ---- Formatting & Serialization ----
    std::string getFormattedDetails() const;
    std::string toTableRow() const;
    std::string toFileRecord() const;
    std::string toDelimitedString(char delimiter = ' ') const;

    // ---- Static Parser ----
    static Product fromDelimitedString(const std::string& line,
                                       char delimiter = ' ',
                                       std::string* errorMsg = nullptr);
};

} // namespace ProductBilling
