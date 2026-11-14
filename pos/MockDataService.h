#pragma once

#include "POSInterfaces.h"
#include <vector>
#include <string>

// ----------------------------------------------------------------------------
// MockDataService
// Temporary, standalone implementation of team interfaces.
// Provides realistic supermarket product data, billing calculations,
// simulated camera scanner status, and in-memory order storage.
// ----------------------------------------------------------------------------
class MockDataService : public IProductService,
                        public IBillingService,
                        public ICameraScannerService,
                        public IOrderStorageService {
public:
    MockDataService();

    // ---- IProductService ----
    bool findProductByBarcode(const std::string& barcode, ProductInfo& outProduct) override;
    bool findProductById(int id, ProductInfo& outProduct) override;
    std::vector<ProductInfo> getAllProducts() override;

    // ---- IBillingService ----
    BillInfo generateBill(const std::string& customerName,
                          const std::string& customerContact,
                          const std::vector<OrderItem>& items) override;

    // ---- ICameraScannerService ----
    bool isCameraAvailable() const override;
    bool startCamera() override;
    void stopCamera() override;
    bool isScanning() const override;
    BarcodeScanResult captureAndScan() override;

    // ---- IOrderStorageService ----
    bool saveBill(const BillInfo& bill) override;
    std::vector<BillInfo> getAllOrders() override;
    bool findOrderById(const std::string& orderId, BillInfo& outBill) override;

private:
    std::vector<ProductInfo> m_catalog;
    std::vector<BillInfo>    m_savedOrders;
    bool                     m_cameraActive{false};
    int                      m_orderCounter{1001};
    int                      m_billCounter{1};

    void initSampleCatalog();
};
