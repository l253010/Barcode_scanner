#include "CashierOrderView.h"
#include "BarcodeGenerator.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QSpinBox>
#include <iomanip>
#include <sstream>

CashierOrderView::CashierOrderView(IProductService* productService,
                                   IBillingService* billingService,
                                   ICameraScannerService* cameraService,
                                   QWidget* parent)
    : QWidget(parent),
      m_productService(productService),
      m_billingService(billingService),
      m_cameraService(cameraService) {
    setupUi();
    updateSummary();
}

void CashierOrderView::setupUi() {
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(24, 20, 24, 20);
    mainLayout->setSpacing(20);

    // Left Column: Customer Details, Barcode Entry, Camera Placeholder, Cart Table
    auto* leftColumn = new QVBoxLayout();
    leftColumn->setSpacing(14);

    // Customer Info Card
    auto* customerCard = new QFrame(this);
    customerCard->setStyleSheet(
        "QFrame {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #D1D5DB;"
        "  border-radius: 4px;"
        "  padding: 10px 14px;"
        "}"
    );
    auto* customerLayout = new QHBoxLayout(customerCard);
    customerLayout->setContentsMargins(10, 8, 10, 8);
    customerLayout->setSpacing(12);

    auto* nameLabel = new QLabel("Customer Name:", customerCard);
    nameLabel->setStyleSheet("font-weight: 600; color: #1F2937;");
    m_customerNameInput = new QLineEdit(customerCard);
    m_customerNameInput->setPlaceholderText("e.g. John Doe (default: Walk-in)");
    m_customerNameInput->setStyleSheet("padding: 5px 8px; border: 1px solid #D1D5DB; border-radius: 3px;");

    auto* contactLabel = new QLabel("Contact (Optional):", customerCard);
    contactLabel->setStyleSheet("font-weight: 600; color: #1F2937;");
    m_customerContactInput = new QLineEdit(customerCard);
    m_customerContactInput->setPlaceholderText("Phone or email");
    m_customerContactInput->setStyleSheet("padding: 5px 8px; border: 1px solid #D1D5DB; border-radius: 3px;");

    customerLayout->addWidget(nameLabel);
    customerLayout->addWidget(m_customerNameInput, 2);
    customerLayout->addWidget(contactLabel);
    customerLayout->addWidget(m_customerContactInput, 2);
    leftColumn->addWidget(customerCard);

    // Barcode Scanning & Camera Preview Bar
    auto* scanCard = new QFrame(this);
    scanCard->setStyleSheet(
        "QFrame {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #D1D5DB;"
        "  border-radius: 4px;"
        "  padding: 12px 14px;"
        "}"
    );
    auto* scanLayout = new QVBoxLayout(scanCard);
    scanLayout->setContentsMargins(12, 10, 12, 10);
    scanLayout->setSpacing(10);

    auto* scanInputRow = new QHBoxLayout();
    scanInputRow->setSpacing(10);

    auto* barcodeTitle = new QLabel("Barcode Scanner:", scanCard);
    barcodeTitle->setStyleSheet("font-weight: bold; color: #1E3A5F; font-size: 13px;");

    m_barcodeInput = new QLineEdit(scanCard);
    m_barcodeInput->setPlaceholderText("Enter or scan 6-digit barcode (e.g. 100001)...");
    m_barcodeInput->setStyleSheet("padding: 6px 10px; font-size: 13px; border: 1px solid #D1D5DB; border-radius: 4px;");

    auto* addBarcodeBtn = new QPushButton("Add to Cart", scanCard);
    addBarcodeBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #1E3A5F;"
        "  color: white;"
        "  font-weight: 600;"
        "  padding: 6px 14px;"
        "  border-radius: 4px;"
        "  border: none;"
        "}"
        "QPushButton:hover { background-color: #2B4C7E; }"
    );
    connect(addBarcodeBtn, &QPushButton::clicked, this, &CashierOrderView::onAddBarcodeClicked);
    connect(m_barcodeInput, &QLineEdit::returnPressed, this, &CashierOrderView::onAddBarcodeClicked);

    auto* quickAddLabel = new QLabel("Quick Sample:", scanCard);
    quickAddLabel->setStyleSheet("color: #4B5563; font-weight: 600;");

    m_quickAddCombo = new QComboBox(scanCard);
    m_quickAddCombo->setStyleSheet("padding: 4px 8px; border: 1px solid #D1D5DB; border-radius: 3px;");
    m_quickAddCombo->addItem("-- Select Sample Item --");
    if (m_productService) {
        for (const auto& prod : m_productService->getAllProducts()) {
            m_quickAddCombo->addItem(
                QString::fromStdString(prod.barcode + " - " + prod.name + " ($" + std::to_string(prod.unitPrice).substr(0, 4) + ")"),
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

    // Status Label for validation feedback
    m_scanStatusLabel = new QLabel("Ready for barcode scan. Supports Code 39 numeric format.", scanCard);
    m_scanStatusLabel->setStyleSheet("font-size: 12px; color: #6B7280;");
    scanLayout->addWidget(m_scanStatusLabel);

    // Camera Preview Placeholder Section (Reserved for Eman 2's module)
    m_cameraFrame = new QFrame(scanCard);
    m_cameraFrame->setStyleSheet(
        "QFrame {"
        "  background-color: #F9FAFB;"
        "  border: 1px dashed #9CA3AF;"
        "  border-radius: 4px;"
        "  padding: 10px;"
        "}"
    );
    auto* cameraLayout = new QHBoxLayout(m_cameraFrame);
    cameraLayout->setContentsMargins(10, 6, 10, 6);

    m_cameraStatusLabel = new QLabel("<b>[Camera Scanner Module Placeholder]</b> — Ready for integration with Eman 2's camera scanner", m_cameraFrame);
    m_cameraStatusLabel->setStyleSheet("color: #374151; font-size: 12px;");

    m_cameraToggleBtn = new QPushButton("Simulate Camera Scan", m_cameraFrame);
    m_cameraToggleBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #D1D5DB;"
        "  border-radius: 3px;"
        "  padding: 4px 10px;"
        "  font-size: 11px;"
        "  font-weight: 600;"
        "  color: #1F2937;"
        "}"
        "QPushButton:hover { background-color: #E5E7EB; }"
    );
    connect(m_cameraToggleBtn, &QPushButton::clicked, this, &CashierOrderView::onToggleCameraPlaceholder);

    cameraLayout->addWidget(m_cameraStatusLabel);
    cameraLayout->addStretch();
    cameraLayout->addWidget(m_cameraToggleBtn);
    scanLayout->addWidget(m_cameraFrame);

    leftColumn->addWidget(scanCard);

    // Cart Table
    m_cartTable = new QTableWidget(this);
    m_cartTable->setColumnCount(7);
    QStringList headers = {"Product Name", "Acronym", "Unit Price", "Qty", "Discount", "Subtotal", "Remove"};
    m_cartTable->setHorizontalHeaderLabels(headers);
    m_cartTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_cartTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_cartTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_cartTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_cartTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_cartTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_cartTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_cartTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_cartTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_cartTable->setAlternatingRowColors(true);
    m_cartTable->setStyleSheet(
        "QTableWidget {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #D1D5DB;"
        "  border-radius: 4px;"
        "  gridline-color: #E5E7EB;"
        "  font-size: 13px;"
        "}"
        "QHeaderView::section {"
        "  background-color: #E5E7EB;"
        "  color: #1F2937;"
        "  font-weight: bold;"
        "  padding: 6px;"
        "  border: none;"
        "}"
    );
    leftColumn->addWidget(m_cartTable, 1);

    mainLayout->addLayout(leftColumn, 7);

    // Right Column: Last Scanned Item, Billing Summary, Action Buttons
    auto* rightColumn = new QVBoxLayout();
    rightColumn->setSpacing(14);

    // Scanned Item Info Card
    auto* infoCard = new QFrame(this);
    infoCard->setStyleSheet(
        "QFrame {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #D1D5DB;"
        "  border-radius: 4px;"
        "  padding: 12px 14px;"
        "}"
    );
    auto* infoLayout = new QVBoxLayout(infoCard);
    infoLayout->setContentsMargins(12, 12, 12, 12);
    infoLayout->setSpacing(6);

    auto* infoTitle = new QLabel("Scanned Product Information", infoCard);
    infoTitle->setStyleSheet("font-weight: bold; color: #1E3A5F; font-size: 13px;");
    infoLayout->addWidget(infoTitle);

    m_scannedInfoLabel = new QLabel("No product scanned yet.\nEnter a barcode or select a sample item to view details.", infoCard);
    m_scannedInfoLabel->setStyleSheet("font-size: 12px; color: #4B5563; line-height: 1.4;");
    m_scannedInfoLabel->setWordWrap(true);
    infoLayout->addWidget(m_scannedInfoLabel);
    rightColumn->addWidget(infoCard);

    // Billing Summary Card
    auto* summaryCard = new QFrame(this);
    summaryCard->setStyleSheet(
        "QFrame {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #D1D5DB;"
        "  border-radius: 4px;"
        "  padding: 16px;"
        "}"
    );
    auto* summaryLayout = new QVBoxLayout(summaryCard);
    summaryLayout->setContentsMargins(16, 16, 16, 16);
    summaryLayout->setSpacing(12);

    auto* summaryTitle = new QLabel("Billing Summary", summaryCard);
    summaryTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #1E3A5F;");
    summaryLayout->addWidget(summaryTitle);

    auto* gridSummary = new QGridLayout();
    gridSummary->setSpacing(8);

    auto* subLabel = new QLabel("Subtotal (excl. Tax):", summaryCard);
    subLabel->setStyleSheet("color: #4B5563; font-size: 13px;");
    m_subtotalLabel = new QLabel("$0.00", summaryCard);
    m_subtotalLabel->setStyleSheet("color: #1F2937; font-weight: 600; font-size: 13px;");
    m_subtotalLabel->setAlignment(Qt::AlignRight);

    auto* discLabel = new QLabel("Total Discount:", summaryCard);
    discLabel->setStyleSheet("color: #4B5563; font-size: 13px;");
    m_discountLabel = new QLabel("-$0.00", summaryCard);
    m_discountLabel->setStyleSheet("color: #166534; font-weight: 600; font-size: 13px;");
    m_discountLabel->setAlignment(Qt::AlignRight);

    auto* taxTitleLabel = new QLabel("Estimated Tax:", summaryCard);
    taxTitleLabel->setStyleSheet("color: #4B5563; font-size: 13px;");
    m_taxLabel = new QLabel("+$0.00", summaryCard);
    m_taxLabel->setStyleSheet("color: #1F2937; font-weight: 600; font-size: 13px;");
    m_taxLabel->setAlignment(Qt::AlignRight);

    gridSummary->addWidget(subLabel, 0, 0);
    gridSummary->addWidget(m_subtotalLabel, 0, 1);
    gridSummary->addWidget(discLabel, 1, 0);
    gridSummary->addWidget(m_discountLabel, 1, 1);
    gridSummary->addWidget(taxTitleLabel, 2, 0);
    gridSummary->addWidget(m_taxLabel, 2, 1);
    summaryLayout->addLayout(gridSummary);

    auto* separator = new QFrame(summaryCard);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    separator->setStyleSheet("color: #D1D5DB;");
    summaryLayout->addWidget(separator);

    auto* totalBox = new QHBoxLayout();
    auto* totalText = new QLabel("TOTAL PAYABLE:", summaryCard);
    totalText->setStyleSheet("font-size: 14px; font-weight: bold; color: #1E3A5F;");
    m_totalPayableLabel = new QLabel("$0.00", summaryCard);
    m_totalPayableLabel->setStyleSheet("font-size: 22px; font-weight: bold; color: #1E3A5F;");
    m_totalPayableLabel->setAlignment(Qt::AlignRight);
    totalBox->addWidget(totalText);
    totalBox->addWidget(m_totalPayableLabel);
    summaryLayout->addLayout(totalBox);

    rightColumn->addWidget(summaryCard);

    // Action Buttons
    auto* descBtn = new QPushButton("View Product Description", this);
    descBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #1E3A5F;"
        "  border-radius: 4px;"
        "  color: #1E3A5F;"
        "  font-weight: 600;"
        "  font-size: 13px;"
        "  padding: 10px 14px;"
        "}"
        "QPushButton:hover { background-color: #EFF6FF; }"
    );
    connect(descBtn, &QPushButton::clicked, this, &CashierOrderView::onDescriptionClicked);
    rightColumn->addWidget(descBtn);

    auto* generateBillBtn = new QPushButton("Generate Bill / Proceed to Checkout", this);
    generateBillBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #166534;"
        "  border: none;"
        "  border-radius: 4px;"
        "  color: #FFFFFF;"
        "  font-weight: bold;"
        "  font-size: 14px;"
        "  padding: 12px 14px;"
        "}"
        "QPushButton:hover { background-color: #15803D; }"
        "QPushButton:pressed { background-color: #14532D; }"
    );
    connect(generateBillBtn, &QPushButton::clicked, this, &CashierOrderView::onGenerateBillClicked);
    rightColumn->addWidget(generateBillBtn);

    auto* clearBtn = new QPushButton("Clear Current Cart", this);
    clearBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #D1D5DB;"
        "  border-radius: 4px;"
        "  color: #B91C1C;"
        "  font-size: 12px;"
        "  padding: 7px 12px;"
        "}"
        "QPushButton:hover { background-color: #FEE2E2; }"
    );
    connect(clearBtn, &QPushButton::clicked, this, &CashierOrderView::onClearCartClicked);
    rightColumn->addWidget(clearBtn);

    rightColumn->addStretch();
    mainLayout->addLayout(rightColumn, 3);
}

void CashierOrderView::onAddBarcodeClicked() {
    QString codeStr = m_barcodeInput->text().trimmed();
    if (codeStr.isEmpty()) {
        m_scanStatusLabel->setText("<font color='#DC2626'>Please enter a 6-digit barcode.</font>");
        return;
    }

    std::string stdCode = codeStr.toStdString();

    // Reusing Increment 1 BarcodeGenerator validation logic
    ValidationResult valResult = BarcodeGenerator::validate(stdCode);
    if (!valResult.valid) {
        m_scanStatusLabel->setText(QString("<font color='#DC2626'>Invalid Barcode: %1</font>")
                                   .arg(QString::fromStdString(valResult.message)));
        return;
    }

    // Lookup in Product Service
    ProductInfo prod;
    if (m_productService && m_productService->findProductByBarcode(stdCode, prod)) {
        addProductToOrder(prod, 1);
        m_scanStatusLabel->setText(QString("<font color='#166534'>Product added: %1 (%2)</font>")
                                   .arg(QString::fromStdString(prod.name))
                                   .arg(QString::fromStdString(prod.barcode)));
        setScannedProductFeedback(prod);
        m_barcodeInput->clear();
    } else {
        m_scanStatusLabel->setText(QString("<font color='#DC2626'>Barcode '%1' is valid Code 39 but not in product catalog.</font>")
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
            m_scanStatusLabel->setText(QString("<font color='#166534'>Quick-added: %1</font>")
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
            return;
        }
    }

    OrderItem newItem;
    newItem.product = product;
    newItem.quantity = quantity;
    m_cartItems.push_back(newItem);

    updateCartTable();
    updateSummary();
}

void CashierOrderView::updateCartTable() {
    m_cartTable->setRowCount(static_cast<int>(m_cartItems.size()));

    for (int i = 0; i < static_cast<int>(m_cartItems.size()); ++i) {
        const auto& item = m_cartItems[i];

        // Column 0: Name
        auto* nameItem = new QTableWidgetItem(QString::fromStdString(item.product.name));
        m_cartTable->setItem(i, 0, nameItem);

        // Column 1: Acronym
        auto* acrItem = new QTableWidgetItem(QString::fromStdString(item.product.acronym));
        acrItem->setTextAlignment(Qt::AlignCenter);
        m_cartTable->setItem(i, 1, acrItem);

        // Column 2: Unit Price
        auto* priceItem = new QTableWidgetItem(QString("$%1").arg(item.product.unitPrice, 0, 'f', 2));
        priceItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_cartTable->setItem(i, 2, priceItem);

        // Column 3: Quantity Controls Widget
        auto* qtySpin = new QSpinBox(m_cartTable);
        qtySpin->setRange(1, 999);
        qtySpin->setValue(item.quantity);
        qtySpin->setStyleSheet("padding: 2px 4px; font-weight: bold; border: 1px solid #D1D5DB; border-radius: 3px;");
        connect(qtySpin, QOverload<int>::of(&QSpinBox::valueChanged), [this, i](int val) {
            onQuantityChanged(i, val);
        });
        m_cartTable->setCellWidget(i, 3, qtySpin);

        // Column 4: Discount
        auto* discItem = new QTableWidgetItem(QString("-$%1").arg(item.getTotalDiscount(), 0, 'f', 2));
        discItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        discItem->setForeground(QBrush(QColor("#166534")));
        m_cartTable->setItem(i, 4, discItem);

        // Column 5: Subtotal
        auto* subItem = new QTableWidgetItem(QString("$%1").arg(item.getSubtotal(), 0, 'f', 2));
        subItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        subItem->setFont(QFont("", -1, QFont::Bold));
        m_cartTable->setItem(i, 5, subItem);

        // Column 6: Remove Button
        auto* removeBtn = new QPushButton("Remove", m_cartTable);
        removeBtn->setStyleSheet(
            "QPushButton {"
            "  background-color: #FFFFFF;"
            "  border: 1px solid #FCA5A5;"
            "  color: #B91C1C;"
            "  font-size: 11px;"
            "  padding: 3px 8px;"
            "  border-radius: 3px;"
            "}"
            "QPushButton:hover { background-color: #FEE2E2; }"
        );
        connect(removeBtn, &QPushButton::clicked, [this, i]() {
            onRemoveItemClicked(i);
        });
        m_cartTable->setCellWidget(i, 6, removeBtn);
    }
}

void CashierOrderView::onQuantityChanged(int row, int newQty) {
    if (row >= 0 && row < static_cast<int>(m_cartItems.size())) {
        m_cartItems[row].quantity = newQty;
        updateSummary();
        // Update subtotal and discount text without recreating table
        if (m_cartTable->item(row, 4)) {
            m_cartTable->item(row, 4)->setText(QString("-$%1").arg(m_cartItems[row].getTotalDiscount(), 0, 'f', 2));
        }
        if (m_cartTable->item(row, 5)) {
            m_cartTable->item(row, 5)->setText(QString("$%1").arg(m_cartItems[row].getSubtotal(), 0, 'f', 2));
        }
    }
}

void CashierOrderView::onRemoveItemClicked(int row) {
    if (row >= 0 && row < static_cast<int>(m_cartItems.size())) {
        m_cartItems.erase(m_cartItems.begin() + row);
        updateCartTable();
        updateSummary();
    }
}

void CashierOrderView::updateSummary() {
    double subtotal = 0.0;
    double discount = 0.0;
    double tax = 0.0;

    for (const auto& item : m_cartItems) {
        subtotal += item.getSubtotal();
        discount += item.getTotalDiscount();
        tax += item.getTotalTax();
    }

    double totalPayable = (subtotal - discount) + tax;
    if (totalPayable < 0.0) totalPayable = 0.0;

    m_subtotalLabel->setText(QString("$%1").arg(subtotal, 0, 'f', 2));
    m_discountLabel->setText(QString("-$%1").arg(discount, 0, 'f', 2));
    m_taxLabel->setText(QString("+$%1").arg(tax, 0, 'f', 2));
    m_totalPayableLabel->setText(QString("$%1").arg(totalPayable, 0, 'f', 2));
}

void CashierOrderView::setScannedProductFeedback(const ProductInfo& prod) {
    QString infoText = QString(
        "<b>Product:</b> %1<br/>"
        "<b>Acronym:</b> %2 | <b>Barcode:</b> %3<br/>"
        "<b>Unit Price:</b> $%4 / %5<br/>"
        "<b>Discount:</b> $%6 | <b>Tax Rate:</b> %7%<br/>"
        "<b>In Stock:</b> %8 units"
    ).arg(QString::fromStdString(prod.name))
     .arg(QString::fromStdString(prod.acronym))
     .arg(QString::fromStdString(prod.barcode))
     .arg(prod.unitPrice, 0, 'f', 2)
     .arg(QString::fromStdString(prod.unit))
     .arg(prod.discountPerUnit, 0, 'f', 2)
     .arg(prod.taxRate * 100.0, 0, 'f', 1)
     .arg(prod.availableQuantity);

    m_scannedInfoLabel->setText(infoText);
}

void CashierOrderView::onDescriptionClicked() {
    int currentRow = m_cartTable->currentRow();
    if (currentRow >= 0 && currentRow < static_cast<int>(m_cartItems.size())) {
        emit inspectProductRequested(m_cartItems[currentRow].product.barcode);
    } else if (!m_cartItems.empty()) {
        emit inspectProductRequested(m_cartItems.back().product.barcode);
    } else {
        // Default to first catalog product if cart empty
        if (m_productService && !m_productService->getAllProducts().empty()) {
            emit inspectProductRequested(m_productService->getAllProducts().front().barcode);
        } else {
            QMessageBox::information(this, "Product Description", "Please add or select a product to inspect.");
        }
    }
}

void CashierOrderView::onGenerateBillClicked() {
    if (m_cartItems.empty()) {
        QMessageBox::warning(this, "Empty Cart", "Cannot generate a bill with an empty cart. Please add at least one product.");
        return;
    }

    std::string custName = m_customerNameInput->text().trimmed().toStdString();
    std::string custContact = m_customerContactInput->text().trimmed().toStdString();

    if (m_billingService) {
        BillInfo bill = m_billingService->generateBill(custName, custContact, m_cartItems);
        emit billGenerated(bill);
    }
}

void CashierOrderView::onClearCartClicked() {
    if (m_cartItems.empty()) return;
    auto res = QMessageBox::question(this, "Clear Cart", "Are you sure you want to clear all items from the current cart?",
                                     QMessageBox::Yes | QMessageBox::No);
    if (res == QMessageBox::Yes) {
        resetOrder();
    }
}

void CashierOrderView::resetOrder() {
    m_cartItems.clear();
    m_customerNameInput->clear();
    m_customerContactInput->clear();
    m_barcodeInput->clear();
    m_scanStatusLabel->setText("Ready for next transaction.");
    m_scannedInfoLabel->setText("No product scanned yet.\nEnter a barcode or select a sample item to view details.");
    updateCartTable();
    updateSummary();
}

void CashierOrderView::onToggleCameraPlaceholder() {
    if (m_cameraService) {
        if (!m_cameraService->isScanning()) {
            m_cameraService->startCamera();
            m_cameraStatusLabel->setText("<font color='#166534'><b>[Camera Scanner Active]</b></font> — Live frame preview placeholder running.");
            m_cameraToggleBtn->setText("Simulate Barcode Capture");

            // Perform scan simulation
            BarcodeScanResult res = m_cameraService->captureAndScan();
            if (res.success && m_productService) {
                ProductInfo prod;
                if (m_productService->findProductByBarcode(res.barcode, prod)) {
                    addProductToOrder(prod, 1);
                    setScannedProductFeedback(prod);
                    m_scanStatusLabel->setText(QString("<font color='#166534'>Camera detected barcode: %1 (%2)</font>")
                                               .arg(QString::fromStdString(res.barcode))
                                               .arg(QString::fromStdString(prod.name)));
                }
            }
        } else {
            m_cameraService->stopCamera();
            m_cameraStatusLabel->setText("<b>[Camera Scanner Module Placeholder]</b> — Ready for integration with Eman 2's module");
            m_cameraToggleBtn->setText("Simulate Camera Scan");
        }
    }
}
