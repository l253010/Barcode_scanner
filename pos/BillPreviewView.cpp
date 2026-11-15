#include "BillPreviewView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QMessageBox>

BillPreviewView::BillPreviewView(IOrderStorageService* storageService, QWidget* parent)
    : QWidget(parent), m_storageService(storageService) {
    setupUi();
    populateBillData();
}

void BillPreviewView::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(32, 24, 32, 24);
    mainLayout->setSpacing(18);

    // Title and Top Actions
    auto* topLayout = new QHBoxLayout();
    auto* titleCol = new QVBoxLayout();
    auto* titleLabel = new QLabel("Customer Invoice Preview", this);
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #1E3A5F;");
    auto* subtitleLabel = new QLabel("Official Retail Sales Receipt & Transaction Summary", this);
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
    connect(backBtn, &QPushButton::clicked, this, &BillPreviewView::backToCashierRequested);

    topLayout->addLayout(titleCol);
    topLayout->addStretch();
    topLayout->addWidget(backBtn);
    mainLayout->addLayout(topLayout);

    // Empty state card
    m_emptyStateFrame = new QFrame(this);
    m_emptyStateFrame->setStyleSheet(
        "QFrame {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #D1D5DB;"
        "  border-radius: 4px;"
        "  padding: 40px;"
        "}"
    );
    auto* emptyLayout = new QVBoxLayout(m_emptyStateFrame);
    emptyLayout->setSpacing(16);
    emptyLayout->setAlignment(Qt::AlignCenter);

    auto* emptyTitle = new QLabel("No Active Customer Bill", m_emptyStateFrame);
    emptyTitle->setStyleSheet("font-size: 18px; font-weight: bold; color: #1E3A5F;");
    emptyTitle->setAlignment(Qt::AlignCenter);

    auto* emptyDesc = new QLabel("There are currently no items in the active transaction.\nAdd products in Cashier / New Order and click 'Generate Bill' to produce a receipt.", m_emptyStateFrame);
    emptyDesc->setStyleSheet("font-size: 13px; color: #4B5563; line-height: 1.5;");
    emptyDesc->setAlignment(Qt::AlignCenter);

    auto* goToCashierBtn = new QPushButton("Open Cashier Terminal", m_emptyStateFrame);
    goToCashierBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #1E3A5F;"
        "  color: #FFFFFF;"
        "  font-weight: bold;"
        "  font-size: 13px;"
        "  padding: 10px 24px;"
        "  border-radius: 4px;"
        "  border: none;"
        "}"
        "QPushButton:hover { background-color: #2B4C7E; }"
    );
    connect(goToCashierBtn, &QPushButton::clicked, this, &BillPreviewView::backToCashierRequested);

    emptyLayout->addWidget(emptyTitle);
    emptyLayout->addWidget(emptyDesc);
    emptyLayout->addWidget(goToCashierBtn, 0, Qt::AlignCenter);
    mainLayout->addWidget(m_emptyStateFrame);

    // Invoice Container Frame (Paper Style)
    m_invoiceFrame = new QFrame(this);
    m_invoiceFrame->setStyleSheet(
        "QFrame {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #D1D5DB;"
        "  border-radius: 4px;"
        "}"
    );
    auto* invoiceLayout = new QVBoxLayout(m_invoiceFrame);
    invoiceLayout->setContentsMargins(24, 20, 24, 20);
    invoiceLayout->setSpacing(14);

    // Invoice Header Bar
    auto* headerBar = new QHBoxLayout();

    auto* brandCol = new QVBoxLayout();
    auto* brandName = new QLabel("RETAIL POS SUPERMARKET", m_invoiceFrame);
    brandName->setStyleSheet("font-size: 16px; font-weight: bold; color: #1E3A5F; letter-spacing: 0.5px; background: transparent;");
    auto* storeInfo = new QLabel("Store: Central Branch | Cash Register #01", m_invoiceFrame);
    storeInfo->setStyleSheet("font-size: 12px; color: #6B7280; background: transparent;");
    brandCol->addWidget(brandName);
    brandCol->addWidget(storeInfo);

    m_paymentStatusBadge = new QLabel("PENDING", m_invoiceFrame);
    m_paymentStatusBadge->setStyleSheet(
        "background-color: #FEF3C7;"
        "color: #92400E;"
        "font-weight: bold;"
        "font-size: 12px;"
        "padding: 6px 16px;"
        "border-radius: 3px;"
        "border: 1px solid #FCD34D;"
    );
    m_paymentStatusBadge->setAlignment(Qt::AlignCenter);

    headerBar->addLayout(brandCol);
    headerBar->addStretch();
    headerBar->addWidget(m_paymentStatusBadge);
    invoiceLayout->addLayout(headerBar);

    auto* divider1 = new QFrame(m_invoiceFrame);
    divider1->setFrameShape(QFrame::HLine);
    divider1->setStyleSheet("color: #E5E7EB;");
    invoiceLayout->addWidget(divider1);

    // Customer & Order Metadata Grid (Parented to m_invoiceFrame)
    auto* metaGrid = new QGridLayout();
    metaGrid->setHorizontalSpacing(24);
    metaGrid->setVerticalSpacing(8);
    metaGrid->setColumnStretch(1, 1);
    metaGrid->setColumnStretch(3, 1);

    auto addMeta = [this, metaGrid](const QString& title, QLabel*& valLabel, int r, int c) {
        auto* t = new QLabel(title, m_invoiceFrame);
        t->setStyleSheet("font-weight: 600; color: #4B5563; font-size: 12px; background: transparent;");
        t->setMinimumWidth(110);
        valLabel = new QLabel("-", m_invoiceFrame);
        valLabel->setStyleSheet("font-size: 13px; color: #111827; font-weight: 600; background: transparent;");
        metaGrid->addWidget(t, r, c * 2);
        metaGrid->addWidget(valLabel, r, c * 2 + 1);
    };

    addMeta("Bill ID:",          m_billIdLabel,          0, 0);
    addMeta("Order ID:",         m_orderIdLabel,         0, 1);
    addMeta("Customer Name:",    m_customerNameLabel,    1, 0);
    addMeta("Contact:",          m_customerContactLabel, 1, 1);
    addMeta("Date & Time:",      m_timestampLabel,       2, 0);

    invoiceLayout->addLayout(metaGrid);

    // Items Table
    m_itemsTable = new QTableWidget(m_invoiceFrame);
    m_itemsTable->setColumnCount(6);
    QStringList headers = {"Product Acronym", "Product Name", "Unit Price (Rs.)", "Discount (Rs.)", "Quantity", "Subtotal (Rs.)"};
    m_itemsTable->setHorizontalHeaderLabels(headers);
    m_itemsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_itemsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_itemsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_itemsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_itemsTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_itemsTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_itemsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_itemsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_itemsTable->setAlternatingRowColors(true);
    m_itemsTable->setStyleSheet(
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
    invoiceLayout->addWidget(m_itemsTable, 1);

    // Totals Grid
    auto* totalsBox = new QHBoxLayout();
    totalsBox->addStretch();

    auto* totalsGrid = new QGridLayout();
    totalsGrid->setHorizontalSpacing(24);
    totalsGrid->setVerticalSpacing(6);

    auto* subTitle = new QLabel("Subtotal (Without Tax):", m_invoiceFrame);
    subTitle->setStyleSheet("font-size: 13px; color: #4B5563; background: transparent;");
    m_subtotalLabel = new QLabel("Rs. 0.00", m_invoiceFrame);
    m_subtotalLabel->setStyleSheet("font-size: 13px; font-weight: 600; color: #1F2937; background: transparent;");
    m_subtotalLabel->setAlignment(Qt::AlignRight);

    auto* discTitle = new QLabel("Total Discount:", m_invoiceFrame);
    discTitle->setStyleSheet("font-size: 13px; color: #4B5563; background: transparent;");
    m_discountLabel = new QLabel("-Rs. 0.00", m_invoiceFrame);
    m_discountLabel->setStyleSheet("font-size: 13px; font-weight: 600; color: #166534; background: transparent;");
    m_discountLabel->setAlignment(Qt::AlignRight);

    auto* taxTitle = new QLabel("Total Tax:", m_invoiceFrame);
    taxTitle->setStyleSheet("font-size: 13px; color: #4B5563; background: transparent;");
    m_taxLabel = new QLabel("+Rs. 0.00", m_invoiceFrame);
    m_taxLabel->setStyleSheet("font-size: 13px; font-weight: 600; color: #1F2937; background: transparent;");
    m_taxLabel->setAlignment(Qt::AlignRight);

    auto* totTitle = new QLabel("FINAL PAYABLE AMOUNT:", m_invoiceFrame);
    totTitle->setStyleSheet("font-size: 15px; font-weight: bold; color: #1E3A5F; background: transparent;");
    m_totalPayableLabel = new QLabel("Rs. 0.00", m_invoiceFrame);
    m_totalPayableLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #1E3A5F; background: transparent;");
    m_totalPayableLabel->setAlignment(Qt::AlignRight);

    totalsGrid->addWidget(subTitle, 0, 0);
    totalsGrid->addWidget(m_subtotalLabel, 0, 1);
    totalsGrid->addWidget(discTitle, 1, 0);
    totalsGrid->addWidget(m_discountLabel, 1, 1);
    totalsGrid->addWidget(taxTitle, 2, 0);
    totalsGrid->addWidget(m_taxLabel, 2, 1);
    totalsGrid->addWidget(totTitle, 3, 0);
    totalsGrid->addWidget(m_totalPayableLabel, 3, 1);

    totalsBox->addLayout(totalsGrid);
    invoiceLayout->addLayout(totalsBox);

    mainLayout->addWidget(m_invoiceFrame, 1);

    // Bottom Action Buttons
    auto* bottomActions = new QHBoxLayout();

    m_saveOrderBtn = new QPushButton("Save Order to Records", this);
    m_saveOrderBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #D1D5DB;"
        "  color: #1F2937;"
        "  font-weight: 600;"
        "  font-size: 13px;"
        "  padding: 10px 18px;"
        "  border-radius: 4px;"
        "}"
        "QPushButton:hover { background-color: #F3F4F6; }"
    );
    connect(m_saveOrderBtn, &QPushButton::clicked, this, &BillPreviewView::onPrintOrSaveClicked);

    m_proceedPaymentBtn = new QPushButton("Proceed to Payment", this);
    m_proceedPaymentBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #166534;"
        "  color: #FFFFFF;"
        "  font-weight: bold;"
        "  font-size: 14px;"
        "  padding: 11px 24px;"
        "  border-radius: 4px;"
        "  border: none;"
        "}"
        "QPushButton:hover { background-color: #15803D; }"
        "QPushButton:pressed { background-color: #14532D; }"
    );
    connect(m_proceedPaymentBtn, &QPushButton::clicked, this, &BillPreviewView::onProceedToPaymentClicked);

    bottomActions->addWidget(m_saveOrderBtn);
    bottomActions->addStretch();
    bottomActions->addWidget(m_proceedPaymentBtn);
    mainLayout->addLayout(bottomActions);
}

void BillPreviewView::loadBill(const BillInfo& bill) {
    m_currentBill = bill;
    populateBillData();
}

void BillPreviewView::populateBillData() {
    bool hasItems = !m_currentBill.items.empty();
    m_emptyStateFrame->setVisible(!hasItems);
    m_invoiceFrame->setVisible(hasItems);
    m_saveOrderBtn->setEnabled(hasItems);
    m_proceedPaymentBtn->setEnabled(hasItems);

    if (!hasItems) return;

    m_billIdLabel->setText(QString::fromStdString(m_currentBill.billId));
    m_orderIdLabel->setText(QString::fromStdString(m_currentBill.orderId));
    m_customerNameLabel->setText(QString::fromStdString(m_currentBill.customerName));
    m_customerContactLabel->setText(m_currentBill.customerContact.empty() ? "None" : QString::fromStdString(m_currentBill.customerContact));
    m_timestampLabel->setText(QString::fromStdString(m_currentBill.timestamp));

    if (m_currentBill.paymentStatus == "Paid") {
        m_paymentStatusBadge->setText("PAID");
        m_paymentStatusBadge->setStyleSheet(
            "background-color: #DCFCE7;"
            "color: #166534;"
            "font-weight: bold;"
            "font-size: 12px;"
            "padding: 6px 16px;"
            "border-radius: 3px;"
            "border: 1px solid #86EFAC;"
        );
        m_proceedPaymentBtn->setEnabled(false);
        m_proceedPaymentBtn->setText("Payment Completed (PAID)");
    } else {
        m_paymentStatusBadge->setText("PENDING");
        m_paymentStatusBadge->setStyleSheet(
            "background-color: #FEF3C7;"
            "color: #92400E;"
            "font-weight: bold;"
            "font-size: 12px;"
            "padding: 6px 16px;"
            "border-radius: 3px;"
            "border: 1px solid #FCD34D;"
        );
        m_proceedPaymentBtn->setEnabled(true);
        m_proceedPaymentBtn->setText("Proceed to Payment");
    }

    m_itemsTable->setRowCount(static_cast<int>(m_currentBill.items.size()));
    for (int i = 0; i < static_cast<int>(m_currentBill.items.size()); ++i) {
        const auto& item = m_currentBill.items[i];

        auto* acr = new QTableWidgetItem(QString::fromStdString(item.product.acronym));
        acr->setTextAlignment(Qt::AlignCenter);

        auto* name = new QTableWidgetItem(QString::fromStdString(item.product.name));

        auto* price = new QTableWidgetItem(QString("Rs. %1").arg(item.product.unitPrice, 0, 'f', 2));
        price->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        auto* disc = new QTableWidgetItem(QString("-Rs. %1").arg(item.product.discountPerUnit, 0, 'f', 2));
        disc->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        disc->setForeground(QBrush(QColor("#166534")));

        auto* qty = new QTableWidgetItem(QString::number(item.quantity));
        qty->setTextAlignment(Qt::AlignCenter);

        auto* sub = new QTableWidgetItem(QString("Rs. %1").arg(item.getSubtotal(), 0, 'f', 2));
        sub->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        sub->setFont(QFont("", -1, QFont::Bold));

        m_itemsTable->setItem(i, 0, acr);
        m_itemsTable->setItem(i, 1, name);
        m_itemsTable->setItem(i, 2, price);
        m_itemsTable->setItem(i, 3, disc);
        m_itemsTable->setItem(i, 4, qty);
        m_itemsTable->setItem(i, 5, sub);
    }

    m_subtotalLabel->setText(QString("Rs. %1").arg(m_currentBill.totalWithoutTax, 0, 'f', 2));
    m_discountLabel->setText(QString("-Rs. %1").arg(m_currentBill.totalDiscount, 0, 'f', 2));
    m_taxLabel->setText(QString("+Rs. %1").arg(m_currentBill.totalTax, 0, 'f', 2));
    m_totalPayableLabel->setText(QString("Rs. %1").arg(m_currentBill.totalPayable, 0, 'f', 2));
}

void BillPreviewView::onProceedToPaymentClicked() {
    emit proceedToPaymentRequested(m_currentBill);
}

void BillPreviewView::onPrintOrSaveClicked() {
    if (m_storageService && !m_currentBill.items.empty()) {
        m_storageService->saveBill(m_currentBill);
        QMessageBox::information(this, "Order Saved",
                                 QString("Order record '%1' (Bill: %2) saved to store records.")
                                 .arg(QString::fromStdString(m_currentBill.orderId))
                                 .arg(QString::fromStdString(m_currentBill.billId)));
    }
}
