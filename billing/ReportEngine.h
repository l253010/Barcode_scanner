#pragma once
#include "Bill.h"
#include <string>
#include <vector>
#include <ctime>

namespace billing {

/**
 * Simple structure representing a bill entry read from Bill.txt.
 */
struct BillRecord {
    std::string billId;
    std::string customerName;
    std::string orderId;
    double totalWithoutTax;
    double totalTax;
    double totalProductDiscounts;
    double seasonalDiscount;
    double specialDiscount;
    double totalPayable;
    double amountReceived;
    double balanceReturned;
    std::string paymentStatus;
    std::time_t timestamp; // seconds since epoch
};

/**
 * ReportEngine aggregates bill data and product‑detail data to produce
 * cash‑sales summaries and the two "Top 10" listings required by the
 * issue.
 */
class ReportEngine {
public:
    /** Load all bills from the given file (default: Bill.txt). */
    static std::vector<BillRecord> loadBills(const std::string &path = "Bill.txt");

    /** Load all bill‑detail lines (productCode, quantity) from BillDetail.txt. */
    static std::vector<std::pair<std::string, int>> loadBillDetails(const std::string &path = "BillDetail.txt");

    /** Generate a cash‑sales report for the inclusive range [start, end] (epoch seconds). */
    static std::string generateReport(std::time_t start, std::time_t end);

    /** Convenience wrappers for common periods. */
    static std::string dailyReport(const std::tm &date);
    static std::string weeklyReport(const std::tm &date); // week containing date (Mon‑Sun)
    static std::string monthlyReport(int year, int month);
    static std::string yearlyReport(int year);
    static std::string customReport(std::time_t start, std::time_t end);

    /** Top‑10 most‑sold products (by total quantity across all bills). */
    static std::string top10SoldProducts();

    /** Top‑10 products with the highest remaining stock (read from product_module/Product.txt). */
    static std::string top10StockProducts();
};

} // namespace billing
