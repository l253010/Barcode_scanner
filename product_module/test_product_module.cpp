// test_product_module.cpp
// ----------------------------------------------------------------------------
// Comprehensive Automated Test Suite for Product Management and Billing Module.
// Free of <vector>, <map>, <sstream>, <cassert>, and <cctype>.
//
// Tests:
//   1. File Loading & Parsing (valid records, comments, invalid records, missing file)
//   2. Barcode Searching (exact matching, nonexistent barcode, empty barcode)
//   3. Product Specifications Retrieval (barcode, id, name, category, price, stock, tax)
//   4. Stock Management (stock query, valid purchases, insufficient stock, invalid quantities)
//   5. Billing Calculations:
//      - Single item subtotal, tax, net total
//      - Multiple products in one bill
//      - Item-level discounts
//      - Bill-level discounts (percentage & fixed amount)
//      - Tax calculation across multiple tax rates (including 0% tax-exempt)
//      - Final total payable amount
//   6. Inventory Commitment & Deduction on Checkout
//   7. GUI-Ready Structured Output & Receipt Generation
//   8. Dedicated DynamicArray Memory Safety & Rule of Five Verification:
//      - Default construction
//      - Insertion and resizing
//      - Self-referential insertion at full capacity (aliased push_back)
//      - Copy construction and deep-copy independence
//      - Copy self-assignment
//      - Move construction
//      - Move assignment
//      - Self-move assignment
//      - Empty-array operations
//      - Resource release in clear() and pop_back()
//      - Erasing first, middle, and last elements
//      - Large-scale resizing (1,000+ elements)
//      - Safe destruction of moved-from objects
// ----------------------------------------------------------------------------

#include "Product.h"
#include "ProductManager.h"
#include "Billing.h"
#include "DynamicArray.h"

#include <iostream>
#include <cmath>
#include <string>
#include <fstream>

using namespace ProductBilling;

namespace {
    int g_testsTotal = 0;
    int g_testsPassed = 0;
    int g_testsFailed = 0;

    void recordTest(const std::string& testName, bool condition, const std::string& details = "") {
        g_testsTotal++;
        if (condition) {
            g_testsPassed++;
            std::cout << "  [PASS] " << testName << "\n";
        } else {
            g_testsFailed++;
            std::cerr << "  [FAIL] " << testName;
            if (!details.empty()) {
                std::cerr << " - " << details;
            }
            std::cerr << "\n";
        }
    }

    bool approxEqual(double a, double b, double epsilon = 0.005) {
        return std::fabs(a - b) <= epsilon;
    }
}

// ----------------------------------------------------------------------------
// Test 1: File Loading & Handling
// ----------------------------------------------------------------------------
void testFileHandling() {
    std::cout << "\n=== TEST SUITE 1: Product File Handling ===\n";
    ProductManager manager;

    // 1.1 Nonexistent file handling
    std::string errMsg;
    bool missingLoaded = manager.loadFromFile("non_existent_file_xyz_123.txt", &errMsg);
    recordTest("Handle missing file safely without crashing",
               !missingLoaded && !errMsg.empty(), errMsg);

    // 1.2 Empty path handling
    bool emptyLoaded = manager.loadFromFile("", &errMsg);
    recordTest("Handle empty file path safely",
               !emptyLoaded && !errMsg.empty(), errMsg);

    // 1.3 Load valid sample file
    std::string loadMsg;
    bool loaded = manager.loadFromFile("Product.txt", &loadMsg);
    recordTest("Load records from Product.txt", loaded, loadMsg);
    recordTest("Verify multiple products loaded (expected >= 10)",
               manager.getProductCount() >= 10,
               "Count: " + std::to_string(manager.getProductCount()));

    // 1.4 Handling of corrupted lines in temp file
    std::string tempPath = "test_corrupt_records.tmp";
    std::ofstream tempFile(tempPath);
    tempFile << "# Comment line\n"
             << "999001|PRD-901|Valid Item 1|Snacks|1.50|10|5.0|Valid description\n"
             << "invalid_corrupt_line_without_enough_delimiters\n"
             << "999002|PRD-902|Valid Item 2|Snacks|invalid_price|10|5.0|Invalid price\n"
             << "999003|PRD-903|Valid Item 3|Snacks|2.00|invalid_stock|5.0|Invalid stock\n"
             << "999004|PRD-904|Valid Item 4|Snacks|3.50|20|0.0|Second valid record\n";
    tempFile.close();

    ProductManager tempManager;
    std::string corruptStatus;
    bool corruptLoaded = tempManager.loadFromFile(tempPath, &corruptStatus);
    recordTest("Safely parse file with invalid lines (skip bad, load valid)",
               corruptLoaded && tempManager.getProductCount() == 2,
               "Loaded: " + std::to_string(tempManager.getProductCount()) + ", Msg: " + corruptStatus);

    // Clean up temporary test file
    std::remove(tempPath.c_str());
}

// ----------------------------------------------------------------------------
// Test 2: Product Searching
// ----------------------------------------------------------------------------
void testProductSearching() {
    std::cout << "\n=== TEST SUITE 2: Product Barcode Searching ===\n";
    ProductManager manager;
    manager.loadFromFile("Product.txt");

    // 2.1 Search existing barcode
    const Product* p1 = manager.findProductByBarcode("100001");
    recordTest("Search for existing barcode '100001'",
               p1 != nullptr && p1->getName() == "Organic Whole Milk 1L");

    // 2.2 Search code 39 standard 6-digit barcode '123456'
    const Product* p2 = manager.findProductByBarcode("123456");
    recordTest("Search for barcode '123456' (Dark Roast Coffee)",
               p2 != nullptr && p2->getProductId() == "PRD-104");

    // 2.3 Search nonexistent barcode
    const Product* pNotFound = manager.findProductByBarcode("999999");
    recordTest("Search for nonexistent barcode '999999' returns nullptr",
               pNotFound == nullptr);

    // 2.4 Product exists check
    recordTest("productExists('100002') returns true", manager.productExists("100002"));
    recordTest("productExists('000000') returns false", !manager.productExists("000000"));

    // 2.5 Structured GUI search result
    ProductSearchResult resGood = manager.searchByBarcode("100003");
    recordTest("Structured search result: found=true for '100003'",
               resGood.found && resGood.product.getName() == "Farm Fresh Large Eggs 12pk");

    ProductSearchResult resBad = manager.searchByBarcode("888888");
    recordTest("Structured search result: found=false and descriptive error for nonexistent barcode",
               !resBad.found && !resBad.errorMessage.empty());

    // 2.6 Prevent incorrect barcode partial matches
    const Product* partial = manager.findProductByBarcode("10000"); // 5 digits instead of 6
    recordTest("Prevent partial barcode matches ('10000' should not match '100001')",
               partial == nullptr);
}

// ----------------------------------------------------------------------------
// Test 3: Product Specifications Retrieval
// ----------------------------------------------------------------------------
void testProductSpecifications() {
    std::cout << "\n=== TEST SUITE 3: Product Specifications Retrieval ===\n";
    ProductManager manager;
    manager.loadFromFile("Product.txt");

    const Product* p = manager.findProductByBarcode("100001");
    if (p == nullptr) {
        recordTest("Find product '100001' for specifications retrieval", false, "Product '100001' was null");
        return;
    }

    recordTest("Retrieve Barcode", p->getBarcode() == "100001");
    recordTest("Retrieve Product ID", p->getProductId() == "PRD-101");
    recordTest("Retrieve Acronym", p->getAcronym() == "MILK-1L");
    recordTest("Retrieve Product Name", p->getName() == "Organic Whole Milk 1L");
    recordTest("Retrieve Measurement Unit", p->getMeasurementUnit() == "count");
    recordTest("Retrieve Price", approxEqual(p->getPrice(), 3.49));
    recordTest("Retrieve Stock", p->getAvailableStock() == 50);
    recordTest("Retrieve Tax Amount (PKR)", approxEqual(p->getTaxAmount(), 0.1745));
    recordTest("Retrieve Tax Rate (%)", approxEqual(p->getTaxRate(), 5.0));
    recordTest("Retrieve Tax Rate Fraction", approxEqual(p->getTaxRateFraction(), 0.05));
    recordTest("Retrieve Discount Per Unit", approxEqual(p->getDiscountPerUnit(), 0.0));
    recordTest("Retrieve Barcode File Path", p->getBarcodeFilePath() == "barcodes/barcode_100001.png");

    // Formatted GUI string contains all fields
    std::string details = p->getFormattedDetails();
    recordTest("GUI-ready formatted details contains specifications",
               details.find("100001") != std::string::npos &&
               details.find("PRD-101") != std::string::npos &&
               details.find("MILK-1L") != std::string::npos &&
               details.find("Organic Whole Milk 1L") != std::string::npos &&
               details.find("count") != std::string::npos &&
               details.find("3.49") != std::string::npos);
}

// ----------------------------------------------------------------------------
// Test 4: Stock Management
// ----------------------------------------------------------------------------
void testStockManagement() {
    std::cout << "\n=== TEST SUITE 4: Stock Management & Validation ===\n";
    ProductManager manager;
    manager.loadFromFile("Product.txt");

    // Check available stock
    recordTest("Query stock for '100001' (expected 50)", manager.getStock("100001") == 50);

    // Valid purchase quantity validation
    StockValidationResult v1 = manager.validateStock("100001", 5);
    recordTest("Validate valid purchase (5 units from 50)",
               v1.isValid && v1.remainingStock == 45);

    // Insufficient stock validation
    StockValidationResult v2 = manager.validateStock("100001", 100);
    recordTest("Reject purchase exceeding stock (100 units from 50)",
               !v2.isValid && !v2.message.empty());

    // Exact stock purchase
    StockValidationResult v3 = manager.validateStock("100001", 50);
    recordTest("Validate exact stock purchase (50 units from 50, remaining 0)",
               v3.isValid && v3.remainingStock == 0);

    // Invalid zero quantity
    StockValidationResult vZero = manager.validateStock("100001", 0);
    recordTest("Reject zero quantity purchase", !vZero.isValid);

    // Invalid negative quantity
    StockValidationResult vNeg = manager.validateStock("100001", -3);
    recordTest("Reject negative quantity purchase", !vNeg.isValid);

    // Unavailable product stock validation
    StockValidationResult vMissing = manager.validateStock("999999", 1);
    recordTest("Reject stock validation for nonexistent product", !vMissing.isValid);

    // In-memory stock reduction
    std::string reason;
    bool reduced = manager.reduceStock("100001", 5, &reason);
    recordTest("Reduce stock in memory (50 -> 45)",
               reduced && manager.getStock("100001") == 45, reason);

    // Reduce beyond available stock
    bool overReduce = manager.reduceStock("100001", 50, &reason);
    recordTest("Prevent reducing stock beyond available level (50 from 45)",
               !overReduce && manager.getStock("100001") == 45, reason);

    // Restock
    bool restocked = manager.addStock("100001", 10, &reason);
    recordTest("Restock inventory (45 + 10 -> 55)",
               restocked && manager.getStock("100001") == 55, reason);

    // Set stock directly (including decrease and setting to 0)
    bool setZero = manager.setStock("100001", 0, &reason);
    recordTest("Set stock to zero directly",
               setZero && manager.getStock("100001") == 0, reason);

    // Increase stock from zero
    bool setFromZero = manager.setStock("100001", 50, &reason);
    recordTest("Increase stock from zero (0 -> 50)",
               setFromZero && manager.getStock("100001") == 50, reason);

    // Reject negative stock setting
    bool setNegative = manager.setStock("100001", -5, &reason);
    recordTest("Reject negative stock setting (-5)",
               !setNegative && manager.getStock("100001") == 50, reason);
}

// ----------------------------------------------------------------------------
// Test 5: Billing Calculation & Multiple Products
// ----------------------------------------------------------------------------
void testBillingCalculations() {
    std::cout << "\n=== TEST SUITE 5: Billing Calculations ===\n";
    ProductManager manager;
    manager.loadFromFile("Product.txt");

    Bill bill("TEST-BILL-101");
    std::string err;

    // 5.1 Single product addition and subtotal
    // Milk: $3.49, Qty 2 -> Subtotal = $6.98, Tax 5% = $0.35, Total = $7.33
    bool added1 = bill.addItemByBarcode(manager, "100001", 2, 0.0, &err);
    recordTest("Add 2 units of Milk ($3.49 each) to bill", added1, err);
    recordTest("Subtotal for 2 units of Milk: $6.98",
               approxEqual(bill.calculateSubtotal(), 6.98));
    recordTest("Tax (5%) for Milk: $0.35",
               approxEqual(bill.calculateTotalTax(), 0.35));
    recordTest("Final payable for Milk: $7.33",
               approxEqual(bill.calculateFinalPayable(), 7.33));

    // 5.2 Multiple products in one bill
    // Bread: $2.89, Qty 1, Tax 0% (tax exempt) -> Subtotal = $2.89
    // Coffee: $7.99, Qty 1, Tax 8% -> Subtotal = $7.99, Tax = 7.99 * 0.08 = $0.64
    bool added2 = bill.addItemByBarcode(manager, "100002", 1, 0.0, &err);
    bool added3 = bill.addItemByBarcode(manager, "123456", 1, 0.0, &err);
    recordTest("Add Bread ($2.89, 0% tax) and Coffee ($7.99, 8% tax) to bill",
               added2 && added3, err);

    // Expected Combined:
    // Subtotal = 6.98 + 2.89 + 7.99 = 17.86
    // Tax = 0.35 (Milk) + 0.00 (Bread) + 0.64 (Coffee) = 0.99
    // Total Payable = 17.86 + 0.99 = 18.85
    recordTest("Combined Subtotal for multiple items: $17.86",
               approxEqual(bill.calculateSubtotal(), 17.86),
               "Got: " + std::to_string(bill.calculateSubtotal()));
    recordTest("Combined Tax across distinct rates (5%, 0%, 8%): $0.99",
               approxEqual(bill.calculateTotalTax(), 0.99),
               "Got: " + std::to_string(bill.calculateTotalTax()));
    recordTest("Combined Final Payable without discount: $18.85",
               approxEqual(bill.calculateFinalPayable(), 18.85),
               "Got: " + std::to_string(bill.calculateFinalPayable()));

    // 5.3 Item-level discount test
    // Create new bill for testing item discounts
    Bill itemDiscBill("TEST-DISC-01");
    // Coffee ($7.99 x 2 = $15.98) with $1.00 item discount
    // Net Coffee = 14.98, Tax 8% = $1.20, Total = 16.18
    const Product* coffee = manager.findProductByBarcode("123456");
    if (coffee) {
        itemDiscBill.addItem(*coffee, 2, 1.00);
    }
    recordTest("Item discount calculation ($1.00 discount on Coffee)",
               approxEqual(itemDiscBill.calculateItemDiscounts(), 1.00));
    recordTest("Tax on item after discount (8% on $14.98): $1.20",
               approxEqual(itemDiscBill.calculateTotalTax(), 1.20),
               "Got: " + std::to_string(itemDiscBill.calculateTotalTax()));
    recordTest("Final payable after item discount: $16.18",
               approxEqual(itemDiscBill.calculateFinalPayable(), 16.18),
               "Got: " + std::to_string(itemDiscBill.calculateFinalPayable()));

    // 5.4 Bill-level percentage discount (10% off bill)
    bill.setBillDiscount(DiscountType::PERCENTAGE, 10.0);
    // Subtotal = 17.86
    // 10% discount = $1.79
    // Net taxable subtotal = 17.86 - 1.79 = 16.07
    // Discount ratio = (17.86 - 1.79) / 17.86 = 0.8998
    // Tax: Milk (6.98 * 0.8998 * 0.05 = 0.314) + Bread (0) + Coffee (7.99 * 0.8998 * 0.08 = 0.575) = 0.89
    // Payable = 16.07 + 0.89 = 16.96
    recordTest("Bill-level 10% discount amount: $1.79",
               approxEqual(bill.calculateBillDiscount(), 1.79),
               "Got: " + std::to_string(bill.calculateBillDiscount()));
    recordTest("Final payable with 10% discount applied: $16.96",
               approxEqual(bill.calculateFinalPayable(), 16.96),
               "Got: " + std::to_string(bill.calculateFinalPayable()));

    // 5.5 Bill-level fixed amount discount ($5.00 off)
    bill.setBillDiscount(DiscountType::FIXED_AMOUNT, 5.00);
    recordTest("Bill-level fixed discount of $5.00",
               approxEqual(bill.calculateBillDiscount(), 5.00));
    // Subtotal 17.86 - 5.00 = 12.86 net taxable
    // Ratio = 12.86 / 17.86 = 0.7200
    // Tax = (6.98 * 0.7200 * 0.05) + (7.99 * 0.7200 * 0.08) = 0.251 + 0.460 = 0.71
    // Payable = 12.86 + 0.71 = 13.57
    recordTest("Final payable with $5.00 fixed discount: $13.57",
               approxEqual(bill.calculateFinalPayable(), 13.57),
               "Got: " + std::to_string(bill.calculateFinalPayable()));

    // 5.6 Excess discount handling (discount cannot exceed subtotal or make total negative)
    bill.setBillDiscount(DiscountType::FIXED_AMOUNT, 50.00); // Exceeds $17.86 subtotal
    recordTest("Excess discount clamped to subtotal (never negative payable)",
               approxEqual(bill.calculateBillDiscount(), 17.86) &&
               approxEqual(bill.calculateFinalPayable(), 0.00));
}

// ----------------------------------------------------------------------------
// Test 6: Inventory Commitment on Checkout
// ----------------------------------------------------------------------------
void testInventoryCommitment() {
    std::cout << "\n=== TEST SUITE 6: Inventory Commitment on Finalization ===\n";
    ProductManager manager;
    manager.loadFromFile("Product.txt");

    int initialMilk = manager.getStock("100001");
    int initialBread = manager.getStock("100002");

    Bill checkoutBill("CHECKOUT-001");
    checkoutBill.addItemByBarcode(manager, "100001", 3); // 3 Milk
    checkoutBill.addItemByBarcode(manager, "100002", 2); // 2 Bread

    std::string commitErr;
    bool committed = checkoutBill.finalizeAndCommitStock(manager, &commitErr);
    recordTest("Commit stock for checkout bill", committed, commitErr);
    recordTest("Verify Milk stock was deducted by 3",
               manager.getStock("100001") == initialMilk - 3);
    recordTest("Verify Bread stock was deducted by 2",
               manager.getStock("100002") == initialBread - 2);

    // Attempt to commit with an item that exceeds remaining stock
    Bill overBill("OVER-001");
    const Product* milk = manager.findProductByBarcode("100001");
    if (milk) {
        // Since addItem validates stock, verify addItem directly rejects over-stock
        std::string addErr;
        bool rejectedAdd = overBill.addItem(*milk, manager.getStock("100001") + 50, 0.0, &addErr);
        recordTest("Reject adding item that exceeds available stock to bill",
                   !rejectedAdd && !addErr.empty(), addErr);
    }
}

// ----------------------------------------------------------------------------
// Test 7: GUI-Ready Data & Receipt Formatting
// ----------------------------------------------------------------------------
void testGuiReadyData() {
    std::cout << "\n=== TEST SUITE 7: GUI-Ready Data Structures & Receipt ===\n";
    ProductManager manager;
    manager.loadFromFile("Product.txt");

    Bill bill("GUI-DEMO-01");
    bill.addItemByBarcode(manager, "100001", 2); // Milk
    bill.addItemByBarcode(manager, "123456", 1); // Coffee
    bill.setBillDiscount(DiscountType::PERCENTAGE, 5.0);

    // BillSummary struct
    BillSummary summary = bill.getSummary();
    recordTest("BillSummary distinct item count is 2", summary.distinctItemCount == 2);
    recordTest("BillSummary total units count is 3", summary.totalUnitsCount == 3);
    recordTest("BillSummary rawSubtotal > 0", summary.rawSubtotal > 0.0);
    recordTest("BillSummary totalPayable > 0", summary.totalPayable > 0.0);

    // Receipt formatting
    std::string receipt = bill.generateReceipt("METRO SUPERMARKET");
    recordTest("Receipt generation produces non-empty string", !receipt.empty());
    recordTest("Receipt contains store header",
               receipt.find("METRO SUPERMARKET") != std::string::npos);
    recordTest("Receipt contains Invoice ID",
               receipt.find("GUI-DEMO-01") != std::string::npos);
    recordTest("Receipt contains product barcodes",
               receipt.find("100001") != std::string::npos &&
               receipt.find("123456") != std::string::npos);
    recordTest("Receipt contains financial totals and breakdown",
               receipt.find("Gross Subtotal") != std::string::npos &&
               receipt.find("TOTAL PAYABLE AMOUNT") != std::string::npos);

    // Individual item formatted row
    std::string row = bill.getItems()[0].toFormattedRow();
    recordTest("BillItem toFormattedRow() contains barcode and price",
               row.find("100001") != std::string::npos &&
               row.find("3.49") != std::string::npos);
}

// ----------------------------------------------------------------------------
// Test 8: Dedicated DynamicArray Memory Safety & Rule of Five Verification
// ----------------------------------------------------------------------------
void testDynamicArrayLifecycle() {
    std::cout << "\n=== TEST SUITE 8: DynamicArray Lifecycle & Memory Safety ===\n";

    // 8.1 Default construction
    DynamicArray<int> defArr;
    recordTest("DynamicArray default construction (size=0, capacity=0, empty=true)",
               defArr.size() == 0 && defArr.capacity() == 0 && defArr.empty());
    recordTest("DynamicArray default begin() and end() both nullptr (no invalid pointer math)",
               defArr.begin() == nullptr && defArr.end() == nullptr);

    // 8.2 Insertion and resizing
    DynamicArray<std::string> strArr;
    strArr.push_back("Alpha");
    strArr.push_back("Beta");
    strArr.push_back("Gamma");
    strArr.push_back("Delta");
    recordTest("DynamicArray basic insertion and size tracking (size=4)",
               strArr.size() == 4 && strArr.capacity() >= 4 && !strArr.empty());
    recordTest("DynamicArray front() and back() access",
               strArr.front() == "Alpha" && strArr.back() == "Delta");

    // 8.3 Self-referential insertion at full capacity (CRITICAL BUG TEST)
    // Make capacity exactly equal to size:
    DynamicArray<std::string> fullArr(4);
    fullArr.push_back("Item_0");
    fullArr.push_back("Item_1");
    fullArr.push_back("Item_2");
    fullArr.push_back("Item_3");
    // fullArr is at capacity 4, size 4
    bool isAtCap = (fullArr.size() == fullArr.capacity());
    // Push an element that is a reference to an element inside fullArr
    fullArr.push_back(fullArr[0]); // Pushing Item_0 by reference
    recordTest("Self-referential push_back(arr[0]) at full capacity without use-after-free",
               isAtCap && fullArr.size() == 5 && fullArr[4] == "Item_0" && fullArr[0] == "Item_0");

    // Push back() by reference when array reaches next capacity boundary
    while (fullArr.size() < fullArr.capacity()) {
        fullArr.push_back("Pad");
    }
    // Now at capacity boundary again
    fullArr.push_back(fullArr.back());
    recordTest("Self-referential push_back(arr.back()) at full capacity boundary",
               fullArr.back() == "Pad");

    // 8.4 Copy construction and deep-copy independence
    DynamicArray<std::string> original;
    original.push_back("Orig_1");
    original.push_back("Orig_2");
    original.push_back("Orig_3");

    DynamicArray<std::string> deepCopy = original;
    recordTest("Copy construction creates equal size and elements",
               deepCopy.size() == original.size() && deepCopy[0] == "Orig_1" && deepCopy[2] == "Orig_3");

    // Mutate the copy and verify original is completely independent
    deepCopy[0] = "Mutated_1";
    deepCopy.push_back("Copy_Only");
    recordTest("Deep-copy independence (mutating copy leaves original untouched)",
               original[0] == "Orig_1" && original.size() == 3 && deepCopy.size() == 4);

    // 8.5 Copy self-assignment
    DynamicArray<std::string>& origRef = original;
    original = origRef; // Self copy-assignment
    recordTest("Self-assignment (copy) safety (size and elements intact)",
               original.size() == 3 && original[0] == "Orig_1" && original[1] == "Orig_2");

    // 8.6 Move construction
    DynamicArray<std::string> donor;
    donor.push_back("D_A");
    donor.push_back("D_B");
    donor.push_back("D_C");

    DynamicArray<std::string> receiver = std::move(donor);
    recordTest("Move constructor transfers elements to destination",
               receiver.size() == 3 && receiver[0] == "D_A" && receiver[2] == "D_C");
    recordTest("Move constructor resets donor to clean empty state (size=0, data=nullptr)",
               donor.size() == 0 && donor.capacity() == 0 && donor.begin() == nullptr && donor.empty());

    // 8.7 Move assignment
    DynamicArray<std::string> moveTarget;
    moveTarget.push_back("Old_1");
    moveTarget.push_back("Old_2");

    DynamicArray<std::string> moveSource;
    moveSource.push_back("New_1");
    moveSource.push_back("New_2");
    moveSource.push_back("New_3");

    moveTarget = std::move(moveSource);
    recordTest("Move assignment replaces existing elements cleanly",
               moveTarget.size() == 3 && moveTarget[0] == "New_1" && moveTarget[2] == "New_3");
    recordTest("Move assignment donor is reset cleanly",
               moveSource.size() == 0 && moveSource.capacity() == 0 && moveSource.begin() == nullptr);

    // 8.8 Self-move assignment
    DynamicArray<std::string>& moveTargetRef = moveTarget;
    moveTarget = std::move(moveTargetRef);
    recordTest("Self-move assignment safety (no crash, elements preserved)",
               moveTarget.size() == 3 && moveTarget[0] == "New_1");

    // 8.9 Empty-array operations
    DynamicArray<int> emptyA;
    emptyA.pop_back(); // Should do nothing safely
    emptyA.erase(0);   // Should do nothing safely
    emptyA.clear();    // Should do nothing safely
    recordTest("Safe operations on empty array (pop_back, erase, clear without crash)",
               emptyA.size() == 0 && emptyA.empty());

    int emptyLoopCount = 0;
    for (const auto& item : emptyA) {
        (void)item;
        emptyLoopCount++;
    }
    recordTest("Range-based for loop on empty array executes 0 iterations safely",
               emptyLoopCount == 0);

    // 8.10 clear() and pop_back() resource cleanup
    DynamicArray<std::string> cleanTest;
    cleanTest.push_back("Str1_Allocated_Resource");
    cleanTest.push_back("Str2_Allocated_Resource");
    cleanTest.push_back("Str3_Allocated_Resource");

    cleanTest.pop_back();
    recordTest("pop_back() decrements size (3 -> 2)", cleanTest.size() == 2 && cleanTest.back() == "Str2_Allocated_Resource");

    cleanTest.clear();
    recordTest("clear() resets size to 0", cleanTest.size() == 0 && cleanTest.empty());

    cleanTest.push_back("ReusedAfterClear");
    recordTest("Reusing array after clear() works cleanly",
               cleanTest.size() == 1 && cleanTest[0] == "ReusedAfterClear");

    // 8.11 Erasing first, middle, and last elements
    DynamicArray<std::string> eraseTest;
    eraseTest.push_back("First");
    eraseTest.push_back("Middle_1");
    eraseTest.push_back("Middle_2");
    eraseTest.push_back("Last");

    // Erase middle: "Middle_1" at index 1
    eraseTest.erase(1);
    recordTest("erase(1) removes middle element (remaining: First, Middle_2, Last)",
               eraseTest.size() == 3 && eraseTest[0] == "First" && eraseTest[1] == "Middle_2" && eraseTest[2] == "Last");

    // Erase first: "First" at index 0
    eraseTest.erase(0);
    recordTest("erase(0) removes first element (remaining: Middle_2, Last)",
               eraseTest.size() == 2 && eraseTest[0] == "Middle_2" && eraseTest[1] == "Last");

    // Erase last: "Last" at index 1
    eraseTest.erase(eraseTest.size() - 1);
    recordTest("erase(last) removes last element (remaining: Middle_2)",
               eraseTest.size() == 1 && eraseTest[0] == "Middle_2");

    // Erase out of bounds: should safely do nothing
    eraseTest.erase(50);
    recordTest("erase(out_of_bounds) safely ignored", eraseTest.size() == 1 && eraseTest[0] == "Middle_2");

    // 8.12 Large-scale repeated resizing with 1,000+ elements
    DynamicArray<int> scaleArr;
    const int targetCount = 1200;
    for (int i = 0; i < targetCount; ++i) {
        scaleArr.push_back(i * 5);
    }
    bool scaleValuesCorrect = true;
    for (int i = 0; i < targetCount; ++i) {
        if (scaleArr[static_cast<size_t>(i)] != i * 5) {
            scaleValuesCorrect = false;
            break;
        }
    }
    recordTest("Large-scale resizing across multiple reallocations (1,200 elements)",
               scaleArr.size() == static_cast<size_t>(targetCount) && scaleArr.capacity() >= static_cast<size_t>(targetCount) && scaleValuesCorrect);

    // 8.13 Destruction of moved-from objects
    bool movedFromScopeSuccess = false;
    {
        DynamicArray<std::string> tempDonor;
        tempDonor.push_back("WillBeMoved_A");
        tempDonor.push_back("WillBeMoved_B");

        DynamicArray<std::string> survivingReceiver = std::move(tempDonor);
        // tempDonor goes out of scope here and its destructor executes
        movedFromScopeSuccess = (survivingReceiver.size() == 2 && survivingReceiver[0] == "WillBeMoved_A");
    }
    recordTest("Destruction of moved-from DynamicArray executes without double-deletion or crash",
               movedFromScopeSuccess);
}

// ----------------------------------------------------------------------------
// Main Execution
// ----------------------------------------------------------------------------
int main() {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;
    std::cout << "=================================================================\n";
    std::cout << "  Product Management & Billing Calculation Module Test Suite   \n";
    std::cout << "=================================================================\n";

    testFileHandling();
    testProductSearching();
    testProductSpecifications();
    testStockManagement();
    testBillingCalculations();
    testInventoryCommitment();
    testGuiReadyData();
    testDynamicArrayLifecycle();

    std::cout << "\n=================================================================\n";
    std::cout << "  TEST RESULTS SUMMARY\n";
    std::cout << "=================================================================\n";
    std::cout << "  Total Tests Run : " << g_testsTotal << "\n";
    std::cout << "  Passed          : " << g_testsPassed << "\n";
    std::cout << "  Failed          : " << g_testsFailed << "\n";
    std::cout << "=================================================================\n";

    if (g_testsFailed == 0) {
        std::cout << "  ALL TESTS PASSED SUCCESSFULLY! (100% PASS RATE)\n";
        return 0;
    } else {
        std::cerr << "  SOME TESTS FAILED! Please check logs above.\n";
        return 1;
    }
}
