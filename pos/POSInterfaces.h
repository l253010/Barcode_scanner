#pragma once

#include <string>
#include <vector>
#include <memory>

// ============================================================================
// Shared Data Structures for Retail POS System
// ============================================================================

// ----------------------------------------------------------------------------
// Product Information Structure
// Represents a single catalog item in the retail POS system.
// ----------------------------------------------------------------------------
struct ProductInfo {
    int         id{0};                 // Unique numeric product ID
    std::string barcode;               // Validated 6-digit barcode (Code 39)
    std::string acronym;               // Short product code (e.g., "MILK-1L")
    std::string name;                  // Full product name (e.g., "Fresh Whole Milk 1L")
    std::string category;              // Product category (e.g., "Dairy & Milk")
    std::string unit;                  // Unit of measurement (e.g., "Bottle", "Kg", "Loaf")
    double      unitPrice{0.0};        // Price per unit in PKR (Rs.)
    double      taxRate{0.0};          // Tax rate (e.g., 0.05 for 5%)
    double      discountPerUnit{0.0};  // Discount per unit in PKR (Rs.)
    int         availableQuantity{0};  // Inventory stock count
};

// ----------------------------------------------------------------------------
// Order Item Structure
// Represents a product added to the active customer cart.
// ----------------------------------------------------------------------------
struct OrderItem {
    ProductInfo product;
    int         quantity{1};

    double getSubtotal() const {
        return product.unitPrice * quantity;
    }

    double getTotalDiscount() const {
        return product.discountPerUnit * quantity;
    }

    double getTotalTax() const {
        double taxableAmount = getSubtotal() - getTotalDiscount();
        if (taxableAmount < 0.0) taxableAmount = 0.0;
        return taxableAmount * product.taxRate;
    }

    double getItemTotal() const {
        return (getSubtotal() - getTotalDiscount()) + getTotalTax();
    }
};

// ----------------------------------------------------------------------------
// Bill / Invoice Information Structure
// Complete billing and payment record.
// ----------------------------------------------------------------------------
struct BillInfo {
    std::string            billId;             // Unique Bill ID (e.g., "BILL-2026-0001")
    std::string            orderId;            // Associated Order ID (e.g., "ORD-1001")
    std::string            customerName;       // Customer name
    std::string            customerContact;    // Optional phone or email
    std::string            timestamp;          // Creation date and time
    std::vector<OrderItem> items;              // Items purchased

    double                 totalWithoutTax{0.0}; // Subtotal before tax and discount
    double                 totalTax{0.0};        // Total tax amount
    double                 totalDiscount{0.0};   // Total discount applied
    double                 totalPayable{0.0};    // Final payable amount

    std::string            paymentStatus{"Pending"}; // "Pending", "Paid", "Cancelled"
    std::string            paymentMode{"Cash"};      // "Cash", "Card", "Wallet"
    std::string            paymentMerchant{"N/A"};   // Merchant or terminal name
    double                 amountTendered{0.0};      // Cash amount received from customer
    double                 changeDue{0.0};           // Change returned to customer
};

// ----------------------------------------------------------------------------
// Barcode Scanner Result Structure
// Outcome of a barcode scan operation.
// ----------------------------------------------------------------------------
struct BarcodeScanResult {
    bool        success{false};
    std::string barcode;
    std::string errorMessage;
};

// ============================================================================
// Abstract Service Interfaces for Team Modularity
// ============================================================================

class IProductService {
public:
    virtual ~IProductService() = default;
    virtual bool findProductByBarcode(const std::string& barcode, ProductInfo& outProduct) = 0;
    virtual bool findProductById(int id, ProductInfo& outProduct) = 0;
    virtual std::vector<ProductInfo> getAllProducts() = 0;
};

class IBillingService {
public:
    virtual ~IBillingService() = default;
    virtual BillInfo generateBill(const std::string& customerName,
                                  const std::string& customerContact,
                                  const std::vector<OrderItem>& items) = 0;
};

class ICameraScannerService {
public:
    virtual ~ICameraScannerService() = default;
    virtual bool isCameraAvailable() const = 0;
    virtual bool startCamera() = 0;
    virtual void stopCamera() = 0;
    virtual bool isScanning() const = 0;
    virtual BarcodeScanResult captureAndScan() = 0;
};

class IOrderStorageService {
public:
    virtual ~IOrderStorageService() = default;
    virtual bool saveBill(const BillInfo& bill) = 0;
    virtual std::vector<BillInfo> getAllOrders() = 0;
    virtual bool findOrderById(const std::string& orderId, BillInfo& outBill) = 0;
};
