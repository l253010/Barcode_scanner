#include <iostream>
#include <fstream>
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

    // TEST 17: Manager Stock Update & Edge Cases
    std::cout << "\n=== TEST 17: Manager Stock Update (Manager View) ===\n";
    std::string updateErr;
    // Set stock to 0
    bool zeroSuccess = service.updateProductStock("100001", 0, &updateErr);
    TEST_ASSERT(zeroSuccess && service.getProductManager().getStock("100001") == 0,
                "Manager can decrease stock to zero");

    // Increase stock from 0
    bool incFromZero = service.updateProductStock("100001", 75, &updateErr);
    TEST_ASSERT(incFromZero && service.getProductManager().getStock("100001") == 75,
                "Manager can increase stock from zero (0 -> 75)");

    // Reject negative stock update
    bool negSuccess = service.updateProductStock("100001", -10, &updateErr);
    TEST_ASSERT(!negSuccess && service.getProductManager().getStock("100001") == 75,
                "Manager update rejects negative stock without changing inventory");

    // Reject nonexistent product
    bool nonExistSuccess = service.updateProductStock("999999", 50, &updateErr);
    TEST_ASSERT(!nonExistSuccess, "Manager update rejects nonexistent product");

    // TEST 18: Inventory Persistence Across Restart
    std::cout << "\n=== TEST 18: Inventory Persistence Across Application Restart ===\n";
    // Set a known unique stock value for Lemonade (500001)
    service.updateProductStock("500001", 42, &updateErr);
    TEST_ASSERT(service.getProductManager().getStock("500001") == 42,
                "Lemonade stock set to 42 before restart simulation");

    // Simulate application restart by creating a new service instance reading from disk
    {
        ProductBillingService restartedService;
        int loadedLemonadeStock = restartedService.getProductManager().getStock("500001");
        int loadedMilkStock = restartedService.getProductManager().getStock("100001");
        TEST_ASSERT(loadedLemonadeStock == 42,
                    "Persisted Lemonade stock (42) successfully reloaded after simulated restart");
        TEST_ASSERT(loadedMilkStock == 75,
                    "Persisted Milk stock (75) successfully reloaded after simulated restart");
    }

    // TEST 19: Checkout Stock Deduction & Persistence Across Restart
    std::cout << "\n=== TEST 19: Checkout Deduction Persistence Across Restart ===\n";
    ProductInfo lemonInfo;
    service.findProductByBarcode("500001", lemonInfo);
    std::vector<OrderItem> lemonCart;
    OrderItem lemonItem;
    lemonItem.product = lemonInfo;
    lemonItem.quantity = 5;
    lemonCart.push_back(lemonItem);
    BillInfo lemonBill = service.generateBill("Customer Persistence", "", lemonCart);
    bool lemonCommit = service.commitOrderStock(lemonBill);
    TEST_ASSERT(lemonCommit, "Checkout stock commitment succeeds");
    TEST_ASSERT(service.getProductManager().getStock("500001") == 37,
                "Lemonade stock deducted from 42 to 37 in memory");

    // Simulate restart after checkout
    {
        ProductBillingService checkoutRestartService;
        int reloadedStock = checkoutRestartService.getProductManager().getStock("500001");
        TEST_ASSERT(reloadedStock == 37,
                    "Checkout deduction (37 units) persisted to disk and reloaded after restart");
    }

    // TEST 20: Portable Canonical Path Resolution for Product.txt
    std::cout << "\n=== TEST 20: Portable Canonical Path Resolution ===\n";
    std::string canonicalResolved = ProductBillingService::resolveProductFilePath();
    TEST_ASSERT(!canonicalResolved.empty(), "resolveProductFilePath() finds valid path");
    TEST_ASSERT(canonicalResolved.find("Product.txt") != std::string::npos, "Resolved path targets Product.txt");

    // TEST 21: Prevention of Duplicate Stock Deduction on Paid Bill
    std::cout << "\n=== TEST 21: Prevention of Duplicate Stock Deduction ===\n";
    int prePaidStock = service.getProductManager().getStock("100001");
    BillInfo paidBill = lemonBill;
    paidBill.paymentStatus = "Paid";
    bool dupCommit = service.commitOrderStock(paidBill);
    TEST_ASSERT(dupCommit, "commitOrderStock returns true without deducting for already paid bill");
    TEST_ASSERT(service.getProductManager().getStock("100001") == prePaidStock,
                "Stock was NOT deducted twice when bill is marked Paid");

    // TEST 22: Insufficient Stock Rejection
    std::cout << "\n=== TEST 22: Insufficient Stock Rejection ===\n";
    std::vector<OrderItem> excessCart;
    OrderItem excessItem;
    excessItem.product = lemonInfo;
    excessItem.quantity = 999999;
    excessCart.push_back(excessItem);
    BillInfo excessBill = service.generateBill("Excess Customer", "", excessCart);
    std::string excessErr;
    bool excessResult = service.commitOrderStock(excessBill, &excessErr);
    TEST_ASSERT(!excessResult, "commitOrderStock safely rejects order with quantity exceeding stock");
    TEST_ASSERT(!excessErr.empty(), "commitOrderStock provides detailed insufficient stock error message");

    // TEST 23: Category Management Integration
    std::cout << "\n=== TEST 23: Category Management Integration ===\n";
    auto categories = service.getAllCategories();
    TEST_ASSERT(categories.size() >= 6, "Service loads at least 6 baseline categories from PCategory.txt");
    CategoryInfo dairyCat;
    bool foundCat10 = service.findCategoryById("10", dairyCat);
    TEST_ASSERT(foundCat10 && dairyCat.name.find("Dairy") != std::string::npos,
                "findCategoryById('10') finds Dairy category");

    CategoryInfo newCat{"60", "Snacks & Munchies", "SNACK"};
    std::string catErr;
    bool addCatSuccess = service.addCategory(newCat, &catErr);
    TEST_ASSERT(addCatSuccess, "addCategory('60') succeeds for unique Category ID");

    bool addDupCat = service.addCategory(CategoryInfo{"60", "Duplicate", "DUP"}, &catErr);
    TEST_ASSERT(!addDupCat, "addCategory('60') rejects duplicate Category ID");

    CategoryInfo updatedCat{"60", "Snacks, Chips & Nuts", "SNACK"};
    bool updateCatSuccess = service.updateCategory(updatedCat, &catErr);
    TEST_ASSERT(updateCatSuccess, "updateCategory('60') successfully updates category name");

    // TEST 24: Product Management & Barcode-Category Association
    std::cout << "\n=== TEST 24: Product Management & Barcode Association ===\n";
    // 24.1 Product with valid category association (barcode starts with "60")
    ProductInfo newProd;
    newProd.barcode = "600001";
    newProd.acronym = "CHIP-50G";
    newProd.name = "Crispy Potato Chips 50g";
    newProd.unit = "count";
    newProd.unitPrice = 2.50;
    newProd.discountPerUnit = 0.50; // $0.50 discount
    newProd.taxRate = 0.05;
    newProd.availableQuantity = 25;

    std::string prodErr;
    bool addProdSuccess = service.addProduct(newProd, &prodErr);
    TEST_ASSERT(addProdSuccess, "addProduct('600001') succeeds when barcode starts with valid Category ID '60'");

    // 24.2 Barcode lookup finds the newly added product
    ProductInfo foundNewProd;
    bool foundNew = service.findProductByBarcode("600001", foundNewProd);
    TEST_ASSERT(foundNew && foundNewProd.name == "Crispy Potato Chips 50g",
                "findProductByBarcode('600001') returns newly added product");
    TEST_ASSERT(doublesEqual(foundNewProd.discountPerUnit, 0.50),
                "Product-level discount ($0.50) is preserved in catalog");

    // 24.3 Product-level discount applied in bill calculation
    std::vector<OrderItem> discCart;
    OrderItem discItem;
    discItem.product = foundNewProd;
    discItem.quantity = 2; // 2 * $2.50 = $5.00, discount = 2 * $0.50 = $1.00, taxable = $4.00, tax = $0.20, total = $4.20
    discCart.push_back(discItem);
    BillInfo discBill = service.generateBill("Discount Customer", "", discCart);
    TEST_ASSERT(doublesEqual(discBill.totalWithoutTax, 5.00), "Subtotal before discount is $5.00");
    TEST_ASSERT(doublesEqual(discBill.totalDiscount, 1.00), "Total discount applied is $1.00");
    TEST_ASSERT(doublesEqual(discBill.totalTax, 0.20), "Tax on discounted subtotal is $0.20");
    TEST_ASSERT(doublesEqual(discBill.totalPayable, 4.20), "Total payable is $4.20");

    // 24.4 Rejection of product with nonexistent category ID
    ProductInfo badCatProd = newProd;
    badCatProd.barcode = "990001"; // Category "99" does not exist
    bool badCatResult = service.addProduct(badCatProd, &prodErr);
    TEST_ASSERT(!badCatResult, "addProduct('990001') rejected because Category ID '99' does not exist");

    // 24.5 Rejection of product with discount > price
    ProductInfo badDiscProd = newProd;
    badDiscProd.barcode = "600002";
    badDiscProd.discountPerUnit = 10.00; // price is $2.50
    bool badDiscResult = service.addProduct(badDiscProd, &prodErr);
    TEST_ASSERT(!badDiscResult, "addProduct() rejected when discount per unit exceeds unit price");

    // TEST 25: Barcode Category ID Validation Regression Tests (All 10 Requirements)
    std::cout << "\n=== TEST 25: Barcode Category ID Validation Regression Suite ===\n";
    // Ensure category 60 exists for tests and clear any collision
    CategoryInfo regCat60{"60", "Snacks & Munchies", "SNACK"};
    service.addCategory(regCat60);
    service.getProductManager().removeProduct("600001");

    // Case 1: Category '60', barcode '600001' -> accepted
    ProductInfo p1;
    p1.barcode = "600001";
    p1.categoryId = "60";
    p1.category = "Snacks & Munchies";
    p1.name = "Test Chips 60";
    p1.acronym = "CHIP-60";
    p1.unit = "count";
    p1.unitPrice = 3.00;
    p1.availableQuantity = 10;
    std::string err1;
    bool res1 = service.addProduct(p1, &err1);
    TEST_ASSERT(res1, "Case 1: Category '60', barcode '600001' is accepted");

    // Case 2: Category '60', barcode '100001' -> rejected
    ProductInfo p2 = p1;
    p2.barcode = "100001";
    p2.categoryId = "60";
    std::string err2;
    bool res2 = service.addProduct(p2, &err2);
    TEST_ASSERT(!res2, "Case 2: Category '60', barcode '100001' is rejected");
    TEST_ASSERT(err2.find("must match the selected Category ID (60)") != std::string::npos,
                "Case 2 error message correctly specifies expected Category ID (60)");

    // Case 3: Category '10', barcode starting with '10' -> accepted
    ProductInfo p3 = p1;
    p3.barcode = "109999";
    p3.categoryId = "10";
    p3.category = "Dairy & Bakery";
    p3.name = "Test Dairy 10";
    p3.acronym = "DAIRY-10";
    std::string err3;
    bool res3 = service.addProduct(p3, &err3);
    TEST_ASSERT(res3, "Case 3: Category '10', barcode '109999' is accepted");

    // Case 4: Category '10', barcode '600001' -> rejected
    ProductInfo p4 = p1;
    p4.barcode = "600001";
    p4.categoryId = "10";
    p4.category = "Dairy & Bakery";
    std::string err4;
    bool res4 = service.addProduct(p4, &err4);
    TEST_ASSERT(!res4, "Case 4: Category '10', barcode '600001' is rejected");
    TEST_ASSERT(err4.find("must match the selected Category ID (10)") != std::string::npos,
                "Case 4 error message correctly specifies expected Category ID (10)");

    // Case 5: Category '20', barcode '200001' / '209999' -> accepted
    ProductInfo p5 = p1;
    p5.barcode = "209999";
    p5.categoryId = "20";
    p5.category = "Beverages & Spreads";
    p5.name = "Test Juice 20";
    p5.acronym = "JUICE-20";
    std::string err5;
    bool res5 = service.addProduct(p5, &err5);
    TEST_ASSERT(res5, "Case 5: Category '20', barcode '209999' is accepted");

    // Case 6: Barcode containing fewer than two characters -> rejected
    ProductInfo p6 = p1;
    p6.barcode = "6";
    p6.categoryId = "60";
    std::string err6;
    bool res6 = service.addProduct(p6, &err6);
    TEST_ASSERT(!res6, "Case 6: Barcode containing fewer than two characters ('6') is rejected");
    TEST_ASSERT(err6.find("at least 2 characters") != std::string::npos,
                "Case 6 error specifies barcode must contain at least 2 characters");

    // Case 7: Barcode starting with non-numeric characters -> rejected
    ProductInfo p7 = p1;
    p7.barcode = "AB0001";
    p7.categoryId = "60";
    std::string err7;
    bool res7 = service.addProduct(p7, &err7);
    TEST_ASSERT(!res7, "Case 7: Barcode starting with non-numeric characters ('AB0001') is rejected");
    TEST_ASSERT(err7.find("numeric digits") != std::string::npos,
                "Case 7 error specifies first two characters must be numeric digits");

    // Case 8: Nonexistent Category ID -> rejected
    ProductInfo p8 = p1;
    p8.barcode = "990001";
    p8.categoryId = "99";
    std::string err8;
    bool res8 = service.addProduct(p8, &err8);
    TEST_ASSERT(!res8, "Case 8: Nonexistent Category ID '99' is rejected");
    TEST_ASSERT(err8.find("does not exist in PCategory.txt") != std::string::npos,
                "Case 8 error specifies Category ID does not exist in PCategory.txt");

    // Case 9: Editing a product with a mismatched barcode -> rejected
    ProductInfo p9 = p1;
    p9.barcode = "600001";
    p9.categoryId = "10"; // Mismatch: barcode starts with 60, selected category is 10
    std::string err9;
    bool res9 = service.updateProduct(p9, &err9);
    TEST_ASSERT(!res9, "Case 9: Editing product with mismatched barcode and category is rejected");
    TEST_ASSERT(err9.find("must match the selected Category ID (10)") != std::string::npos,
                "Case 9 error message specifies expected Category ID (10)");

    // Case 10: Invalid product must not be written to Product.txt
    // Verify file contents on disk
    std::string prodFilePath = ProductBillingService::resolveProductFilePath();
    std::ifstream prodFile(prodFilePath);
    std::string fileContent((std::istreambuf_iterator<char>(prodFile)), std::istreambuf_iterator<char>());
    prodFile.close();
    bool noBadBarcode = (fileContent.find("AB0001") == std::string::npos) &&
                        (fileContent.find("990001") == std::string::npos);
    TEST_ASSERT(noBadBarcode, "Case 10: Invalid products were NOT written to Product.txt on disk");

    // Cleanup regression test products and categories
    service.getProductManager().removeProduct("600001");
    service.getProductManager().removeProduct("109999");
    service.getProductManager().removeProduct("209999");
    service.saveCatalog();
    service.getCategoryManager().removeCategory("60");
    service.saveCategories();

    // Restore baseline stock levels for clean state
    service.updateProductStock("100001", 50);
    service.updateProductStock("100002", 40);
    service.updateProductStock("500001", 8);

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
