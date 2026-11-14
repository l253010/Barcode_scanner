#include "ProductDescriptionView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPainter>
#include <QMessageBox>

// ============================================================================
// BarcodeDisplayWidget Implementation
// ============================================================================

BarcodeDisplayWidget::BarcodeDisplayWidget(QWidget* parent) : QWidget(parent) {
    setMinimumHeight(110);
    setStyleSheet("background-color: #FFFFFF; border: 1px solid #D1D5DB; border-radius: 4px;");
}

void BarcodeDisplayWidget::setBarcode(const std::string& code) {
    m_code = code;
    ValidationResult val = BarcodeGenerator::validate(code);
    if (val.valid) {
        m_elements = BarcodeGenerator::encode(code);
        m_hasData = true;
    } else {
        m_elements.clear();
        m_hasData = false;
    }
    update();
}

void BarcodeDisplayWidget::clear() {
    m_code.clear();
    m_elements.clear();
    m_hasData = false;
    update();
}

void BarcodeDisplayWidget::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    // Background
    painter.fillRect(rect(), QColor("#FFFFFF"));
    painter.setPen(QColor("#D1D5DB"));
    painter.drawRect(0, 0, width() - 1, height() - 1);

    if (!m_hasData || m_elements.empty()) {
        painter.setPen(QColor("#9CA3AF"));
        painter.setFont(QFont("Arial", 11));
        painter.drawText(rect(), Qt::AlignCenter, "No valid Code 39 barcode to display");
        return;
    }

    // Total width in logical units for a 6-digit Code 39 barcode is 127 units
    int totalUnits = 0;
    for (const auto& elem : m_elements) {
        totalUnits += elem.width;
    }

    int marginX = 20;
    int availableWidth = width() - (2 * marginX);
    if (availableWidth < 100) availableWidth = 100;

    double unitWidth = static_cast<double>(availableWidth) / static_cast<double>(totalUnits);
    if (unitWidth < 1.0) unitWidth = 1.0;

    double currentX = (width() - (totalUnits * unitWidth)) / 2.0;
    if (currentX < marginX) currentX = marginX;

    int barTop = 15;
    int barHeight = height() - 42;

    for (const auto& elem : m_elements) {
        double w = elem.width * unitWidth;
        if (elem.isBar) {
            painter.fillRect(QRectF(currentX, barTop, w, barHeight), QColor("#000000"));
        } else {
            painter.fillRect(QRectF(currentX, barTop, w, barHeight), QColor("#FFFFFF"));
        }
        currentX += w;
    }

    // Draw barcode human-readable text beneath the bars
    painter.setPen(QColor("#1F2937"));
    QFont font("Arial", 11, QFont::Bold);
    font.setLetterSpacing(QFont::AbsoluteSpacing, 3);
    painter.setFont(font);

    QRect textRect(0, height() - 25, width(), 20);
    painter.drawText(textRect, Qt::AlignCenter, QString("*%1*").arg(QString::fromStdString(m_code)));
}

// ============================================================================
// ProductDescriptionView Implementation
// ============================================================================

ProductDescriptionView::ProductDescriptionView(IProductService* productService, QWidget* parent)
    : QWidget(parent), m_productService(productService) {
    setupUi();
    populateDropdown();
    if (m_productService && !m_productService->getAllProducts().empty()) {
        displayProduct(m_productService->getAllProducts().front().barcode);
    }
}

void ProductDescriptionView::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(28, 24, 28, 24);
    mainLayout->setSpacing(20);

    // Header
    auto* headerLayout = new QHBoxLayout();
    auto* titleCol = new QVBoxLayout();
    auto* titleLabel = new QLabel("Product Description & Specification", this);
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #1E3A5F;");
    auto* subtitleLabel = new QLabel("Lookup detailed product data, inventory status, and Code 39 barcode pattern", this);
    subtitleLabel->setStyleSheet("font-size: 13px; color: #4B5563;");
    titleCol->addWidget(titleLabel);
    titleCol->addWidget(subtitleLabel);

    auto* backBtn = new QPushButton("Back to Cashier", this);
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
    connect(backBtn, &QPushButton::clicked, this, &ProductDescriptionView::backToCashierRequested);

    headerLayout->addLayout(titleCol);
    headerLayout->addStretch();
    headerLayout->addWidget(backBtn);
    mainLayout->addLayout(headerLayout);

    // Barcode Lookup Card
    auto* lookupCard = new QFrame(this);
    lookupCard->setStyleSheet(
        "QFrame {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #D1D5DB;"
        "  border-radius: 4px;"
        "  padding: 12px 16px;"
        "}"
    );
    auto* lookupLayout = new QVBoxLayout(lookupCard);
    lookupLayout->setContentsMargins(12, 12, 12, 12);
    lookupLayout->setSpacing(10);

    auto* lookupRow = new QHBoxLayout();
    lookupRow->setSpacing(12);

    auto* lookupTitle = new QLabel("Barcode Lookup:", lookupCard);
    lookupTitle->setStyleSheet("font-weight: bold; color: #1E3A5F; font-size: 13px;");

    m_lookupInput = new QLineEdit(lookupCard);
    m_lookupInput->setPlaceholderText("Enter 6-digit barcode (e.g. 100001)...");
    m_lookupInput->setStyleSheet("padding: 6px 10px; font-size: 13px; border: 1px solid #D1D5DB; border-radius: 4px;");

    auto* lookupBtn = new QPushButton("Lookup", lookupCard);
    lookupBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #1E3A5F;"
        "  color: white;"
        "  font-weight: 600;"
        "  padding: 6px 16px;"
        "  border-radius: 4px;"
        "  border: none;"
        "}"
        "QPushButton:hover { background-color: #2B4C7E; }"
    );
    connect(lookupBtn, &QPushButton::clicked, this, &ProductDescriptionView::onLookupClicked);
    connect(m_lookupInput, &QLineEdit::returnPressed, this, &ProductDescriptionView::onLookupClicked);

    auto* selectLabel = new QLabel("Or Select Product:", lookupCard);
    selectLabel->setStyleSheet("color: #4B5563; font-weight: 600;");

    m_productCombo = new QComboBox(lookupCard);
    m_productCombo->setStyleSheet("padding: 5px 8px; border: 1px solid #D1D5DB; border-radius: 3px; font-size: 13px;");
    connect(m_productCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ProductDescriptionView::onProductDropdownChanged);

    lookupRow->addWidget(lookupTitle);
    lookupRow->addWidget(m_lookupInput, 2);
    lookupRow->addWidget(lookupBtn);
    lookupRow->addWidget(selectLabel);
    lookupRow->addWidget(m_productCombo, 3);
    lookupLayout->addLayout(lookupRow);

    m_lookupStatusLabel = new QLabel("Enter or select a product above to inspect full specifications.", lookupCard);
    m_lookupStatusLabel->setStyleSheet("font-size: 12px; color: #6B7280;");
    lookupLayout->addWidget(m_lookupStatusLabel);

    mainLayout->addWidget(lookupCard);

    // Detail Panel
    auto* detailCard = new QFrame(this);
    detailCard->setStyleSheet(
        "QFrame {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #D1D5DB;"
        "  border-radius: 4px;"
        "  padding: 20px;"
        "}"
    );
    auto* detailLayout = new QVBoxLayout(detailCard);
    detailLayout->setContentsMargins(20, 20, 20, 20);
    detailLayout->setSpacing(16);

    auto* grid = new QGridLayout();
    grid->setHorizontalSpacing(24);
    grid->setVerticalSpacing(12);

    auto makeField = [this, grid](const QString& title, QLabel*& valLabel, int row, int col) {
        auto* tLabel = new QLabel(title, this);
        tLabel->setStyleSheet("font-weight: 600; color: #4B5563; font-size: 13px;");
        valLabel = new QLabel("-", this);
        valLabel->setStyleSheet("font-size: 14px; color: #1F2937; font-weight: 500;");
        grid->addWidget(tLabel, row, col * 2);
        grid->addWidget(valLabel, row, col * 2 + 1);
    };

    makeField("Product ID:",          m_idLabel,       0, 0);
    makeField("Product Barcode:",     m_barcodeLabel,  0, 1);
    makeField("Product Acronym:",     m_acronymLabel,  1, 0);
    makeField("Product Name:",        m_nameLabel,     1, 1);
    makeField("Measurement Unit:",    m_unitLabel,     2, 0);
    makeField("Unit Price ($):",      m_priceLabel,    2, 1);
    makeField("Tax Rate (%):",        m_taxLabel,      3, 0);
    makeField("Discount / Unit ($):", m_discountLabel, 3, 1);
    makeField("Available Stock:",     m_stockLabel,    4, 0);

    detailLayout->addLayout(grid);

    // Visual Code 39 Barcode section
    auto* barcodeSectionTitle = new QLabel("Visual Code 39 Barcode Rendering (Increment 1 Engine):", detailCard);
    barcodeSectionTitle->setStyleSheet("font-weight: bold; color: #1E3A5F; font-size: 13px; margin-top: 10px;");
    detailLayout->addWidget(barcodeSectionTitle);

    m_barcodeWidget = new BarcodeDisplayWidget(detailCard);
    detailLayout->addWidget(m_barcodeWidget);

    // Action button
    m_addToCartBtn = new QPushButton("Add this Product to Current Cart", detailCard);
    m_addToCartBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #1E3A5F;"
        "  color: #FFFFFF;"
        "  font-weight: bold;"
        "  font-size: 13px;"
        "  padding: 10px 18px;"
        "  border-radius: 4px;"
        "  border: none;"
        "}"
        "QPushButton:hover { background-color: #2B4C7E; }"
    );
    connect(m_addToCartBtn, &QPushButton::clicked, this, &ProductDescriptionView::onAddToCartClicked);
    detailLayout->addWidget(m_addToCartBtn, 0, Qt::AlignLeft);

    mainLayout->addWidget(detailCard);
    mainLayout->addStretch();
}

void ProductDescriptionView::populateDropdown() {
    m_productCombo->clear();
    m_productCombo->addItem("-- Select from Catalog --");
    if (!m_productService) return;

    for (const auto& prod : m_productService->getAllProducts()) {
        m_productCombo->addItem(
            QString::fromStdString(prod.barcode + " - " + prod.name),
            QVariant(QString::fromStdString(prod.barcode))
        );
    }
}

void ProductDescriptionView::onLookupClicked() {
    QString codeStr = m_lookupInput->text().trimmed();
    if (codeStr.isEmpty()) {
        m_lookupStatusLabel->setText("<font color='#DC2626'>Please enter a 6-digit barcode to lookup.</font>");
        return;
    }

    displayProduct(codeStr.toStdString());
}

void ProductDescriptionView::onProductDropdownChanged(int index) {
    if (index <= 0) return;
    QString codeStr = m_productCombo->itemData(index).toString();
    if (!codeStr.isEmpty()) {
        displayProduct(codeStr.toStdString());
    }
}

void ProductDescriptionView::displayProduct(const std::string& barcode) {
    if (!m_productService) return;

    ValidationResult val = BarcodeGenerator::validate(barcode);
    if (!val.valid) {
        m_lookupStatusLabel->setText(QString("<font color='#DC2626'>Invalid Code 39 format: %1</font>")
                                     .arg(QString::fromStdString(val.message)));
        return;
    }

    ProductInfo prod;
    if (m_productService->findProductByBarcode(barcode, prod)) {
        m_currentProduct = prod;
        updateFields(prod);
        m_lookupStatusLabel->setText(QString("<font color='#166534'>Found product: <b>%1</b></font>")
                                     .arg(QString::fromStdString(prod.name)));
    } else {
        m_lookupStatusLabel->setText(QString("<font color='#DC2626'>Barcode '%1' not found in store database.</font>")
                                     .arg(QString::fromStdString(barcode)));
    }
}

void ProductDescriptionView::updateFields(const ProductInfo& prod) {
    m_idLabel->setText(QString::number(prod.id));
    m_barcodeLabel->setText(QString("<b>%1</b>").arg(QString::fromStdString(prod.barcode)));
    m_acronymLabel->setText(QString::fromStdString(prod.acronym));
    m_nameLabel->setText(QString("<b>%1</b>").arg(QString::fromStdString(prod.name)));
    m_unitLabel->setText(QString::fromStdString(prod.unit));
    m_priceLabel->setText(QString("$%1").arg(prod.unitPrice, 0, 'f', 2));
    m_taxLabel->setText(QString("%1%").arg(prod.taxRate * 100.0, 0, 'f', 1));
    m_discountLabel->setText(QString("$%1").arg(prod.discountPerUnit, 0, 'f', 2));
    m_stockLabel->setText(QString("<b>%1</b> %2(s) available in store")
                          .arg(prod.availableQuantity)
                          .arg(QString::fromStdString(prod.unit)));

    m_barcodeWidget->setBarcode(prod.barcode);
}

void ProductDescriptionView::onAddToCartClicked() {
    if (m_currentProduct.id > 0) {
        emit addProductToCart(m_currentProduct);
        QMessageBox::information(this, "Added to Order",
                                 QString("Added 1x '%1' to the active cart.")
                                 .arg(QString::fromStdString(m_currentProduct.name)));
    }
}
