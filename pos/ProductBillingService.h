#pragma once
// ProductBillingService.h
// ----------------------------------------------------------------------------
// Real Product Management and Billing Service Adapter
// Connects Jannat's Retail POS System with the pure C++ Product Management
// and Billing Calculation Module (ProductBilling).
// ----------------------------------------------------------------------------

#include "POSInterfaces.h"
#include "Product.h"
#include "ProductManager.h"
#include "Billing.h"
#include <string>
#include <vector>
#include <memory>

class ProductBillingService : public IProductService, public IBillingService {
public:
    ProductBillingService();
    explicit ProductBillingService(const std::string& catalogFilePath);
    ~ProductBillingService() override = default;

    // ---- IProductService Interface Implementation ----
    bool findProductByBarcode(const std::string& barcode, ProductInfo& outProduct) override;
    bool findProductById(int id, ProductInfo& outProduct) override;
    std::vector<ProductInfo> getAllProducts() override;
    bool updateProductStock(const std::string& barcode, int newStock, std::string* errorMsg = nullptr) override;

    // ---- IBillingService Interface Implementation ----
    BillInfo generateBill(const std::string& customerName,
                          const std::string& customerContact,
                          const std::vector<OrderItem>& items) override;

    // ---- Extended POS Integration Capabilities ----
    bool loadCatalog(const std::string& filePath = "");
    bool saveCatalog(std::string* errorMsg = nullptr);
    bool commitOrderStock(const BillInfo& bill, std::string* errorMsg = nullptr);
    ProductBilling::ProductManager& getProductManager() { return m_productManager; }
    const ProductBilling::ProductManager& getProductManager() const { return m_productManager; }

    // Portable cross-platform path discovery for Product.txt
    static std::string resolveProductFilePath(const std::string& explicitPath = "");

    // Helper conversions between POS and ProductBilling structures
    static ProductInfo toProductInfo(const ProductBilling::Product& product);
    static ProductBilling::Product toProduct(const ProductInfo& info);
    static int extractNumericId(const std::string& productIdStr);

private:
    ProductBilling::ProductManager m_productManager;
    int                            m_orderCounter{1001};
    int                            m_billCounter{1};

    void initFallbackCatalog();
};
