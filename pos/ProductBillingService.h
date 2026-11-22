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
#include "Category.h"
#include "Billing.h"
#include <string>
#include <vector>
#include <memory>
#include <set>

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

    // Category Management
    std::vector<CategoryInfo> getAllCategories() override;
    bool addCategory(const CategoryInfo& category, std::string* errorMsg = nullptr) override;
    bool updateCategory(const CategoryInfo& category, std::string* errorMsg = nullptr) override;
    bool findCategoryById(const std::string& id, CategoryInfo& outCategory) override;

    // Product Management
    bool addProduct(const ProductInfo& product, std::string* errorMsg = nullptr) override;
    bool updateProduct(const ProductInfo& product, std::string* errorMsg = nullptr) override;
    std::string resolveCategoryId(const ProductInfo& product) const;
    bool validateBarcodeCategory(const ProductInfo& product, std::string* errorMsg) const;

    // ---- IBillingService Interface Implementation ----
    BillInfo generateBill(const std::string& customerName,
                          const std::string& customerContact,
                          const std::vector<OrderItem>& items) override;

    // ---- Extended POS Integration Capabilities ----
    bool loadCatalog(const std::string& filePath = "");
    bool saveCatalog(std::string* errorMsg = nullptr);
    bool loadCategories(const std::string& filePath = "");
    bool saveCategories(std::string* errorMsg = nullptr);
    bool commitOrderStock(const BillInfo& bill, std::string* errorMsg = nullptr);
    ProductBilling::ProductManager& getProductManager() { return m_productManager; }
    const ProductBilling::ProductManager& getProductManager() const { return m_productManager; }
    ProductBilling::CategoryManager& getCategoryManager() { return m_categoryManager; }
    const ProductBilling::CategoryManager& getCategoryManager() const { return m_categoryManager; }

    // Portable cross-platform path discovery for Product.txt and PCategory.txt
    static std::string resolveProductFilePath(const std::string& explicitPath = "");
    static std::string resolveCategoryFilePath(const std::string& explicitPath = "");

    // Helper conversions between POS and ProductBilling structures
    ProductInfo toProductInfo(const ProductBilling::Product& product) const;
    static ProductBilling::Product toProduct(const ProductInfo& info);
    static int extractNumericId(const std::string& productIdStr);

private:
    ProductBilling::ProductManager  m_productManager;
    ProductBilling::CategoryManager m_categoryManager;
    std::set<std::string>           m_committedOrderIds;
    int                             m_orderCounter{1001};
    int                             m_billCounter{1};

    void initFallbackCatalog();
    void initFallbackCategories();
};
