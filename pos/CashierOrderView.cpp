#include "CashierOrderView.h"
#include "BarcodeGenerator.h"
#include "BarcodeDecoder.h"
#include "OrderModule.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QSpinBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDate>
#include <QDateTime>
#include <fstream>
#include <sstream>
#include <iomanip>

// ---------------------------------------------------------------------------
// Helper: build a styled button
// ---------------------------------------------------------------------------
QPushButton* CashierOrderView::makeBtn(const QString& text, const QString& bg, QWidget* parent) {
    auto* btn = new QPushButton(text, parent);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet(
        QString("QPushButton {"
                "  background-color: %1;"
                "  color: #FFFFFF;"
                "  font-weight: 600;"
                "  font-size: 13px;"
                "  padding: 8px 14px;"
                "  border-radius: 4px;"
                "  border: none;"
                "}"
                "QPushButton:hover { opacity: 0.9; }"
                "QPushButton:disabled { background-color: #9CA3AF; }").arg(bg)
    );
    return btn;
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
CashierOrderView::CashierOrderView(IProductService*       productService,
                                   IBillingService*       billingService,
                                   ICameraScannerService* cameraService,
                                   QWidget*               parent)
    : QWidget(parent),
      m_productService(productService),
      m_billingService(billingService),
      m_cameraService(cameraService) {
    setupUi();
    updateSummary();
    updateDashboardMetrics();
}

// ---------------------------------------------------------------------------
// setupUi
// ---------------------------------------------------------------------------
void CashierOrderView::setupUi() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // Header Frame
    auto* headerFrame = new QFrame(this);
    headerFrame->setStyleSheet(
        "QFrame {"
        "  background-color: #FFFFFF;"
        "  border-bottom: 1px solid #D1D5DB;"
        "}"
    );
    auto* headerLayout = new QHBoxLayout(headerFrame);
    headerLayout->setContentsMargins(32, 14, 32, 14);

    auto* titleCol = new QVBoxLayout();
    auto* titleLabel = new QLabel("Cashier Terminal — Checkout & Sales", headerFrame);
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #1E3A5F;");
    auto* subtitleLabel = new QLabel("Register #01 · Fast Barcode Checkout · Hold & Resume · Order Retrieval", headerFrame);
    subtitleLabel->setStyleSheet("font-size: 12px; color: #4B5563;");
    titleCol->addWidget(titleLabel);
    titleCol->addWidget(subtitleLabel);
    headerLayout->addLayout(titleCol);
    headerLayout->addStretch();

    auto* statusBadge = new QLabel("● Register Online", headerFrame);
    statusBadge->setStyleSheet("color: #166534; font-weight: bold; font-size: 13px;");
    headerLayout->addWidget(statusBadge);

    rootLayout->addWidget(headerFrame);

    // Tab Widget
    m_tabs = new QTabWidget(this);
    m_tabs->setStyleSheet(
        "QTabWidget::pane { border: none; background-color: #F4F5F7; }"
        "QTabBar::tab {"
        "  padding: 10px 22px;"
        "  font-size: 13px; font-weight: 600;"
        "  color: #4B5563; background-color: #E5E7EB;"
        "  border: none; margin-right: 2px;"
        "}"
        "QTabBar::tab:selected {"
        "  background-color: #FFFFFF; color: #1E3A5F;"
        "  border-top: 3px solid #1E3A5F;"
        "}"
        "QTabBar::tab:hover { background-color: #F3F4F6; }"
    );

    m_tabs->addTab(buildDashboardTab(),   "Cashier Dashboard");
    m_tabs->addTab(buildActiveOrderTab(),  "Order Recording");
    m_tabs->addTab(buildHeldOrdersTab(),   "Hold & Resume");
    m_tabs->addTab(buildFindOrdersTab(),   "Existing Orders");

    rootLayout->addWidget(m_tabs, 1);
}

// ---------------------------------------------------------------------------
// Tab 0: Cashier Dashboard
// ---------------------------------------------------------------------------
QWidget* CashierOrderView::buildDashboardTab() {
    auto* page = new QWidget();
    auto* mainLayout = new QVBoxLayout(page);
    mainLayout->setContentsMargins(32, 24, 32, 24);
    mainLayout->setSpacing(20);

    // Top Status Banner
    auto* bannerFrame = new QFrame(page);
    bannerFrame->setStyleSheet(
        "QFrame { background-color: #FFFFFF; border: 1px solid #D1D5DB; border-radius: 6px; padding: 14px; }");
    auto* bannerLayout = new QHBoxLayout(bannerFrame);
    bannerLayout->setContentsMargins(16, 12, 16, 12);

    auto makeMetric = [](const QString& label, QLabel*& valLabel, const QString& initVal, QFrame* parent) {
        auto* box = new QVBoxLayout();
        box->setSpacing(2);
        auto* l = new QLabel(label, parent);
        l->setStyleSheet("font-size: 12px; color: #6B7280; font-weight: 500;");
        valLabel = new QLabel(initVal, parent);
        valLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #1E3A5F;");
        box->addWidget(l);
        box->addWidget(valLabel);
        return box;
    };

    bannerLayout->addLayout(makeMetric("Cart Items:", m_dashCartCountLabel, "0 items", bannerFrame));
    bannerLayout->addSpacing(30);
    bannerLayout->addLayout(makeMetric("Orders On Hold:", m_dashHeldCountLabel, "0 held", bannerFrame));
    bannerLayout->addSpacing(30);
    bannerLayout->addLayout(makeMetric("Catalog Products:", m_dashCatalogCountLabel, "10 items", bannerFrame));
    bannerLayout->addStretch();

    mainLayout->addWidget(bannerFrame);

    // Quick Action Cards Grid
    auto* gridLayout = new QGridLayout();
    gridLayout->setSpacing(18);

    auto makeCard = [this](const QString& title, const QString& desc, const QString& btnText,
                           const QString& color, auto onClick) {
        auto* card = new QFrame(this);
        card->setStyleSheet(
            "QFrame { background-color: #FFFFFF; border: 1px solid #D1D5DB; border-radius: 6px; padding: 16px; }");
        auto* layout = new QVBoxLayout(card);
        layout->setContentsMargins(18, 16, 18, 16);
        layout->setSpacing(10);

        auto* t = new QLabel(title, card);
        t->setStyleSheet("font-size: 16px; font-weight: bold; color: #1E3A5F;");
        auto* d = new QLabel(desc, card);
        d->setStyleSheet("font-size: 12px; color: #4B5563; line-height: 1.4;");
        d->setWordWrap(true);

        auto* btn = new QPushButton(btnText, card);
        btn->setStyleSheet(
            QString("QPushButton {"
                    "  background-color: %1; color: white; font-weight: bold;"
                    "  font-size: 13px; padding: 9px 18px; border-radius: 4px; border: none; }"
                    "QPushButton:hover { opacity: 0.9; }").arg(color)
        );
        btn->setCursor(Qt::PointingHandCursor);
        connect(btn, &QPushButton::clicked, this, onClick);

        layout->addWidget(t);
        layout->addWidget(d);
        layout->addStretch();
        layout->addWidget(btn, 0, Qt::AlignLeft);
        return card;
    };

    // Card 1: Start New Order
    gridLayout->addWidget(
        makeCard("New Order / Checkout",
                 "Start recording a new sale. Scan product barcodes via camera or keyboard, adjust quantities, and generate customer bill.",
                 "Start New Order", "#166534",
                 [this]() { onNewOrderCardClicked(); }),
        0, 0);

    // Card 2: Hold & Resume
    gridLayout->addWidget(
        makeCard("Held Orders",
                 "Switch between customers or resume previously parked transactions. Preserves active cart items without losing data.",
                 "View Held Orders", "#D97706",
                 [this]() { onHeldOrdersCardClicked(); }),
        0, 1);

    // Card 3: Existing Orders
    gridLayout->addWidget(
        makeCard("Retrieve Existing Orders",
                 "Search historical orders by ID or customer. Review bill items, check payment status, and edit eligible orders.",
                 "Find Orders", "#1E3A5F",
                 [this]() { onExistingOrdersCardClicked(); }),
        1, 0);

    // Card 4: Catalog Lookup
    gridLayout->addWidget(
        makeCard("Product Catalog Lookup",
                 "Lookup product pricing, measurement unit, available inventory stock, and Code 39 barcode representation.",
                 "Lookup Products", "#0284C7",
                 [this]() {
                     if (m_productService && !m_productService->getAllProducts().empty()) {
                         emit inspectProductRequested(m_productService->getAllProducts().front().barcode);
                     }
                 }),
        1, 1);

    mainLayout->addLayout(gridLayout);

    // Recent Held Notice
    m_dashRecentHeldNotice = new QLabel("No active orders on hold. Terminal is ready for new transactions.", page);
    m_dashRecentHeldNotice->setStyleSheet("font-size: 12px; color: #6B7280;");
    mainLayout->addWidget(m_dashRecentHeldNotice);

    mainLayout->addStretch();
    return page;
}

// ---------------------------------------------------------------------------
// Tab 1: Order Recording Screen
// ---------------------------------------------------------------------------
QWidget* CashierOrderView::buildActiveOrderTab() {
    auto* page = new QWidget();
    auto* mainLayout = new QHBoxLayout(page);
    mainLayout->setContentsMargins(24, 20, 24, 20);
    mainLayout->setSpacing(20);

    // ---- Left Column ----
    auto* leftCol = new QVBoxLayout();
    leftCol->setSpacing(14);

    // Customer Info Card
    auto* customerCard = new QFrame(page);
    customerCard->setStyleSheet(
        "QFrame { background-color: #FFFFFF; border: 1px solid #D1D5DB; border-radius: 4px; }");
    auto* customerLayout = new QHBoxLayout(customerCard);
    customerLayout->setContentsMargins(12, 10, 12, 10);
    customerLayout->setSpacing(12);

    auto* nameLabel = new QLabel("Customer Name:", customerCard);
    nameLabel->setStyleSheet("font-weight: 600; color: #1F2937; font-size: 13px;");
    m_customerNameInput = new QLineEdit(customerCard);
    m_customerNameInput->setPlaceholderText("Walk-in Customer (optional)");
    m_customerNameInput->setStyleSheet(
        "background-color: #FFFFFF; color: #111827; border: 1px solid #D1D5DB;"
        "border-radius: 4px; padding: 6px 10px; font-size: 13px;"
    );

    auto* contactLabel = new QLabel("Contact:", customerCard);
    contactLabel->setStyleSheet("font-weight: 600; color: #1F2937; font-size: 13px;");
    m_customerContactInput = new QLineEdit(customerCard);
    m_customerContactInput->setPlaceholderText("Phone number (optional)");
    m_customerContactInput->setStyleSheet(
        "background-color: #FFFFFF; color: #111827; border: 1px solid #D1D5DB;"
        "border-radius: 4px; padding: 6px 10px; font-size: 13px;"
    );

    customerLayout->addWidget(nameLabel);
    customerLayout->addWidget(m_customerNameInput, 2);
    customerLayout->addWidget(contactLabel);
    customerLayout->addWidget(m_customerContactInput, 2);
    leftCol->addWidget(customerCard);

    // Barcode Scanning Bar
    auto* scanCard = new QFrame(page);
    scanCard->setStyleSheet(
        "QFrame { background-color: #FFFFFF; border: 1px solid #D1D5DB; border-radius: 4px; }");
    auto* scanLayout = new QVBoxLayout(scanCard);
    scanLayout->setContentsMargins(12, 10, 12, 10);
    scanLayout->setSpacing(10);

    auto* scanInputRow = new QHBoxLayout();
    scanInputRow->setSpacing(10);

    auto* barcodeTitle = new QLabel("Scan Barcode:", scanCard);
    barcodeTitle->setStyleSheet("font-weight: bold; color: #1E3A5F; font-size: 13px;");

    m_barcodeInput = new QLineEdit(scanCard);
    m_barcodeInput->setPlaceholderText("Enter or scan 6-digit barcode (e.g. 100001)...");
    m_barcodeInput->setStyleSheet(
        "background-color: #FFFFFF; color: #111827; border: 1px solid #D1D5DB;"
        "border-radius: 4px; padding: 7px 12px; font-size: 13px;"
    );

    auto* addBarcodeBtn = new QPushButton("Add to Cart", scanCard);
    addBarcodeBtn->setStyleSheet(
        "QPushButton { background-color: #1E3A5F; color: white; font-weight: 600;"
        "  padding: 7px 16px; border-radius: 4px; border: none; }"
        "QPushButton:hover { background-color: #2B4C7E; }"
    );
    addBarcodeBtn->setCursor(Qt::PointingHandCursor);
    connect(addBarcodeBtn,  &QPushButton::clicked,     this, &CashierOrderView::onAddBarcodeClicked);
    connect(m_barcodeInput, &QLineEdit::returnPressed, this, &CashierOrderView::onAddBarcodeClicked);

    auto* quickAddLabel = new QLabel("Quick Select:", scanCard);
    quickAddLabel->setStyleSheet("color: #4B5563; font-weight: 600; font-size: 13px;");

    m_quickAddCombo = new QComboBox(scanCard);
    m_quickAddCombo->setStyleSheet(
        "background-color: #FFFFFF; color: #111827; border: 1px solid #D1D5DB;"
        "border-radius: 4px; padding: 6px 10px; font-size: 13px;"
    );
    m_quickAddCombo->addItem("-- Select Catalog Item --");
    if (m_productService) {
        for (const auto& prod : m_productService->getAllProducts()) {
            std::ostringstream pss;
            pss << std::fixed << std::setprecision(2) << prod.unitPrice;
            m_quickAddCombo->addItem(
                QString::fromStdString(prod.barcode + " - " + prod.name + " (Rs. " + pss.str() + ")"),
                QVariant(QString::fromStdString(prod.barcode))
            );
        }
    }
    connect(m_quickAddCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CashierOrderView::onQuickAddSelected);

    scanInputRow->addWidget(barcodeTitle);
    scanInputRow->addWidget(m_barcodeInput, 2);
    scanInputRow->addWidget(addBarcodeBtn);
    scanInputRow->addWidget(quickAddLabel);
    scanInputRow->addWidget(m_quickAddCombo, 2);
    scanLayout->addLayout(scanInputRow);

    m_scanStatusLabel = new QLabel("Ready for barcode scan. Position item under scanner or pick from Quick Select.", scanCard);
    m_scanStatusLabel->setStyleSheet("font-size: 12px; color: #6B7280;");
    scanLayout->addWidget(m_scanStatusLabel);

    // Optical Camera Preview Section
    m_cameraFrame = new QFrame(scanCard);
    m_cameraFrame->setStyleSheet(
        "QFrame { background-color: #F9FAFB; border: 1px dashed #9CA3AF; border-radius: 4px; }");
    auto* cameraLayout = new QHBoxLayout(m_cameraFrame);
    cameraLayout->setContentsMargins(10, 6, 10, 6);

    m_cameraStatusLabel = new QLabel("<b>[Live Camera Scanner Ready]</b>", m_cameraFrame);
    m_cameraStatusLabel->setStyleSheet("color: #374151; font-size: 12px;");

    m_cameraToggleBtn = new QPushButton("Scan Barcode (Camera)", m_cameraFrame);
    m_cameraToggleBtn->setStyleSheet(
        "QPushButton { background-color: #FFFFFF; border: 1px solid #D1D5DB; border-radius: 3px;"
        "  padding: 4px 10px; font-size: 11px; font-weight: 600; color: #1F2937; }"
        "QPushButton:hover { background-color: #E5E7EB; }"
    );
    m_cameraToggleBtn->setCursor(Qt::PointingHandCursor);
    connect(m_cameraToggleBtn, &QPushButton::clicked, this, &CashierOrderView::onToggleCameraPlaceholder);

    auto* uploadImgBtn = new QPushButton("Upload Barcode Image...", m_cameraFrame);
    uploadImgBtn->setStyleSheet(
        "QPushButton { background-color: #FFFFFF; border: 1px solid #1E3A5F; border-radius: 3px;"
        "  padding: 4px 10px; font-size: 11px; font-weight: 600; color: #1E3A5F; }"
        "QPushButton:hover { background-color: #EFF6FF; }"
    );
    uploadImgBtn->setCursor(Qt::PointingHandCursor);
    connect(uploadImgBtn, &QPushButton::clicked, this, &CashierOrderView::onUploadBarcodeImageClicked);

    cameraLayout->addWidget(m_cameraStatusLabel);
    cameraLayout->addStretch();
    cameraLayout->addWidget(uploadImgBtn);
    cameraLayout->addWidget(m_cameraToggleBtn);
    scanLayout->addWidget(m_cameraFrame);
    leftCol->addWidget(scanCard);

    // Cart Table
    m_cartTable = new QTableWidget(page);
    m_cartTable->setColumnCount(7);
    m_cartTable->setHorizontalHeaderLabels(
        {"Product Name", "Acronym", "Unit Price (Rs.)", "Qty", "Discount (Rs.)", "Subtotal (Rs.)", "Remove"});
    m_cartTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int c = 1; c <= 6; ++c)
        m_cartTable->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    m_cartTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_cartTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_cartTable->setAlternatingRowColors(true);
    m_cartTable->setStyleSheet(
        "QTableWidget { background-color: #FFFFFF !important; color: #111827 !important;"
        "  border: 1px solid #D1D5DB; border-radius: 4px; gridline-color: #E5E7EB; font-size: 13px; }"
        "QHeaderView::section { background-color: #F3F4F6 !important; color: #1F2937 !important;"
        "  font-weight: bold; padding: 7px; border: 1px solid #E5E7EB; }"
    );
    leftCol->addWidget(m_cartTable, 1);
    mainLayout->addLayout(leftCol, 7);

    // ---- Right Column ----
    auto* rightCol = new QVBoxLayout();
    rightCol->setSpacing(14);

    // Scanned product info
    auto* infoCard = new QFrame(page);
    infoCard->setStyleSheet(
        "QFrame { background-color: #FFFFFF; border: 1px solid #D1D5DB; border-radius: 4px; }");
    auto* infoLayout = new QVBoxLayout(infoCard);
    infoLayout->setContentsMargins(14, 14, 14, 14);
    infoLayout->setSpacing(6);
    auto* infoTitle = new QLabel("Scanned Product Details", infoCard);
    infoTitle->setStyleSheet("font-weight: bold; color: #1E3A5F; font-size: 13px;");
    infoLayout->addWidget(infoTitle);
    m_scannedInfoLabel = new QLabel("No product scanned yet.\nScan a barcode or use Quick Select to view details.", infoCard);
    m_scannedInfoLabel->setStyleSheet("font-size: 12px; color: #4B5563; line-height: 1.4;");
    m_scannedInfoLabel->setWordWrap(true);
    infoLayout->addWidget(m_scannedInfoLabel);
    rightCol->addWidget(infoCard);

    // Billing Summary Card
    auto* summaryCard = new QFrame(page);
    summaryCard->setStyleSheet(
        "QFrame { background-color: #FFFFFF; border: 1px solid #D1D5DB; border-radius: 4px; }");
    auto* summaryLayout = new QVBoxLayout(summaryCard);
    summaryLayout->setContentsMargins(16, 16, 16, 16);
    summaryLayout->setSpacing(10);

    auto* summaryTitle = new QLabel("Billing Summary", summaryCard);
    summaryTitle->setStyleSheet("font-size: 15px; font-weight: bold; color: #1E3A5F;");
    summaryLayout->addWidget(summaryTitle);

    auto* gridSummary = new QGridLayout();
    gridSummary->setSpacing(8);

    auto addRow = [&](const QString& lText, QLabel*& valLabel, const QString& initVal,
                      const QString& labelStyle, const QString& valStyle) {
        auto* lLbl = new QLabel(lText, summaryCard);
        lLbl->setStyleSheet(labelStyle);
        valLabel = new QLabel(initVal, summaryCard);
        valLabel->setStyleSheet(valStyle);
        valLabel->setAlignment(Qt::AlignRight);
        int row = gridSummary->rowCount();
        gridSummary->addWidget(lLbl,     row, 0);
        gridSummary->addWidget(valLabel, row, 1);
    };

    addRow("Subtotal (excl. Tax):", m_subtotalLabel, "Rs. 0.00",
           "color: #4B5563; font-size: 13px;", "color: #1F2937; font-weight: 600; font-size: 13px;");
    addRow("Total Discount:", m_discountLabel, "-Rs. 0.00",
           "color: #4B5563; font-size: 13px;", "color: #166534; font-weight: 600; font-size: 13px;");
    addRow("Estimated Tax:", m_taxLabel, "+Rs. 0.00",
           "color: #4B5563; font-size: 13px;", "color: #1F2937; font-weight: 600; font-size: 13px;");
    summaryLayout->addLayout(gridSummary);

    auto* sep = new QFrame(summaryCard);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: #D1D5DB;");
    summaryLayout->addWidget(sep);

    auto* totalBox = new QHBoxLayout();
    auto* totalText = new QLabel("TOTAL PAYABLE:", summaryCard);
    totalText->setStyleSheet("font-size: 14px; font-weight: bold; color: #1E3A5F;");
    m_totalPayableLabel = new QLabel("Rs. 0.00", summaryCard);
    m_totalPayableLabel->setStyleSheet("font-size: 22px; font-weight: bold; color: #1E3A5F;");
    m_totalPayableLabel->setAlignment(Qt::AlignRight);
    totalBox->addWidget(totalText);
    totalBox->addWidget(m_totalPayableLabel);
    summaryLayout->addLayout(totalBox);
    rightCol->addWidget(summaryCard);

    // Action Buttons
    auto* descBtn = new QPushButton("View Product Description", page);
    descBtn->setStyleSheet(
        "QPushButton { background-color: #FFFFFF; border: 1px solid #1E3A5F; border-radius: 4px;"
        "  color: #1E3A5F; font-weight: 600; font-size: 13px; padding: 10px 14px; }"
        "QPushButton:hover { background-color: #EFF6FF; }"
    );
    descBtn->setCursor(Qt::PointingHandCursor);
    connect(descBtn, &QPushButton::clicked, this, &CashierOrderView::onDescriptionClicked);
    rightCol->addWidget(descBtn);

    // Hold Order button
    auto* holdBtn = new QPushButton("⏸  Hold This Order", page);
    holdBtn->setStyleSheet(
        "QPushButton { background-color: #D97706; border: none; border-radius: 4px;"
        "  color: #FFFFFF; font-weight: bold; font-size: 13px; padding: 10px 14px; }"
        "QPushButton:hover { background-color: #B45309; }"
    );
    holdBtn->setCursor(Qt::PointingHandCursor);
    connect(holdBtn, &QPushButton::clicked, this, &CashierOrderView::onHoldOrderClicked);
    rightCol->addWidget(holdBtn);

    m_holdCountLabel = new QLabel("No orders on hold.", page);
    m_holdCountLabel->setStyleSheet("font-size: 11px; color: #6B7280;");
    m_holdCountLabel->setAlignment(Qt::AlignCenter);
    rightCol->addWidget(m_holdCountLabel);

    auto* generateBillBtn = new QPushButton("Generate Bill / Proceed to Checkout", page);
    generateBillBtn->setStyleSheet(
        "QPushButton { background-color: #166534; border: none; border-radius: 4px;"
        "  color: #FFFFFF; font-weight: bold; font-size: 14px; padding: 12px 14px; }"
        "QPushButton:hover { background-color: #15803D; }"
        "QPushButton:pressed { background-color: #14532D; }"
    );
    generateBillBtn->setCursor(Qt::PointingHandCursor);
    connect(generateBillBtn, &QPushButton::clicked, this, &CashierOrderView::onGenerateBillClicked);
    rightCol->addWidget(generateBillBtn);

    auto* clearBtn = new QPushButton("Clear Cart", page);
    clearBtn->setStyleSheet(
        "QPushButton { background-color: #FFFFFF; border: 1px solid #D1D5DB; border-radius: 4px;"
        "  color: #B91C1C; font-size: 12px; padding: 7px 12px; }"
        "QPushButton:hover { background-color: #FEE2E2; }"
    );
    clearBtn->setCursor(Qt::PointingHandCursor);
    connect(clearBtn, &QPushButton::clicked, this, &CashierOrderView::onClearCartClicked);
    rightCol->addWidget(clearBtn);

    rightCol->addStretch();
    mainLayout->addLayout(rightCol, 3);
    return page;
}

// ---------------------------------------------------------------------------
// Tab 2: Held Orders
// ---------------------------------------------------------------------------
QWidget* CashierOrderView::buildHeldOrdersTab() {
    auto* page = new QWidget();
    auto* mainLayout = new QVBoxLayout(page);
    mainLayout->setContentsMargins(24, 20, 24, 20);
    mainLayout->setSpacing(14);

    auto* topRow = new QHBoxLayout();
    auto* lbl = new QLabel("Parked / Held Customer Orders", page);
    lbl->setStyleSheet("font-size: 15px; font-weight: bold; color: #1E3A5F;");
    auto* info = new QLabel("Select any held transaction below and click 'Resume' to continue checkout.", page);
    info->setStyleSheet("font-size: 12px; color: #4B5563;");
    topRow->addWidget(lbl);
    topRow->addWidget(info);
    topRow->addStretch();
    mainLayout->addLayout(topRow);

    m_heldOrdersTable = new QTableWidget(page);
    m_heldOrdersTable->setColumnCount(6);
    m_heldOrdersTable->setHorizontalHeaderLabels({"#", "Customer Name", "Contact", "Items", "Est. Total (Rs.)", "Held At"});
    m_heldOrdersTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_heldOrdersTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_heldOrdersTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_heldOrdersTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_heldOrdersTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_heldOrdersTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_heldOrdersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_heldOrdersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_heldOrdersTable->setAlternatingRowColors(true);
    m_heldOrdersTable->verticalHeader()->hide();
    m_heldOrdersTable->setStyleSheet(
        "QTableWidget { background-color: #FFFFFF !important; color: #111827 !important;"
        "  border: 1px solid #D1D5DB; border-radius: 4px; gridline-color: #E5E7EB; font-size: 13px; }"
        "QHeaderView::section { background-color: #F3F4F6 !important; color: #1F2937 !important;"
        "  font-weight: bold; padding: 7px; border: 1px solid #E5E7EB; }"
    );
    mainLayout->addWidget(m_heldOrdersTable, 1);

    auto* btnRow = new QHBoxLayout();
    auto* resumeBtn = new QPushButton("▶  Resume Selected Order", page);
    resumeBtn->setStyleSheet(
        "QPushButton { background-color: #166534; color: #FFFFFF; font-weight: bold;"
        "  font-size: 13px; padding: 10px 22px; border-radius: 4px; border: none; }"
        "QPushButton:hover { background-color: #15803D; }"
    );
    resumeBtn->setCursor(Qt::PointingHandCursor);
    connect(resumeBtn, &QPushButton::clicked, this, &CashierOrderView::onResumeHeldOrderClicked);

    auto* deleteHeldBtn = new QPushButton("🗑  Remove Held Order", page);
    deleteHeldBtn->setStyleSheet(
        "QPushButton { background-color: #FFFFFF; border: 1px solid #FCA5A5; color: #B91C1C;"
        "  font-weight: 600; font-size: 13px; padding: 10px 20px; border-radius: 4px; }"
        "QPushButton:hover { background-color: #FEE2E2; }"
    );
    deleteHeldBtn->setCursor(Qt::PointingHandCursor);
    connect(deleteHeldBtn, &QPushButton::clicked, this, &CashierOrderView::onDeleteHeldOrderClicked);

    btnRow->addWidget(resumeBtn);
    btnRow->addWidget(deleteHeldBtn);
    btnRow->addStretch();
    mainLayout->addLayout(btnRow);

    m_heldStatusLabel = new QLabel("Select a held order and click Resume.", page);
    m_heldStatusLabel->setStyleSheet("font-size: 12px; color: #6B7280;");
    mainLayout->addWidget(m_heldStatusLabel);

    return page;
}

// ---------------------------------------------------------------------------
// Tab 3: Existing Orders Management
// ---------------------------------------------------------------------------
QWidget* CashierOrderView::buildFindOrdersTab() {
    auto* page = new QWidget();
    auto* mainLayout = new QVBoxLayout(page);
    mainLayout->setContentsMargins(24, 20, 24, 20);
    mainLayout->setSpacing(14);

    auto* topRow = new QHBoxLayout();
    auto* lbl = new QLabel("Existing Order Management & Retrieval", page);
    lbl->setStyleSheet("font-size: 15px; font-weight: bold; color: #1E3A5F;");
    topRow->addWidget(lbl);
    topRow->addStretch();

    auto* demoBtn = new QPushButton("Load Demo Orders (Test Data)", page);
    demoBtn->setStyleSheet(
        "QPushButton { background-color: #F3F4F6; border: 1px solid #D1D5DB; color: #374151;"
        "  font-size: 12px; font-weight: 600; padding: 5px 12px; border-radius: 4px; }"
        "QPushButton:hover { background-color: #E5E7EB; }"
    );
    demoBtn->setCursor(Qt::PointingHandCursor);
    connect(demoBtn, &QPushButton::clicked, this, &CashierOrderView::onLoadDemoOrdersClicked);
    topRow->addWidget(demoBtn);

    mainLayout->addLayout(topRow);

    // Business Policy Notice Box
    auto* infoFrame = new QFrame(page);
    infoFrame->setStyleSheet(
        "QFrame { background-color: #EFF6FF; border: 1px solid #BFDBFE; border-radius: 4px; }");
    auto* infoLayout = new QVBoxLayout(infoFrame);
    infoLayout->setContentsMargins(12, 10, 12, 10);
    auto* infoLbl = new QLabel(
        "<b>Order Status & Modification Policy:</b><br>"
        "• <b>Incomplete Orders (Pending)</b>: Can be freely edited and updated by the cashier.<br>"
        "• <b>Paid Orders (≤ 7 Days Old)</b>: Eligible for return/exchange, but <b>require Manager Approval</b> before modification.<br>"
        "• <b>Paid Orders (> 7 Days Old)</b>: Policy expired. Modifications are strictly not permitted.<br>"
        "• <b>Cancelled Orders</b>: Finalized as cancelled and cannot be edited.", infoFrame);
    infoLbl->setStyleSheet("font-size: 12px; color: #1E40AF;");
    infoLbl->setWordWrap(true);
    infoLayout->addWidget(infoLbl);
    mainLayout->addWidget(infoFrame);

    // Search and Status Filter Row
    auto* searchRow = new QHBoxLayout();
    searchRow->setSpacing(10);

    auto* searchLbl = new QLabel("Search:", page);
    searchLbl->setStyleSheet("font-weight: 600; color: #1F2937; font-size: 13px;");

    m_orderSearchInput = new QLineEdit(page);
    m_orderSearchInput->setPlaceholderText("Filter by Order ID or customer name...");
    m_orderSearchInput->setStyleSheet(
        "background-color: #FFFFFF; color: #111827; border: 1px solid #D1D5DB;"
        "border-radius: 4px; padding: 6px 12px; font-size: 13px;"
    );
    connect(m_orderSearchInput, &QLineEdit::textChanged, this, &CashierOrderView::onSearchOrderClicked);

    auto* filterLbl = new QLabel("Status:", page);
    filterLbl->setStyleSheet("font-weight: 600; color: #1F2937; font-size: 13px;");

    m_orderStatusFilterCombo = new QComboBox(page);
    m_orderStatusFilterCombo->addItems({"All Orders", "Incomplete (Pending)", "Paid (Completed)", "Cancelled"});
    m_orderStatusFilterCombo->setStyleSheet(
        "background-color: #FFFFFF; color: #111827; border: 1px solid #D1D5DB;"
        "border-radius: 4px; padding: 6px 10px; font-size: 13px;"
    );
    connect(m_orderStatusFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CashierOrderView::onStatusFilterChanged);

    auto* searchBtn = new QPushButton("Refresh List", page);
    searchBtn->setStyleSheet(
        "QPushButton { background-color: #1E3A5F; color: #FFFFFF; font-weight: 600;"
        "  font-size: 13px; padding: 7px 18px; border-radius: 4px; border: none; }"
        "QPushButton:hover { background-color: #2B4C7E; }"
    );
    searchBtn->setCursor(Qt::PointingHandCursor);
    connect(searchBtn, &QPushButton::clicked, this, &CashierOrderView::onSearchOrderClicked);

    searchRow->addWidget(searchLbl);
    searchRow->addWidget(m_orderSearchInput, 2);
    searchRow->addWidget(filterLbl);
    searchRow->addWidget(m_orderStatusFilterCombo, 1);
    searchRow->addWidget(searchBtn);
    mainLayout->addLayout(searchRow);

    // Results table
    m_orderResultTable = new QTableWidget(page);
    m_orderResultTable->setColumnCount(7);
    m_orderResultTable->setHorizontalHeaderLabels(
        {"Order ID", "Customer Name", "Date & Time", "Status", "Payment Status", "Payment Mode", "Total (Rs.)"});
    m_orderResultTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_orderResultTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_orderResultTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_orderResultTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_orderResultTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_orderResultTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_orderResultTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_orderResultTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_orderResultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_orderResultTable->setAlternatingRowColors(true);
    m_orderResultTable->verticalHeader()->hide();
    m_orderResultTable->setStyleSheet(
        "QTableWidget { background-color: #FFFFFF !important; color: #111827 !important;"
        "  border: 1px solid #D1D5DB; border-radius: 4px; gridline-color: #E5E7EB; font-size: 13px; }"
        "QHeaderView::section { background-color: #F3F4F6 !important; color: #1F2937 !important;"
        "  font-weight: bold; padding: 7px; border: 1px solid #E5E7EB; }"
    );
    connect(m_orderResultTable, &QTableWidget::cellDoubleClicked, [this](int /*row*/, int /*col*/) {
        onViewOrderDetailClicked();
    });
    mainLayout->addWidget(m_orderResultTable, 1);

    auto* actionRow = new QHBoxLayout();
    m_viewDetailBtn = new QPushButton("View Order Details", page);
    m_viewDetailBtn->setStyleSheet(
        "QPushButton { background-color: #1E3A5F; color: #FFFFFF; font-weight: 600;"
        "  font-size: 13px; padding: 9px 20px; border-radius: 4px; border: none; }"
        "QPushButton:hover { background-color: #2B4C7E; }"
    );
    m_viewDetailBtn->setCursor(Qt::PointingHandCursor);
    connect(m_viewDetailBtn, &QPushButton::clicked, this, &CashierOrderView::onViewOrderDetailClicked);

    m_editOrderBtn = new QPushButton("Edit Selected Order", page);
    m_editOrderBtn->setStyleSheet(
        "QPushButton { background-color: #0284C7; color: #FFFFFF; font-weight: bold;"
        "  font-size: 13px; padding: 9px 20px; border-radius: 4px; border: none; }"
        "QPushButton:hover { background-color: #0369A1; }"
    );
    m_editOrderBtn->setCursor(Qt::PointingHandCursor);
    connect(m_editOrderBtn, &QPushButton::clicked, this, &CashierOrderView::onEditSelectedOrderClicked);

    actionRow->addWidget(m_viewDetailBtn);
    actionRow->addWidget(m_editOrderBtn);
    actionRow->addStretch();
    mainLayout->addLayout(actionRow);

    m_orderSearchStatusLabel = new QLabel(
        "Click an order and choose 'View Order Details' or 'Edit Selected Order'.", page);
    m_orderSearchStatusLabel->setStyleSheet("font-size: 12px; color: #6B7280;");
    mainLayout->addWidget(m_orderSearchStatusLabel);

    refreshExistingOrders();
    return page;
}

// ---------------------------------------------------------------------------
// Dashboard slot handlers
// ---------------------------------------------------------------------------
void CashierOrderView::onNewOrderCardClicked() {
    m_tabs->setCurrentIndex(1); // Switch to Order Recording
    if (m_barcodeInput) {
        m_barcodeInput->setFocus();
    }
}

void CashierOrderView::onHeldOrdersCardClicked() {
    m_tabs->setCurrentIndex(2); // Switch to Held Orders
}

void CashierOrderView::onExistingOrdersCardClicked() {
    m_tabs->setCurrentIndex(3); // Switch to Existing Orders
    refreshExistingOrders();
}

void CashierOrderView::updateDashboardMetrics() {
    if (m_dashCartCountLabel) {
        int cartCount = 0;
        for (const auto& it : m_cartItems) cartCount += it.quantity;
        m_dashCartCountLabel->setText(QString("%1 unit(s)").arg(cartCount));
    }
    if (m_dashHeldCountLabel) {
        m_dashHeldCountLabel->setText(QString("%1 held").arg(m_heldOrders.size()));
    }
    if (m_dashCatalogCountLabel && m_productService) {
        m_dashCatalogCountLabel->setText(QString("%1 products").arg(m_productService->getAllProducts().size()));
    }
    if (m_dashRecentHeldNotice) {
        if (m_heldOrders.empty()) {
            m_dashRecentHeldNotice->setText("No active orders on hold. Terminal is ready for new transactions.");
        } else {
            m_dashRecentHeldNotice->setText(
                QString("<font color='#D97706'><b>Notice:</b> %1 order(s) currently parked on hold. Switch to 'Hold & Resume' tab to resume.</font>")
                .arg(m_heldOrders.size()));
        }
    }
}

// ---------------------------------------------------------------------------
// Order Recording – Barcode and Cart Operations
// ---------------------------------------------------------------------------
void CashierOrderView::onAddBarcodeClicked() {
    QString codeStr = m_barcodeInput->text().trimmed();
    if (codeStr.isEmpty()) {
        m_scanStatusLabel->setText("<font color='#DC2626'>Please enter a 6-digit barcode.</font>");
        return;
    }
    std::string stdCode = codeStr.toStdString();
    ValidationResult valResult = BarcodeGenerator::validate(stdCode);
    if (!valResult.valid) {
        m_scanStatusLabel->setText(QString("<font color='#DC2626'>Invalid Barcode: %1</font>")
                                   .arg(QString::fromStdString(valResult.message)));
        return;
    }
    ProductInfo prod;
    if (m_productService && m_productService->findProductByBarcode(stdCode, prod)) {
        addProductToOrder(prod, 1);
        m_scanStatusLabel->setText(QString("<font color='#166534'>Added: %1 (%2)</font>")
                                   .arg(QString::fromStdString(prod.name))
                                   .arg(QString::fromStdString(prod.barcode)));
        setScannedProductFeedback(prod);
        m_barcodeInput->clear();
    } else {
        m_scanStatusLabel->setText(QString("<font color='#DC2626'>Barcode '%1' not found in store catalog.</font>")
                                   .arg(codeStr));
    }
}

void CashierOrderView::onQuickAddSelected(int index) {
    if (index <= 0) return;
    QString barcodeStr = m_quickAddCombo->itemData(index).toString();
    if (!barcodeStr.isEmpty() && m_productService) {
        ProductInfo prod;
        if (m_productService->findProductByBarcode(barcodeStr.toStdString(), prod)) {
            addProductToOrder(prod, 1);
            setScannedProductFeedback(prod);
            m_scanStatusLabel->setText(QString("<font color='#166534'>Added item: %1</font>")
                                       .arg(QString::fromStdString(prod.name)));
        }
    }
    m_quickAddCombo->setCurrentIndex(0);
}

void CashierOrderView::addProductToOrder(const ProductInfo& product, int quantity) {
    for (auto& item : m_cartItems) {
        if (item.product.id == product.id) {
            item.quantity += quantity;
            updateCartTable();
            updateSummary();
            updateDashboardMetrics();
            return;
        }
    }
    OrderItem newItem;
    newItem.product  = product;
    newItem.quantity = quantity;
    m_cartItems.push_back(newItem);
    updateCartTable();
    updateSummary();
    updateDashboardMetrics();
}

void CashierOrderView::updateCartTable() {
    m_cartTable->setRowCount(static_cast<int>(m_cartItems.size()));
    for (int i = 0; i < static_cast<int>(m_cartItems.size()); ++i) {
        const auto& item = m_cartItems[i];

        auto setItem = [&](int col, const QString& text, Qt::Alignment align){
            auto* it = new QTableWidgetItem(text);
            it->setTextAlignment(align);
            m_cartTable->setItem(i, col, it);
        };
        setItem(0, QString::fromStdString(item.product.name), Qt::AlignLeft | Qt::AlignVCenter);
        setItem(1, QString::fromStdString(item.product.acronym), Qt::AlignCenter);
        setItem(2, QString("Rs. %1").arg(item.product.unitPrice, 0, 'f', 2), Qt::AlignRight | Qt::AlignVCenter);

        // Qty spinner
        auto* qtySpin = new QSpinBox(m_cartTable);
        qtySpin->setRange(1, 999);
        qtySpin->setValue(item.quantity);
        qtySpin->setStyleSheet(
            "background-color: #FFFFFF; color: #111827; padding: 2px 4px;"
            "font-weight: bold; border: 1px solid #D1D5DB; border-radius: 3px;"
        );
        connect(qtySpin, QOverload<int>::of(&QSpinBox::valueChanged), [this, i](int val){
            onQuantityChanged(i, val);
        });
        m_cartTable->setCellWidget(i, 3, qtySpin);

        auto* discItem = new QTableWidgetItem(QString("-Rs. %1").arg(item.getTotalDiscount(), 0, 'f', 2));
        discItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        discItem->setForeground(QBrush(QColor("#166534")));
        m_cartTable->setItem(i, 4, discItem);

        auto* subItem = new QTableWidgetItem(QString("Rs. %1").arg(item.getSubtotal(), 0, 'f', 2));
        subItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        subItem->setFont(QFont("", -1, QFont::Bold));
        m_cartTable->setItem(i, 5, subItem);

        auto* removeBtn = new QPushButton("Remove", m_cartTable);
        removeBtn->setStyleSheet(
            "QPushButton { background-color: #FFFFFF; border: 1px solid #FCA5A5;"
            "  color: #B91C1C; font-size: 11px; padding: 3px 8px; border-radius: 3px; }"
            "QPushButton:hover { background-color: #FEE2E2; }"
        );
        removeBtn->setCursor(Qt::PointingHandCursor);
        connect(removeBtn, &QPushButton::clicked, [this, i](){ onRemoveItemClicked(i); });
        m_cartTable->setCellWidget(i, 6, removeBtn);
    }
}

void CashierOrderView::onQuantityChanged(int row, int newQty) {
    if (row >= 0 && row < static_cast<int>(m_cartItems.size())) {
        m_cartItems[row].quantity = newQty;
        updateSummary();
        updateDashboardMetrics();
        if (m_cartTable->item(row, 4))
            m_cartTable->item(row, 4)->setText(QString("-Rs. %1").arg(m_cartItems[row].getTotalDiscount(), 0, 'f', 2));
        if (m_cartTable->item(row, 5))
            m_cartTable->item(row, 5)->setText(QString("Rs. %1").arg(m_cartItems[row].getSubtotal(), 0, 'f', 2));
    }
}

void CashierOrderView::onRemoveItemClicked(int row) {
    if (row >= 0 && row < static_cast<int>(m_cartItems.size())) {
        m_cartItems.erase(m_cartItems.begin() + row);
        updateCartTable();
        updateSummary();
        updateDashboardMetrics();
    }
}

void CashierOrderView::updateSummary() {
    double subtotal = 0.0, discount = 0.0, tax = 0.0;
    for (const auto& item : m_cartItems) {
        subtotal += item.getSubtotal();
        discount += item.getTotalDiscount();
        tax      += item.getTotalTax();
    }
    double totalPayable = (subtotal - discount) + tax;
    if (totalPayable < 0.0) totalPayable = 0.0;

    m_subtotalLabel->setText(QString("Rs. %1").arg(subtotal, 0, 'f', 2));
    m_discountLabel->setText(QString("-Rs. %1").arg(discount, 0, 'f', 2));
    m_taxLabel->setText(QString("+Rs. %1").arg(tax, 0, 'f', 2));
    m_totalPayableLabel->setText(QString("Rs. %1").arg(totalPayable, 0, 'f', 2));
}

void CashierOrderView::setScannedProductFeedback(const ProductInfo& prod) {
    m_scannedInfoLabel->setText(QString(
        "<b>Product:</b> %1<br/>"
        "<b>Acronym:</b> %2 | <b>Barcode:</b> %3<br/>"
        "<b>Price:</b> Rs. %4 / %5<br/>"
        "<b>Discount:</b> Rs. %6 | <b>Tax:</b> %7%<br/>"
        "<b>Stock:</b> %8 units"
    ).arg(QString::fromStdString(prod.name))
     .arg(QString::fromStdString(prod.acronym))
     .arg(QString::fromStdString(prod.barcode))
     .arg(prod.unitPrice, 0, 'f', 2)
     .arg(QString::fromStdString(prod.unit))
     .arg(prod.discountPerUnit, 0, 'f', 2)
     .arg(prod.taxRate * 100.0, 0, 'f', 1)
     .arg(prod.availableQuantity));
}

void CashierOrderView::onDescriptionClicked() {
    int row = m_cartTable->currentRow();
    if (row >= 0 && row < static_cast<int>(m_cartItems.size())) {
        emit inspectProductRequested(m_cartItems[row].product.barcode);
    } else if (!m_cartItems.empty()) {
        emit inspectProductRequested(m_cartItems.back().product.barcode);
    } else {
        if (m_productService && !m_productService->getAllProducts().empty())
            emit inspectProductRequested(m_productService->getAllProducts().front().barcode);
        else
            QMessageBox::information(this, "Product Description",
                "Add a product to the cart to view its complete catalog description.");
    }
}

void CashierOrderView::onGenerateBillClicked() {
    if (m_cartItems.empty()) {
        QMessageBox::warning(this, "Empty Cart",
            "Cannot generate a bill with an empty cart. Please scan or select at least one product.");
        return;
    }
    std::string custName    = m_customerNameInput->text().trimmed().toStdString();
    std::string custContact = m_customerContactInput->text().trimmed().toStdString();
    if (m_billingService) {
        BillInfo bill = m_billingService->generateBill(custName, custContact, m_cartItems);
        emit billGenerated(bill);
    }
}

void CashierOrderView::onClearCartClicked() {
    if (m_cartItems.empty()) return;
    auto res = QMessageBox::question(this, "Clear Cart",
        "Are you sure you want to clear all items from the current cart?",
        QMessageBox::Yes | QMessageBox::No);
    if (res == QMessageBox::Yes) resetOrder();
}

void CashierOrderView::resetOrder() {
    m_cartItems.clear();
    if (m_customerNameInput)    m_customerNameInput->clear();
    if (m_customerContactInput) m_customerContactInput->clear();
    if (m_barcodeInput)         m_barcodeInput->clear();
    if (m_scanStatusLabel)      m_scanStatusLabel->setText("Ready for next transaction.");
    if (m_scannedInfoLabel)     m_scannedInfoLabel->setText(
        "No product scanned yet.\nScan a barcode or use Quick Select to view details.");
    updateCartTable();
    updateSummary();
    updateDashboardMetrics();
}

void CashierOrderView::onToggleCameraPlaceholder() {
    if (m_cameraService) {
        m_cameraStatusLabel->setText(
            "<font color='#166534'><b>[Camera Active]</b></font> Position barcode in camera view...");
        BarcodeScanResult res = m_cameraService->captureAndScan();
        if (res.success && m_productService) {
            ProductInfo prod;
            if (m_productService->findProductByBarcode(res.barcode, prod)) {
                addProductToOrder(prod, 1);
                setScannedProductFeedback(prod);
                m_scanStatusLabel->setText(QString("<font color='#166534'>Scanned: %1 (%2)</font>")
                    .arg(QString::fromStdString(prod.name))
                    .arg(QString::fromStdString(res.barcode)));
                m_cameraStatusLabel->setText(QString("<font color='#166534'><b>[Confirmed]</b></font> %1")
                    .arg(QString::fromStdString(res.barcode)));
            } else {
                m_scanStatusLabel->setText(QString("<font color='#DC2626'>Barcode '%1' not in catalog.</font>")
                    .arg(QString::fromStdString(res.barcode)));
                m_cameraStatusLabel->setText("<font color='#DC2626'><b>[Not Found]</b></font>");
            }
        } else if (!res.errorMessage.empty()) {
            if (res.errorMessage.find("OpenCV was not found") != std::string::npos ||
                res.errorMessage.find("not available") != std::string::npos) {
                QMessageBox::information(
                    this,
                    "Live Camera Scanner Dependency",
                    QString("<b>Camera Scanner Dependency Notice</b><br><br>"
                            "Live webcam scanning requires the OpenCV library (version 4.x), "
                            "which was not present on this machine during build.<br><br>"
                            "<b>To enable live camera scanning:</b><br>"
                            "• <b>macOS:</b> Install via Homebrew:<br>"
                            "&nbsp;&nbsp;<code>brew install opencv</code><br><br>"
                            "• <b>Windows:</b> Extract OpenCV and set the <code>OpenCV_DIR</code> environment variable:<br>"
                            "&nbsp;&nbsp;<code>set OpenCV_DIR=C:\\opencv\\build</code><br><br>"
                            "Then rebuild the project with CMake.<br><br>"
                            "<b>Alternative Scanning Methods:</b><br>"
                            "You can click <b>'Upload Barcode Image...'</b> to decode barcode images (PNG/BMP/JPG) "
                            "or enter/paste barcodes manually.")
                );
            }
            m_scanStatusLabel->setText(QString("<font color='#DC2626'>%1</font>")
                .arg(QString::fromStdString(res.errorMessage)));
            m_cameraStatusLabel->setText("<b>[Camera Scanner Ready]</b>");
        } else {
            m_cameraStatusLabel->setText("<b>[Camera Scanner Ready]</b>");
        }
    }
}

void CashierOrderView::onUploadBarcodeImageClicked() {
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Select Barcode Image to Scan",
        QString(),
        "Barcode Images (*.png *.bmp *.jpg *.jpeg);;All Files (*.*)"
    );
    if (fileName.isEmpty()) return;

    DecodeResult dec = BarcodeDecoder::decodeImage(fileName.toStdString());
    if (dec.success) {
        m_barcodeInput->setText(QString::fromStdString(dec.code));
        onAddBarcodeClicked();
        m_scanStatusLabel->setText(QString("<font color='#166534'><b>[Image Decoded]</b> Successfully scanned barcode '%1' from image file.</font>")
            .arg(QString::fromStdString(dec.code)));
    } else {
        QMessageBox::warning(
            this,
            "Barcode Image Decoding Failed",
            QString("Could not decode a valid Code 39 barcode from image file:\n\n%1\n\nEnsure image is clear and contains a valid Code 39 barcode.")
                .arg(QString::fromStdString(dec.errorMessage))
        );
        m_scanStatusLabel->setText(QString("<font color='#DC2626'>Image scan failed: %1</font>")
            .arg(QString::fromStdString(dec.errorMessage)));
    }
}

// ---------------------------------------------------------------------------
// Hold & Resume Operations
// ---------------------------------------------------------------------------
void CashierOrderView::onHoldOrderClicked() {
    if (m_cartItems.empty()) {
        QMessageBox::information(this, "Hold Order",
            "The active cart is empty. Add products before placing an order on hold.");
        return;
    }

    double totalVal = 0.0;
    for (const auto& it : m_cartItems) totalVal += it.getItemTotal();

    HeldOrder held;
    held.customerName    = m_customerNameInput->text().trimmed();
    held.customerContact = m_customerContactInput->text().trimmed();
    held.items           = m_cartItems;
    held.timestamp       = QDateTime::currentDateTime().toString("hh:mm:ss");
    held.totalValue      = totalVal;
    m_heldOrders.push_back(held);

    // Clear active cart for the next customer
    m_cartItems.clear();
    if (m_customerNameInput)    m_customerNameInput->clear();
    if (m_customerContactInput) m_customerContactInput->clear();
    if (m_scanStatusLabel)
        m_scanStatusLabel->setText(
            QString("<font color='#166534'>Order for '%1' placed on hold.</font>")
            .arg(held.customerName.isEmpty() ? "Walk-in" : held.customerName));
    if (m_scannedInfoLabel)
        m_scannedInfoLabel->setText("Active cart placed on hold.\nTerminal ready for next customer.");

    updateCartTable();
    updateSummary();
    refreshHeldOrdersTable();
    updateDashboardMetrics();

    m_holdCountLabel->setText(
        QString("%1 order(s) on hold. Open 'Hold & Resume' tab to switch.")
        .arg(m_heldOrders.size()));
}

void CashierOrderView::refreshHeldOrdersTable() {
    m_heldOrdersTable->setRowCount(static_cast<int>(m_heldOrders.size()));
    for (int i = 0; i < static_cast<int>(m_heldOrders.size()); ++i) {
        const auto& h = m_heldOrders[i];
        auto* numItem  = new QTableWidgetItem(QString::number(i + 1));
        auto* nameItem = new QTableWidgetItem(h.customerName.isEmpty() ? "Walk-in Customer" : h.customerName);
        auto* ctctItem = new QTableWidgetItem(h.customerContact.isEmpty() ? "None" : h.customerContact);
        auto* cntItem  = new QTableWidgetItem(QString("%1 line item(s)").arg(h.items.size()));
        auto* valItem  = new QTableWidgetItem(QString("Rs. %1").arg(h.totalValue, 0, 'f', 2));
        auto* timeItem = new QTableWidgetItem(h.timestamp);

        numItem->setTextAlignment(Qt::AlignCenter);
        cntItem->setTextAlignment(Qt::AlignCenter);
        valItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        timeItem->setTextAlignment(Qt::AlignCenter);

        m_heldOrdersTable->setItem(i, 0, numItem);
        m_heldOrdersTable->setItem(i, 1, nameItem);
        m_heldOrdersTable->setItem(i, 2, ctctItem);
        m_heldOrdersTable->setItem(i, 3, cntItem);
        m_heldOrdersTable->setItem(i, 4, valItem);
        m_heldOrdersTable->setItem(i, 5, timeItem);
    }
    if (m_heldStatusLabel) {
        m_heldStatusLabel->setText(
            m_heldOrders.empty() ?
            "No orders currently parked on hold." :
            QString("%1 parked order(s) available. Select an order and click 'Resume Selected Order'.").arg(m_heldOrders.size()));
    }
}

void CashierOrderView::onResumeHeldOrderClicked() {
    int row = m_heldOrdersTable->currentRow();
    if (row < 0 || row >= static_cast<int>(m_heldOrders.size())) {
        QMessageBox::information(this, "Select Order",
            "Please select a held order from the table to resume.");
        return;
    }

    if (!m_cartItems.empty()) {
        auto res = QMessageBox::question(
            this, "Active Cart Not Empty",
            "The active cart currently has items in it.\n\n"
            "Would you like to put the current active cart on hold before resuming this order?",
            QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel),
            QMessageBox::Yes);

        if (res == QMessageBox::Cancel) {
            return;
        }
        if (res == QMessageBox::Yes) {
            HeldOrder current;
            current.customerName    = m_customerNameInput->text().trimmed();
            current.customerContact = m_customerContactInput->text().trimmed();
            current.items           = m_cartItems;
            current.timestamp       = QDateTime::currentDateTime().toString("hh:mm:ss");
            double tVal = 0.0;
            for (const auto& it : m_cartItems) tVal += it.getItemTotal();
            current.totalValue      = tVal;
            m_heldOrders.push_back(current);
        }
    }

    const HeldOrder& held = m_heldOrders[row];
    m_cartItems = held.items;
    if (m_customerNameInput)    m_customerNameInput->setText(held.customerName);
    if (m_customerContactInput) m_customerContactInput->setText(held.customerContact);

    m_heldOrders.erase(m_heldOrders.begin() + row);

    updateCartTable();
    updateSummary();
    refreshHeldOrdersTable();
    updateDashboardMetrics();

    // Switch to Tab 1 (Order Recording)
    m_tabs->setCurrentIndex(1);
    if (m_scanStatusLabel)
        m_scanStatusLabel->setText(
            QString("<font color='#166534'>Resumed parked order for '%1'.</font>")
            .arg(held.customerName.isEmpty() ? "Walk-in Customer" : held.customerName));
    m_holdCountLabel->setText(
        m_heldOrders.empty() ?
        "No orders on hold." :
        QString("%1 order(s) on hold.").arg(m_heldOrders.size()));
}

void CashierOrderView::onDeleteHeldOrderClicked() {
    int row = m_heldOrdersTable->currentRow();
    if (row < 0 || row >= static_cast<int>(m_heldOrders.size())) {
        QMessageBox::information(this, "Select Order",
            "Please select a held order to remove.");
        return;
    }
    auto res = QMessageBox::question(
        this, "Remove Held Order",
        QString("Remove parked order #%1 for '%2'?\nThis operation cannot be undone.")
        .arg(row + 1)
        .arg(m_heldOrders[row].customerName.isEmpty() ? "Walk-in Customer" : m_heldOrders[row].customerName),
        QMessageBox::Yes | QMessageBox::No);
    if (res == QMessageBox::Yes) {
        m_heldOrders.erase(m_heldOrders.begin() + row);
        refreshHeldOrdersTable();
        updateDashboardMetrics();
        m_holdCountLabel->setText(
            m_heldOrders.empty() ?
            "No orders on hold." :
            QString("%1 order(s) on hold.").arg(m_heldOrders.size()));
    }
}

// ---------------------------------------------------------------------------
// Existing Orders – Stored Order Loading & Filtering
// ---------------------------------------------------------------------------
std::vector<CashierOrderView::StoredOrder> CashierOrderView::loadStoredOrders() {
    std::vector<StoredOrder> list;
    std::ifstream file("Order.txt");
    if (!file.is_open()) {
        file.open("../Order.txt");
    }
    if (!file.is_open()) {
        return list;
    }
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        if (!line.empty() && line.back() == '\r') line.pop_back();

        std::stringstream ss(line);
        std::string idStr, name, date, status, method, payStatus, totalStr;
        std::getline(ss, idStr, '|');
        std::getline(ss, name, '|');
        std::getline(ss, date, '|');
        std::getline(ss, status, '|');
        std::getline(ss, method, '|');
        std::getline(ss, payStatus, '|');
        std::getline(ss, totalStr, '|');

        StoredOrder o;
        o.orderId       = std::atoi(idStr.c_str());
        o.customerName  = QString::fromStdString(name);
        o.date          = QString::fromStdString(date);
        o.status        = QString::fromStdString(status);
        o.paymentMethod = QString::fromStdString(method);
        o.paymentStatus = QString::fromStdString(payStatus);
        o.totalAmount   = std::atof(totalStr.c_str());
        list.push_back(o);
    }
    return list;
}

void CashierOrderView::refreshExistingOrders() {
    if (!m_orderResultTable) return;

    auto allOrders = loadStoredOrders();
    QString query = m_orderSearchInput ? m_orderSearchInput->text().trimmed().toLower() : "";
    int filterIdx = m_orderStatusFilterCombo ? m_orderStatusFilterCombo->currentIndex() : 0;

    std::vector<StoredOrder> filtered;
    for (const auto& o : allOrders) {
        // Status filter
        if (filterIdx == 1 && o.status == "Completed") continue; // Incomplete only
        if (filterIdx == 2 && o.paymentStatus != "Paid") continue; // Paid only
        if (filterIdx == 3 && o.status != "Cancelled") continue; // Cancelled only

        // Query filter
        if (!query.isEmpty()) {
            QString idStr = QString::number(o.orderId);
            if (!idStr.contains(query) && !o.customerName.toLower().contains(query)) {
                continue;
            }
        }
        filtered.push_back(o);
    }

    m_orderResultTable->setRowCount(static_cast<int>(filtered.size()));
    for (int i = 0; i < static_cast<int>(filtered.size()); ++i) {
        const auto& o = filtered[i];

        auto* idIt = new QTableWidgetItem(QString::number(o.orderId));
        idIt->setTextAlignment(Qt::AlignCenter);
        idIt->setFont(QFont("", -1, QFont::Bold));

        auto* nameIt = new QTableWidgetItem(o.customerName);
        auto* dateIt = new QTableWidgetItem(o.date);
        dateIt->setTextAlignment(Qt::AlignCenter);

        auto* statusIt = new QTableWidgetItem(o.status);
        statusIt->setTextAlignment(Qt::AlignCenter);
        statusIt->setFont(QFont("", -1, QFont::Bold));
        if (o.status == "Completed") {
            statusIt->setForeground(QBrush(QColor("#166534")));
        } else if (o.status == "Pending") {
            statusIt->setForeground(QBrush(QColor("#D97706")));
        } else if (o.status == "Cancelled") {
            statusIt->setForeground(QBrush(QColor("#DC2626")));
        }

        auto* payStatIt = new QTableWidgetItem(o.paymentStatus);
        payStatIt->setTextAlignment(Qt::AlignCenter);
        if (o.paymentStatus == "Paid") {
            payStatIt->setForeground(QBrush(QColor("#166534")));
        } else {
            payStatIt->setForeground(QBrush(QColor("#D97706")));
        }

        auto* methodIt = new QTableWidgetItem(o.paymentMethod);
        methodIt->setTextAlignment(Qt::AlignCenter);

        auto* totIt = new QTableWidgetItem(QString("Rs. %1").arg(o.totalAmount, 0, 'f', 2));
        totIt->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        totIt->setFont(QFont("", -1, QFont::Bold));

        m_orderResultTable->setItem(i, 0, idIt);
        m_orderResultTable->setItem(i, 1, nameIt);
        m_orderResultTable->setItem(i, 2, dateIt);
        m_orderResultTable->setItem(i, 3, statusIt);
        m_orderResultTable->setItem(i, 4, payStatIt);
        m_orderResultTable->setItem(i, 5, methodIt);
        m_orderResultTable->setItem(i, 6, totIt);
    }

    if (m_orderSearchStatusLabel) {
        m_orderSearchStatusLabel->setText(
            QString("Displayed %1 order records from Order.txt. Double-click or select an order to view items or edit.")
            .arg(filtered.size()));
    }
}

void CashierOrderView::onSearchOrderClicked() {
    refreshExistingOrders();
}

void CashierOrderView::onStatusFilterChanged(int /*index*/) {
    refreshExistingOrders();
}

void CashierOrderView::onViewOrderDetailClicked() {
    int row = m_orderResultTable->currentRow();
    if (row < 0 || !m_orderResultTable->item(row, 0)) {
        QMessageBox::information(this, "Select Order",
            "Please select an order from the table to view its line items.");
        return;
    }

    int orderId = m_orderResultTable->item(row, 0)->text().toInt();
    QString customer = m_orderResultTable->item(row, 1) ? m_orderResultTable->item(row, 1)->text() : "Customer";
    QString dateStr  = m_orderResultTable->item(row, 2) ? m_orderResultTable->item(row, 2)->text() : "";
    QString status   = m_orderResultTable->item(row, 3) ? m_orderResultTable->item(row, 3)->text() : "";
    QString payStat  = m_orderResultTable->item(row, 4) ? m_orderResultTable->item(row, 4)->text() : "";

    OrderDetailRecord items[MAX_ITEMS_PER_ORDER];
    int count = getOrderDetails(orderId, items, MAX_ITEMS_PER_ORDER);

    QDialog dlg(this);
    dlg.setWindowTitle(QString("Order Details — Order #%1").arg(orderId));
    dlg.resize(600, 420);

    auto* layout = new QVBoxLayout(&dlg);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(12);

    auto* header = new QLabel(
        QString("<b>Order ID:</b> %1 &nbsp;&nbsp;|&nbsp;&nbsp; <b>Customer:</b> %2<br>"
                "<b>Date:</b> %3 &nbsp;&nbsp;|&nbsp;&nbsp; <b>Status:</b> %4 &nbsp;&nbsp;|&nbsp;&nbsp; <b>Payment:</b> %5")
        .arg(orderId).arg(customer).arg(dateStr).arg(status).arg(payStat), &dlg);
    header->setStyleSheet("font-size: 13px; color: #1F2937; line-height: 1.5;");
    layout->addWidget(header);

    auto* table = new QTableWidget(&dlg);
    table->setColumnCount(5);
    table->setHorizontalHeaderLabels({"Product Acronym", "Barcode", "Product Name", "Qty", "Unit Price"});
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->verticalHeader()->hide();
    table->setStyleSheet(
        "QTableWidget { background-color: #FFFFFF !important; color: #111827 !important;"
        "  border: 1px solid #D1D5DB; gridline-color: #E5E7EB; font-size: 13px; }"
        "QHeaderView::section { background-color: #F3F4F6 !important; font-weight: bold; padding: 6px; }"
    );

    table->setRowCount(count);
    for (int i = 0; i < count; ++i) {
        auto* acrIt = new QTableWidgetItem(QString::fromStdString(items[i].productId));
        acrIt->setTextAlignment(Qt::AlignCenter);

        auto* barIt = new QTableWidgetItem(QString::fromStdString(items[i].barcode));
        barIt->setTextAlignment(Qt::AlignCenter);

        QString prodName = "Catalog Item";
        double unitPrice = 0.0;
        if (m_productService) {
            ProductInfo p;
            if (m_productService->findProductByBarcode(items[i].barcode, p)) {
                prodName = QString::fromStdString(p.name);
                unitPrice = p.unitPrice;
            }
        }
        auto* nameIt = new QTableWidgetItem(prodName);
        auto* qtyIt  = new QTableWidgetItem(QString::number(items[i].quantity));
        qtyIt->setTextAlignment(Qt::AlignCenter);

        auto* pIt = new QTableWidgetItem(QString("Rs. %1").arg(unitPrice, 0, 'f', 2));
        pIt->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        table->setItem(i, 0, acrIt);
        table->setItem(i, 1, barIt);
        table->setItem(i, 2, nameIt);
        table->setItem(i, 3, qtyIt);
        table->setItem(i, 4, pIt);
    }
    layout->addWidget(table, 1);

    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
    connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::accept);
    layout->addWidget(btnBox);

    dlg.exec();
}

// ---------------------------------------------------------------------------
// Existing Orders – 7-Day Rule and Manager Approval Edit Flow
// ---------------------------------------------------------------------------
void CashierOrderView::onEditSelectedOrderClicked() {
    int row = m_orderResultTable->currentRow();
    if (row < 0 || !m_orderResultTable->item(row, 0)) {
        QMessageBox::information(this, "Select Order",
            "Please select an order from the list to edit.");
        return;
    }

    int orderId      = m_orderResultTable->item(row, 0)->text().toInt();
    QString customer = m_orderResultTable->item(row, 1) ? m_orderResultTable->item(row, 1)->text() : "Customer";
    QString dateStr  = m_orderResultTable->item(row, 2) ? m_orderResultTable->item(row, 2)->text() : "";
    QString status   = m_orderResultTable->item(row, 3) ? m_orderResultTable->item(row, 3)->text() : "";
    QString payStat  = m_orderResultTable->item(row, 4) ? m_orderResultTable->item(row, 4)->text() : "";

    // Rule 1: Cancelled orders can never be edited
    if (status == "Cancelled") {
        QMessageBox::warning(this, "Editing Not Permitted",
            QString("Order #%1 is CANCELLED.\n\nCancelled orders cannot be reopened or edited.")
            .arg(orderId));
        return;
    }

    // Rule 2: Paid / Completed orders -> 7-Day Window check
    bool isPaid = (payStat == "Paid" || status == "Completed");
    if (isPaid) {
        QDate orderDate = QDate::fromString(dateStr.left(10), "yyyy-MM-dd");
        if (!orderDate.isValid()) {
            orderDate = QDate::currentDate();
        }
        qint64 daysOld = orderDate.daysTo(QDate::currentDate());

        // Past permitted period (> 7 days)
        if (daysOld > 7) {
            QMessageBox::critical(
                this, "Operation Not Permitted — Return Window Expired",
                QString("<b>Editing Not Permitted:</b><br><br>"
                        "Order #%1 was completed on <b>%2</b> (%3 days ago).<br><br>"
                        "Under store policy, paid orders can only be edited or modified "
                        "within the allowed <b>7-day return/exchange window</b>.<br>"
                        "This order is outside the permitted period.")
                .arg(orderId).arg(dateStr).arg(daysOld)
            );
            return;
        }

        // Within allowed 7-day period -> Manager Approval Required
        QDialog approvalDlg(this);
        approvalDlg.setWindowTitle("Manager Approval Required — Edit Paid Order");
        approvalDlg.setMinimumWidth(440);

        auto* dlgLayout = new QVBoxLayout(&approvalDlg);
        dlgLayout->setContentsMargins(20, 18, 20, 18);
        dlgLayout->setSpacing(12);

        auto* titleLbl = new QLabel(
            QString("<b>Manager Approval Required for Order #%1</b>").arg(orderId), &approvalDlg);
        titleLbl->setStyleSheet("font-size: 15px; color: #1E3A5F;");
        dlgLayout->addWidget(titleLbl);

        auto* descLbl = new QLabel(
            QString("This is a <b>PAID</b> order completed on <b>%1</b> (%2 day(s) ago).<br>"
                    "Because it is within the permitted 7-day window, it can be modified "
                    "with authorized store manager approval.<br><br>"
                    "Please enter the Manager Passcode to authorize editing:")
            .arg(dateStr).arg(daysOld), &approvalDlg);
        descLbl->setStyleSheet("font-size: 12px; color: #4B5563; line-height: 1.4;");
        descLbl->setWordWrap(true);
        dlgLayout->addWidget(descLbl);

        auto* passInput = new QLineEdit(&approvalDlg);
        passInput->setEchoMode(QLineEdit::Password);
        passInput->setPlaceholderText("Enter Manager Passcode (default: admin)...");
        passInput->setStyleSheet(
            "background-color: #FFFFFF; color: #111827; border: 1px solid #D1D5DB;"
            "border-radius: 4px; padding: 7px 10px; font-size: 13px;"
        );
        dlgLayout->addWidget(passInput);

        auto* errLbl = new QLabel("", &approvalDlg);
        errLbl->setStyleSheet("font-size: 12px; color: #DC2626;");
        dlgLayout->addWidget(errLbl);

        auto* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &approvalDlg);
        btns->button(QDialogButtonBox::Ok)->setText("Approve & Edit Order");
        btns->button(QDialogButtonBox::Ok)->setStyleSheet(
            "background-color: #166534; color: white; font-weight: bold; padding: 7px 18px; border-radius: 4px; border: none;");
        dlgLayout->addWidget(btns);

        bool authorized = false;
        connect(btns, &QDialogButtonBox::accepted, [&]() {
            QString code = passInput->text().trimmed();
            if (code == "admin" || code == "1234" || code == "manager") {
                authorized = true;
                approvalDlg.accept();
            } else {
                errLbl->setText("Incorrect manager passcode. Authorization denied.");
            }
        });
        connect(btns, &QDialogButtonBox::rejected, &approvalDlg, &QDialog::reject);

        if (approvalDlg.exec() != QDialog::Accepted || !authorized) {
            return;
        }
    }

    // Load order line items into active cart for editing
    OrderDetailRecord items[MAX_ITEMS_PER_ORDER];
    int count = getOrderDetails(orderId, items, MAX_ITEMS_PER_ORDER);

    if (count == 0) {
        QMessageBox::warning(this, "Empty Order", "No line items found for this order.");
        return;
    }

    if (!m_cartItems.empty()) {
        auto res = QMessageBox::question(
            this, "Replace Current Cart",
            "The active cart currently has items in it.\n"
            "Loading this order for editing will replace the active cart.\n\n"
            "Would you like to put the current active cart on hold first?",
            QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel),
            QMessageBox::Yes);
        if (res == QMessageBox::Cancel) return;
        if (res == QMessageBox::Yes) {
            onHoldOrderClicked();
        } else {
            m_cartItems.clear();
        }
    }

    m_cartItems.clear();
    for (int i = 0; i < count; ++i) {
        ProductInfo p;
        if (m_productService && m_productService->findProductByBarcode(items[i].barcode, p)) {
            OrderItem it;
            it.product  = p;
            it.quantity = items[i].quantity;
            m_cartItems.push_back(it);
        }
    }

    if (m_customerNameInput)    m_customerNameInput->setText(customer);
    if (m_customerContactInput) m_customerContactInput->clear();

    updateCartTable();
    updateSummary();
    updateDashboardMetrics();

    // Switch to Tab 1 (Order Recording)
    m_tabs->setCurrentIndex(1);
    if (m_scanStatusLabel) {
        m_scanStatusLabel->setText(
            QString("<font color='#166534'><b>Order #%1 loaded for editing</b> (Customer: %2).</font>")
            .arg(orderId).arg(customer));
    }
}

// ---------------------------------------------------------------------------
// Load Demonstration Orders for testing
// ---------------------------------------------------------------------------
void CashierOrderView::onLoadDemoOrdersClicked() {
    clearAllFiles();

    // 1. Paid order from today (Milk + Bread)
    int o1 = createOrder("Fatima Zahra", "Cash");
    addOrderDetail(o1, "MILK-1L", "100001", 2);
    addOrderDetail(o1, "BREAD-W", "100002", 1);
    createBill(o1, 9.87, 0.00, 10.00, "Cash");

    // 2. Paid order from 2 days ago (within 7-day rule, eligible with manager approval)
    int o2 = createOrder("Kamran Ali", "Card");
    addOrderDetail(o2, "COF-250", "123456", 1);
    addOrderDetail(o2, "PB-500G", "200002", 2);
    createBill(o2, 16.99, 1.00, 16.00, "Card");

    // 3. Paid order from 10 days ago (past 7-day rule, cannot edit)
    int o3 = createOrder("Hassan Raza", "Cash");
    addOrderDetail(o3, "OIL-750", "300002", 1);
    addOrderDetail(o3, "RICE-2K", "400001", 1);
    createBill(o3, 20.74, 0.00, 25.00, "Cash");

    // 4. Pending / Incomplete order (Eggs, unpaid)
    int o4 = createOrder("Zainab Bibi", "Cash");
    addOrderDetail(o4, "EGGS-12", "100003", 2);
    updateOrderTotal(o4, 8.50);

    // 5. Cancelled order
    int o5 = createOrder("Bilal Ahmed", "Cash");
    addOrderDetail(o5, "WAT-500", "200001", 3);
    cancelOrder(o5);

    refreshExistingOrders();
    QMessageBox::information(this, "Demo Orders Loaded",
        "Successfully initialized 5 demonstration orders into Order.txt and Bill.txt:\n\n"
        "1. Order #1: Paid today (Cash) — Milk + Bread\n"
        "2. Order #2: Paid within 7 days (Card) — Eligible for edit under Manager Approval\n"
        "3. Order #3: Paid > 7 days ago — Editing strictly not permitted\n"
        "4. Order #4: Incomplete / Pending (Unpaid) — Freely editable\n"
        "5. Order #5: Cancelled — Editing not permitted\n\n"
        "All Cashier and Manager Report features can now be thoroughly tested.");
}
