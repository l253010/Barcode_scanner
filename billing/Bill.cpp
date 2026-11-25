#include "Bill.h"
#include <sstream>
#include <iomanip>
#include <ctime>

namespace billing {

std::string Bill::serialize() const {
    // Pipe‑separated line for easy parsing later
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << billId << "|"
        << customerName << "|"
        << orderId << "|"
        << totalWithoutTax << "|"
        << totalTax << "|"
        << totalProductDiscounts << "|"
        << seasonalDiscount << "|"
        << specialDiscount << "|"
        << totalPayable << "|"
        << amountReceived << "|"
        << balanceReturned << "|"
        << paymentStatus << "|"
        << billTimestamp;
    return oss.str();
}

void Bill::computeTotals() {
    // Re‑calculate all derived monetary fields from the item list.
    totalWithoutTax = 0.0;
    totalProductDiscounts = 0.0;

    for (const auto &item : items) {
        double line = item.unitPriceWithoutTax * static_cast<double>(item.quantity);
        totalWithoutTax += line;
        // Discount is a percentage of the line amount.
        totalProductDiscounts += line * (item.discountPercent / 100.0);
    }

    // Simple tax model – 10 % of the amount after product discounts.
    const double taxRate = 0.10; // 10 %
    double taxableAmount = totalWithoutTax - totalProductDiscounts;
    if (taxableAmount < 0) taxableAmount = 0;
    totalTax = taxableAmount * taxRate;

    // totalPayable = (subtotal - discounts) + tax - seasonal - special
    double net = taxableAmount + totalTax - seasonalDiscount - specialDiscount;
    if (net < 0) net = 0;
    totalPayable = net;

    // Ensure we have a timestamp.
    if (billTimestamp == 0) {
        billTimestamp = std::time(nullptr);
    }
}

} // namespace billing
