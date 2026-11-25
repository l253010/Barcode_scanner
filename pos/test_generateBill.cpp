// test_generateBill.cpp
#include "ProductBillingService.h"
#include <iostream>

int main() {
    // --- 1️⃣ Set up a minimal product catalog (the service will load the fallback one) ---
    ProductBillingService service;          // loads fallback catalog automatically

    // --- 2️⃣ Build a tiny order -------------------------------------------------
    OrderItem item1;
    item1.product.barcode = "100001";   // milk (fallback catalog entry)
    item1.product.acronym = "MILK-1L";
    item1.product.name   = "Organic Whole Milk 1L";
    item1.product.unitPriceWithoutTax = 3.49;
    item1.quantity = 2;                // 2 units
    item1.discountPercent = 0.0;       // no per‑item discount

    OrderItem item2;
    item2.product.barcode = "100003";   // eggs
    item2.product.acronym = "EGGS-12";
    item2.product.name   = "Farm Fresh Large Eggs 12pk";
    item2.product.unitPriceWithoutTax = 4.25;
    item2.quantity = 1;
    item2.discountPercent = 5.0;       // 5 % item‑level discount

    std::vector<OrderItem> items = { item1, item2 };

    // --- 3️⃣ Call the function we are interested in ----------------------------
    BillInfo bill = service.generateBill(
        "John Doe",                // customer name
        "john@example.com",        // contact
        items);

    // --- 4️⃣ Print the result -------------------------------------------------
    std::cout << "=== BILL SUMMARY ===\n";
    std::cout << "Bill ID:   " << bill.billId << "\n";
    std::cout << "Order ID:  " << bill.orderId << "\n";
    std::cout << "Customer:  " << bill.customerName << " (" << bill.customerContact << ")\n";
    std::cout << "Timestamp: " << bill.timestamp << "\n";
    std::cout << "Subtotal (no tax):          " << bill.totalWithoutTax << "\n";
    std::cout << "Total product discounts:    " << bill.totalDiscount << "\n";
    std::cout << "Tax:                        " << bill.totalTax << "\n";
    std::cout << "Payable (final amount):     " << bill.totalPayable << "\n";
    std::cout << "Payment status: " << bill.paymentStatus << "\n";
    std::cout << "=====================\n";

    return 0;
}