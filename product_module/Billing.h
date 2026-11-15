#pragma once
// Billing.h
// ----------------------------------------------------------------------------
// Billing Calculation and Order Processing Engine
// Part of the Product Management and Billing Calculation Module.
//
// Encapsulates individual line items, discount models, multiple-product bills,
// tax and discount computations, and GUI-ready summary and receipt generation.
// Free of <vector>, <map>, <sstream>, <cassert>, and <cctype>.
// ----------------------------------------------------------------------------

#include "Product.h"
#include "ProductManager.h"
#include "DynamicArray.h"
#include <string>

namespace ProductBilling {

/**
 * @brief Represents an individual product line in a customer's bill.
 */
class BillItem {
private:
    Product m_product;      // Snapshot of product information
    int     m_quantity;     // Purchased unit count
    double  m_unitPrice;    // Price applied per unit
    double  m_itemDiscount; // Fixed discount amount applied to this item line

public:
    BillItem();
    BillItem(const Product& product, int quantity, double itemDiscount = 0.0);

    // Getters
    const Product& getProduct() const;
    int            getQuantity() const;
    double         getUnitPrice() const;
    double         getItemDiscount() const;
    double         getTaxRate() const;

    // Setters
    void setQuantity(int quantity);
    void setItemDiscount(double discount);

    // ---- Financial Computations ----
    /**
     * @brief Raw item subtotal: unitPrice * quantity
     */
    double getSubtotal() const;

    /**
     * @brief Taxable amount after item-level discount: max(0, subtotal - itemDiscount)
     */
    double getTaxableAmount() const;

    /**
     * @brief Computed tax: taxableAmount * (taxRate / 100.0)
     */
    double getTaxAmount() const;

    /**
     * @brief Net line total: subtotal - itemDiscount + taxAmount
     */
    double getLineTotal() const;

    // ---- GUI Display ----
    /**
     * @brief Tabulated row string suitable for listview / grid display.
     */
    std::string toFormattedRow() const;
};

/**
 * @brief Type of overall discount applied to an entire bill.
 */
enum class DiscountType {
    NONE = 0,
    PERCENTAGE,   // Value represents % (e.g., 10.0 for 10% off subtotal)
    FIXED_AMOUNT  // Value represents currency (e.g., 5.0 for $5.00 off subtotal)
};

/**
 * @brief Configured bill-level discount.
 */
struct BillDiscount {
    DiscountType type  = DiscountType::NONE;
    double       value = 0.0; // % or $ depending on type

    BillDiscount() = default;
    BillDiscount(DiscountType t, double v) : type(t), value(v >= 0.0 ? v : 0.0) {}

    /**
     * @brief Computes effective discount amount applied to the given subtotal.
     */
    double computeDiscount(double baseSubtotal) const;
};

/**
 * @brief High-level structured summary ready for GUI binding.
 */
struct BillSummary {
    int    distinctItemCount = 0;
    int    totalUnitsCount   = 0;
    double rawSubtotal       = 0.0;
    double itemDiscounts     = 0.0;
    double billDiscount      = 0.0;
    double totalDiscount     = 0.0;
    double netTaxableAmount  = 0.0;
    double totalTax          = 0.0;
    double totalPayable      = 0.0;
};

/**
 * @brief Complete customer bill supporting multiple products, discounts, tax, and stock commit.
 */
class Bill {
private:
    std::string           m_billId;
    DynamicArray<BillItem> m_items;
    BillDiscount          m_billDiscount;

public:
    Bill(const std::string& billId = "BILL-0001");

    // ---- Identification ----
    const std::string& getBillId() const;
    void setBillId(const std::string& billId);

    // ---- Item Management ----
    /**
     * @brief Adds a product to the bill after checking stock availability.
     * @param product The product to add.
     * @param quantity Number of units to add (> 0).
     * @param itemDiscount Optional line item discount amount.
     * @param errorMsg Optional pointer to receive error description on failure.
     * @return true if added successfully, false if validation fails.
     */
    bool addItem(const Product& product,
                 int quantity,
                 double itemDiscount = 0.0,
                 std::string* errorMsg = nullptr);

    /**
     * @brief Adds an item by barcode lookup through a ProductManager instance.
     *        Validates barcode existence and stock sufficiency.
     */
    bool addItemByBarcode(ProductManager& manager,
                          const std::string& barcode,
                          int quantity,
                          double itemDiscount = 0.0,
                          std::string* errorMsg = nullptr);

    /**
     * @brief Removes an item at specified 0-based index.
     */
    bool removeItem(size_t index, std::string* errorMsg = nullptr);

    /**
     * @brief Updates quantity for an item at specified index.
     */
    bool updateItemQuantity(size_t index,
                            int newQuantity,
                            const ProductManager* manager = nullptr,
                            std::string* errorMsg = nullptr);

    /**
     * @brief Removes all items from the bill.
     */
    void clear();

    // ---- Discounts ----
    void setBillDiscount(DiscountType type, double value);
    void clearBillDiscount();
    const BillDiscount& getBillDiscount() const;

    // ---- Financial Calculations ----
    /**
     * @brief Sum of all item subtotals (price * quantity).
     */
    double calculateSubtotal() const;

    /**
     * @brief Sum of all line-item discounts.
     */
    double calculateItemDiscounts() const;

    /**
     * @brief Computed bill-level discount amount.
     */
    double calculateBillDiscount() const;

    /**
     * @brief Total discount = item discounts + bill discount.
     */
    double calculateTotalDiscount() const;

    /**
     * @brief Total tax across all items.
     */
    double calculateTotalTax() const;

    /**
     * @brief Final total payable amount: (Subtotal - Total Discount) + Total Tax.
     */
    double calculateFinalPayable() const;

    // ---- Inventory Commitment ----
    /**
     * @brief Atomically commits and deducts stock for all bill items from the ProductManager.
     *        Validates all items first before deducting to ensure consistency.
     * @param manager Target product manager.
     * @param errorMsg Optional pointer to receive failure details.
     * @return true if stock was committed, false if any item had insufficient stock.
     */
    bool finalizeAndCommitStock(ProductManager& manager, std::string* errorMsg = nullptr);

    // ---- Inspection & GUI Data ----
    size_t getItemCount() const;
    const DynamicArray<BillItem>& getItems() const;
    const BillItem* getItem(size_t index) const;

    /**
     * @brief Generates a structured summary DTO for GUI components.
     */
    BillSummary getSummary() const;

    /**
     * @brief Generates a printable, complete receipt string without <sstream>.
     */
    std::string generateReceipt(const std::string& storeName = "BARCODE POS SCANNER STORE") const;
};

} // namespace ProductBilling
