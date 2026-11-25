#include "OrderModule.h"

OrderModule::OrderModule(const string& oFile, const string& odFile)
    : orderFile(oFile), orderDetailFile(odFile) {}

bool OrderModule::createOrder(const Order& order) {
    ofstream outFile(orderFile, ios::app);
    if (!outFile.is_open()) return false;

    outFile << order.orderId << "," << order.customerName << "," << order.status << "," 
            << order.totalAmount << "," << order.date << "\n";
    outFile.close();

    ofstream detailFile(orderDetailFile, ios::app);
    if (!detailFile.is_open()) return false;

    for (int i = 0; i < order.itemCount; ++i) {
        detailFile << order.orderId << "," << order.items[i].productId << "," << order.items[i].quantity << "," 
                   << order.items[i].unitPrice << "," << order.items[i].subtotal << "\n";
    }
    detailFile.close();
    return true;
}

Order OrderModule::getOrder(const string& targetOrderId) {
    Order order;
    order.orderId = "";
    
    ifstream inFile(orderFile);
    string line;
    while (getline(inFile, line)) {
        stringstream ss(line);
        string id, name, status, amountStr, date;
        if (getline(ss, id, ',') && getline(ss, name, ',') &&
            getline(ss, status, ',') && getline(ss, amountStr, ',') &&
            getline(ss, date, ',')) {
            if (id == targetOrderId) {
                order.orderId = id;
                order.customerName = name;
                order.status = status;
                order.totalAmount = stod(amountStr);
                order.date = date;
                break;
            }
        }
    }
    inFile.close();

    if (order.orderId.empty()) return order;

    ifstream detailIn(orderDetailFile);
    while (getline(detailIn, line)) {
        stringstream ss(line);
        string oId, pId, qtyStr, priceStr, subStr;
        if (getline(ss, oId, ',') && getline(ss, pId, ',') &&
            getline(ss, qtyStr, ',') && getline(ss, priceStr, ',') &&
            getline(ss, subStr, ',')) {
            if (oId == targetOrderId) {
                OrderItem item;
                item.productId = pId;
                item.quantity = stoi(qtyStr);
                item.unitPrice = stod(priceStr);
                item.subtotal = stod(subStr);
                order.addItem(item);
            }
        }
    }
    detailIn.close();
    return order;
}

bool OrderModule::holdOrder(const string& orderId) {
    Order ord = getOrder(orderId);
    if (ord.orderId.empty()) return false;
    ord.status = "HELD";
    return updateOrder(ord);
}

bool OrderModule::resumeOrder(const string& orderId) {
    Order ord = getOrder(orderId);
    if (ord.orderId.empty()) return false;
    ord.status = "ACTIVE";
    return updateOrder(ord);
}

bool OrderModule::completeAndPayOrder(const string& orderId) {
    Order ord = getOrder(orderId);
    if (ord.orderId.empty()) return false;
    ord.status = "COMPLETED";
    return updateOrder(ord);
}

bool OrderModule::archiveOrder(const string& orderId) {
    Order ord = getOrder(orderId);
    if (ord.orderId.empty()) return false;
    ord.status = "ARCHIVED";
    return updateOrder(ord);
}

bool OrderModule::editPaidOrderWithApproval(const string& orderId, OrderItem* newItems, int newCount, bool managerApproved, int daysElapsed) {
    if (daysElapsed > 7 || !managerApproved) {
        cout << "[ERROR] Paid order editing requires manager approval and must be within 7 days.\n";
        return false;
    }
    Order ord = getOrder(orderId);
    if (ord.orderId.empty()) return false;
    
    delete[] ord.items;
    ord.itemCount = newCount;
    ord.itemCapacity = newCount > 0 ? newCount : 1;
    ord.items = new OrderItem[ord.itemCapacity];
    double newTotal = 0;
    for (int i = 0; i < newCount; ++i) {
        ord.items[i] = newItems[i];
        newTotal += newItems[i].subtotal;
    }
    ord.totalAmount = newTotal;
    return updateOrder(ord);
}

bool OrderModule::updateOrder(const Order& updatedOrder) {
    return true; 
}

void OrderModule::generateIncompleteOrderReport() const {
    cout << "=== INCOMPLETE / HELD ORDERS REPORT ===\n";
    ifstream inFile(orderFile);
    string line;
    while (getline(inFile, line)) {
        if (line.find("HELD") != string::npos || line.find("ACTIVE") != string::npos) {
            cout << line << "\n";
        }
    }
}

void OrderModule::generateCompletedOrderReport() const {
    cout << "=== COMPLETED ORDERS REPORT ===\n";
    ifstream inFile(orderFile);
    string line;
    while (getline(inFile, line)) {
        if (line.find("COMPLETED") != string::npos) {
            cout << line << "\n";
        }
    }
}
