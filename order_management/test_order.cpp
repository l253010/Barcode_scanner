#include <iostream>
#include <fstream>
#include "OrderModule.h"

int passed = 0;
int failed = 0;

void check(string name, bool ok) {
    if (ok) {
        passed++;
        cout << "PASS: " << name << endl;
    } else {
        failed++;
        cout << "FAIL: " << name << endl;
    }
}

int countLines(string fileName) {
    ifstream file(fileName.c_str());
    string line;
    int count = 0;
    while (getline(file, line)) {
        if (line.length() > 0) count++;
    }
    return count;
}

int main() {
    clearAllFiles();

    int ids[20];
    bool allEightDigits = true;
    bool allUnique = true;
    for (int i = 0; i < 20; i++) {
        ids[i] = createOrder("Test", "Cash");
        if (ids[i] < 10000000 || ids[i] > 99999999) allEightDigits = false;
        for (int j = 0; j < i; j++) {
            if (ids[i] == ids[j]) allUnique = false;
        }
    }
    check("20 orders have 8-digit ids", allEightDigits);
    check("20 orders have unique ids", allUnique);
    check("Order.txt has 20 lines", countLines("Order.txt") == 20);

    clearAllFiles();

    int order1 = createOrder("Ali Khan", "Cash");
    int order2 = createOrder("Sara Ahmed", "Card");
    int order3 = createOrder("Hina Malik", "Online");
    check("create sample orders", order1 > 0 && order2 > 0 && order3 > 0);
    check("invalid payment method rejected", createOrder("Bad", "Bitcoin") == -1);

    OrderRecord rec;
    check("find order", findOrder(order1, rec));
    check("new order is Pending", rec.status == "Pending");
    check("new order is Unpaid", rec.paymentStatus == "Unpaid");
    check("customer name saved", rec.customerName == "Ali Khan");
    check("unknown order not found", !findOrder(12345678, rec));

    check("add product 1", addOrderDetail(order1, "P101", "123456", 2));
    check("add product 2", addOrderDetail(order1, "P102", "789012", 1));
    check("add same product merges quantity", addOrderDetail(order1, "P101", "123456", 3));
    check("zero quantity rejected", !addOrderDetail(order1, "P103", "555555", 0));
    check("missing order rejected", !addOrderDetail(12345678, "P101", "123456", 1));

    OrderDetailRecord items[MAX_ITEMS_PER_ORDER];
    int count = getOrderDetails(order1, items, MAX_ITEMS_PER_ORDER);
    check("order 1 has 2 detail lines", count == 2);
    check("merged quantity is 5", items[0].quantity == 5);
    check("barcode saved", items[0].barcode == "123456");
    check("OrderDetail.txt has 2 lines", countLines("OrderDetail.txt") == 2);

    check("update quantity", updateDetailQuantity(order1, "P101", 4));
    getOrderDetails(order1, items, MAX_ITEMS_PER_ORDER);
    check("quantity updated to 4", items[0].quantity == 4);

    addOrderDetail(order1, "P103", "555555", 1);
    check("remove product", removeOrderDetail(order1, "P103"));
    check("remove missing product fails", !removeOrderDetail(order1, "P999"));
    check("order 1 back to 2 detail lines", getOrderDetails(order1, items, MAX_ITEMS_PER_ORDER) == 2);

    check("update order total", updateOrderTotal(order1, 1200.50));
    check("cannot complete unpaid order", !updateOrderStatus(order1, "Completed"));
    check("confirm order", updateOrderStatus(order1, "Confirmed"));
    check("invalid status rejected", !updateOrderStatus(order1, "Shipped"));
    check("cannot add items after confirm", !addOrderDetail(order1, "P104", "111111", 1));
    updateOrderStatus(order1, "Pending");
    findOrder(order1, rec);
    check("status updated in file", rec.status == "Pending");
    check("total saved in file", rec.totalAmount > 1200.49 && rec.totalAmount < 1200.51);

    int bill1 = createBill(order1, 1200.00, 100.00, 1500.00, "Cash");
    check("create paid bill", bill1 >= 10000000 && bill1 <= 99999999);
    BillRecord bill;
    check("find bill", findBill(bill1, bill));
    check("bill total is 1100", bill.total > 1099.99 && bill.total < 1100.01);
    check("bill change is 400", bill.changeReturned > 399.99 && bill.changeReturned < 400.01);
    check("bill is Paid", bill.paymentStatus == "Paid");
    check("find bill by order", findBillByOrder(order1, bill) && bill.billId == bill1);
    findOrder(order1, rec);
    check("order became Completed", rec.status == "Completed");
    check("order payment is Paid", rec.paymentStatus == "Paid");
    check("second bill for same order rejected", createBill(order1, 100, 0, 100, "Cash") == -1);
    check("cannot edit completed order", !addOrderDetail(order1, "P105", "222222", 1));
    check("cannot cancel completed order", !cancelOrder(order1));

    addOrderDetail(order2, "P201", "900001", 2);
    addOrderDetail(order2, "P202", "900002", 1);
    int bill2 = createBill(order2, 800.00, 0, 300.00, "Card");
    check("create partial bill", bill2 > 0);
    findBill(bill2, bill);
    check("bill is Partial", bill.paymentStatus == "Partial");
    findOrder(order2, rec);
    check("order is Confirmed", rec.status == "Confirmed");
    check("order payment is Partial", rec.paymentStatus == "Partial");
    check("pay rest of bill", payBill(bill2, 600.00));
    findBill(bill2, bill);
    check("bill now Paid", bill.paymentStatus == "Paid");
    check("change is 100", bill.changeReturned > 99.99 && bill.changeReturned < 100.01);
    findOrder(order2, rec);
    check("order now Completed", rec.status == "Completed");
    check("cannot pay a paid bill", !payBill(bill2, 10));

    check("bill with no items rejected", createBill(order3, 100, 0, 100, "Online") == -1);
    addOrderDetail(order3, "P301", "700001", 1);
    check("discount bigger than subtotal rejected", createBill(order3, 100, 200, 100, "Online") == -1);
    check("cancel order", cancelOrder(order3));
    check("cannot bill cancelled order", createBill(order3, 100, 0, 100, "Online") == -1);
    check("cannot update cancelled order", !updateOrderStatus(order3, "Pending"));

    check("Order.txt has 3 lines", countLines("Order.txt") == 3);
    check("OrderDetail.txt has 5 lines", countLines("OrderDetail.txt") == 5);
    check("Bill.txt has 2 lines", countLines("Bill.txt") == 2);

    cout << endl;
    listAllOrders();
    cout << endl;
    printOrder(order1);
    printBill(bill1);
    printOrder(order2);
    printBill(bill2);

    cout << endl << passed << " passed, " << failed << " failed" << endl;
    if (failed == 0) return 0;
    return 1;
}
