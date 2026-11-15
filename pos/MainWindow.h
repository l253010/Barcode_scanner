#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <memory>

#include "POSInterfaces.h"
#include "MockDataService.h"

// Forward screen declarations
class DashboardView;
class CashierOrderView;
class ProductDescriptionView;
class BillPreviewView;
class PaymentView;
class ManagerView;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

public slots:
    void navigateToScreen(int index);
    void onBillGenerated(const BillInfo& bill);
    void onInspectProductRequested(const std::string& barcode);
    void onProceedToPaymentRequested(const BillInfo& bill);
    void onPaymentCompleted(const BillInfo& updatedBill);
    void onAddProductToCartRequested(const ProductInfo& product);

private:
    std::unique_ptr<MockDataService> m_mockService;
    BillInfo                         m_activeBill;

    // UI Structure
    QStackedWidget*           m_stack{nullptr};
    std::vector<QPushButton*> m_navButtons;

    // Screens
    DashboardView*          m_dashboardView{nullptr};
    CashierOrderView*       m_cashierView{nullptr};
    ProductDescriptionView* m_descView{nullptr};
    BillPreviewView*        m_billView{nullptr};
    PaymentView*            m_paymentView{nullptr};
    ManagerView*            m_managerView{nullptr};

    void setupUi();
    void syncActiveBill();
    void updateNavHighlights(int activeIndex);
};
