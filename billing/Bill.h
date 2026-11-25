#pragma once
#include <string>
#include <vector>
#include <ctime>

namespace billing {

/**
 * Represents a single line‑item in a bill.
 */
struct BillItem {
    std::string productCode;          // e.g. "ABC"
    double unitPriceWithoutTax;       // price per unit before tax
    int quantity;                     // number of units
    double discountPercent;           // product‑level discount (0‑100)
};

/**
 * Data structure for a complete bill.
 */
class Bill {
public:
    // Basic identifiers
    std::string billId;
    std::string customerName;
    std::string orderId;

    // Monetary totals (computed)
    double totalWithoutTax = 0.0;
    double totalTax = 0.0;
    double totalProductDiscounts = 0.0; // sum of per‑product discounts
    double seasonalDiscount = 0.0;
    double specialDiscount = 0.0;
    double totalPayable = 0.0; // after all discounts and tax

    // Payment info
    double amountReceived = 0.0;
    double balanceReturned = 0.0;
    std::string paymentStatus; // e.g. "PAID", "PENDING"

    // Timestamp
    std::time_t billTimestamp = 0; // seconds since epoch

    // Items
    std::vector<BillItem> items;

    /** Serialize the bill into a pipe‑separated line suitable for Bill.txt */
    std::string serialize() const;

    /** Compute derived totals (tax, discounts, payable) from items */
    void computeTotals();
};

} // namespace billing
