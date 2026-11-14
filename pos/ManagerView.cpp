#include "ManagerView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <algorithm>

ManagerView::ManagerView(IProductService* productService, QWidget* parent)
    : QWidget(parent), m_productService(productService) {
    setupUi();
    refreshCatalog();
}

void ManagerView::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(32, 24, 32, 24);
    mainLayout->setSpacing(18);

    // Header
    auto* topLayout = new QHBoxLayout();
    auto* titleCol = new QVBoxLayout();
    auto* titleLabel = new QLabel("Manager Terminal — Store Inventory & Catalog", this);
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #1E3A5F;");
    auto* subtitleLabel = new QLabel("View store inventory stock levels, product catalog, and pricing records", this);
    subtitleLabel->setStyleSheet("font-size: 13px; color: #4B5563;");
    titleCol->addWidget(titleLabel);
    titleCol->addWidget(subtitleLabel);

    auto* backBtn = new QPushButton("Back to Dashboard", this);
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

    topLayout->addLayout(titleCol);
    topLayout->addStretch();
    topLayout->addWidget(backBtn);
    mainLayout->addLayout(topLayout);

    // Search and Filter Bar
    auto* filterLayout = new QHBoxLayout();
    filterLayout->setSpacing(12);

    auto* searchTitle = new QLabel("Search Catalog:", this);
    searchTitle->setStyleSheet("font-weight: 600; color: #1F2937; font-size: 13px;");

    m_searchInput = new QLineEdit(this);
    m_searchInput->setPlaceholderText("Filter by barcode, name, category, or acronym...");
    m_searchInput->setStyleSheet(
        "background-color: #FFFFFF !important;"
        "color: #111827 !important;"
        "padding: 7px 12px;"
        "font-size: 13px;"
        "border: 1px solid #D1D5DB;"
        "border-radius: 4px;"
    );
    connect(m_searchInput, &QLineEdit::textChanged, this, &ManagerView::onSearchTextChanged);

    auto* inspectBtn = new QPushButton("View Full Product Description", this);
    inspectBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #1E3A5F;"
        "  color: #FFFFFF;"
        "  font-weight: 600;"
        "  font-size: 13px;"
        "  padding: 7px 18px;"
        "  border-radius: 4px;"
        "  border: none;"
        "}"
        "QPushButton:hover { background-color: #2B4C7E; }"
    );
    connect(inspectBtn, &QPushButton::clicked, this, &ManagerView::onInspectClicked);

    filterLayout->addWidget(searchTitle);
    filterLayout->addWidget(m_searchInput, 2);
    filterLayout->addWidget(inspectBtn);
    mainLayout->addLayout(filterLayout);

    // Catalog Table
    m_catalogTable = new QTableWidget(this);
    m_catalogTable->setColumnCount(10);
    QStringList headers = {"ID", "Barcode", "Acronym", "Product Name", "Category", "Unit", "Unit Price (Rs.)", "Tax", "Discount (Rs.)", "Stock"};
    m_catalogTable->setHorizontalHeaderLabels(headers);
    m_catalogTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_catalogTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_catalogTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_catalogTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_catalogTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_catalogTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_catalogTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_catalogTable->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
    m_catalogTable->horizontalHeader()->setSectionResizeMode(8, QHeaderView::ResizeToContents);
    m_catalogTable->horizontalHeader()->setSectionResizeMode(9, QHeaderView::ResizeToContents);
    m_catalogTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_catalogTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_catalogTable->setAlternatingRowColors(true);
    m_catalogTable->setStyleSheet(
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
    connect(m_catalogTable, &QTableWidget::cellDoubleClicked, [this](int row, int /*col*/) {
        if (row >= 0 && m_catalogTable->item(row, 1)) {
            emit inspectProductRequested(m_catalogTable->item(row, 1)->text().toStdString());
        }
    });

    mainLayout->addWidget(m_catalogTable, 1);

    m_summaryLabel = new QLabel("Total products loaded: 0", this);
    m_summaryLabel->setStyleSheet("font-size: 12px; color: #6B7280;");
    mainLayout->addWidget(m_summaryLabel);
}

void ManagerView::refreshCatalog() {
    if (!m_productService) return;
    populateTable(m_productService->getAllProducts());
}

void ManagerView::populateTable(const std::vector<ProductInfo>& products) {
    m_catalogTable->setRowCount(static_cast<int>(products.size()));

    for (int i = 0; i < static_cast<int>(products.size()); ++i) {
        const auto& p = products[i];

        auto* idItem = new QTableWidgetItem(QString::number(p.id));
        idItem->setTextAlignment(Qt::AlignCenter);

        auto* barItem = new QTableWidgetItem(QString::fromStdString(p.barcode));
        barItem->setTextAlignment(Qt::AlignCenter);
        barItem->setFont(QFont("", -1, QFont::Bold));

        auto* acrItem = new QTableWidgetItem(QString::fromStdString(p.acronym));
        acrItem->setTextAlignment(Qt::AlignCenter);

        auto* nameItem = new QTableWidgetItem(QString::fromStdString(p.name));

        auto* catItem = new QTableWidgetItem(p.category.empty() ? "General" : QString::fromStdString(p.category));

        auto* unitItem = new QTableWidgetItem(QString::fromStdString(p.unit));
        unitItem->setTextAlignment(Qt::AlignCenter);

        auto* priceItem = new QTableWidgetItem(QString("Rs. %1").arg(p.unitPrice, 0, 'f', 2));
        priceItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        auto* taxItem = new QTableWidgetItem(QString("%1%").arg(p.taxRate * 100.0, 0, 'f', 1));
        taxItem->setTextAlignment(Qt::AlignCenter);

        auto* discItem = new QTableWidgetItem(QString("-Rs. %1").arg(p.discountPerUnit, 0, 'f', 2));
        discItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        discItem->setForeground(QBrush(QColor("#166534")));

        auto* stockItem = new QTableWidgetItem(QString::number(p.availableQuantity));
        stockItem->setTextAlignment(Qt::AlignCenter);
        stockItem->setFont(QFont("", -1, QFont::Bold));

        m_catalogTable->setItem(i, 0, idItem);
        m_catalogTable->setItem(i, 1, barItem);
        m_catalogTable->setItem(i, 2, acrItem);
        m_catalogTable->setItem(i, 3, nameItem);
        m_catalogTable->setItem(i, 4, catItem);
        m_catalogTable->setItem(i, 5, unitItem);
        m_catalogTable->setItem(i, 6, priceItem);
        m_catalogTable->setItem(i, 7, taxItem);
        m_catalogTable->setItem(i, 8, discItem);
        m_catalogTable->setItem(i, 9, stockItem);
    }

    m_summaryLabel->setText(QString("Total catalog entries displayed: %1 (Double-click any row to view description)").arg(products.size()));
}

void ManagerView::onSearchTextChanged(const QString& text) {
    if (!m_productService) return;
    std::string query = text.trimmed().toLower().toStdString();

    if (query.empty()) {
        populateTable(m_productService->getAllProducts());
        return;
    }

    std::vector<ProductInfo> filtered;
    for (const auto& prod : m_productService->getAllProducts()) {
        std::string bar = prod.barcode;
        std::string acr = prod.acronym;
        std::string name = prod.name;
        std::string cat = prod.category;
        std::transform(acr.begin(), acr.end(), acr.begin(), ::tolower);
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        std::transform(cat.begin(), cat.end(), cat.begin(), ::tolower);

        if (bar.find(query) != std::string::npos ||
            acr.find(query) != std::string::npos ||
            name.find(query) != std::string::npos ||
            cat.find(query) != std::string::npos) {
            filtered.push_back(prod);
        }
    }

    populateTable(filtered);
}

void ManagerView::onInspectClicked() {
    int row = m_catalogTable->currentRow();
    if (row >= 0 && m_catalogTable->item(row, 1)) {
        emit inspectProductRequested(m_catalogTable->item(row, 1)->text().toStdString());
    } else {
        QMessageBox::information(this, "Select Product", "Please select a product row from the table to view its description.");
    }
}
