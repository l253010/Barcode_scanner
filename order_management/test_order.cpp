#include "OrderModule.h"
#include <cassert>

int main() {
    cout << "Running Order Management Unit Tests...\n";
    
    OrderModule orderMod("TestOrder.txt", "TestOrderDetail.txt");
    
    Order testOrder;
    testOrder.orderId = "999999001";
    testOrder.customerName = "Javeria";
    testOrder.status = "HELD";
    testOrder.date = "2026-11-25";
    
    OrderItem item;
    item.productId = "100001";
    item.quantity = 1;
    item.unitPrice = 500.0;
    item.subtotal = 500.0;
    testOrder.addItem(item);
    testOrder.totalAmount = 500.0;

    assert(orderMod.createOrder(testOrder) == true);
    
    Order fetched = orderMod.getOrder("999999001");
    assert(fetched.orderId == "999999001");
    assert(fetched.status == "HELD");

    assert(orderMod.completeAndPayOrder("999999001") == true);
    
    cout << "All Order Management tests passed successfully!\n";
    return 0;
}
