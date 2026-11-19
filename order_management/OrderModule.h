#ifndef ORDERMODULE_H
#define ORDERMODULE_H

#include <string>
using namespace std;

const int MAX_ORDERS = 1000;
const int MAX_DETAILS = 5000;
const int MAX_BILLS = 1000;
const int MAX_ITEMS_PER_ORDER = 100;

struct OrderRecord {
    int orderId;
    string customerName;
    string date;
    string status;
    string paymentMethod;
    string paymentStatus;
    double totalAmount;
};

struct OrderDetailRecord {
    int orderId;
    string productId;
    string barcode;
    int quantity;
};

struct BillRecord {
    int billId;
    int orderId;
    string date;
    double subtotal;
    double discount;
    double total;
    double amountPaid;
    double changeReturned;
    string paymentMethod;
    string paymentStatus;
};

int createOrder(string customerName, string paymentMethod);
bool findOrder(int orderId, OrderRecord &result);
bool updateOrderStatus(int orderId, string status);
bool updatePayment(int orderId, string paymentMethod, string paymentStatus);
bool updateOrderTotal(int orderId, double total);
bool cancelOrder(int orderId);

bool addOrderDetail(int orderId, string productId, string barcode, int quantity);
bool updateDetailQuantity(int orderId, string productId, int quantity);
bool removeOrderDetail(int orderId, string productId);
int getOrderDetails(int orderId, OrderDetailRecord result[], int maxCount);

int createBill(int orderId, double subtotal, double discount, double amountPaid, string paymentMethod);
bool payBill(int billId, double amount);
bool findBill(int billId, BillRecord &result);
bool findBillByOrder(int orderId, BillRecord &result);

void printOrder(int orderId);
void printBill(int billId);
void listAllOrders();
void clearAllFiles();

#endif
