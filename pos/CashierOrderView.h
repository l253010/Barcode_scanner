#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QComboBox>
#include <QFrame>
#include "POSInterfaces.h"

class CashierOrderView : public QWidget {
    Q_OBJECT

public:
    explicit CashierOrderView(IProductService* productService,
                              IBillingService* billingService,
                              ICameraScannerService* cameraService,
                              QWidget* parent = nullptr);

    void resetOrder();
    void addProductToOrder(const ProductInfo& product, int quantity = 1);
    const std::vector<OrderItem>& getCartItems() const { return m_cartItems; }

signals:
    void billGenerated(const BillInfo& bill);
    void inspectProductRequested(const std::string& barcode);

private slots:
    void onAddBarcodeClicked();
    void onQuickAddSelected(int index);
    void onQuantityChanged(int row, int newQty);
    void onRemoveItemClicked(int row);
    void onDescriptionClicked();
    void onGenerateBillClicked();
    void onClearCartClicked();
    void onToggleCameraPlaceholder();

private:
    IProductService*       m_productService;
    IBillingService*       m_billingService;
    ICameraScannerService* m_cameraService;

    std::vector<OrderItem> m_cartItems;

    // UI Widgets
    QLineEdit*    m_customerNameInput{nullptr};
    QLineEdit*    m_customerContactInput{nullptr};
    QLineEdit*    m_barcodeInput{nullptr};
    QLabel*       m_scanStatusLabel{nullptr};
    QComboBox*    m_quickAddCombo{nullptr};

    // Camera Placeholder
    QFrame*       m_cameraFrame{nullptr};
    QLabel*       m_cameraStatusLabel{nullptr};
    QPushButton*  m_cameraToggleBtn{nullptr};

    // Scanned product info
    QLabel*       m_scannedInfoLabel{nullptr};

    // Cart Table
    QTableWidget* m_cartTable{nullptr};

    // Summary Labels
    QLabel*       m_subtotalLabel{nullptr};
    QLabel*       m_discountLabel{nullptr};
    QLabel*       m_taxLabel{nullptr};
    QLabel*       m_totalPayableLabel{nullptr};

    void setupUi();
    void updateCartTable();
    void updateSummary();
    void setScannedProductFeedback(const ProductInfo& prod);
};
