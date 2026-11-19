#include "OrderModule.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <ctime>

const string ORDER_FILE = "Order.txt";
const string DETAIL_FILE = "OrderDetail.txt";
const string BILL_FILE = "Bill.txt";

static OrderRecord orders[MAX_ORDERS];
static int orderCount = 0;

static OrderDetailRecord details[MAX_DETAILS];
static int detailCount = 0;

static BillRecord bills[MAX_BILLS];
static int billCount = 0;

static bool seeded = false;

static string cleanText(string text) {
    for (int i = 0; i < (int)text.length(); i++) {
        if (text[i] == '|' || text[i] == '\n' || text[i] == '\r') {
            text[i] = ' ';
        }
    }
    return text;
}

static string getCurrentDate() {
    time_t now = time(0);
    tm timeInfo;
#ifdef _WIN32
    localtime_s(&timeInfo, &now);
#else
    localtime_r(&now, &timeInfo);
#endif
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeInfo);
    return string(buffer);
}

static bool isValidStatus(string status) {
    return status == "Pending" || status == "Confirmed" || status == "Completed" || status == "Cancelled";
}

static bool isValidPaymentStatus(string status) {
    return status == "Unpaid" || status == "Partial" || status == "Paid" || status == "Refunded";
}

static bool isValidMethod(string method) {
    return method == "Cash" || method == "Card" || method == "Online";
}

static void removeCarriageReturn(string &line) {
    if (line.length() > 0 && line[line.length() - 1] == '\r') {
        line.erase(line.length() - 1);
    }
}

static int randomEightDigit() {
    if (!seeded) {
        srand((unsigned int)time(0));
        seeded = true;
    }
    long long r = (long long)rand() * 32768 + rand();
    return 10000000 + (int)(r % 90000000);
}

static void loadOrders() {
    orderCount = 0;
    ifstream file(ORDER_FILE.c_str());
    string line;
    while (orderCount < MAX_ORDERS && getline(file, line)) {
        removeCarriageReturn(line);
        if (line.length() == 0) continue;
        stringstream ss(line);
        string idText, name, date, status, method, payStatus, totalText;
        getline(ss, idText, '|');
        getline(ss, name, '|');
        getline(ss, date, '|');
        getline(ss, status, '|');
        getline(ss, method, '|');
        getline(ss, payStatus, '|');
        getline(ss, totalText, '|');
        orders[orderCount].orderId = atoi(idText.c_str());
        orders[orderCount].customerName = name;
        orders[orderCount].date = date;
        orders[orderCount].status = status;
        orders[orderCount].paymentMethod = method;
        orders[orderCount].paymentStatus = payStatus;
        orders[orderCount].totalAmount = atof(totalText.c_str());
        orderCount++;
    }
    file.close();
}

static void saveOrders() {
    ofstream file(ORDER_FILE.c_str());
    file << fixed << setprecision(2);
    for (int i = 0; i < orderCount; i++) {
        file << orders[i].orderId << "|"
             << orders[i].customerName << "|"
             << orders[i].date << "|"
             << orders[i].status << "|"
             << orders[i].paymentMethod << "|"
             << orders[i].paymentStatus << "|"
             << orders[i].totalAmount << endl;
    }
    file.close();
}

static void loadDetails() {
    detailCount = 0;
    ifstream file(DETAIL_FILE.c_str());
    string line;
    while (detailCount < MAX_DETAILS && getline(file, line)) {
        removeCarriageReturn(line);
        if (line.length() == 0) continue;
        stringstream ss(line);
        string idText, productId, barcode, quantityText;
        getline(ss, idText, '|');
        getline(ss, productId, '|');
        getline(ss, barcode, '|');
        getline(ss, quantityText, '|');
        details[detailCount].orderId = atoi(idText.c_str());
        details[detailCount].productId = productId;
        details[detailCount].barcode = barcode;
        details[detailCount].quantity = atoi(quantityText.c_str());
        detailCount++;
    }
    file.close();
}

static void saveDetails() {
    ofstream file(DETAIL_FILE.c_str());
    for (int i = 0; i < detailCount; i++) {
        file << details[i].orderId << "|"
             << details[i].productId << "|"
             << details[i].barcode << "|"
             << details[i].quantity << endl;
    }
    file.close();
}

static void loadBills() {
    billCount = 0;
    ifstream file(BILL_FILE.c_str());
    string line;
    while (billCount < MAX_BILLS && getline(file, line)) {
        removeCarriageReturn(line);
        if (line.length() == 0) continue;
        stringstream ss(line);
        string billIdText, orderIdText, date, subtotalText, discountText, totalText;
        string paidText, changeText, method, payStatus;
        getline(ss, billIdText, '|');
        getline(ss, orderIdText, '|');
        getline(ss, date, '|');
        getline(ss, subtotalText, '|');
        getline(ss, discountText, '|');
        getline(ss, totalText, '|');
        getline(ss, paidText, '|');
        getline(ss, changeText, '|');
        getline(ss, method, '|');
        getline(ss, payStatus, '|');
        bills[billCount].billId = atoi(billIdText.c_str());
        bills[billCount].orderId = atoi(orderIdText.c_str());
        bills[billCount].date = date;
        bills[billCount].subtotal = atof(subtotalText.c_str());
        bills[billCount].discount = atof(discountText.c_str());
        bills[billCount].total = atof(totalText.c_str());
        bills[billCount].amountPaid = atof(paidText.c_str());
        bills[billCount].changeReturned = atof(changeText.c_str());
        bills[billCount].paymentMethod = method;
        bills[billCount].paymentStatus = payStatus;
        billCount++;
    }
    file.close();
}

static void saveBills() {
    ofstream file(BILL_FILE.c_str());
    file << fixed << setprecision(2);
    for (int i = 0; i < billCount; i++) {
        file << bills[i].billId << "|"
             << bills[i].orderId << "|"
             << bills[i].date << "|"
             << bills[i].subtotal << "|"
             << bills[i].discount << "|"
             << bills[i].total << "|"
             << bills[i].amountPaid << "|"
             << bills[i].changeReturned << "|"
             << bills[i].paymentMethod << "|"
             << bills[i].paymentStatus << endl;
    }
    file.close();
}

static int findOrderIndex(int orderId) {
    for (int i = 0; i < orderCount; i++) {
        if (orders[i].orderId == orderId) return i;
    }
    return -1;
}

static int findBillIndex(int billId) {
    for (int i = 0; i < billCount; i++) {
        if (bills[i].billId == billId) return i;
    }
    return -1;
}

static int findBillIndexByOrder(int orderId) {
    for (int i = 0; i < billCount; i++) {
        if (bills[i].orderId == orderId) return i;
    }
    return -1;
}

static int findDetailIndex(int orderId, string productId) {
    for (int i = 0; i < detailCount; i++) {
        if (details[i].orderId == orderId && details[i].productId == productId) return i;
    }
    return -1;
}

static int generateOrderId() {
    int id = randomEightDigit();
    while (findOrderIndex(id) != -1) {
        id = randomEightDigit();
    }
    return id;
}

static int generateBillId() {
    int id = randomEightDigit();
    while (findBillIndex(id) != -1) {
        id = randomEightDigit();
    }
    return id;
}

static void applyPaymentToBillAndOrder(int billIndex) {
    double total = bills[billIndex].total;
    double paid = bills[billIndex].amountPaid;

    if (paid + 0.001 >= total) {
        bills[billIndex].paymentStatus = "Paid";
        bills[billIndex].changeReturned = paid - total;
    } else if (paid > 0) {
        bills[billIndex].paymentStatus = "Partial";
        bills[billIndex].changeReturned = 0;
    } else {
        bills[billIndex].paymentStatus = "Unpaid";
        bills[billIndex].changeReturned = 0;
    }

    loadOrders();
    int orderIndex = findOrderIndex(bills[billIndex].orderId);
    if (orderIndex != -1) {
        orders[orderIndex].totalAmount = total;
        orders[orderIndex].paymentMethod = bills[billIndex].paymentMethod;
        orders[orderIndex].paymentStatus = bills[billIndex].paymentStatus;
        if (bills[billIndex].paymentStatus == "Paid") {
            orders[orderIndex].status = "Completed";
        } else {
            orders[orderIndex].status = "Confirmed";
        }
        saveOrders();
    }
}

int createOrder(string customerName, string paymentMethod) {
    if (!isValidMethod(paymentMethod)) return -1;
    if (customerName == "") customerName = "Walk-in";

    loadOrders();
    if (orderCount >= MAX_ORDERS) return -1;

    int id = generateOrderId();
    orders[orderCount].orderId = id;
    orders[orderCount].customerName = cleanText(customerName);
    orders[orderCount].date = getCurrentDate();
    orders[orderCount].status = "Pending";
    orders[orderCount].paymentMethod = paymentMethod;
    orders[orderCount].paymentStatus = "Unpaid";
    orders[orderCount].totalAmount = 0;
    orderCount++;
    saveOrders();
    return id;
}

bool findOrder(int orderId, OrderRecord &result) {
    loadOrders();
    int index = findOrderIndex(orderId);
    if (index == -1) return false;
    result = orders[index];
    return true;
}

bool updateOrderStatus(int orderId, string status) {
    if (!isValidStatus(status)) return false;

    loadOrders();
    int index = findOrderIndex(orderId);
    if (index == -1) return false;

    if (orders[index].status == "Completed" || orders[index].status == "Cancelled") return false;
    if (status == "Completed" && orders[index].paymentStatus != "Paid") return false;

    orders[index].status = status;
    saveOrders();
    return true;
}

bool updatePayment(int orderId, string paymentMethod, string paymentStatus) {
    if (!isValidMethod(paymentMethod)) return false;
    if (!isValidPaymentStatus(paymentStatus)) return false;

    loadOrders();
    int index = findOrderIndex(orderId);
    if (index == -1) return false;
    if (orders[index].status == "Cancelled") return false;

    orders[index].paymentMethod = paymentMethod;
    orders[index].paymentStatus = paymentStatus;
    saveOrders();
    return true;
}

bool updateOrderTotal(int orderId, double total) {
    if (total < 0) return false;

    loadOrders();
    int index = findOrderIndex(orderId);
    if (index == -1) return false;
    if (orders[index].status == "Completed" || orders[index].status == "Cancelled") return false;

    orders[index].totalAmount = total;
    saveOrders();
    return true;
}

bool cancelOrder(int orderId) {
    return updateOrderStatus(orderId, "Cancelled");
}

bool addOrderDetail(int orderId, string productId, string barcode, int quantity) {
    if (quantity <= 0) return false;
    if (productId == "" || barcode == "") return false;
    productId = cleanText(productId);
    barcode = cleanText(barcode);

    loadOrders();
    int orderIndex = findOrderIndex(orderId);
    if (orderIndex == -1) return false;
    if (orders[orderIndex].status != "Pending") return false;

    loadDetails();
    int detailIndex = findDetailIndex(orderId, productId);
    if (detailIndex != -1) {
        details[detailIndex].quantity = details[detailIndex].quantity + quantity;
    } else {
        if (detailCount >= MAX_DETAILS) return false;
        details[detailCount].orderId = orderId;
        details[detailCount].productId = productId;
        details[detailCount].barcode = barcode;
        details[detailCount].quantity = quantity;
        detailCount++;
    }
    saveDetails();
    return true;
}

bool updateDetailQuantity(int orderId, string productId, int quantity) {
    if (quantity <= 0) return false;

    loadOrders();
    int orderIndex = findOrderIndex(orderId);
    if (orderIndex == -1) return false;
    if (orders[orderIndex].status != "Pending") return false;

    loadDetails();
    int detailIndex = findDetailIndex(orderId, productId);
    if (detailIndex == -1) return false;

    details[detailIndex].quantity = quantity;
    saveDetails();
    return true;
}

bool removeOrderDetail(int orderId, string productId) {
    loadOrders();
    int orderIndex = findOrderIndex(orderId);
    if (orderIndex == -1) return false;
    if (orders[orderIndex].status != "Pending") return false;

    loadDetails();
    int detailIndex = findDetailIndex(orderId, productId);
    if (detailIndex == -1) return false;

    for (int i = detailIndex; i < detailCount - 1; i++) {
        details[i] = details[i + 1];
    }
    detailCount--;
    saveDetails();
    return true;
}

int getOrderDetails(int orderId, OrderDetailRecord result[], int maxCount) {
    loadDetails();
    int found = 0;
    for (int i = 0; i < detailCount; i++) {
        if (details[i].orderId == orderId && found < maxCount) {
            result[found] = details[i];
            found++;
        }
    }
    return found;
}

int createBill(int orderId, double subtotal, double discount, double amountPaid, string paymentMethod) {
    if (!isValidMethod(paymentMethod)) return -1;
    if (subtotal < 0 || discount < 0 || discount > subtotal || amountPaid < 0) return -1;

    loadOrders();
    int orderIndex = findOrderIndex(orderId);
    if (orderIndex == -1) return -1;
    if (orders[orderIndex].status == "Cancelled" || orders[orderIndex].status == "Completed") return -1;

    OrderDetailRecord items[MAX_ITEMS_PER_ORDER];
    if (getOrderDetails(orderId, items, MAX_ITEMS_PER_ORDER) == 0) return -1;

    loadBills();
    if (findBillIndexByOrder(orderId) != -1) return -1;
    if (billCount >= MAX_BILLS) return -1;

    int billId = generateBillId();
    int index = billCount;
    bills[index].billId = billId;
    bills[index].orderId = orderId;
    bills[index].date = getCurrentDate();
    bills[index].subtotal = subtotal;
    bills[index].discount = discount;
    bills[index].total = subtotal - discount;
    bills[index].amountPaid = amountPaid;
    bills[index].changeReturned = 0;
    bills[index].paymentMethod = paymentMethod;
    bills[index].paymentStatus = "Unpaid";
    billCount++;

    applyPaymentToBillAndOrder(index);
    saveBills();
    return billId;
}

bool payBill(int billId, double amount) {
    if (amount <= 0) return false;

    loadBills();
    int index = findBillIndex(billId);
    if (index == -1) return false;
    if (bills[index].paymentStatus == "Paid") return false;

    bills[index].amountPaid = bills[index].amountPaid + amount;
    applyPaymentToBillAndOrder(index);
    saveBills();
    return true;
}

bool findBill(int billId, BillRecord &result) {
    loadBills();
    int index = findBillIndex(billId);
    if (index == -1) return false;
    result = bills[index];
    return true;
}

bool findBillByOrder(int orderId, BillRecord &result) {
    loadBills();
    int index = findBillIndexByOrder(orderId);
    if (index == -1) return false;
    result = bills[index];
    return true;
}

void printOrder(int orderId) {
    OrderRecord order;
    if (!findOrder(orderId, order)) {
        cout << "Order " << orderId << " not found" << endl;
        return;
    }

    cout << "-------------------------------------" << endl;
    cout << "Order ID       : " << order.orderId << endl;
    cout << "Customer       : " << order.customerName << endl;
    cout << "Date           : " << order.date << endl;
    cout << "Status         : " << order.status << endl;
    cout << "Payment Method : " << order.paymentMethod << endl;
    cout << "Payment Status : " << order.paymentStatus << endl;
    cout << fixed << setprecision(2);
    cout << "Total Amount   : " << order.totalAmount << endl;
    cout << "Items:" << endl;

    OrderDetailRecord items[MAX_ITEMS_PER_ORDER];
    int count = getOrderDetails(orderId, items, MAX_ITEMS_PER_ORDER);
    for (int i = 0; i < count; i++) {
        cout << "  Product " << items[i].productId
             << "  Barcode " << items[i].barcode
             << "  Qty " << items[i].quantity << endl;
    }
    cout << "-------------------------------------" << endl;
}

void printBill(int billId) {
    BillRecord bill;
    if (!findBill(billId, bill)) {
        cout << "Bill " << billId << " not found" << endl;
        return;
    }

    cout << "=====================================" << endl;
    cout << "Bill ID        : " << bill.billId << endl;
    cout << "Order ID       : " << bill.orderId << endl;
    cout << "Date           : " << bill.date << endl;
    cout << fixed << setprecision(2);
    cout << "Subtotal       : " << bill.subtotal << endl;
    cout << "Discount       : " << bill.discount << endl;
    cout << "Total          : " << bill.total << endl;
    cout << "Amount Paid    : " << bill.amountPaid << endl;
    cout << "Change         : " << bill.changeReturned << endl;
    cout << "Payment Method : " << bill.paymentMethod << endl;
    cout << "Payment Status : " << bill.paymentStatus << endl;
    cout << "=====================================" << endl;
}

void listAllOrders() {
    loadOrders();
    cout << "All orders: " << orderCount << endl;
    for (int i = 0; i < orderCount; i++) {
        cout << orders[i].orderId << "  "
             << orders[i].customerName << "  "
             << orders[i].status << "  "
             << orders[i].paymentStatus << endl;
    }
}

void clearAllFiles() {
    ofstream orderFile(ORDER_FILE.c_str());
    orderFile.close();
    ofstream detailFile(DETAIL_FILE.c_str());
    detailFile.close();
    ofstream billFile(BILL_FILE.c_str());
    billFile.close();
}
