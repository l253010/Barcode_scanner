#ifndef ORDER_MODULE_H
#define ORDER_MODULE_H

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

struct OrderItem {
    string productId;
    int quantity;
    double unitPrice;
    double subtotal;
};

struct Order {
    string orderId;
    string customerName;
    string status; // "HELD", "COMPLETED", "ARCHIVED", "ACTIVE"
    double totalAmount;
    string date;
    
    OrderItem* items;
    int itemCount;
    int itemCapacity;

    Order() {
        itemCount = 0;
        itemCapacity = 5;
        items = new OrderItem[itemCapacity];
        totalAmount = 0.0;
    }

    // Copy constructor
    Order(const Order& other) {
        orderId = other.orderId;
        customerName = other.customerName;
        status = other.status;
        totalAmount = other.totalAmount;
        date = other.date;
        itemCount = other.itemCount;
        itemCapacity = other.itemCapacity;
        items = new OrderItem[itemCapacity];
        for (int i = 0; i < itemCount; ++i) {
            items[i] = other.items[i];
        }
    }

    // Assignment operator
    Order& operator=(const Order& other) {
        if (this == &other) return *this;
        delete[] items;
        orderId = other.orderId;
        customerName = other.customerName;
        status = other.status;
        totalAmount = other.totalAmount;
        date = other.date;
        itemCount = other.itemCount;
        itemCapacity = other.itemCapacity;
        items = new OrderItem[itemCapacity];
        for (int i = 0; i < itemCount; ++i) {
            items[i] = other.items[i];
        }
        return *this;
    }

    ~Order() {
        delete[] items;
    }

    void addItem(const OrderItem& item) {
        if (itemCount >= itemCapacity) {
            int newCapacity = itemCapacity * 2;
            OrderItem* newItems = new OrderItem[newCapacity];
            for (int i = 0; i < itemCount; ++i) {
                newItems[i] = items[i];
            }
            delete[] items;
            items = newItems;
            itemCapacity = newCapacity;
        }
        items[itemCount++] = item;
    }
};

class OrderModule {
private:
    string orderFile;
    string orderDetailFile;

public:
    OrderModule(const string& oFile = "Order.txt", const string& odFile = "OrderDetail.txt");

    bool createOrder(const Order& order);
    Order getOrder(const string& orderId);
    bool updateOrder(const Order& order);
    bool holdOrder(const string& orderId);
    bool resumeOrder(const string& orderId);
    bool completeAndPayOrder(const string& orderId);
    bool archiveOrder(const string& orderId);
    bool editPaidOrderWithApproval(const string& orderId, OrderItem* newItems, int newCount, bool managerApproved, int daysElapsed);

    void generateIncompleteOrderReport() const;
    void generateCompletedOrderReport() const;
};

#endif
