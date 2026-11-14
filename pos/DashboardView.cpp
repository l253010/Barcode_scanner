#include "DashboardView.h"
#include <QHBoxLayout>

DashboardView::DashboardView(QWidget* parent) : QWidget(parent) {
    setupUi();
}

void DashboardView::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(32, 28, 32, 28);
    mainLayout->setSpacing(24);

    // Header Section
    auto* headerLayout = new QVBoxLayout();
    headerLayout->setSpacing(4);

    auto* titleLabel = new QLabel("Store Dashboard", this);
    titleLabel->setStyleSheet("font-size: 22px; font-weight: bold; color: #1E3A5F;");

    auto* subtitleLabel = new QLabel("Point of Sale (POS) — Central Retail Operations & Terminal Management", this);
    subtitleLabel->setStyleSheet("font-size: 13px; color: #4B5563;");

    headerLayout->addWidget(titleLabel);
    headerLayout->addWidget(subtitleLabel);
    mainLayout->addLayout(headerLayout);

    // Quick Status Banner
    auto* statusBanner = new QFrame(this);
    statusBanner->setStyleSheet(
        "QFrame {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #D1D5DB;"
        "  border-radius: 4px;"
        "  padding: 12px 16px;"
        "}"
    );
    auto* bannerLayout = new QHBoxLayout(statusBanner);
    bannerLayout->setContentsMargins(12, 8, 12, 8);

    auto* terminalStatus = new QLabel("Terminal Status: <b>Online</b> (Cash Register #01)", this);
    terminalStatus->setStyleSheet("font-size: 13px; color: #1F2937;");

    auto* sessionInfo = new QLabel("Session: Active | Barcode Scanner: Connected", this);
    sessionInfo->setStyleSheet("font-size: 13px; color: #4B5563;");

    bannerLayout->addWidget(terminalStatus);
    bannerLayout->addStretch();
    bannerLayout->addWidget(sessionInfo);
    mainLayout->addWidget(statusBanner);

    // Section Cards Grid
    auto* gridLayout = new QGridLayout();
    gridLayout->setSpacing(20);

    // Card 1: Cashier Section / New Order
    auto* cashierCard = createCard(
        "Cashier / New Order",
        "Open checkout interface. Scan product barcodes, adjust item quantities, calculate totals, and process customer sales transactions.",
        "Open Cashier Terminal",
        [this]() { emit navigateToCashier(); }
    );
    gridLayout->addWidget(cashierCard, 0, 0);

    // Card 2: Product Description & Lookup
    auto* productCard = createCard(
        "Product Description",
        "Lookup comprehensive product details, unit pricing, measurement units, available stock, and barcode verification.",
        "Lookup Products",
        [this]() { emit navigateToProductDescription(); }
    );
    gridLayout->addWidget(productCard, 0, 1);

    // Card 3: Orders Section / Invoices
    auto* ordersCard = createCard(
        "Bill & Invoice Records",
        "Access active and finalized customer invoices. Review item breakdowns, payment methods, and receipt status.",
        "View Invoices",
        [this]() { emit navigateToOrders(); }
    );
    gridLayout->addWidget(ordersCard, 1, 0);

    // Card 4: Manager Section
    auto* managerCard = createCard(
        "Manager Section",
        "Inspect store inventory levels, product catalog, unit pricing structure, and stock availability.",
        "Open Manager View",
        [this]() { emit navigateToManager(); }
    );
    gridLayout->addWidget(managerCard, 1, 1);

    mainLayout->addLayout(gridLayout);
    mainLayout->addStretch();
}

QFrame* DashboardView::createCard(const QString& title,
                                  const QString& description,
                                  const QString& buttonText,
                                  std::function<void()> onClick) {
    auto* card = new QFrame(this);
    card->setStyleSheet(
        "QFrame {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #D1D5DB;"
        "  border-radius: 4px;"
        "  padding: 16px;"
        "}"
    );

    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(18, 18, 18, 18);
    cardLayout->setSpacing(12);

    auto* cardTitle = new QLabel(title, card);
    cardTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #1E3A5F;");

    auto* cardDesc = new QLabel(description, card);
    cardDesc->setStyleSheet("font-size: 13px; color: #4B5563; line-height: 1.4;");
    cardDesc->setWordWrap(true);

    auto* actionBtn = new QPushButton(buttonText, card);
    actionBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #1E3A5F;"
        "  color: #FFFFFF;"
        "  font-size: 13px;"
        "  font-weight: 600;"
        "  border: none;"
        "  border-radius: 4px;"
        "  padding: 9px 18px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #2B4C7E;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #152A47;"
        "}"
    );
    actionBtn->setCursor(Qt::PointingHandCursor);

    connect(actionBtn, &QPushButton::clicked, this, onClick);

    cardLayout->addWidget(cardTitle);
    cardLayout->addWidget(cardDesc);
    cardLayout->addStretch();
    cardLayout->addWidget(actionBtn, 0, Qt::AlignLeft);

    return card;
}
