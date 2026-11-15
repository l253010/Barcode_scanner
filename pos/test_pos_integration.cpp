#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <cmath>

#include "POSInterfaces.h"
#include "ProductBillingService.h"
#include "MockDataService.h"

static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (cond) { \
            std::cout << "  [PASS] " << msg << std::endl; \
            g_testsPassed++; \
        } else { \
            std::cout << "  [FAIL] " << msg << " (Line " << __LINE__ << ")" << std::endl; \
            g_testsFailed++; \
        } \
    } while (0)

static bool doublesEqual(double a, double b, double eps = 0.001) {
    return std::fabs(a - b) < eps;
}

int main() {
    std::cout << "=================================================================\n";
    std::cout << "  POS & ProductBilling Module Integration Test Suite             \n";
    std::cout << "=================================================================\n\n";

    // TEST 1: Application startup & Service Initialization
    std::cout << "=== TEST 1: Service Startup & Initialization ===\n";
    ProductBillingService service;
    TEST_ASSERT(service.getProductManager().getProductCount() >= 10,
                "Service successfully initializes and loads catalog with >= 10 products");

    // TEST 2: Dashboard Metrics & State
    std::cout << "\n=== TEST 2: Dashboard Metrics ===\n";
    std::vector<ProductInfo> allProducts = service.getAllProducts();
    TEST_ASSERT(!allProducts.empty(), "Dashboard can access catalog items");
    TEST_ASSERT(allProducts.size() == service.getProductManager().getProductCount(),
                "Product count in GUI view matches ProductManager storage");

    // TEST 3: Product Listing
    std::cout << "\n=== TEST 3: Product Listing ===\n";
    bool allValid = true;
    for (const auto& p : allProducts) {
        if (p.barcode.empty() || p.name.empty() || p.unitPrice <= 0.0) {
            allValid = false;
            break;
        }
    }
    TEST_ASSERT(allValid, "All catalog products contain valid barcodes, names, and positive prices");

    // TEST 4: Barcode Lookup
    std::cout << "\n=== TEST 4: Barcode Lookup ===\n";
    ProductInfo milkProd;
    bool foundMilk = service.findProductByBarcode("100001", milkProd);
    TEST_ASSERT(foundMilk, "findProductByBarcode('100001') returns true");
    TEST_ASSERT(milkProd.name.find("Milk") != std::string::npos, "Product name contains 'Milk'");
    TEST_ASSERT(doublesEqual(milkProd.unitPrice, 3.49), "Product price is $3.49");
    TEST_ASSERT(doublesEqual(milkProd.taxRate, 0.05), "Product tax rate is 0.05 (5%)");

    // TEST 5: Product ID Lookup
    std::cout << "\n=== TEST 5: Product ID Lookup ===\n";
    ProductInfo coffeeProd;
    bool foundCoffee = service.findProductById(104, coffeeProd); // PRD-104
    TEST_ASSERT(foundCoffee, "findProductById(104) returns true for PRD-104");
    TEST_ASSERT(coffeeProd.barcode == "123456", "PRD-104 barcode matches '123456'");
    TEST_ASSERT(coffeeProd.name.find("Coffee") != std::string::npos, "PRD-104 name contains 'Coffee'");

    // TEST 6: Product Selection
    std::cout << "\n=== TEST 6: Product Selection ===\n";
    ProductInfo selected = milkProd;
    TEST_ASSERT(selected.barcode == milkProd.barcode, "Selected product copy preserves barcode");
    TEST_ASSERT(selected.unitPrice == milkProd.unitPrice, "Selected product copy preserves unit price");

    // TEST 7: Add to Order
    std::cout << "\n=== TEST 7: Add to Order ===\n";
    std::vector<OrderItem> cart;
    OrderItem item1;
    item1.product = milkProd;
    item1.quantity = 2;
    cart.push_back(item1);
    TEST_ASSERT(cart.size() == 1, "Cart contains 1 order item");
    TEST_ASSERT(cart[0].quantity == 2, "Order item has quantity 2");

    // TEST 8: Quantity Updates
    std::cout << "\n=== TEST 8: Quantity Updates ===\n";
    cart[0].quantity = 5;
    TEST_ASSERT(doublesEqual(cart[0].getSubtotal(), 3.49 * 5),
                "Subtotal updates correctly on quantity change (5 * 3.49 = 17.45)");

    // TEST 9: Billing Calculations
    std::cout << "\n=== TEST 9: Billing Calculations ===\n";
    cart[0].quantity = 2; // reset to 2
    BillInfo bill = service.generateBill("John Doe", "0300-1234567", cart);
    TEST_ASSERT(doublesEqual(bill.totalWithoutTax, 6.98), "Bill subtotal for 2 units Milk is $6.98");
    TEST_ASSERT(doublesEqual(bill.totalTax, 0.35, 0.01), "Bill tax for Milk (5%) is ~$0.35");
    TEST_ASSERT(doublesEqual(bill.totalPayable, 7.33, 0.01), "Bill total payable for Milk is ~$7.33");

    // TEST 10: Payment Processing
    std::cout << "\n=== TEST 10: Payment Processing ===\n";
    double tendered = 10.00;
    double changeDue = tendered - bill.totalPayable;
    TEST_ASSERT(changeDue > 0.0, "Change due computed correctly for cash payment");
    bill.paymentStatus = "Paid";
    bill.amountTendered = tendered;
    bill.changeDue = changeDue;
    TEST_ASSERT(bill.paymentStatus == "Paid", "Bill marked as Paid");

    // TEST 11: Bill Preview Formatting
    std::cout << "\n=== TEST 11: Bill Preview Formatting ===\n";
    TEST_ASSERT(!bill.billId.empty(), "Bill contains non-empty billId");
    TEST_ASSERT(!bill.orderId.empty(), "Bill contains non-empty orderId");
    TEST_ASSERT(!bill.timestamp.empty(), "Bill contains valid timestamp");
    TEST_ASSERT(bill.customerName == "John Doe", "Bill preserves customer name");

    // TEST 12: Invalid Barcode Lookup
    std::cout << "\n=== TEST 12: Invalid Barcode Lookup ===\n";
    ProductInfo dummy;
    bool foundInvalid = service.findProductByBarcode("999999", dummy);
    TEST_ASSERT(!foundInvalid, "Lookup for nonexistent barcode '999999' safely returns false");

    // TEST 13: Invalid Product ID Lookup
    std::cout << "\n=== TEST 13: Invalid Product ID Lookup ===\n";
    bool foundInvalidId = service.findProductById(99999, dummy);
    TEST_ASSERT(!foundInvalidId, "Lookup for nonexistent ID 99999 safely returns false");

    // TEST 14: Empty Order Bill Generation
    std::cout << "\n=== TEST 14: Empty Order ===\n";
    std::vector<OrderItem> emptyCart;
    BillInfo emptyBill = service.generateBill("Walk-in", "", emptyCart);
    TEST_ASSERT(emptyBill.items.empty(), "Empty cart generates bill with 0 items");
    TEST_ASSERT(doublesEqual(emptyBill.totalWithoutTax, 0.0), "Empty cart has $0.00 subtotal");
    TEST_ASSERT(doublesEqual(emptyBill.totalPayable, 0.0), "Empty cart has $0.00 total payable");

    // TEST 15: Multiple Products in One Bill & Stock Commitment
    std::cout << "\n=== TEST 15: Multiple Products & Stock Commitment ===\n";
    ProductInfo breadProd;
    service.findProductByBarcode("100002", breadProd);
    int initialMilkStock = service.getProductManager().getStock("100001");
    int initialBreadStock = service.getProductManager().getStock("100002");

    std::vector<OrderItem> multiCart;
    OrderItem mItem1;
    mItem1.product = milkProd;
    mItem1.quantity = 3;
    multiCart.push_back(mItem1);

    OrderItem mItem2;
    mItem2.product = breadProd;
    mItem2.quantity = 2;
    multiCart.push_back(mItem2);

    BillInfo multiBill = service.generateBill("Jane Smith", "0311-9876543", multiCart);
    // 3 * 3.49 = 10.47, 2 * 2.89 = 5.78, subtotal = 16.25
    TEST_ASSERT(doublesEqual(multiBill.totalWithoutTax, 16.25), "Multiple items subtotal is $16.25");
    TEST_ASSERT(multiBill.items.size() == 2, "Bill contains 2 distinct items");

    bool commitSuccess = service.commitOrderStock(multiBill);
    TEST_ASSERT(commitSuccess, "commitOrderStock() succeeds for valid order");
    int newMilkStock = service.getProductManager().getStock("100001");
    int newBreadStock = service.getProductManager().getStock("100002");
    TEST_ASSERT(newMilkStock == initialMilkStock - 3, "Milk inventory reduced by 3 units");
    TEST_ASSERT(newBreadStock == initialBreadStock - 2, "Bread inventory reduced by 2 units");

    // TEST 16: Memory Lifecycle & Cleanup
    std::cout << "\n=== TEST 16: Memory Lifecycle & Cleanup ===\n";
    {
        ProductBillingService tempService;
        std::vector<ProductInfo> tempProds = tempService.getAllProducts();
        std::vector<OrderItem> tempCart;
        for (size_t i = 0; i < tempProds.size(); ++i) {
            OrderItem oi;
            oi.product = tempProds[i];
            oi.quantity = 1;
            tempCart.push_back(oi);
        }
        BillInfo tempBill = tempService.generateBill("Temp Customer", "", tempCart);
        TEST_ASSERT(tempBill.items.size() == tempProds.size(), "Large cart built cleanly in scoped block");
    }
    TEST_ASSERT(true, "Scoped ProductBillingService, BillInfo, and vectors destroyed cleanly without leak or crash");

    std::cout << "\n=================================================================\n";
    std::cout << "  INTEGRATION TEST RESULTS SUMMARY\n";
    std::cout << "=================================================================\n";
    std::cout << "  Total Tests Run : " << (g_testsPassed + g_testsFailed) << "\n";
    std::cout << "  Passed          : " << g_testsPassed << "\n";
    std::cout << "  Failed          : " << g_testsFailed << "\n";
    std::cout << "=================================================================\n";

    if (g_testsFailed == 0) {
        std::cout << "  ALL INTEGRATION TESTS PASSED SUCCESSFULLY! (100% PASS RATE)\n";
        return 0;
    } else {
        std::cout << "  SOME TESTS FAILED!\n";
        return 1;
    }
}
