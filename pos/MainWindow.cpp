#include "MainWindow.h"
#include "DashboardView.h"
#include "CashierOrderView.h"
#include "ProductDescriptionView.h"
#include "BillPreviewView.h"
#include "PaymentView.h"
#include "ManagerView.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QDateTime>
#include <QApplication>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    m_mockService = std::make_unique<MockDataService>();
    setupUi();
    navigateToScreen(0); // Start on Dashboard
}

void MainWindow::setupUi() {
    setWindowTitle("Retail Point of Sale (POS) System — Increment 2 Terminal");
    resize(1200, 780);
    setMinimumSize(980, 640);

    // Global application stylesheet: Light neutral background, dark charcoal text, muted navy accents
    setStyleSheet(
        "QMainWindow {"
        "  background-color: #F4F5F7;"
        "}"
        "QWidget {"
        "  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;"
        "  color: #1F2937;"
        "}"
        "QScrollBar:vertical {"
        "  background: #F3F4F6;"
        "  width: 10px;"
        "  margin: 0px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: #CBD5E1;"
        "  min-height: 20px;"
        "  border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "  background: #94A3B8;"
        "}"
    );

    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto* rootLayout = new QHBoxLayout(centralWidget);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // Sidebar Container
    auto* sidebarFrame = new QFrame(centralWidget);
    sidebarFrame->setFixedWidth(240);
    sidebarFrame->setStyleSheet(
        "QFrame {"
        "  background-color: #FFFFFF;"
        "  border-right: 1px solid #D1D5DB;"
        "}"
    );
    auto* sidebarLayout = new QVBoxLayout(sidebarFrame);
    sidebarLayout->setContentsMargins(0, 0, 0, 16);
    sidebarLayout->setSpacing(4);

    // Brand Header in Sidebar
    auto* brandBox = new QFrame(sidebarFrame);
    brandBox->setStyleSheet("background-color: #1E3A5F; padding: 18px 16px; border: none;");
    auto* brandLayout = new QVBoxLayout(brandBox);
    brandLayout->setContentsMargins(16, 16, 16, 16);
    brandLayout->setSpacing(4);

    auto* brandTitle = new QLabel("RETAIL POS", brandBox);
    brandTitle->setStyleSheet("font-size: 17px; font-weight: bold; color: #FFFFFF; letter-spacing: 1px;");

    auto* brandSub = new QLabel("Increment 2 Terminal", brandBox);
    brandSub->setStyleSheet("font-size: 12px; color: #93C5FD;");

    brandLayout->addWidget(brandTitle);
    brandLayout->addWidget(brandSub);
    sidebarLayout->addWidget(brandBox);

    sidebarLayout->addSpacing(12);

    // Navigation Buttons
    struct NavItem {
        QString label;
        int screenIndex;
    };

    std::vector<NavItem> navItems = {
        {"1. Dashboard",             0},
        {"2. Cashier / New Order",   1},
        {"3. Product Description",   2},
        {"4. Bill Preview",          3},
        {"5. Payment Processing",    4},
        {"6. Manager View",          5}
    };

    m_navButtons.clear();
    for (const auto& item : navItems) {
        auto* btn = new QPushButton(item.label, sidebarFrame);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setCheckable(true);
        btn->setStyleSheet(
            "QPushButton {"
            "  text-align: left;"
            "  padding: 12px 20px;"
            "  font-size: 13px;"
            "  font-weight: 600;"
            "  color: #374151;"
            "  background-color: transparent;"
            "  border: none;"
            "  border-left: 4px solid transparent;"
            "}"
            "QPushButton:hover {"
            "  background-color: #F3F4F6;"
            "  color: #1E3A5F;"
            "}"
            "QPushButton:checked {"
            "  background-color: #EFF6FF;"
            "  color: #1E3A5F;"
            "  border-left: 4px solid #1E3A5F;"
            "  font-weight: bold;"
            "}"
        );
        int idx = item.screenIndex;
        connect(btn, &QPushButton::clicked, [this, idx]() {
            navigateToScreen(idx);
        });
        sidebarLayout->addWidget(btn);
        m_navButtons.push_back(btn);
    }

    sidebarLayout->addStretch();

    // Bottom Sidebar System Info
    auto* sysInfoBox = new QFrame(sidebarFrame);
    sysInfoBox->setStyleSheet("border: none; padding: 12px 16px;");
    auto* sysLayout = new QVBoxLayout(sysInfoBox);
    sysLayout->setContentsMargins(12, 12, 12, 12);
    sysLayout->setSpacing(2);

    auto* termLbl = new QLabel("Terminal #01 (Active)", sysInfoBox);
    termLbl->setStyleSheet("font-size: 11px; font-weight: bold; color: #166534;");

    auto* verLbl = new QLabel("Increment 2 • Qt 6 C++17", sysInfoBox);
    verLbl->setStyleSheet("font-size: 11px; color: #9CA3AF;");

    sysLayout->addWidget(termLbl);
    sysLayout->addWidget(verLbl);
    sidebarLayout->addWidget(sysInfoBox);

    rootLayout->addWidget(sidebarFrame);

    // Main Content Area with Stacked Widget
    m_stack = new QStackedWidget(centralWidget);

    // Screen 0: Dashboard
    m_dashboardView = new DashboardView(this);
    connect(m_dashboardView, &DashboardView::navigateToCashier, [this]() { navigateToScreen(1); });
    connect(m_dashboardView, &DashboardView::navigateToProductDescription, [this]() { navigateToScreen(2); });
    connect(m_dashboardView, &DashboardView::navigateToOrders, [this]() { navigateToScreen(3); });
    connect(m_dashboardView, &DashboardView::navigateToManager, [this]() { navigateToScreen(5); });
    m_stack->addWidget(m_dashboardView);

    // Screen 1: Cashier / New Order
    m_cashierView = new CashierOrderView(m_mockService.get(), m_mockService.get(), m_mockService.get(), this);
    connect(m_cashierView, &CashierOrderView::billGenerated, this, &MainWindow::onBillGenerated);
    connect(m_cashierView, &CashierOrderView::inspectProductRequested, this, &MainWindow::onInspectProductRequested);
    m_stack->addWidget(m_cashierView);

    // Screen 2: Product Description
    m_descView = new ProductDescriptionView(m_mockService.get(), this);
    connect(m_descView, &ProductDescriptionView::addProductToCart, this, &MainWindow::onAddProductToCartRequested);
    connect(m_descView, &ProductDescriptionView::backToCashierRequested, [this]() { navigateToScreen(1); });
    m_stack->addWidget(m_descView);

    // Screen 3: Bill Preview
    m_billView = new BillPreviewView(m_mockService.get(), this);
    connect(m_billView, &BillPreviewView::proceedToPaymentRequested, this, &MainWindow::onProceedToPaymentRequested);
    connect(m_billView, &BillPreviewView::backToCashierRequested, [this]() { navigateToScreen(1); });
    m_stack->addWidget(m_billView);

    // Screen 4: Payment
    m_paymentView = new PaymentView(m_mockService.get(), this);
    connect(m_paymentView, &PaymentView::paymentCompleted, this, &MainWindow::onPaymentCompleted);
    connect(m_paymentView, &PaymentView::backToBillRequested, [this](const BillInfo& b) {
        m_billView->loadBill(b);
        navigateToScreen(3);
    });
    m_stack->addWidget(m_paymentView);

    // Screen 5: Manager View
    m_managerView = new ManagerView(m_mockService.get(), this);
    connect(m_managerView, &ManagerView::inspectProductRequested, this, &MainWindow::onInspectProductRequested);
    connect(m_managerView, &ManagerView::backToDashboardRequested, [this]() { navigateToScreen(0); });
    m_stack->addWidget(m_managerView);

    rootLayout->addWidget(m_stack, 1);
}

void MainWindow::navigateToScreen(int index) {
    if (index >= 0 && index < m_stack->count()) {
        m_stack->setCurrentIndex(index);
        updateNavHighlights(index);
    }
}

void MainWindow::updateNavHighlights(int activeIndex) {
    for (size_t i = 0; i < m_navButtons.size(); ++i) {
        m_navButtons[i]->setChecked(static_cast<int>(i) == activeIndex);
    }
}

void MainWindow::onBillGenerated(const BillInfo& bill) {
    m_billView->loadBill(bill);
    navigateToScreen(3); // Navigate to Bill Preview
}

void MainWindow::onInspectProductRequested(const std::string& barcode) {
    m_descView->displayProduct(barcode);
    navigateToScreen(2); // Navigate to Product Description
}

void MainWindow::onProceedToPaymentRequested(const BillInfo& bill) {
    m_paymentView->loadBill(bill);
    navigateToScreen(4); // Navigate to Payment screen
}

void MainWindow::onPaymentCompleted(const BillInfo& updatedBill) {
    m_billView->loadBill(updatedBill);
    m_cashierView->resetOrder(); // Clear cart for next customer
    navigateToScreen(3); // Return to Bill Preview showing PAID status
}

void MainWindow::onAddProductToCartRequested(const ProductInfo& product) {
    m_cashierView->addProductToOrder(product, 1);
}
