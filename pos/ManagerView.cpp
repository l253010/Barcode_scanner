#include "ManagerView.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QDate>
#include <QDateTime>
#include <QScrollArea>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <map>
#include <algorithm>
#include <cctype>

// ---------------------------------------------------------------------------
// Helper: build a styled button
// ---------------------------------------------------------------------------
QPushButton* ManagerView::makeButton(const QString& text, const QString& color, QWidget* parent) {
    auto* btn = new QPushButton(text, parent);
    btn->setCursor(Qt::PointingHandCursor);
    QString hoverColor = color;
    btn->setStyleSheet(
        QString("QPushButton {"
                "  background-color: %1;"
                "  color: #FFFFFF;"
                "  font-weight: 600;"
                "  font-size: 13px;"
                "  padding: 7px 16px;"
                "  border-radius: 4px;"
                "  border: none;"
                "}"
                "QPushButton:hover { background-color: %2; }"
                "QPushButton:disabled { background-color: #9CA3AF; }").arg(color).arg(hoverColor)
    );
    return btn;
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
ManagerView::ManagerView(IProductService* productService, QWidget* parent)
    : QWidget(parent), m_productService(productService) {
    if (m_productService) {
        m_categories = m_productService->getAllCategories();
    }

    // Seed a demo season
    m_seasons.push_back({"Eid Sale", "2026-03-20", "2026-04-05", 15.0, false});
    m_seasons.push_back({"Summer Clearance", "2026-06-01", "2026-06-30", 10.0, false});

    setupUi();
    refreshCatalog();
}

// ---------------------------------------------------------------------------
// setupUi – top-level layout with header and tabs
// ---------------------------------------------------------------------------
void ManagerView::setupUi() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // Header bar
    auto* headerFrame = new QFrame(this);
    headerFrame->setStyleSheet(
        "QFrame {"
        "  background-color: #FFFFFF;"
        "  border-bottom: 1px solid #D1D5DB;"
        "}"
    );
    auto* headerLayout = new QHBoxLayout(headerFrame);
    headerLayout->setContentsMargins(32, 16, 32, 16);

    auto* titleCol = new QVBoxLayout();
    auto* titleLabel = new QLabel("Manager Dashboard", headerFrame);
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #1E3A5F;");
    auto* subtitleLabel = new QLabel("Category · Product · Inventory · Discount · Reports Management", headerFrame);
    subtitleLabel->setStyleSheet("font-size: 12px; color: #4B5563;");
    titleCol->addWidget(titleLabel);
    titleCol->addWidget(subtitleLabel);

    auto* backBtn = makeButton("← Dashboard", "#6B7280", headerFrame);
    backBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #D1D5DB;"
        "  color: #1F2937;"
        "  font-weight: 600;"
        "  font-size: 13px;"
        "  padding: 8px 16px;"
        "  border-radius: 4px;"
        "}"
        "QPushButton:hover { background-color: #F3F4F6; }"
    );
    connect(backBtn, &QPushButton::clicked, this, &ManagerView::backToDashboardRequested);

    headerLayout->addLayout(titleCol);
    headerLayout->addStretch();
    headerLayout->addWidget(backBtn);
    rootLayout->addWidget(headerFrame);

    // Tab widget
    m_tabs = new QTabWidget(this);
    m_tabs->setStyleSheet(
        "QTabWidget::pane {"
        "  border: none;"
        "  background-color: #F4F5F7;"
        "}"
        "QTabBar::tab {"
        "  padding: 10px 20px;"
        "  font-size: 13px;"
        "  font-weight: 600;"
        "  color: #4B5563;"
        "  background-color: #E5E7EB;"
        "  border: none;"
        "  margin-right: 2px;"
        "}"
        "QTabBar::tab:selected {"
        "  background-color: #FFFFFF;"
        "  color: #1E3A5F;"
        "  border-top: 3px solid #1E3A5F;"
        "}"
        "QTabBar::tab:hover {"
        "  background-color: #F3F4F6;"
        "}"
    );

    m_tabs->addTab(buildCategoryTab(),  "Categories");
    m_tabs->addTab(buildProductTab(),   "Products");
    m_tabs->addTab(buildInventoryTab(), "Inventory");
    m_tabs->addTab(buildDiscountTab(),  "Discounts");
    m_tabs->addTab(buildReportsTab(),   "Reports");

    rootLayout->addWidget(m_tabs, 1);
}

// ---------------------------------------------------------------------------
// Helper: standard table factory
// ---------------------------------------------------------------------------
static QTableWidget* makeTable(const QStringList& headers, QWidget* parent) {
    auto* t = new QTableWidget(parent);
    t->setColumnCount(headers.size());
    t->setHorizontalHeaderLabels(headers);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setAlternatingRowColors(true);
    t->verticalHeader()->hide();
    t->horizontalHeader()->setStretchLastSection(true);
    t->setStyleSheet(
        "QTableWidget {"
        "  background-color: #FFFFFF !important;"
        "  color: #111827 !important;"
        "  border: 1px solid #D1D5DB;"
        "  gridline-color: #E5E7EB;"
        "  font-size: 13px;"
        "}"
        "QHeaderView::section {"
        "  background-color: #F3F4F6 !important;"
        "  color: #1F2937 !important;"
        "  font-weight: bold;"
        "  padding: 7px;"
        "  border: 1px solid #E5E7EB;"
        "}"
    );
    return t;
}

// ---------------------------------------------------------------------------
// Tab 0 – Categories
// ---------------------------------------------------------------------------
QWidget* ManagerView::buildCategoryTab() {
    auto* page = new QWidget();
    auto* mainLayout = new QHBoxLayout(page);
    mainLayout->setContentsMargins(24, 20, 24, 20);
    mainLayout->setSpacing(20);

    // Left: table
    auto* leftCol = new QVBoxLayout();
    auto* lTitle = new QLabel("Existing Categories", page);
    lTitle->setStyleSheet("font-size: 15px; font-weight: bold; color: #1E3A5F;");
    leftCol->addWidget(lTitle);

    m_categoryTable = makeTable({"Category ID", "Category Name", "Acronym"}, page);
    m_categoryTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_categoryTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_categoryTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    connect(m_categoryTable, &QTableWidget::itemSelectionChanged,
            this, &ManagerView::onCategoryTableSelectionChanged);
    leftCol->addWidget(m_categoryTable, 1);
    mainLayout->addLayout(leftCol, 2);

    // Right: form
    auto* rightFrame = new QFrame(page);
    rightFrame->setStyleSheet(
        "QFrame {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #D1D5DB;"
        "  border-radius: 4px;"
        "  padding: 16px;"
        "}"
    );
    auto* rightLayout = new QVBoxLayout(rightFrame);
    rightLayout->setContentsMargins(16, 16, 16, 16);
    rightLayout->setSpacing(12);

    auto* formTitle = new QLabel("Add / Edit Category", rightFrame);
    formTitle->setStyleSheet("font-size: 15px; font-weight: bold; color: #1E3A5F;");
    rightLayout->addWidget(formTitle);

    auto* form = new QFormLayout();
    form->setSpacing(10);

    m_catIdInput = new QLineEdit(rightFrame);
    m_catIdInput->setPlaceholderText("2-digit ID (e.g. 10 through 99)");
    m_catIdInput->setMaxLength(2);
    m_catIdInput->setStyleSheet(
        "background-color: #FFFFFF; color: #111827; border: 1px solid #D1D5DB;"
        "border-radius: 4px; padding: 6px 10px; font-size: 13px;"
    );

    m_catNameInput = new QLineEdit(rightFrame);
    m_catNameInput->setPlaceholderText("e.g. Dairy & Bakery");
    m_catNameInput->setStyleSheet(
        "background-color: #FFFFFF; color: #111827; border: 1px solid #D1D5DB;"
        "border-radius: 4px; padding: 6px 10px; font-size: 13px;"
    );

    m_catAcrInput = new QLineEdit(rightFrame);
    m_catAcrInput->setPlaceholderText("e.g. DAIRY (max 8 characters)");
    m_catAcrInput->setMaxLength(8);
    m_catAcrInput->setStyleSheet(
        "background-color: #FFFFFF; color: #111827; border: 1px solid #D1D5DB;"
        "border-radius: 4px; padding: 6px 10px; font-size: 13px;"
    );

    form->addRow("Category ID (2-digit) *:", m_catIdInput);
    form->addRow("Category Name *:", m_catNameInput);
    form->addRow("Category Acronym (max 8) *:", m_catAcrInput);
    rightLayout->addLayout(form);

    auto* btnRow = new QHBoxLayout();
    m_addCatBtn  = makeButton("Add Category",  "#166534", rightFrame);
    m_editCatBtn = makeButton("Save Edit",      "#0284C7", rightFrame);
    m_editCatBtn->setEnabled(false);
    connect(m_addCatBtn,  &QPushButton::clicked, this, &ManagerView::onAddCategoryClicked);
    connect(m_editCatBtn, &QPushButton::clicked, this, &ManagerView::onEditCategoryClicked);
    btnRow->addWidget(m_addCatBtn);
    btnRow->addWidget(m_editCatBtn);
    rightLayout->addLayout(btnRow);

    m_catStatusLabel = new QLabel("", rightFrame);
    m_catStatusLabel->setStyleSheet("font-size: 12px; color: #166534;");
    m_catStatusLabel->setWordWrap(true);
    rightLayout->addWidget(m_catStatusLabel);
    rightLayout->addStretch();

    mainLayout->addWidget(rightFrame, 1);
    return page;
}

// ---------------------------------------------------------------------------
// Tab 1 – Products
// ---------------------------------------------------------------------------
QWidget* ManagerView::buildProductTab() {
    auto* page = new QWidget();
    auto* mainLayout = new QVBoxLayout(page);
    mainLayout->setContentsMargins(24, 20, 24, 20);
    mainLayout->setSpacing(14);

    // Top toolbar
    auto* toolbar = new QHBoxLayout();
    auto* lbl = new QLabel("Product Catalog", page);
    lbl->setStyleSheet("font-size: 15px; font-weight: bold; color: #1E3A5F;");

    m_productSearchInput = new QLineEdit(page);
    m_productSearchInput->setPlaceholderText("Search by name, barcode, category or acronym...");
    m_productSearchInput->setStyleSheet(
        "background-color: #FFFFFF; color: #111827; border: 1px solid #D1D5DB;"
        "border-radius: 4px; padding: 6px 12px; font-size: 13px;"
    );
    connect(m_productSearchInput, &QLineEdit::textChanged, this, &ManagerView::onProductSearchChanged);

    m_addProductBtn  = makeButton("+ Add Product", "#166534", page);
    m_editProductBtn = makeButton("Edit Selected",  "#0284C7", page);
    m_editProductBtn->setEnabled(false);
    auto* inspBtn    = makeButton("View Details",   "#1E3A5F", page);

    connect(m_addProductBtn,  &QPushButton::clicked, this, [this]{ onAddProductClicked(); });
    connect(m_editProductBtn, &QPushButton::clicked, this, [this]{ onEditProductClicked(); });
    connect(inspBtn,          &QPushButton::clicked, this, &ManagerView::onInspectClicked);

    toolbar->addWidget(lbl);
    toolbar->addWidget(m_productSearchInput, 2);
    toolbar->addWidget(m_addProductBtn);
    toolbar->addWidget(m_editProductBtn);
    toolbar->addWidget(inspBtn);
    mainLayout->addLayout(toolbar);

    m_productTable = makeTable(
        {"ID", "Barcode", "Acronym", "Product Name", "Category", "Unit",
         "Price (Rs.)", "Tax (%)", "Discount (Rs.)", "Stock"}, page);
    for (int c = 0; c <= 9; ++c) {
        m_productTable->horizontalHeader()->setSectionResizeMode(c,
            (c == 3) ? QHeaderView::Stretch : QHeaderView::ResizeToContents);
    }
    connect(m_productTable, &QTableWidget::itemSelectionChanged,
            this, &ManagerView::onProductTableSelectionChanged);
    connect(m_productTable, &QTableWidget::cellDoubleClicked, [this](int row, int /*col*/){
        if (row >= 0 && m_productTable->item(row, 1))
            emit inspectProductRequested(m_productTable->item(row, 1)->text().toStdString());
    });
    mainLayout->addWidget(m_productTable, 1);

    m_productSummaryLabel = new QLabel("", page);
    m_productSummaryLabel->setStyleSheet("font-size: 12px; color: #6B7280;");
    mainLayout->addWidget(m_productSummaryLabel);
    return page;
}

// ---------------------------------------------------------------------------
// Tab 2 – Inventory
// ---------------------------------------------------------------------------
QWidget* ManagerView::buildInventoryTab() {
    auto* page = new QWidget();
    auto* mainLayout = new QVBoxLayout(page);
    mainLayout->setContentsMargins(24, 20, 24, 20);
    mainLayout->setSpacing(14);

    auto* toolbar = new QHBoxLayout();
    auto* lbl = new QLabel("Inventory & Stock Levels", page);
    lbl->setStyleSheet("font-size: 15px; font-weight: bold; color: #1E3A5F;");

    m_inventorySearchInput = new QLineEdit(page);
    m_inventorySearchInput->setPlaceholderText("Search by product name, barcode or category...");
    m_inventorySearchInput->setStyleSheet(
        "background-color: #FFFFFF; color: #111827; border: 1px solid #D1D5DB;"
        "border-radius: 4px; padding: 6px 12px; font-size: 13px;"
    );
    connect(m_inventorySearchInput, &QLineEdit::textChanged, this, &ManagerView::onInventorySearchChanged);

    auto* updateBtn = makeButton("Update Stock", "#0284C7", page);
    auto* inspBtn2  = makeButton("View Details",  "#1E3A5F", page);
    connect(updateBtn, &QPushButton::clicked, this, &ManagerView::onUpdateStockClicked);
    connect(inspBtn2,  &QPushButton::clicked, this, &ManagerView::onInspectClicked);

    toolbar->addWidget(lbl);
    toolbar->addWidget(m_inventorySearchInput, 2);
    toolbar->addWidget(updateBtn);
    toolbar->addWidget(inspBtn2);
    mainLayout->addLayout(toolbar);

    m_inventoryTable = makeTable(
        {"Barcode", "Product Name", "Category", "Unit", "Price (Rs.)", "Stock", "Status"}, page);
    m_inventoryTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    connect(m_inventoryTable, &QTableWidget::cellDoubleClicked, [this](int /*r*/, int col){
        if (col == 5) onUpdateStockClicked();
    });
    mainLayout->addWidget(m_inventoryTable, 1);

    m_inventorySummaryLabel = new QLabel("", page);
    m_inventorySummaryLabel->setStyleSheet("font-size: 12px; color: #6B7280;");
    mainLayout->addWidget(m_inventorySummaryLabel);
    return page;
}

// ---------------------------------------------------------------------------
// Tab 3 – Discounts
// ---------------------------------------------------------------------------
QWidget* ManagerView::buildDiscountTab() {
    auto* page = new QWidget();
    auto* mainLayout = new QHBoxLayout(page);
    mainLayout->setContentsMargins(24, 20, 24, 20);
    mainLayout->setSpacing(20);

    // Left column: season table
    auto* leftCol = new QVBoxLayout();
    auto* lTitle = new QLabel("Seasonal Discounts", page);
    lTitle->setStyleSheet("font-size: 15px; font-weight: bold; color: #1E3A5F;");
    leftCol->addWidget(lTitle);

    m_seasonTable = makeTable(
        {"Season Name", "Start Date", "End Date", "Discount %", "Status"}, page);
    connect(m_seasonTable, &QTableWidget::itemSelectionChanged,
            this, &ManagerView::onSeasonTableSelectionChanged);
    leftCol->addWidget(m_seasonTable, 1);

    auto* seasonBtnRow = new QHBoxLayout();
    m_addSeasonBtn    = makeButton("+ Add Season",   "#166534", page);
    m_editSeasonBtn   = makeButton("Edit Season",    "#0284C7", page);
    m_toggleSeasonBtn = makeButton("Activate/Deactivate", "#92400E", page);
    m_editSeasonBtn->setEnabled(false);
    m_toggleSeasonBtn->setEnabled(false);
    connect(m_addSeasonBtn,    &QPushButton::clicked, this, &ManagerView::onAddSeasonClicked);
    connect(m_editSeasonBtn,   &QPushButton::clicked, this, &ManagerView::onEditSeasonClicked);
    connect(m_toggleSeasonBtn, &QPushButton::clicked, this, &ManagerView::onToggleSeasonClicked);
    seasonBtnRow->addWidget(m_addSeasonBtn);
    seasonBtnRow->addWidget(m_editSeasonBtn);
    seasonBtnRow->addWidget(m_toggleSeasonBtn);
    leftCol->addLayout(seasonBtnRow);
    mainLayout->addLayout(leftCol, 2);

    // Right column: special discount panel
    auto* rightFrame = new QFrame(page);
    rightFrame->setStyleSheet(
        "QFrame {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #D1D5DB;"
        "  border-radius: 4px;"
        "}"
    );
    auto* rightLayout = new QVBoxLayout(rightFrame);
    rightLayout->setContentsMargins(16, 16, 16, 16);
    rightLayout->setSpacing(12);

    auto* spTitle = new QLabel("Special / Global Discount", rightFrame);
    spTitle->setStyleSheet("font-size: 15px; font-weight: bold; color: #1E3A5F;");
    rightLayout->addWidget(spTitle);

    auto* spDesc = new QLabel(
        "Apply a store-wide special discount percentage to all new bills.\n"
        "This overrides product-level discounts.", rightFrame);
    spDesc->setStyleSheet("font-size: 12px; color: #4B5563;");
    spDesc->setWordWrap(true);
    rightLayout->addWidget(spDesc);

    auto* spRow = new QHBoxLayout();
    auto* spLbl = new QLabel("Discount %:", rightFrame);
    spLbl->setStyleSheet("font-weight: 600; color: #1F2937; font-size: 13px;");
    m_specialDiscountSpin = new QDoubleSpinBox(rightFrame);
    m_specialDiscountSpin->setRange(0.0, 100.0);
    m_specialDiscountSpin->setDecimals(2);
    m_specialDiscountSpin->setSuffix(" %");
    m_specialDiscountSpin->setStyleSheet(
        "background-color: #FFFFFF; color: #111827; border: 1px solid #D1D5DB;"
        "border-radius: 4px; padding: 4px 8px; font-size: 13px;"
    );
    spRow->addWidget(spLbl);
    spRow->addWidget(m_specialDiscountSpin);
    rightLayout->addLayout(spRow);

    m_applySpecialBtn = makeButton("Apply Special Discount", "#7C3AED", rightFrame);
    connect(m_applySpecialBtn, &QPushButton::clicked, this, &ManagerView::onApplySpecialDiscountClicked);
    rightLayout->addWidget(m_applySpecialBtn);

    m_specialDiscountLabel = new QLabel("No special discount currently active.", rightFrame);
    m_specialDiscountLabel->setStyleSheet("font-size: 12px; color: #4B5563;");
    m_specialDiscountLabel->setWordWrap(true);
    rightLayout->addWidget(m_specialDiscountLabel);

    rightLayout->addSpacing(20);
    auto* divider = new QFrame(rightFrame);
    divider->setFrameShape(QFrame::HLine);
    divider->setStyleSheet("color: #E5E7EB;");
    rightLayout->addWidget(divider);

    m_discountStatusLabel = new QLabel("", rightFrame);
    m_discountStatusLabel->setStyleSheet("font-size: 12px; color: #166534;");
    m_discountStatusLabel->setWordWrap(true);
    rightLayout->addWidget(m_discountStatusLabel);
    rightLayout->addStretch();

    mainLayout->addWidget(rightFrame, 1);

    refreshSeasonTable();
    return page;
}

// ---------------------------------------------------------------------------
// Tab 4 – Reports
// ---------------------------------------------------------------------------
QWidget* ManagerView::buildReportsTab() {
    auto* page = new QWidget();
    auto* mainLayout = new QVBoxLayout(page);
    mainLayout->setContentsMargins(24, 20, 24, 20);
    mainLayout->setSpacing(14);

    // Control bar
    auto* ctrlBar = new QHBoxLayout();
    auto* rLbl = new QLabel("Report Type:", page);
    rLbl->setStyleSheet("font-weight: 600; color: #1F2937; font-size: 13px;");

    m_reportTypeCombo = new QComboBox(page);
    m_reportTypeCombo->addItems({
        "Incomplete Orders (with totals)",
        "Completed Orders (with totals)",
        "Daily Cash Sales",
        "Weekly Cash Sales",
        "Monthly Cash Sales",
        "Yearly Cash Sales",
        "Custom Date Range Sales",
        "All Sales (amounts & discounts)",
        "Top 10 Most-Sold Products",
        "Top 10 Highest Remaining Stock"
    });
    m_reportTypeCombo->setStyleSheet(
        "background-color: #FFFFFF; color: #111827; border: 1px solid #D1D5DB;"
        "border-radius: 4px; padding: 6px 10px; font-size: 13px;"
    );

    auto* fromLbl = new QLabel("From:", page);
    fromLbl->setStyleSheet("font-weight: 600; color: #1F2937; font-size: 13px;");
    m_fromDateEdit = new QDateEdit(QDate::currentDate().addDays(-7), page);
    m_fromDateEdit->setCalendarPopup(true);
    m_fromDateEdit->setDisplayFormat("yyyy-MM-dd");
    m_fromDateEdit->setStyleSheet(
        "background-color: #FFFFFF; color: #111827; border: 1px solid #D1D5DB;"
        "border-radius: 4px; padding: 4px 8px; font-size: 13px;"
    );

    auto* toLbl = new QLabel("To:", page);
    toLbl->setStyleSheet("font-weight: 600; color: #1F2937; font-size: 13px;");
    m_toDateEdit = new QDateEdit(QDate::currentDate(), page);
    m_toDateEdit->setCalendarPopup(true);
    m_toDateEdit->setDisplayFormat("yyyy-MM-dd");
    m_toDateEdit->setStyleSheet(
        "background-color: #FFFFFF; color: #111827; border: 1px solid #D1D5DB;"
        "border-radius: 4px; padding: 4px 8px; font-size: 13px;"
    );

    auto* genBtn = makeButton("Generate Report", "#1E3A5F", page);
    connect(genBtn, &QPushButton::clicked, this, &ManagerView::onGenerateReportClicked);

    ctrlBar->addWidget(rLbl);
    ctrlBar->addWidget(m_reportTypeCombo, 2);
    ctrlBar->addWidget(fromLbl);
    ctrlBar->addWidget(m_fromDateEdit);
    ctrlBar->addWidget(toLbl);
    ctrlBar->addWidget(m_toDateEdit);
    ctrlBar->addWidget(genBtn);
    mainLayout->addLayout(ctrlBar);

    // Info label about date filters
    auto* filterNote = new QLabel(
        "Note: Date range applies to 'Custom Date Range Sales'. "
        "Other report types use all available data from stored orders.", page);
    filterNote->setStyleSheet("font-size: 11px; color: #6B7280;");
    filterNote->setWordWrap(true);
    mainLayout->addWidget(filterNote);

    // Report table
    m_reportTable = makeTable({"Column 1", "Column 2", "Column 3", "Column 4", "Column 5"}, page);
    mainLayout->addWidget(m_reportTable, 1);

    m_reportSummaryLabel = new QLabel("Select a report type and click 'Generate Report'.", page);
    m_reportSummaryLabel->setStyleSheet("font-size: 12px; color: #6B7280;");
    mainLayout->addWidget(m_reportSummaryLabel);

    return page;
}

// ---------------------------------------------------------------------------
// refreshCatalog – called by MainWindow after payment / stock update
// ---------------------------------------------------------------------------
void ManagerView::refreshCatalog() {
    if (!m_productService) return;
    m_categories = m_productService->getAllCategories();
    refreshCategoryTable();
    auto products = m_productService->getAllProducts();
    refreshProductTable(products);
    refreshInventoryTable(products);
}

// ---------------------------------------------------------------------------
// Category table helpers
// ---------------------------------------------------------------------------
void ManagerView::refreshCategoryTable() {
    m_categoryTable->setRowCount(static_cast<int>(m_categories.size()));
    for (int i = 0; i < static_cast<int>(m_categories.size()); ++i) {
        auto* idItem   = new QTableWidgetItem(QString::fromStdString(m_categories[i].id));
        auto* nameItem = new QTableWidgetItem(QString::fromStdString(m_categories[i].name));
        auto* acrItem  = new QTableWidgetItem(QString::fromStdString(m_categories[i].acronym));
        idItem->setTextAlignment(Qt::AlignCenter);
        acrItem->setTextAlignment(Qt::AlignCenter);
        m_categoryTable->setItem(i, 0, idItem);
        m_categoryTable->setItem(i, 1, nameItem);
        m_categoryTable->setItem(i, 2, acrItem);
    }
}

// ---------------------------------------------------------------------------
// Product table helpers
// ---------------------------------------------------------------------------
void ManagerView::refreshProductTable(const std::vector<ProductInfo>& products) {
    m_productTable->setRowCount(static_cast<int>(products.size()));
    for (int i = 0; i < static_cast<int>(products.size()); ++i) {
        const auto& p = products[i];

        auto setItem = [&](int col, const QString& text, Qt::Alignment align = Qt::AlignLeft | Qt::AlignVCenter) {
            auto* it = new QTableWidgetItem(text);
            it->setTextAlignment(align);
            m_productTable->setItem(i, col, it);
        };

        setItem(0, QString::number(p.id),                            Qt::AlignCenter);
        setItem(1, QString::fromStdString(p.barcode),                Qt::AlignCenter);
        setItem(2, QString::fromStdString(p.acronym),                Qt::AlignCenter);
        setItem(3, QString::fromStdString(p.name));
        setItem(4, p.category.empty() ? "General" : QString::fromStdString(p.category));
        setItem(5, QString::fromStdString(p.unit),                   Qt::AlignCenter);
        setItem(6, QString("Rs. %1").arg(p.unitPrice, 0, 'f', 2),    Qt::AlignRight | Qt::AlignVCenter);
        setItem(7, QString("%1%").arg(p.taxRate * 100.0, 0, 'f', 1), Qt::AlignCenter);
        setItem(8, QString("-Rs. %1").arg(p.discountPerUnit, 0, 'f', 2), Qt::AlignRight | Qt::AlignVCenter);

        auto* stockItem = new QTableWidgetItem(QString::number(p.availableQuantity));
        stockItem->setTextAlignment(Qt::AlignCenter);
        stockItem->setFont(QFont("", -1, QFont::Bold));
        if (p.availableQuantity == 0) {
            stockItem->setForeground(QBrush(QColor("#DC2626")));
        } else if (p.availableQuantity <= 5) {
            stockItem->setForeground(QBrush(QColor("#D97706")));
        } else {
            stockItem->setForeground(QBrush(QColor("#166534")));
        }
        m_productTable->setItem(i, 9, stockItem);
    }
    m_productSummaryLabel->setText(
        QString("Total products: %1  |  Double-click a row to view full details.")
        .arg(products.size()));
}

// ---------------------------------------------------------------------------
// Inventory table helpers
// ---------------------------------------------------------------------------
void ManagerView::refreshInventoryTable(const std::vector<ProductInfo>& products) {
    m_inventoryTable->setRowCount(static_cast<int>(products.size()));
    for (int i = 0; i < static_cast<int>(products.size()); ++i) {
        const auto& p = products[i];

        auto setItem = [&](int col, const QString& text, Qt::Alignment align = Qt::AlignLeft | Qt::AlignVCenter) {
            auto* it = new QTableWidgetItem(text);
            it->setTextAlignment(align);
            m_inventoryTable->setItem(i, col, it);
        };

        setItem(0, QString::fromStdString(p.barcode), Qt::AlignCenter);
        setItem(1, QString::fromStdString(p.name));
        setItem(2, p.category.empty() ? "General" : QString::fromStdString(p.category));
        setItem(3, QString::fromStdString(p.unit), Qt::AlignCenter);
        setItem(4, QString("Rs. %1").arg(p.unitPrice, 0, 'f', 2), Qt::AlignRight | Qt::AlignVCenter);

        auto* stockItem = new QTableWidgetItem(QString::number(p.availableQuantity));
        stockItem->setTextAlignment(Qt::AlignCenter);
        stockItem->setFont(QFont("", -1, QFont::Bold));

        QString statusText;
        QColor  statusColor;
        if (p.availableQuantity == 0) {
            statusText  = "Out of Stock";
            statusColor = QColor("#DC2626");
            stockItem->setForeground(QBrush(QColor("#DC2626")));
        } else if (p.availableQuantity <= 5) {
            statusText  = "Low Stock";
            statusColor = QColor("#D97706");
            stockItem->setForeground(QBrush(QColor("#D97706")));
        } else {
            statusText  = "In Stock";
            statusColor = QColor("#166534");
            stockItem->setForeground(QBrush(QColor("#166534")));
        }
        m_inventoryTable->setItem(i, 5, stockItem);

        auto* statusItem = new QTableWidgetItem(statusText);
        statusItem->setTextAlignment(Qt::AlignCenter);
        statusItem->setForeground(QBrush(statusColor));
        statusItem->setFont(QFont("", -1, QFont::Bold));
        m_inventoryTable->setItem(i, 6, statusItem);
    }

    int outOfStock = 0, lowStock = 0;
    for (const auto& p : products) {
        if (p.availableQuantity == 0) ++outOfStock;
        else if (p.availableQuantity <= 5) ++lowStock;
    }
    m_inventorySummaryLabel->setText(
        QString("Total products: %1  |  Out of stock: %2  |  Low stock (≤5): %3  |  "
                "Double-click Stock column to update. Click row + 'Update Stock' to edit.")
        .arg(products.size()).arg(outOfStock).arg(lowStock));
}

// ---------------------------------------------------------------------------
// Season table helper
// ---------------------------------------------------------------------------
void ManagerView::refreshSeasonTable() {
    m_seasonTable->setRowCount(static_cast<int>(m_seasons.size()));
    for (int i = 0; i < static_cast<int>(m_seasons.size()); ++i) {
        const auto& s = m_seasons[i];
        auto* nameItem = new QTableWidgetItem(s.name);
        auto* startItem = new QTableWidgetItem(s.startDate);
        auto* endItem = new QTableWidgetItem(s.endDate);
        auto* discItem = new QTableWidgetItem(QString("%1%").arg(s.discountPercent, 0, 'f', 1));
        discItem->setTextAlignment(Qt::AlignCenter);

        auto* statusItem = new QTableWidgetItem(s.active ? "ACTIVE" : "Inactive");
        statusItem->setTextAlignment(Qt::AlignCenter);
        statusItem->setFont(QFont("", -1, QFont::Bold));
        statusItem->setForeground(QBrush(s.active ? QColor("#166534") : QColor("#6B7280")));

        m_seasonTable->setItem(i, 0, nameItem);
        m_seasonTable->setItem(i, 1, startItem);
        m_seasonTable->setItem(i, 2, endItem);
        m_seasonTable->setItem(i, 3, discItem);
        m_seasonTable->setItem(i, 4, statusItem);
    }
}

// ---------------------------------------------------------------------------
// Category slots
// ---------------------------------------------------------------------------
void ManagerView::onCategoryTableSelectionChanged() {
    int row = m_categoryTable->currentRow();
    bool sel = (row >= 0 && row < static_cast<int>(m_categories.size()));
    m_editCatBtn->setEnabled(sel);
    if (sel) {
        m_catIdInput->setText(QString::fromStdString(m_categories[row].id));
        m_catIdInput->setReadOnly(true);
        m_catNameInput->setText(QString::fromStdString(m_categories[row].name));
        m_catAcrInput->setText(QString::fromStdString(m_categories[row].acronym));
        m_catStatusLabel->setText("");
    } else {
        m_catIdInput->setReadOnly(false);
    }
}

void ManagerView::onAddCategoryClicked() {
    QString id   = m_catIdInput->text().trimmed();
    QString name = m_catNameInput->text().trimmed();
    QString acr  = m_catAcrInput->text().trimmed();

    if (id.length() != 2 || !id[0].isDigit() || !id[1].isDigit()) {
        m_catStatusLabel->setStyleSheet("font-size: 12px; color: #DC2626;");
        m_catStatusLabel->setText("Category ID must be exactly 2 numeric digits (e.g. 10 through 99).");
        return;
    }
    if (id[0] == '0') {
        m_catStatusLabel->setStyleSheet("font-size: 12px; color: #DC2626;");
        m_catStatusLabel->setText("Category ID must start with a non-zero digit (10-99).");
        return;
    }
    if (name.isEmpty()) {
        m_catStatusLabel->setStyleSheet("font-size: 12px; color: #DC2626;");
        m_catStatusLabel->setText("Category Name cannot be empty.");
        return;
    }
    if (acr.isEmpty() || acr.length() > 8) {
        m_catStatusLabel->setStyleSheet("font-size: 12px; color: #DC2626;");
        m_catStatusLabel->setText("Category Acronym must be between 1 and 8 characters.");
        return;
    }

    CategoryInfo cat;
    cat.id = id.toStdString();
    cat.name = name.toStdString();
    cat.acronym = acr.toUpper().toStdString();

    std::string err;
    if (m_productService && m_productService->addCategory(cat, &err)) {
        refreshCatalog();
        m_catIdInput->clear();
        m_catIdInput->setReadOnly(false);
        m_catNameInput->clear();
        m_catAcrInput->clear();
        m_catStatusLabel->setStyleSheet("font-size: 12px; color: #166534;");
        m_catStatusLabel->setText(QString("Category '%1' (ID %2) added successfully and saved to PCategory.txt.")
                                    .arg(name).arg(id));
    } else {
        m_catStatusLabel->setStyleSheet("font-size: 12px; color: #DC2626;");
        m_catStatusLabel->setText(QString("Failed to add category: %1").arg(QString::fromStdString(err)));
    }
}

void ManagerView::onEditCategoryClicked() {
    int row = m_categoryTable->currentRow();
    if (row < 0 || row >= static_cast<int>(m_categories.size())) {
        m_catStatusLabel->setStyleSheet("font-size: 12px; color: #DC2626;");
        m_catStatusLabel->setText("Select a category from the table to edit it.");
        return;
    }
    QString id   = m_catIdInput->text().trimmed();
    QString name = m_catNameInput->text().trimmed();
    QString acr  = m_catAcrInput->text().trimmed();

    if (name.isEmpty()) {
        m_catStatusLabel->setStyleSheet("font-size: 12px; color: #DC2626;");
        m_catStatusLabel->setText("Category Name cannot be empty.");
        return;
    }
    if (acr.isEmpty() || acr.length() > 8) {
        m_catStatusLabel->setStyleSheet("font-size: 12px; color: #DC2626;");
        m_catStatusLabel->setText("Category Acronym must be between 1 and 8 characters.");
        return;
    }

    CategoryInfo cat;
    cat.id = id.toStdString();
    cat.name = name.toStdString();
    cat.acronym = acr.toUpper().toStdString();

    std::string err;
    if (m_productService && m_productService->updateCategory(cat, &err)) {
        refreshCatalog();
        m_catStatusLabel->setStyleSheet("font-size: 12px; color: #166534;");
        m_catStatusLabel->setText(QString("Category '%1' updated successfully and persisted to PCategory.txt.").arg(name));
    } else {
        m_catStatusLabel->setStyleSheet("font-size: 12px; color: #DC2626;");
        m_catStatusLabel->setText(QString("Failed to update category: %1").arg(QString::fromStdString(err)));
    }
}

// ---------------------------------------------------------------------------
// Product slots
// ---------------------------------------------------------------------------
void ManagerView::onProductTableSelectionChanged() {
    int row = m_productTable->currentRow();
    m_editProductBtn->setEnabled(row >= 0 && m_productTable->item(row, 1) != nullptr);
}

void ManagerView::onProductSearchChanged(const QString& text) {
    if (!m_productService) return;
    std::string q = text.trimmed().toLower().toStdString();
    if (q.empty()) {
        refreshProductTable(m_productService->getAllProducts());
        return;
    }
    std::vector<ProductInfo> filtered;
    for (const auto& p : m_productService->getAllProducts()) {
        std::string bar  = p.barcode;
        std::string acr  = p.acronym;
        std::string name = p.name;
        std::string cat  = p.category;
        std::transform(acr.begin(),  acr.end(),  acr.begin(),  ::tolower);
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        std::transform(cat.begin(),  cat.end(),  cat.begin(),  ::tolower);
        if (bar.find(q) != std::string::npos ||
            acr.find(q) != std::string::npos ||
            name.find(q) != std::string::npos ||
            cat.find(q) != std::string::npos) {
            filtered.push_back(p);
        }
    }
    refreshProductTable(filtered);
}

void ManagerView::onAddProductClicked() {
    showProductDialog(false);
}

void ManagerView::onEditProductClicked() {
    int row = m_productTable->currentRow();
    if (row < 0 || !m_productTable->item(row, 1)) {
        QMessageBox::information(this, "Select Product",
            "Please select a product row from the table first.");
        return;
    }
    std::string barcode = m_productTable->item(row, 1)->text().toStdString();
    ProductInfo existing;
    if (m_productService && m_productService->findProductByBarcode(barcode, existing)) {
        showProductDialog(true, existing);
    }
}

void ManagerView::onInspectClicked() {
    // Works for both Product and Inventory tabs depending on which table is focused
    QTableWidget* activeTable = nullptr;
    int tabIdx = m_tabs->currentIndex();
    if (tabIdx == 1) activeTable = m_productTable;
    else if (tabIdx == 2) activeTable = m_inventoryTable;
    if (!activeTable) return;

    int row = activeTable->currentRow();
    if (row >= 0 && activeTable->item(row, tabIdx == 1 ? 1 : 0)) {
        int barcodeCol = (tabIdx == 1) ? 1 : 0;
        emit inspectProductRequested(activeTable->item(row, barcodeCol)->text().toStdString());
    } else {
        QMessageBox::information(this, "Select Product",
            "Please select a product row to view its full details.");
    }
}

void ManagerView::showProductDialog(bool isEdit, const ProductInfo& existing) {
    if (m_productService) {
        m_categories = m_productService->getAllCategories();
    }

    QDialog dlg(this);
    dlg.setWindowTitle(isEdit ? "Edit Product" : "Add New Product");
    dlg.setMinimumWidth(500);

    auto* form = new QFormLayout(&dlg);
    form->setSpacing(10);
    form->setContentsMargins(16, 16, 16, 16);

    auto makeLE = [&](const QString& val) {
        auto* le = new QLineEdit(&dlg);
        le->setText(val);
        le->setStyleSheet(
            "background-color: #FFFFFF; color: #111827; border: 1px solid #D1D5DB;"
            "border-radius: 4px; padding: 6px 10px; font-size: 13px;"
        );
        return le;
    };
    auto makeDSpin = [&](double val, double min, double max) {
        auto* sp = new QDoubleSpinBox(&dlg);
        sp->setRange(min, max);
        sp->setDecimals(2);
        sp->setValue(val);
        sp->setStyleSheet(
            "background-color: #FFFFFF; color: #111827; border: 1px solid #D1D5DB;"
            "border-radius: 4px; padding: 4px 8px; font-size: 13px;"
        );
        return sp;
    };
    auto makeISpin = [&](int val) {
        auto* sp = new QSpinBox(&dlg);
        sp->setRange(0, 999999);
        sp->setValue(val);
        sp->setStyleSheet(
            "background-color: #FFFFFF; color: #111827; border: 1px solid #D1D5DB;"
            "border-radius: 4px; padding: 4px 8px; font-size: 13px;"
        );
        return sp;
    };

    auto* barcodeLE = makeLE(QString::fromStdString(existing.barcode));
    auto* nameLE    = makeLE(QString::fromStdString(existing.name));
    auto* acronymLE = makeLE(QString::fromStdString(existing.acronym));
    auto* unitLE    = makeLE(QString::fromStdString(existing.unit));

    // Category combo populated from fresh catalog categories
    auto* catCombo = new QComboBox(&dlg);
    catCombo->setStyleSheet(
        "background-color: #FFFFFF; color: #111827; border: 1px solid #D1D5DB;"
        "border-radius: 4px; padding: 6px 10px; font-size: 13px;"
    );
    int catIdx = 0;
    for (size_t i = 0; i < m_categories.size(); ++i) {
        const auto& c = m_categories[i];
        QString label = QString("[%1] %2").arg(QString::fromStdString(c.id)).arg(QString::fromStdString(c.name));
        catCombo->addItem(label, QString::fromStdString(c.id));
        if (c.id == existing.categoryId || c.name == existing.category ||
            (!existing.barcode.empty() && existing.barcode.rfind(c.id, 0) == 0)) {
            catIdx = static_cast<int>(i);
        }
    }
    if (catCombo->count() > 0) {
        catCombo->setCurrentIndex(catIdx);
    }

    if (!isEdit && barcodeLE->text().isEmpty() && catCombo->count() > 0) {
        barcodeLE->setText(catCombo->currentData().toString());
    }
    QObject::connect(catCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int idx){
        if (!isEdit && idx >= 0) {
            QString selectedCatId = catCombo->itemData(idx).toString();
            QString currentBarcode = barcodeLE->text().trimmed();
            if (currentBarcode.isEmpty() || currentBarcode.length() <= 2) {
                barcodeLE->setText(selectedCatId);
            }
        }
    });

    auto* priceSpin   = makeDSpin(existing.unitPrice,       0.0, 999999.0);
    auto* taxSpin     = makeDSpin(existing.taxRate * 100.0, 0.0, 100.0);
    taxSpin->setSuffix(" %");
    auto* discSpin    = makeDSpin(existing.discountPerUnit, 0.0, 999999.0);
    auto* stockSpin   = makeISpin(existing.availableQuantity);

    form->addRow("Barcode (6-digit) *:", barcodeLE);
    form->addRow("Product Name *:",      nameLE);
    form->addRow("Acronym (max 8) *:",   acronymLE);
    form->addRow("Category (2-digit ID) *:", catCombo);
    form->addRow("Unit of Measure *:",   unitLE);
    form->addRow("Unit Price (Rs.) *:",  priceSpin);
    form->addRow("Tax Rate:",            taxSpin);
    form->addRow("Discount/Unit (Rs.):", discSpin);
    form->addRow("Available Stock:",     stockSpin);

    auto* msgLabel = new QLabel("", &dlg);
    msgLabel->setStyleSheet("font-size: 12px; color: #DC2626; font-weight: bold;");
    msgLabel->setWordWrap(true);
    form->addRow("", msgLabel);

    auto* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    btns->button(QDialogButtonBox::Ok)->setText(isEdit ? "Save Changes" : "Add Product");
    btns->button(QDialogButtonBox::Ok)->setStyleSheet(
        "background-color: #166534; color: #FFFFFF; font-weight: bold;"
        "padding: 7px 18px; border-radius: 4px; border: none;"
    );
    form->addRow(btns);

    connect(btns, &QDialogButtonBox::accepted, [&](){
        QString barcode = barcodeLE->text().trimmed();
        QString name    = nameLE->text().trimmed();
        QString acronym = acronymLE->text().trimmed();
        QString unit    = unitLE->text().trimmed();

        if (barcode.isEmpty() || name.isEmpty() || acronym.isEmpty() || unit.isEmpty()) {
            QString err = "Barcode, Product Name, Acronym and Unit of Measure are required fields.";
            msgLabel->setText(err);
            QMessageBox::warning(&dlg, "Validation Error", err);
            return;
        }

        QString selectedCatId = catCombo->currentData().toString();
        if (selectedCatId.isEmpty() && catCombo->currentIndex() >= 0 &&
            catCombo->currentIndex() < static_cast<int>(m_categories.size())) {
            selectedCatId = QString::fromStdString(m_categories[catCombo->currentIndex()].id);
        }

        // 1. Check barcode contains at least 2 characters
        if (barcode.length() < 2) {
            QString err = "Invalid barcode: Barcode must contain at least 2 characters.";
            msgLabel->setText(err);
            QMessageBox::warning(&dlg, "Invalid Barcode", err);
            return;
        }

        // 2. Check first two characters are numeric digits
        if (!barcode[0].isDigit() || !barcode[1].isDigit()) {
            QString err = "Invalid barcode: The first two characters must be numeric digits.";
            msgLabel->setText(err);
            QMessageBox::warning(&dlg, "Invalid Barcode", err);
            return;
        }

        // 3. Compare those exact two digits with the selected Category ID
        QString barcodePrefix = barcode.left(2);
        if (barcodePrefix != selectedCatId) {
            QString err = QString("Invalid barcode: The first two digits must match the selected Category ID (%1).\n\n"
                                  "Selected Category ID : %1\n"
                                  "Entered Barcode      : %2 (starts with '%3')\n\n"
                                  "Please correct the barcode so that it begins with '%1' (e.g. %10001).")
                            .arg(selectedCatId)
                            .arg(barcode)
                            .arg(barcodePrefix);
            msgLabel->setText(QString("Invalid barcode: The first two digits must match the selected Category ID (%1).").arg(selectedCatId));
            QMessageBox::warning(&dlg, "Barcode Category Mismatch", err);
            return;
        }

        // 4. Validate exact 6-digit length & format
        if (barcode.length() != 6) {
            QString err = "Invalid barcode: Barcode must be exactly 6 digits.";
            msgLabel->setText(err);
            QMessageBox::warning(&dlg, "Invalid Barcode", err);
            return;
        }
        for (QChar ch : barcode) {
            if (!ch.isDigit()) {
                QString err = "Invalid barcode: Barcode must contain numeric digits only.";
                msgLabel->setText(err);
                QMessageBox::warning(&dlg, "Invalid Barcode", err);
                return;
            }
        }
        if (barcode[0] == '0') {
            QString err = "Invalid barcode: Barcode must start with a non-zero digit (1-9).";
            msgLabel->setText(err);
            QMessageBox::warning(&dlg, "Invalid Barcode", err);
            return;
        }
        if (acronym.length() > 8) {
            QString err = "Product acronym must be 8 characters or fewer.";
            msgLabel->setText(err);
            QMessageBox::warning(&dlg, "Invalid Acronym", err);
            return;
        }

        if (discSpin->value() > priceSpin->value()) {
            QString err = QString("Discount/Unit (Rs. %1) cannot exceed Unit Price (Rs. %2).")
                            .arg(discSpin->value(), 0, 'f', 2).arg(priceSpin->value(), 0, 'f', 2);
            msgLabel->setText(err);
            QMessageBox::warning(&dlg, "Invalid Discount", err);
            return;
        }

        // Build ProductInfo with explicit categoryId and persist via service
        ProductInfo updated;
        updated.id                = existing.id;
        updated.barcode           = barcode.toStdString();
        updated.name              = name.toStdString();
        updated.acronym           = acronym.toStdString();
        updated.categoryId        = selectedCatId.toStdString();
        QString catText           = catCombo->currentText();
        int bracketClose          = catText.indexOf("] ");
        updated.category          = (bracketClose >= 0) ? catText.mid(bracketClose + 2).toStdString() : catText.toStdString();
        updated.unit              = unit.toStdString();
        updated.unitPrice         = priceSpin->value();
        updated.taxRate           = taxSpin->value() / 100.0;
        updated.discountPerUnit   = discSpin->value();
        updated.availableQuantity = stockSpin->value();

        std::string err;
        if (m_productService) {
            bool success = false;
            if (isEdit) {
                success = m_productService->updateProduct(updated, &err);
            } else {
                success = m_productService->addProduct(updated, &err);
            }
            if (!success) {
                msgLabel->setText(QString::fromStdString(err));
                QMessageBox::warning(&dlg, isEdit ? "Update Failed" : "Add Product Failed",
                    QString::fromStdString(err));
                return;
            }
        }

        dlg.accept();
        refreshCatalog();
        QMessageBox::information(this, isEdit ? "Product Updated" : "Product Added",
            isEdit ?
            QString("Product '%1' (%2) updated successfully and saved to Product.txt.")
                .arg(name).arg(barcode) :
            QString("Product '%1' (%2) added successfully and saved to Product.txt.")
                .arg(name).arg(barcode));
    });
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    dlg.exec();
}

// ---------------------------------------------------------------------------
// Inventory slots
// ---------------------------------------------------------------------------
void ManagerView::onInventorySearchChanged(const QString& text) {
    if (!m_productService) return;
    std::string q = text.trimmed().toLower().toStdString();
    if (q.empty()) {
        refreshInventoryTable(m_productService->getAllProducts());
        return;
    }
    std::vector<ProductInfo> filtered;
    for (const auto& p : m_productService->getAllProducts()) {
        std::string bar  = p.barcode;
        std::string name = p.name;
        std::string cat  = p.category;
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        std::transform(cat.begin(),  cat.end(),  cat.begin(),  ::tolower);
        if (bar.find(q) != std::string::npos ||
            name.find(q) != std::string::npos ||
            cat.find(q) != std::string::npos) {
            filtered.push_back(p);
        }
    }
    refreshInventoryTable(filtered);
}

void ManagerView::onUpdateStockClicked() {
    QTableWidget* t = (m_tabs->currentIndex() == 2) ? m_inventoryTable : m_productTable;
    int barcodeCol = (m_tabs->currentIndex() == 2) ? 0 : 1;
    int nameCol    = (m_tabs->currentIndex() == 2) ? 1 : 3;
    int stockCol   = (m_tabs->currentIndex() == 2) ? 5 : 9;

    int row = t->currentRow();
    if (row < 0 || !t->item(row, barcodeCol)) {
        QMessageBox::information(this, "Select Product",
            "Please select a product row from the table to update its stock.");
        return;
    }

    std::string barcode    = t->item(row, barcodeCol)->text().toStdString();
    QString     productName = t->item(row, nameCol) ? t->item(row, nameCol)->text() : "Product";
    int         currentStock = t->item(row, stockCol) ? t->item(row, stockCol)->text().toInt() : 0;

    bool ok = false;
    int newStock = QInputDialog::getInt(
        this,
        "Update Inventory Stock",
        QString("New stock quantity for '%1' (%2):")
            .arg(productName).arg(QString::fromStdString(barcode)),
        currentStock, 0, 999999, 1, &ok
    );
    if (!ok || newStock == currentStock) return;

    std::string err;
    if (m_productService && m_productService->updateProductStock(barcode, newStock, &err)) {
        refreshCatalog();
        QMessageBox::information(this, "Stock Updated",
            QString("Stock for '%1' updated to %2 and saved.").arg(productName).arg(newStock));
    } else {
        QMessageBox::critical(this, "Stock Update Failed",
            QString("Failed to update stock: %1").arg(QString::fromStdString(err)));
    }
}

// ---------------------------------------------------------------------------
// Discount slots
// ---------------------------------------------------------------------------
void ManagerView::onSeasonTableSelectionChanged() {
    int row = m_seasonTable->currentRow();
    bool sel = (row >= 0 && row < static_cast<int>(m_seasons.size()));
    m_editSeasonBtn->setEnabled(sel);
    m_toggleSeasonBtn->setEnabled(sel);
    if (sel) {
        m_toggleSeasonBtn->setText(m_seasons[row].active ? "Deactivate Season" : "Activate Season");
    }
}

void ManagerView::onAddSeasonClicked() {
    showSeasonDialog(false);
}

void ManagerView::onEditSeasonClicked() {
    int row = m_seasonTable->currentRow();
    if (row < 0 || row >= static_cast<int>(m_seasons.size())) {
        QMessageBox::information(this, "Select Season", "Please select a season to edit.");
        return;
    }
    showSeasonDialog(true, row);
}

void ManagerView::showSeasonDialog(bool isEdit, int index) {
    QDialog dlg(this);
    dlg.setWindowTitle(isEdit ? "Edit Season" : "Add Seasonal Discount");
    dlg.setMinimumWidth(380);

    auto* form = new QFormLayout(&dlg);
    form->setSpacing(10);
    form->setContentsMargins(16, 16, 16, 16);

    SeasonRecord current = (isEdit && index >= 0) ? m_seasons[index] : SeasonRecord{};

    auto* nameLE = new QLineEdit(&dlg);
    nameLE->setText(current.name);
    nameLE->setStyleSheet(
        "background-color: #FFFFFF; color: #111827; border: 1px solid #D1D5DB;"
        "border-radius: 4px; padding: 6px 10px; font-size: 13px;"
    );
    nameLE->setPlaceholderText("e.g. Eid Sale");

    auto* startEdit = new QDateEdit(&dlg);
    startEdit->setCalendarPopup(true);
    startEdit->setDisplayFormat("yyyy-MM-dd");
    if (!current.startDate.isEmpty()) {
        startEdit->setDate(QDate::fromString(current.startDate, "yyyy-MM-dd"));
    } else {
        startEdit->setDate(QDate::currentDate());
    }
    startEdit->setStyleSheet(
        "background-color: #FFFFFF; color: #111827; border: 1px solid #D1D5DB;"
        "border-radius: 4px; padding: 4px 8px; font-size: 13px;"
    );

    auto* endEdit = new QDateEdit(&dlg);
    endEdit->setCalendarPopup(true);
    endEdit->setDisplayFormat("yyyy-MM-dd");
    if (!current.endDate.isEmpty()) {
        endEdit->setDate(QDate::fromString(current.endDate, "yyyy-MM-dd"));
    } else {
        endEdit->setDate(QDate::currentDate().addDays(7));
    }
    endEdit->setStyleSheet(
        "background-color: #FFFFFF; color: #111827; border: 1px solid #D1D5DB;"
        "border-radius: 4px; padding: 4px 8px; font-size: 13px;"
    );

    auto* discSpin = new QDoubleSpinBox(&dlg);
    discSpin->setRange(0.0, 100.0);
    discSpin->setDecimals(1);
    discSpin->setSuffix(" %");
    discSpin->setValue(current.discountPercent);
    discSpin->setStyleSheet(
        "background-color: #FFFFFF; color: #111827; border: 1px solid #D1D5DB;"
        "border-radius: 4px; padding: 4px 8px; font-size: 13px;"
    );

    form->addRow("Season Name *:",    nameLE);
    form->addRow("Start Date:",       startEdit);
    form->addRow("End Date:",         endEdit);
    form->addRow("Discount %:",       discSpin);

    auto* msgLabel = new QLabel("", &dlg);
    msgLabel->setStyleSheet("font-size: 12px; color: #DC2626;");
    form->addRow("", msgLabel);

    auto* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    btns->button(QDialogButtonBox::Ok)->setText(isEdit ? "Save Changes" : "Add Season");
    btns->button(QDialogButtonBox::Ok)->setStyleSheet(
        "background-color: #166534; color: #FFFFFF; font-weight: bold;"
        "padding: 7px 18px; border-radius: 4px; border: none;"
    );
    form->addRow(btns);

    connect(btns, &QDialogButtonBox::accepted, [&](){
        QString name = nameLE->text().trimmed();
        if (name.isEmpty()) {
            msgLabel->setText("Season name is required.");
            return;
        }
        SeasonRecord rec;
        rec.name           = name;
        rec.startDate      = startEdit->date().toString("yyyy-MM-dd");
        rec.endDate        = endEdit->date().toString("yyyy-MM-dd");
        rec.discountPercent = discSpin->value();
        rec.active         = isEdit ? m_seasons[index].active : false;

        if (isEdit) m_seasons[index] = rec;
        else        m_seasons.push_back(rec);

        dlg.accept();
        refreshSeasonTable();
        m_discountStatusLabel->setStyleSheet("font-size: 12px; color: #166534;");
        m_discountStatusLabel->setText(isEdit ?
            QString("Season '%1' updated.").arg(name) :
            QString("Season '%1' added.").arg(name));
    });
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    dlg.exec();
}

void ManagerView::onToggleSeasonClicked() {
    int row = m_seasonTable->currentRow();
    if (row < 0 || row >= static_cast<int>(m_seasons.size())) return;
    m_seasons[row].active = !m_seasons[row].active;
    refreshSeasonTable();
    m_discountStatusLabel->setStyleSheet("font-size: 12px; color: #166534;");
    m_discountStatusLabel->setText(
        QString("Season '%1' is now %2.")
        .arg(m_seasons[row].name)
        .arg(m_seasons[row].active ? "ACTIVE" : "inactive"));
    m_toggleSeasonBtn->setText(m_seasons[row].active ? "Deactivate Season" : "Activate Season");
}

void ManagerView::onApplySpecialDiscountClicked() {
    double val = m_specialDiscountSpin->value();
    m_currentSpecialDiscount = val;
    if (val <= 0.0) {
        m_specialDiscountLabel->setText("Special discount cleared (0%). No global discount active.");
        m_discountStatusLabel->setText("Special discount removed.");
    } else {
        m_specialDiscountLabel->setText(
            QString("ACTIVE: %1% special discount applied to all new bills.")
            .arg(val, 0, 'f', 2));
        m_discountStatusLabel->setStyleSheet("font-size: 12px; color: #166534;");
        m_discountStatusLabel->setText(
            QString("Special discount of %1% is now active.").arg(val, 0, 'f', 2));
    }
    // NOTE: Connecting this discount value to the billing engine (Billing.h's setBillDiscount)
    // is Eman 2's billing backend responsibility. The GUI is ready; integration point is
    // ProductBillingService::generateBill() where BillDiscount can be applied.
}

// ---------------------------------------------------------------------------
// Reports
// ---------------------------------------------------------------------------
void ManagerView::onGenerateReportClicked() {
    int type = m_reportTypeCombo->currentIndex();
    QString from = m_fromDateEdit->date().toString("yyyy-MM-dd");
    QString to   = m_toDateEdit->date().toString("yyyy-MM-dd");
    generateReport(type, from, to);
}

void ManagerView::onCustomDateRangeClicked() {
    m_reportTypeCombo->setCurrentIndex(6); // Custom Date Range
    onGenerateReportClicked();
}

struct MgrOrder {
    int orderId{0};
    QString customerName;
    QString date;
    QString status;
    QString paymentMethod;
    QString paymentStatus;
    double totalAmount{0.0};
};

static std::vector<MgrOrder> loadOrdersFromDisk() {
    std::vector<MgrOrder> list;
    std::ifstream file("Order.txt");
    if (!file.is_open()) file.open("../Order.txt");
    if (!file.is_open()) return list;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::stringstream ss(line);
        std::string idStr, name, date, status, method, payStat, totStr;
        std::getline(ss, idStr, '|');
        std::getline(ss, name, '|');
        std::getline(ss, date, '|');
        std::getline(ss, status, '|');
        std::getline(ss, method, '|');
        std::getline(ss, payStat, '|');
        std::getline(ss, totStr, '|');
        MgrOrder o;
        o.orderId = std::atoi(idStr.c_str());
        o.customerName = QString::fromStdString(name);
        o.date = QString::fromStdString(date);
        o.status = QString::fromStdString(status);
        o.paymentMethod = QString::fromStdString(method);
        o.paymentStatus = QString::fromStdString(payStat);
        o.totalAmount = std::atof(totStr.c_str());
        list.push_back(o);
    }
    return list;
}

struct MgrBill {
    int billId{0};
    int orderId{0};
    QString date;
    double subtotal{0.0};
    double discount{0.0};
    double total{0.0};
    double amountPaid{0.0};
    double changeReturned{0.0};
    QString paymentMethod;
    QString paymentStatus;
};

static std::vector<MgrBill> loadBillsFromDisk() {
    std::vector<MgrBill> list;
    std::ifstream file("Bill.txt");
    if (!file.is_open()) file.open("../Bill.txt");
    if (!file.is_open()) return list;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::stringstream ss(line);
        std::string bId, oId, date, subStr, discStr, totStr, paidStr, chgStr, method, payStat;
        std::getline(ss, bId, '|');
        std::getline(ss, oId, '|');
        std::getline(ss, date, '|');
        std::getline(ss, subStr, '|');
        std::getline(ss, discStr, '|');
        std::getline(ss, totStr, '|');
        std::getline(ss, paidStr, '|');
        std::getline(ss, chgStr, '|');
        std::getline(ss, method, '|');
        std::getline(ss, payStat, '|');
        MgrBill b;
        b.billId = std::atoi(bId.c_str());
        b.orderId = std::atoi(oId.c_str());
        b.date = QString::fromStdString(date);
        b.subtotal = std::atof(subStr.c_str());
        b.discount = std::atof(discStr.c_str());
        b.total = std::atof(totStr.c_str());
        b.amountPaid = std::atof(paidStr.c_str());
        b.changeReturned = std::atof(chgStr.c_str());
        b.paymentMethod = QString::fromStdString(method);
        b.paymentStatus = QString::fromStdString(payStat);
        list.push_back(b);
    }
    return list;
}

struct MgrDetail {
    int orderId{0};
    QString productId;
    QString barcode;
    int quantity{0};
};

static std::vector<MgrDetail> loadDetailsFromDisk() {
    std::vector<MgrDetail> list;
    std::ifstream file("OrderDetail.txt");
    if (!file.is_open()) file.open("../OrderDetail.txt");
    if (!file.is_open()) return list;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::stringstream ss(line);
        std::string oId, pId, bar, qStr;
        std::getline(ss, oId, '|');
        std::getline(ss, pId, '|');
        std::getline(ss, bar, '|');
        std::getline(ss, qStr, '|');
        MgrDetail d;
        d.orderId = std::atoi(oId.c_str());
        d.productId = QString::fromStdString(pId);
        d.barcode = QString::fromStdString(bar);
        d.quantity = std::atoi(qStr.c_str());
        list.push_back(d);
    }
    return list;
}

void ManagerView::generateReport(int type, const QString& from, const QString& to) {
    if (!m_productService) {
        m_reportSummaryLabel->setText("Product service not available.");
        return;
    }

    auto products = m_productService->getAllProducts();
    auto orders   = loadOrdersFromDisk();
    auto bills    = loadBillsFromDisk();
    auto details  = loadDetailsFromDisk();

    auto setTableItem = [&](int row, int col, const QString& txt, Qt::Alignment align = Qt::AlignLeft | Qt::AlignVCenter) {
        auto* it = new QTableWidgetItem(txt);
        it->setTextAlignment(align);
        m_reportTable->setItem(row, col, it);
    };

    // -----------------------------------------------------------------------
    // Report 0: Incomplete Orders (with total amounts)
    // -----------------------------------------------------------------------
    if (type == 0) {
        m_reportTable->setColumnCount(6);
        m_reportTable->setHorizontalHeaderLabels(
            {"Order ID", "Customer Name", "Date & Time", "Order Status", "Payment Status", "Total Amount (Rs.)"});
        m_reportTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

        std::vector<MgrOrder> incomplete;
        double sum = 0.0;
        for (const auto& o : orders) {
            if (o.status != "Completed" && o.status != "Cancelled") {
                incomplete.push_back(o);
                sum += o.totalAmount;
            }
        }

        m_reportTable->setRowCount(static_cast<int>(incomplete.size()));
        for (int i = 0; i < static_cast<int>(incomplete.size()); ++i) {
            const auto& o = incomplete[i];
            setTableItem(i, 0, QString::number(o.orderId), Qt::AlignCenter);
            setTableItem(i, 1, o.customerName);
            setTableItem(i, 2, o.date, Qt::AlignCenter);
            setTableItem(i, 3, o.status, Qt::AlignCenter);
            setTableItem(i, 4, o.paymentStatus, Qt::AlignCenter);
            setTableItem(i, 5, QString("Rs. %1").arg(o.totalAmount, 0, 'f', 2), Qt::AlignRight | Qt::AlignVCenter);
        }

        if (incomplete.empty()) {
            m_reportSummaryLabel->setText("No incomplete orders currently on record in Order.txt.");
        } else {
            m_reportSummaryLabel->setText(
                QString("<b>Incomplete Orders:</b> %1 record(s) &nbsp;&nbsp;|&nbsp;&nbsp; <b>Total Outstanding Balance:</b> Rs. %2")
                .arg(incomplete.size()).arg(sum, 0, 'f', 2));
        }

    // -----------------------------------------------------------------------
    // Report 1: Completed Orders (with total amounts)
    // -----------------------------------------------------------------------
    } else if (type == 1) {
        m_reportTable->setColumnCount(6);
        m_reportTable->setHorizontalHeaderLabels(
            {"Order ID", "Customer Name", "Date & Time", "Order Status", "Payment Mode", "Total Amount (Rs.)"});
        m_reportTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

        std::vector<MgrOrder> completed;
        double sum = 0.0;
        for (const auto& o : orders) {
            if (o.status == "Completed" || o.paymentStatus == "Paid") {
                completed.push_back(o);
                sum += o.totalAmount;
            }
        }

        m_reportTable->setRowCount(static_cast<int>(completed.size()));
        for (int i = 0; i < static_cast<int>(completed.size()); ++i) {
            const auto& o = completed[i];
            setTableItem(i, 0, QString::number(o.orderId), Qt::AlignCenter);
            setTableItem(i, 1, o.customerName);
            setTableItem(i, 2, o.date, Qt::AlignCenter);
            setTableItem(i, 3, o.status, Qt::AlignCenter);
            setTableItem(i, 4, o.paymentMethod, Qt::AlignCenter);
            setTableItem(i, 5, QString("Rs. %1").arg(o.totalAmount, 0, 'f', 2), Qt::AlignRight | Qt::AlignVCenter);
        }

        if (completed.empty()) {
            m_reportSummaryLabel->setText("No completed orders currently on record in Order.txt.");
        } else {
            m_reportSummaryLabel->setText(
                QString("<b>Completed Orders:</b> %1 record(s) &nbsp;&nbsp;|&nbsp;&nbsp; <b>Total Completed Sales Revenue:</b> Rs. %2")
                .arg(completed.size()).arg(sum, 0, 'f', 2));
        }

    // -----------------------------------------------------------------------
    // Report 2: Daily Cash Sales
    // -----------------------------------------------------------------------
    } else if (type == 2) {
        m_reportTable->setColumnCount(6);
        m_reportTable->setHorizontalHeaderLabels(
            {"Bill ID", "Order ID", "Date & Time", "Payment Method", "Amount Paid (Rs.)", "Total Cash (Rs.)"});
        m_reportTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);

        QString todayStr = QDate::currentDate().toString("yyyy-MM-dd");
        std::vector<MgrBill> cashBills;
        double sum = 0.0;
        for (const auto& b : bills) {
            if (b.paymentMethod == "Cash") {
                cashBills.push_back(b);
                sum += b.total;
            }
        }

        m_reportTable->setRowCount(static_cast<int>(cashBills.size()));
        for (int i = 0; i < static_cast<int>(cashBills.size()); ++i) {
            const auto& b = cashBills[i];
            setTableItem(i, 0, QString::number(b.billId), Qt::AlignCenter);
            setTableItem(i, 1, QString::number(b.orderId), Qt::AlignCenter);
            setTableItem(i, 2, b.date);
            setTableItem(i, 3, b.paymentMethod, Qt::AlignCenter);
            setTableItem(i, 4, QString("Rs. %1").arg(b.amountPaid, 0, 'f', 2), Qt::AlignRight | Qt::AlignVCenter);
            setTableItem(i, 5, QString("Rs. %1").arg(b.total, 0, 'f', 2), Qt::AlignRight | Qt::AlignVCenter);
        }

        m_reportSummaryLabel->setText(
            QString("<b>Daily Cash Sales:</b> %1 recorded cash transaction(s) &nbsp;&nbsp;|&nbsp;&nbsp; <b>Total Cash Revenue:</b> Rs. %2")
            .arg(cashBills.size()).arg(sum, 0, 'f', 2));

    // -----------------------------------------------------------------------
    // Report 3: Weekly Cash Sales
    // -----------------------------------------------------------------------
    } else if (type == 3) {
        m_reportTable->setColumnCount(6);
        m_reportTable->setHorizontalHeaderLabels(
            {"Bill ID", "Order ID", "Date & Time", "Payment Method", "Amount Paid (Rs.)", "Cash Total (Rs.)"});
        m_reportTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);

        std::vector<MgrBill> weeklyCash;
        double sum = 0.0;
        for (const auto& b : bills) {
            if (b.paymentMethod == "Cash") {
                QDate billDate = QDate::fromString(b.date.left(10), "yyyy-MM-dd");
                if (!billDate.isValid() || billDate.daysTo(QDate::currentDate()) <= 7) {
                    weeklyCash.push_back(b);
                    sum += b.total;
                }
            }
        }

        m_reportTable->setRowCount(static_cast<int>(weeklyCash.size()));
        for (int i = 0; i < static_cast<int>(weeklyCash.size()); ++i) {
            const auto& b = weeklyCash[i];
            setTableItem(i, 0, QString::number(b.billId), Qt::AlignCenter);
            setTableItem(i, 1, QString::number(b.orderId), Qt::AlignCenter);
            setTableItem(i, 2, b.date);
            setTableItem(i, 3, b.paymentMethod, Qt::AlignCenter);
            setTableItem(i, 4, QString("Rs. %1").arg(b.amountPaid, 0, 'f', 2), Qt::AlignRight | Qt::AlignVCenter);
            setTableItem(i, 5, QString("Rs. %1").arg(b.total, 0, 'f', 2), Qt::AlignRight | Qt::AlignVCenter);
        }

        m_reportSummaryLabel->setText(
            QString("<b>Weekly Cash Sales (Past 7 Days):</b> %1 transaction(s) &nbsp;&nbsp;|&nbsp;&nbsp; <b>Total Weekly Cash:</b> Rs. %2")
            .arg(weeklyCash.size()).arg(sum, 0, 'f', 2));

    // -----------------------------------------------------------------------
    // Report 4: Monthly Cash Sales
    // -----------------------------------------------------------------------
    } else if (type == 4) {
        m_reportTable->setColumnCount(6);
        m_reportTable->setHorizontalHeaderLabels(
            {"Bill ID", "Order ID", "Date & Time", "Payment Method", "Amount Paid (Rs.)", "Cash Total (Rs.)"});
        m_reportTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);

        std::vector<MgrBill> monthlyCash;
        double sum = 0.0;
        for (const auto& b : bills) {
            if (b.paymentMethod == "Cash") {
                QDate billDate = QDate::fromString(b.date.left(10), "yyyy-MM-dd");
                if (!billDate.isValid() || billDate.daysTo(QDate::currentDate()) <= 30) {
                    monthlyCash.push_back(b);
                    sum += b.total;
                }
            }
        }

        m_reportTable->setRowCount(static_cast<int>(monthlyCash.size()));
        for (int i = 0; i < static_cast<int>(monthlyCash.size()); ++i) {
            const auto& b = monthlyCash[i];
            setTableItem(i, 0, QString::number(b.billId), Qt::AlignCenter);
            setTableItem(i, 1, QString::number(b.orderId), Qt::AlignCenter);
            setTableItem(i, 2, b.date);
            setTableItem(i, 3, b.paymentMethod, Qt::AlignCenter);
            setTableItem(i, 4, QString("Rs. %1").arg(b.amountPaid, 0, 'f', 2), Qt::AlignRight | Qt::AlignVCenter);
            setTableItem(i, 5, QString("Rs. %1").arg(b.total, 0, 'f', 2), Qt::AlignRight | Qt::AlignVCenter);
        }

        m_reportSummaryLabel->setText(
            QString("<b>Monthly Cash Sales (Past 30 Days):</b> %1 transaction(s) &nbsp;&nbsp;|&nbsp;&nbsp; <b>Total Monthly Cash:</b> Rs. %2")
            .arg(monthlyCash.size()).arg(sum, 0, 'f', 2));

    // -----------------------------------------------------------------------
    // Report 5: Yearly Cash Sales
    // -----------------------------------------------------------------------
    } else if (type == 5) {
        m_reportTable->setColumnCount(6);
        m_reportTable->setHorizontalHeaderLabels(
            {"Bill ID", "Order ID", "Date & Time", "Payment Method", "Amount Paid (Rs.)", "Cash Total (Rs.)"});
        m_reportTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);

        QString currentYear = QString::number(QDate::currentDate().year());
        std::vector<MgrBill> yearlyCash;
        double sum = 0.0;
        for (const auto& b : bills) {
            if (b.paymentMethod == "Cash" && (b.date.startsWith(currentYear) || b.date.isEmpty())) {
                yearlyCash.push_back(b);
                sum += b.total;
            }
        }

        m_reportTable->setRowCount(static_cast<int>(yearlyCash.size()));
        for (int i = 0; i < static_cast<int>(yearlyCash.size()); ++i) {
            const auto& b = yearlyCash[i];
            setTableItem(i, 0, QString::number(b.billId), Qt::AlignCenter);
            setTableItem(i, 1, QString::number(b.orderId), Qt::AlignCenter);
            setTableItem(i, 2, b.date);
            setTableItem(i, 3, b.paymentMethod, Qt::AlignCenter);
            setTableItem(i, 4, QString("Rs. %1").arg(b.amountPaid, 0, 'f', 2), Qt::AlignRight | Qt::AlignVCenter);
            setTableItem(i, 5, QString("Rs. %1").arg(b.total, 0, 'f', 2), Qt::AlignRight | Qt::AlignVCenter);
        }

        m_reportSummaryLabel->setText(
            QString("<b>Yearly Cash Sales (%1):</b> %2 transaction(s) &nbsp;&nbsp;|&nbsp;&nbsp; <b>Total Yearly Cash:</b> Rs. %3")
            .arg(currentYear).arg(yearlyCash.size()).arg(sum, 0, 'f', 2));

    // -----------------------------------------------------------------------
    // Report 6: Custom Date Range Sales
    // -----------------------------------------------------------------------
    } else if (type == 6) {
        m_reportTable->setColumnCount(7);
        m_reportTable->setHorizontalHeaderLabels(
            {"Bill ID", "Order ID", "Date & Time", "Payment Method", "Subtotal (Rs.)", "Discount (Rs.)", "Total Cash (Rs.)"});
        m_reportTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);

        QDate dFrom = QDate::fromString(from, "yyyy-MM-dd");
        QDate dTo   = QDate::fromString(to, "yyyy-MM-dd");
        std::vector<MgrBill> rangeBills;
        double sum = 0.0;

        for (const auto& b : bills) {
            if (b.paymentMethod == "Cash") {
                QDate bDate = QDate::fromString(b.date.left(10), "yyyy-MM-dd");
                if (!bDate.isValid() || (bDate >= dFrom && bDate <= dTo)) {
                    rangeBills.push_back(b);
                    sum += b.total;
                }
            }
        }

        m_reportTable->setRowCount(static_cast<int>(rangeBills.size()));
        for (int i = 0; i < static_cast<int>(rangeBills.size()); ++i) {
            const auto& b = rangeBills[i];
            setTableItem(i, 0, QString::number(b.billId), Qt::AlignCenter);
            setTableItem(i, 1, QString::number(b.orderId), Qt::AlignCenter);
            setTableItem(i, 2, b.date);
            setTableItem(i, 3, b.paymentMethod, Qt::AlignCenter);
            setTableItem(i, 4, QString("Rs. %1").arg(b.subtotal, 0, 'f', 2), Qt::AlignRight | Qt::AlignVCenter);
            setTableItem(i, 5, QString("-Rs. %1").arg(b.discount, 0, 'f', 2), Qt::AlignRight | Qt::AlignVCenter);
            setTableItem(i, 6, QString("Rs. %1").arg(b.total, 0, 'f', 2), Qt::AlignRight | Qt::AlignVCenter);
        }

        m_reportSummaryLabel->setText(
            QString("<b>Cash Sales [%1 to %2]:</b> %3 transaction(s) &nbsp;&nbsp;|&nbsp;&nbsp; <b>Total Cash:</b> Rs. %4")
            .arg(from).arg(to).arg(rangeBills.size()).arg(sum, 0, 'f', 2));

    // -----------------------------------------------------------------------
    // Report 7: All Sales with Total Amounts and Discounts
    // -----------------------------------------------------------------------
    } else if (type == 7) {
        m_reportTable->setColumnCount(8);
        m_reportTable->setHorizontalHeaderLabels(
            {"Bill ID", "Order ID", "Date & Time", "Subtotal (Rs.)", "Discount (Rs.)", "Total Payable (Rs.)", "Payment Mode", "Status"});
        m_reportTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);

        double grossSub = 0.0;
        double totalDisc = 0.0;
        double netTotal = 0.0;

        m_reportTable->setRowCount(static_cast<int>(bills.size()));
        for (int i = 0; i < static_cast<int>(bills.size()); ++i) {
            const auto& b = bills[i];
            grossSub  += b.subtotal;
            totalDisc += b.discount;
            netTotal  += b.total;

            setTableItem(i, 0, QString::number(b.billId), Qt::AlignCenter);
            setTableItem(i, 1, QString::number(b.orderId), Qt::AlignCenter);
            setTableItem(i, 2, b.date);
            setTableItem(i, 3, QString("Rs. %1").arg(b.subtotal, 0, 'f', 2), Qt::AlignRight | Qt::AlignVCenter);
            setTableItem(i, 4, QString("-Rs. %1").arg(b.discount, 0, 'f', 2), Qt::AlignRight | Qt::AlignVCenter);
            setTableItem(i, 5, QString("Rs. %1").arg(b.total, 0, 'f', 2), Qt::AlignRight | Qt::AlignVCenter);
            setTableItem(i, 6, b.paymentMethod, Qt::AlignCenter);
            setTableItem(i, 7, b.paymentStatus, Qt::AlignCenter);
        }

        m_reportSummaryLabel->setText(
            QString("<b>All Recorded Sales:</b> %1 bills &nbsp;&nbsp;|&nbsp;&nbsp; <b>Gross:</b> Rs. %2 &nbsp;&nbsp;|&nbsp;&nbsp; <b>Total Discounts:</b> Rs. %3 &nbsp;&nbsp;|&nbsp;&nbsp; <b>Net Revenue:</b> Rs. %4")
            .arg(bills.size()).arg(grossSub, 0, 'f', 2).arg(totalDisc, 0, 'f', 2).arg(netTotal, 0, 'f', 2));

    // -----------------------------------------------------------------------
    // Report 8: Top 10 Most-Sold Products (in descending order, including remaining stock)
    // -----------------------------------------------------------------------
    } else if (type == 8) {
        m_reportTable->setColumnCount(8);
        m_reportTable->setHorizontalHeaderLabels(
            {"Rank", "Barcode", "Acronym", "Product Name", "Category", "Units Sold", "Unit Price (Rs.)", "Remaining Stock"});
        m_reportTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);

        // Aggregate units sold from OrderDetail.txt
        std::map<std::string, int> unitsSoldByBarcode;
        for (const auto& d : details) {
            unitsSoldByBarcode[d.barcode.toStdString()] += d.quantity;
        }

        struct ProductSalesStat {
            ProductInfo prod;
            int         unitsSold{0};
        };
        std::vector<ProductSalesStat> stats;
        for (const auto& p : products) {
            int sold = unitsSoldByBarcode[p.barcode];
            stats.push_back({p, sold});
        }

        // Sort descending by units sold. If tied, sort by price descending
        std::sort(stats.begin(), stats.end(), [](const ProductSalesStat& a, const ProductSalesStat& b) {
            if (a.unitsSold != b.unitsSold) return a.unitsSold > b.unitsSold;
            return a.prod.availableQuantity < b.prod.availableQuantity;
        });

        int count = std::min(10, static_cast<int>(stats.size()));
        m_reportTable->setRowCount(count);
        for (int i = 0; i < count; ++i) {
            const auto& s = stats[i];
            setTableItem(i, 0, QString("#%1").arg(i + 1), Qt::AlignCenter);
            setTableItem(i, 1, QString::fromStdString(s.prod.barcode), Qt::AlignCenter);
            setTableItem(i, 2, QString::fromStdString(s.prod.acronym), Qt::AlignCenter);
            setTableItem(i, 3, QString::fromStdString(s.prod.name));
            setTableItem(i, 4, QString::fromStdString(s.prod.category));
            setTableItem(i, 5, QString::number(s.unitsSold), Qt::AlignCenter);
            setTableItem(i, 6, QString("Rs. %1").arg(s.prod.unitPrice, 0, 'f', 2), Qt::AlignRight | Qt::AlignVCenter);
            setTableItem(i, 7, QString::number(s.prod.availableQuantity), Qt::AlignCenter);
        }

        m_reportSummaryLabel->setText(
            QString("<b>Top 10 Most-Sold Products</b> (Ranked in descending order by units sold from OrderDetail.txt, including live remaining stock)."));

    // -----------------------------------------------------------------------
    // Report 9: Top 10 Products with the Highest Remaining Stock
    // -----------------------------------------------------------------------
    } else if (type == 9) {
        m_reportTable->setColumnCount(8);
        m_reportTable->setHorizontalHeaderLabels(
            {"Rank", "Barcode", "Acronym", "Product Name", "Category", "Unit", "Unit Price (Rs.)", "Remaining Stock"});
        m_reportTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);

        auto sorted = products;
        std::sort(sorted.begin(), sorted.end(), [](const ProductInfo& a, const ProductInfo& b){
            return a.availableQuantity > b.availableQuantity;
        });

        int count = std::min(10, static_cast<int>(sorted.size()));
        m_reportTable->setRowCount(count);
        for (int i = 0; i < count; ++i) {
            const auto& p = sorted[i];
            setTableItem(i, 0, QString("#%1").arg(i + 1), Qt::AlignCenter);
            setTableItem(i, 1, QString::fromStdString(p.barcode),  Qt::AlignCenter);
            setTableItem(i, 2, QString::fromStdString(p.acronym),  Qt::AlignCenter);
            setTableItem(i, 3, QString::fromStdString(p.name));
            setTableItem(i, 4, QString::fromStdString(p.category));
            setTableItem(i, 5, QString::fromStdString(p.unit), Qt::AlignCenter);
            setTableItem(i, 6, QString("Rs. %1").arg(p.unitPrice, 0, 'f', 2), Qt::AlignRight | Qt::AlignVCenter);
            setTableItem(i, 7, QString::number(p.availableQuantity), Qt::AlignCenter);
        }

        m_reportSummaryLabel->setText(
            QString("<b>Top 10 Products with Highest Remaining Stock</b> (Ranked in descending order by stock count from Product.txt)."));
    }
}
