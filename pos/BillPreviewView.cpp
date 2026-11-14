#include "BillPreviewView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QMessageBox>

BillPreviewView::BillPreviewView(IOrderStorageService* storageService, QWidget* parent)
    : QWidget(parent), m_storageService(storageService) {
    setupUi();
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

    auto* backBtn = new QPushButton("Back to Order", this);
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

    // Receipt Container Frame
    auto* invoiceFrame = new QFrame(this);
    invoiceFrame->setStyleSheet(
        "QFrame {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #D1D5DB;"
        "  border-radius: 4px;"
        "  padding: 20px;"
        "}"
    );
    auto* invoiceLayout = new QVBoxLayout(invoiceFrame);
    invoiceLayout->setContentsMargins(20, 20, 20, 20);
    invoiceLayout->setSpacing(16);

    // Invoice Header Bar
    auto* headerBar = new QHBoxLayout();

    auto* brandCol = new QVBoxLayout();
    auto* brandName = new QLabel("RETAIL POS SUPERMARKET", invoiceFrame);
    brandName->setStyleSheet("font-size: 16px; font-weight: bold; color: #1E3A5F; letter-spacing: 0.5px;");
    auto* storeInfo = new QLabel("Store: Central Branch | Cash Register #01", invoiceFrame);
    storeInfo->setStyleSheet("font-size: 12px; color: #6B7280;");
    brandCol->addWidget(brandName);
    brandCol->addWidget(storeInfo);

    m_paymentStatusBadge = new QLabel("PENDING", invoiceFrame);
    m_paymentStatusBadge->setStyleSheet(
        "background-color: #FEF3C7;"
        "color: #92400E;"
        "font-weight: bold;"
        "font-size: 12px;"
        "padding: 6px 14px;"
        "border-radius: 3px;"
        "border: 1px solid #FCD34D;"
    );
    m_paymentStatusBadge->setAlignment(Qt::AlignCenter);

    headerBar->addLayout(brandCol);
    headerBar->addStretch();
    headerBar->addWidget(m_paymentStatusBadge);
    invoiceLayout->addLayout(headerBar);

    auto* divider1 = new QFrame(invoiceFrame);
    divider1->setFrameShape(QFrame::HLine);
    divider1->setStyleSheet("color: #E5E7EB;");
    invoiceLayout->addWidget(divider1);

    // Customer & Order Metadata Grid
    auto* metaGrid = new QGridLayout();
    metaGrid->setHorizontalSpacing(28);
    metaGrid->setVerticalSpacing(10);

    auto addMeta = [this, metaGrid](const QString& title, QLabel*& valLabel, int r, int c) {
        auto* t = new QLabel(title, this);
        t->setStyleSheet("font-weight: 600; color: #4B5563; font-size: 12px;");
        valLabel = new QLabel("-", this);
        valLabel->setStyleSheet("font-size: 13px; color: #1F2937; font-weight: 500;");
        metaGrid->addWidget(t, r, c * 2);
        metaGrid->addWidget(valLabel, r, c * 2 + 1);
    };

    addMeta("Bill ID:",          m_billIdLabel,          0, 0);
    addMeta("Order ID:",         m_orderIdLabel,         0, 1);
    addMeta("Customer Name:",    m_customerNameLabel,    1, 0);
    addMeta("Customer Contact:", m_customerContactLabel, 1, 1);
    addMeta("Date & Time:",      m_timestampLabel,       2, 0);

    invoiceLayout->addLayout(metaGrid);

    // Items Table
    m_itemsTable = new QTableWidget(invoiceFrame);
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
    totalsGrid->setVerticalSpacing(8);

    auto* subTitle = new QLabel("Total (Without Tax):", invoiceFrame);
    subTitle->setStyleSheet("font-size: 13px; color: #4B5563;");
    m_subtotalLabel = new QLabel("Rs. 0.00", invoiceFrame);
    m_subtotalLabel->setStyleSheet("font-size: 13px; font-weight: 600; color: #1F2937;");
    m_subtotalLabel->setAlignment(Qt::AlignRight);

    auto* discTitle = new QLabel("Total Discount:", invoiceFrame);
    discTitle->setStyleSheet("font-size: 13px; color: #4B5563;");
    m_discountLabel = new QLabel("-Rs. 0.00", invoiceFrame);
    m_discountLabel->setStyleSheet("font-size: 13px; font-weight: 600; color: #166534;");
    m_discountLabel->setAlignment(Qt::AlignRight);

    auto* taxTitle = new QLabel("Total Tax:", invoiceFrame);
    taxTitle->setStyleSheet("font-size: 13px; color: #4B5563;");
    m_taxLabel = new QLabel("+Rs. 0.00", invoiceFrame);
    m_taxLabel->setStyleSheet("font-size: 13px; font-weight: 600; color: #1F2937;");
    m_taxLabel->setAlignment(Qt::AlignRight);

    auto* totTitle = new QLabel("TOTAL PAYABLE:", invoiceFrame);
    totTitle->setStyleSheet("font-size: 15px; font-weight: bold; color: #1E3A5F;");
    m_totalPayableLabel = new QLabel("Rs. 0.00", invoiceFrame);
    m_totalPayableLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #1E3A5F;");
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

    mainLayout->addWidget(invoiceFrame, 1);

    // Bottom Action Buttons
    auto* bottomActions = new QHBoxLayout();

    auto* printBtn = new QPushButton("Save Order to Records", this);
    printBtn->setStyleSheet(
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
    connect(printBtn, &QPushButton::clicked, this, &BillPreviewView::onPrintOrSaveClicked);

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

    bottomActions->addWidget(printBtn);
    bottomActions->addStretch();
    bottomActions->addWidget(m_proceedPaymentBtn);
    mainLayout->addLayout(bottomActions);
}

void BillPreviewView::loadBill(const BillInfo& bill) {
    m_currentBill = bill;
    populateBillData();
}

void BillPreviewView::populateBillData() {
    m_billIdLabel->setText(QString("<b>%1</b>").arg(QString::fromStdString(m_currentBill.billId)));
    m_orderIdLabel->setText(QString("<b>%1</b>").arg(QString::fromStdString(m_currentBill.orderId)));
    m_customerNameLabel->setText(QString::fromStdString(m_currentBill.customerName));
    m_customerContactLabel->setText(m_currentBill.customerContact.empty() ? "None provided" : QString::fromStdString(m_currentBill.customerContact));
    m_timestampLabel->setText(QString::fromStdString(m_currentBill.timestamp));

    if (m_currentBill.paymentStatus == "Paid") {
        m_paymentStatusBadge->setText("PAID");
        m_paymentStatusBadge->setStyleSheet(
            "background-color: #DCFCE7;"
            "color: #166534;"
            "font-weight: bold;"
            "font-size: 12px;"
            "padding: 6px 14px;"
            "border-radius: 3px;"
            "border: 1px solid #86EFAC;"
        );
        m_proceedPaymentBtn->setEnabled(false);
        m_proceedPaymentBtn->setText("Payment Completed");
    } else {
        m_paymentStatusBadge->setText("PENDING");
        m_paymentStatusBadge->setStyleSheet(
            "background-color: #FEF3C7;"
            "color: #92400E;"
            "font-weight: bold;"
            "font-size: 12px;"
            "padding: 6px 14px;"
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
    if (m_storageService) {
        m_storageService->saveBill(m_currentBill);
        QMessageBox::information(this, "Order Saved",
                                 QString("Order record '%1' (Bill: %2) saved to store records.")
                                 .arg(QString::fromStdString(m_currentBill.orderId))
                                 .arg(QString::fromStdString(m_currentBill.billId)));
    }
}
