#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QComboBox>
#include <QFrame>
#include <QTabWidget>
#include "POSInterfaces.h"

// ============================================================================
// CashierOrderView – Increment 3
// Tabs:
//   0 – Cashier Dashboard (quick access, metrics, status)
//   1 – Order Recording   (barcode scan, camera, cart, billing summary, hold)
//   2 – Held Orders       (retrieve, switch, resume held orders)
//   3 – Existing Orders   (search, filter, view details, 7-day edit with manager approval)
// ============================================================================
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

public slots:
    void refreshExistingOrders();

private slots:
    // Dashboard actions
    void onNewOrderCardClicked();
    void onHeldOrdersCardClicked();
    void onExistingOrdersCardClicked();

    // Order Recording tab
    void onAddBarcodeClicked();
    void onQuickAddSelected(int index);
    void onQuantityChanged(int row, int newQty);
    void onRemoveItemClicked(int row);
    void onDescriptionClicked();
    void onGenerateBillClicked();
    void onClearCartClicked();
    void onToggleCameraPlaceholder();
    void onHoldOrderClicked();

    // Held Orders tab
    void onResumeHeldOrderClicked();
    void onDeleteHeldOrderClicked();

    // Existing Orders tab
    void onSearchOrderClicked();
    void onStatusFilterChanged(int index);
    void onViewOrderDetailClicked();
    void onEditSelectedOrderClicked();
    void onLoadDemoOrdersClicked();

private:
    IProductService*       m_productService;
    IBillingService*       m_billingService;
    ICameraScannerService* m_cameraService;

    // Active cart
    std::vector<OrderItem> m_cartItems;

    // Hold slot: each held order is a snapshot { customerName, customerContact, items, timestamp }
    struct HeldOrder {
        QString                customerName;
        QString                customerContact;
        std::vector<OrderItem> items;
        QString                timestamp;
        double                 totalValue{0.0};
    };
    std::vector<HeldOrder> m_heldOrders;

    // Tab widget
    QTabWidget* m_tabs{nullptr};

    // ---- Tab 0: Cashier Dashboard ----
    QLabel*       m_dashCartCountLabel{nullptr};
    QLabel*       m_dashHeldCountLabel{nullptr};
    QLabel*       m_dashCatalogCountLabel{nullptr};
    QLabel*       m_dashRecentHeldNotice{nullptr};

    // ---- Tab 1: Order Recording Screen ----
    QLineEdit*    m_customerNameInput{nullptr};
    QLineEdit*    m_customerContactInput{nullptr};
    QLineEdit*    m_barcodeInput{nullptr};
    QLabel*       m_scanStatusLabel{nullptr};
    QComboBox*    m_quickAddCombo{nullptr};
    QFrame*       m_cameraFrame{nullptr};
    QLabel*       m_cameraStatusLabel{nullptr};
    QPushButton*  m_cameraToggleBtn{nullptr};
    QLabel*       m_scannedInfoLabel{nullptr};
    QTableWidget* m_cartTable{nullptr};
    QLabel*       m_subtotalLabel{nullptr};
    QLabel*       m_discountLabel{nullptr};
    QLabel*       m_taxLabel{nullptr};
    QLabel*       m_totalPayableLabel{nullptr};
    QLabel*       m_holdCountLabel{nullptr};

    // ---- Tab 2: Held Orders ----
    QTableWidget* m_heldOrdersTable{nullptr};
    QLabel*       m_heldStatusLabel{nullptr};

    // ---- Tab 3: Existing Orders ----
    QLineEdit*    m_orderSearchInput{nullptr};
    QComboBox*    m_orderStatusFilterCombo{nullptr};
    QTableWidget* m_orderResultTable{nullptr};
    QLabel*       m_orderSearchStatusLabel{nullptr};
    QPushButton*  m_editOrderBtn{nullptr};
    QPushButton*  m_viewDetailBtn{nullptr};

    void setupUi();
    QWidget* buildDashboardTab();
    QWidget* buildActiveOrderTab();
    QWidget* buildHeldOrdersTab();
    QWidget* buildFindOrdersTab();

    void updateCartTable();
    void updateSummary();
    void setScannedProductFeedback(const ProductInfo& prod);
    void refreshHeldOrdersTable();
    void updateDashboardMetrics();
    QPushButton* makeBtn(const QString& text, const QString& bg, QWidget* parent);

    // Order file helpers
    struct StoredOrder {
        int     orderId{0};
        QString customerName;
        QString date;
        QString status;
        QString paymentMethod;
        QString paymentStatus;
        double  totalAmount{0.0};
    };
    std::vector<StoredOrder> loadStoredOrders();
};
