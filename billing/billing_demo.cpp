#include "billing/BillIdGenerator.h"
#include "billing/Bill.h"
#include "billing/DiscountEngine.h"
#include "billing/PdfGenerator.h"
#include <iostream>
#include <vector>
#include <string>

int main() {
    // ---------------------------------------------------------------------
    // 1️⃣ Create a dummy order (simulating what the UI would pass).
    // ---------------------------------------------------------------------
    billing::Bill bill;
    bill.billId = billing::BillIdGenerator::nextId();
    bill.customerName = "Demo Customer";
    bill.orderId = "ORD-DEM-001";
    bill.billTimestamp = std::time(nullptr);

    // Sample items – productCode, unit price, quantity, product‑level discount %
    billing::BillItem item1{"PRD-101", 3.49, 2, 5.0};   // 5 % discount on milk
    billing::BillItem item2{"PRD-104", 7.99, 1, 0.0};   // coffee, no discount
    billing::BillItem item3{"PRD-110", 1.75, 3, 0.0};   // lemonade, no discount

    bill.items = {item1, item2, item3};

    // ---------------------------------------------------------------------
    // 2️⃣ Compute base totals (product discounts + tax)
    // ---------------------------------------------------------------------
    bill.computeTotals();

    // ---------------------------------------------------------------------
    // 3️⃣ Apply a special manager discount (e.g., 7 %).
    // ---------------------------------------------------------------------
    billing::DiscountEngine::applySpecialDiscount(bill, 7.0);

    // ---------------------------------------------------------------------
    // 4️⃣ Apply seasonal discounts if any are defined in Seasons.txt.
    // ---------------------------------------------------------------------
    auto seasons = billing::DiscountEngine::loadSeasons();
    billing::DiscountEngine::applySeasonalDiscount(bill, seasons);

    // Re‑compute after discounts
    bill.computeTotals();

    // ---------------------------------------------------------------------
    // 5️⃣ Persist the bill to text files.
    // ---------------------------------------------------------------------
    {
        std::ofstream out("Bill.txt", std::ios::app);
        if (out) out << bill.serialize() << '\n';
    }
    {
        std::ofstream out("BillDetail.txt", std::ios::app);
        if (out) {
            for (const auto &bi : bill.items) {
                out << bill.billId << '|' << bi.productCode << '|'
                    << bi.unitPriceWithoutTax << '|' << bi.quantity << '|'
                    << bi.discountPercent << '\n';
            }
        }
    }

    // ---------------------------------------------------------------------
    // 6️⃣ Generate a PDF receipt in an "output" folder.
    // ---------------------------------------------------------------------
    std::filesystem::create_directory("output");
    std::string pdfPath = std::string("output/Bill_") + bill.billId + ".pdf";
    bool ok = billing::PdfGenerator::createPdf(bill, QString::fromStdString(pdfPath));
    std::cout << "PDF generated: " << (ok ? "yes" : "no") << " -> " << pdfPath << '\n';

    // ---------------------------------------------------------------------
    // 7️⃣ Quick console summary.
    // ---------------------------------------------------------------------
    std::cout << "--- Bill Summary ---\n";
    std::cout << "Bill ID: " << bill.billId << "\n";
    std::cout << "Customer: " << bill.customerName << "\n";
    std::cout << "Total (payable): $" << bill.totalPayable << "\n";
    std::cout << "(includes tax, product discounts, special, seasonal)\n";
    return 0;
}
