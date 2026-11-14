#include "PaymentView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QMessageBox>

PaymentView::PaymentView(IOrderStorageService* storageService, QWidget* parent)
    : QWidget(parent), m_storageService(storageService) {
    setupUi();
}

void PaymentView::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(36, 24, 36, 24);
    mainLayout->setSpacing(20);

    // Title & Back Button
    auto* topLayout = new QHBoxLayout();
    auto* titleCol = new QVBoxLayout();
    auto* titleLabel = new QLabel("Payment Processing Terminal", this);
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #1E3A5F;");
    auto* subtitleLabel = new QLabel("Select customer payment mode, record transaction details, and finalize sale", this);
    subtitleLabel->setStyleSheet("font-size: 13px; color: #4B5563;");
    titleCol->addWidget(titleLabel);
    titleCol->addWidget(subtitleLabel);

    auto* cancelBtn = new QPushButton("Cancel / Back to Bill", this);
    cancelBtn->setStyleSheet(
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
    connect(cancelBtn, &QPushButton::clicked, [this]() {
        emit backToBillRequested(m_currentBill);
    });

    topLayout->addLayout(titleCol);
    topLayout->addStretch();
    topLayout->addWidget(cancelBtn);
    mainLayout->addLayout(topLayout);

    // Order Overview Banner
    auto* overviewFrame = new QFrame(this);
    overviewFrame->setStyleSheet(
        "QFrame {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #D1D5DB;"
        "  border-radius: 4px;"
        "  padding: 16px;"
        "}"
    );
    auto* overviewLayout = new QHBoxLayout(overviewFrame);
    overviewLayout->setContentsMargins(16, 12, 16, 12);

    auto* metaCol = new QVBoxLayout();
    m_billIdLabel = new QLabel("Bill ID: -", overviewFrame);
    m_billIdLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #1F2937;");
    m_customerLabel = new QLabel("Customer: -", overviewFrame);
    m_customerLabel->setStyleSheet("font-size: 13px; color: #4B5563;");
    metaCol->addWidget(m_billIdLabel);
    metaCol->addWidget(m_customerLabel);

    auto* amountCol = new QVBoxLayout();
    auto* amtTitle = new QLabel("TOTAL AMOUNT DUE:", overviewFrame);
    amtTitle->setStyleSheet("font-size: 12px; font-weight: bold; color: #4B5563; text-align: right;");
    amtTitle->setAlignment(Qt::AlignRight);
    m_amountDueLabel = new QLabel("$0.00", overviewFrame);
    m_amountDueLabel->setStyleSheet("font-size: 26px; font-weight: bold; color: #1E3A5F;");
    m_amountDueLabel->setAlignment(Qt::AlignRight);
    amountCol->addWidget(amtTitle);
    amountCol->addWidget(m_amountDueLabel);

    overviewLayout->addLayout(metaCol);
    overviewLayout->addStretch();
    overviewLayout->addLayout(amountCol);
    mainLayout->addWidget(overviewFrame);

    // Payment Mode Selection Card
    auto* modeFrame = new QFrame(this);
    modeFrame->setStyleSheet(
        "QFrame {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #D1D5DB;"
        "  border-radius: 4px;"
        "  padding: 18px;"
        "}"
    );
    auto* modeLayout = new QVBoxLayout(modeFrame);
    modeLayout->setContentsMargins(18, 16, 18, 16);
    modeLayout->setSpacing(14);

    auto* modeTitle = new QLabel("Select Payment Mode:", modeFrame);
    modeTitle->setStyleSheet("font-size: 15px; font-weight: bold; color: #1E3A5F;");
    modeLayout->addWidget(modeTitle);

    auto* radioLayout = new QHBoxLayout();
    radioLayout->setSpacing(24);

    m_modeGroup = new QButtonGroup(this);

    m_cashRadio = new QRadioButton("Cash Payment", modeFrame);
    m_cashRadio->setStyleSheet("font-size: 14px; font-weight: 600; color: #1F2937; padding: 6px 12px;");
    m_cashRadio->setChecked(true);
    m_modeGroup->addButton(m_cashRadio, 0);

    m_cardRadio = new QRadioButton("Card (Debit / Credit)", modeFrame);
    m_cardRadio->setStyleSheet("font-size: 14px; font-weight: 600; color: #1F2937; padding: 6px 12px;");
    m_modeGroup->addButton(m_cardRadio, 1);

    m_walletRadio = new QRadioButton("Digital Wallet", modeFrame);
    m_walletRadio->setStyleSheet("font-size: 14px; font-weight: 600; color: #1F2937; padding: 6px 12px;");
    m_modeGroup->addButton(m_walletRadio, 2);

    radioLayout->addWidget(m_cashRadio);
    radioLayout->addWidget(m_cardRadio);
    radioLayout->addWidget(m_walletRadio);
    radioLayout->addStretch();
    modeLayout->addLayout(radioLayout);

    connect(m_modeGroup, &QButtonGroup::idClicked, this, &PaymentView::onPaymentModeChanged);

    // Dynamic Stacked Panels
    m_detailsStack = new QStackedWidget(modeFrame);
    m_detailsStack->addWidget(createCashPanel());
    m_detailsStack->addWidget(createCardPanel());
    m_detailsStack->addWidget(createWalletPanel());
    modeLayout->addWidget(m_detailsStack);

    mainLayout->addWidget(modeFrame);

    // Confirm Payment Button
    auto* confirmBtn = new QPushButton("Confirm Payment & Finalize Transaction", this);
    confirmBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #166534;"
        "  color: #FFFFFF;"
        "  font-weight: bold;"
        "  font-size: 15px;"
        "  padding: 13px 28px;"
        "  border-radius: 4px;"
        "  border: none;"
        "}"
        "QPushButton:hover { background-color: #15803D; }"
        "QPushButton:pressed { background-color: #14532D; }"
    );
    connect(confirmBtn, &QPushButton::clicked, this, &PaymentView::onConfirmPaymentClicked);
    mainLayout->addWidget(confirmBtn, 0, Qt::AlignRight);

    mainLayout->addStretch();
}

QWidget* PaymentView::createCashPanel() {
    auto* panel = new QFrame();
    panel->setStyleSheet("background-color: #F9FAFB; border: 1px solid #E5E7EB; border-radius: 4px; padding: 14px;");
    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(12);

    auto* grid = new QGridLayout();
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(10);

    auto* tendLabel = new QLabel("Amount Tendered ($):", panel);
    tendLabel->setStyleSheet("font-weight: 600; color: #1F2937; font-size: 13px;");
    m_tenderedInput = new QLineEdit(panel);
    m_tenderedInput->setPlaceholderText("0.00");
    m_tenderedInput->setStyleSheet("padding: 7px 10px; font-size: 14px; font-weight: bold; border: 1px solid #D1D5DB; border-radius: 4px; background-color: #FFFFFF;");
    connect(m_tenderedInput, &QLineEdit::textChanged, this, &PaymentView::onTenderedAmountChanged);

    auto* changeLabel = new QLabel("Change Due ($):", panel);
    changeLabel->setStyleSheet("font-weight: 600; color: #1F2937; font-size: 13px;");
    m_changeDueLabel = new QLabel("$0.00", panel);
    m_changeDueLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #166534;");

    grid->addWidget(tendLabel, 0, 0);
    grid->addWidget(m_tenderedInput, 0, 1);
    grid->addWidget(changeLabel, 1, 0);
    grid->addWidget(m_changeDueLabel, 1, 1);
    layout->addLayout(grid);

    // Quick Cash Buttons
    auto* quickLayout = new QHBoxLayout();
    quickLayout->setSpacing(8);
    auto* qLabel = new QLabel("Quick Cash:", panel);
    qLabel->setStyleSheet("color: #4B5563; font-size: 12px; font-weight: 600;");
    quickLayout->addWidget(qLabel);

    for (double amt : {10.0, 20.0, 50.0, 100.0}) {
        auto* qBtn = new QPushButton(QString("$%1").arg(static_cast<int>(amt)), panel);
        qBtn->setStyleSheet(
            "QPushButton {"
            "  background-color: #FFFFFF;"
            "  border: 1px solid #D1D5DB;"
            "  border-radius: 3px;"
            "  padding: 4px 10px;"
            "  font-weight: 600;"
            "  font-size: 12px;"
            "  color: #1F2937;"
            "}"
            "QPushButton:hover { background-color: #E5E7EB; }"
        );
        connect(qBtn, &QPushButton::clicked, [this, amt]() { onQuickCashClicked(amt); });
        quickLayout->addWidget(qBtn);
    }

    auto* exactBtn = new QPushButton("Exact Cash", panel);
    exactBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #EFF6FF;"
        "  border: 1px solid #93C5FD;"
        "  border-radius: 3px;"
        "  padding: 4px 10px;"
        "  font-weight: 600;"
        "  font-size: 12px;"
        "  color: #1E3A5F;"
        "}"
        "QPushButton:hover { background-color: #DBEAFE; }"
    );
    connect(exactBtn, &QPushButton::clicked, [this]() {
        onQuickCashClicked(m_currentBill.totalPayable);
    });
    quickLayout->addWidget(exactBtn);
    quickLayout->addStretch();
    layout->addLayout(quickLayout);

    return panel;
}

QWidget* PaymentView::createCardPanel() {
    auto* panel = new QFrame();
    panel->setStyleSheet("background-color: #F9FAFB; border: 1px solid #E5E7EB; border-radius: 4px; padding: 14px;");
    auto* grid = new QGridLayout(panel);
    grid->setContentsMargins(14, 14, 14, 14);
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(10);

    auto* mLabel = new QLabel("Merchant Terminal Name:", panel);
    mLabel->setStyleSheet("font-weight: 600; color: #1F2937; font-size: 13px;");
    m_cardMerchantInput = new QLineEdit("HBL Visa / Mastercard POS Terminal #1", panel);
    m_cardMerchantInput->setStyleSheet("padding: 6px 10px; border: 1px solid #D1D5DB; border-radius: 4px; background-color: #FFFFFF;");

    auto* aLabel = new QLabel("Card Auth / Transaction Code:", panel);
    aLabel->setStyleSheet("font-weight: 600; color: #1F2937; font-size: 13px;");
    m_cardAuthCodeInput = new QLineEdit("AUTH-892401", panel);
    m_cardAuthCodeInput->setStyleSheet("padding: 6px 10px; border: 1px solid #D1D5DB; border-radius: 4px; background-color: #FFFFFF;");

    grid->addWidget(mLabel, 0, 0);
    grid->addWidget(m_cardMerchantInput, 0, 1);
    grid->addWidget(aLabel, 1, 0);
    grid->addWidget(m_cardAuthCodeInput, 1, 1);

    return panel;
}

QWidget* PaymentView::createWalletPanel() {
    auto* panel = new QFrame();
    panel->setStyleSheet("background-color: #F9FAFB; border: 1px solid #E5E7EB; border-radius: 4px; padding: 14px;");
    auto* grid = new QGridLayout(panel);
    grid->setContentsMargins(14, 14, 14, 14);
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(10);

    auto* mLabel = new QLabel("Wallet Merchant Name:", panel);
    mLabel->setStyleSheet("font-weight: 600; color: #1F2937; font-size: 13px;");
    m_walletMerchantInput = new QLineEdit("JazzCash / EasyPaisa / Apple Pay Terminal", panel);
    m_walletMerchantInput->setStyleSheet("padding: 6px 10px; border: 1px solid #D1D5DB; border-radius: 4px; background-color: #FFFFFF;");

    auto* tLabel = new QLabel("Wallet Reference / Txn ID:", panel);
    tLabel->setStyleSheet("font-weight: 600; color: #1F2937; font-size: 13px;");
    m_walletTxnIdInput = new QLineEdit("WAL-TXN-559102", panel);
    m_walletTxnIdInput->setStyleSheet("padding: 6px 10px; border: 1px solid #D1D5DB; border-radius: 4px; background-color: #FFFFFF;");

    grid->addWidget(mLabel, 0, 0);
    grid->addWidget(m_walletMerchantInput, 0, 1);
    grid->addWidget(tLabel, 1, 0);
    grid->addWidget(m_walletTxnIdInput, 1, 1);

    return panel;
}

void PaymentView::loadBill(const BillInfo& bill) {
    m_currentBill = bill;
    m_billIdLabel->setText(QString("Bill ID: <b>%1</b> (Order: %2)")
                           .arg(QString::fromStdString(bill.billId))
                           .arg(QString::fromStdString(bill.orderId)));
    m_customerLabel->setText(QString("Customer: <b>%1</b>").arg(QString::fromStdString(bill.customerName)));
    m_amountDueLabel->setText(QString("$%1").arg(bill.totalPayable, 0, 'f', 2));

    // Reset default inputs
    m_tenderedInput->setText(QString("%1").arg(bill.totalPayable, 0, 'f', 2));
    updateChangeCalculation();
}

void PaymentView::onPaymentModeChanged(int id) {
    m_detailsStack->setCurrentIndex(id);
}

void PaymentView::onTenderedAmountChanged() {
    updateChangeCalculation();
}

void PaymentView::onQuickCashClicked(double amount) {
    m_tenderedInput->setText(QString("%1").arg(amount, 0, 'f', 2));
    updateChangeCalculation();
}

void PaymentView::updateChangeCalculation() {
    bool ok = false;
    double tendered = m_tenderedInput->text().toDouble(&ok);
    if (!ok || tendered < 0.0) {
        m_changeDueLabel->setText("<font color='#DC2626'>Invalid amount</font>");
        return;
    }

    double diff = tendered - m_currentBill.totalPayable;
    if (diff < -0.001) {
        m_changeDueLabel->setText(QString("<font color='#DC2626'>Short by $%1</font>").arg(-diff, 0, 'f', 2));
    } else {
        m_changeDueLabel->setText(QString("<font color='#166534'>$%1</font>").arg(diff, 0, 'f', 2));
    }
}

void PaymentView::onConfirmPaymentClicked() {
    int modeId = m_modeGroup->checkedId();

    if (modeId == 0) { // Cash
        bool ok = false;
        double tendered = m_tenderedInput->text().toDouble(&ok);
        if (!ok || tendered < m_currentBill.totalPayable - 0.001) {
            QMessageBox::warning(this, "Payment Insufficient",
                                 QString("Amount tendered ($%1) is less than the total amount due ($%2).")
                                 .arg(tendered, 0, 'f', 2)
                                 .arg(m_currentBill.totalPayable, 0, 'f', 2));
            return;
        }
        m_currentBill.paymentMode = "Cash";
        m_currentBill.paymentMerchant = "Cash Register Drawer #01";
        m_currentBill.amountTendered = tendered;
        m_currentBill.changeDue = tendered - m_currentBill.totalPayable;
    } else if (modeId == 1) { // Card
        m_currentBill.paymentMode = "Card";
        m_currentBill.paymentMerchant = m_cardMerchantInput->text().trimmed().toStdString();
        m_currentBill.amountTendered = m_currentBill.totalPayable;
        m_currentBill.changeDue = 0.0;
    } else { // Wallet
        m_currentBill.paymentMode = "Wallet";
        m_currentBill.paymentMerchant = m_walletMerchantInput->text().trimmed().toStdString();
        m_currentBill.amountTendered = m_currentBill.totalPayable;
        m_currentBill.changeDue = 0.0;
    }

    m_currentBill.paymentStatus = "Paid";

    if (m_storageService) {
        m_storageService->saveBill(m_currentBill);
    }

    QMessageBox::information(
        this,
        "Payment Approved",
        QString("Transaction successfully approved!\n\n"
                "Bill ID: %1\n"
                "Payment Mode: %2\n"
                "Merchant: %3\n"
                "Amount Paid: $%4\n"
                "Change Due: $%5\n\n"
                "Receipt marked as PAID.")
        .arg(QString::fromStdString(m_currentBill.billId))
        .arg(QString::fromStdString(m_currentBill.paymentMode))
        .arg(QString::fromStdString(m_currentBill.paymentMerchant))
        .arg(m_currentBill.amountTendered, 0, 'f', 2)
        .arg(m_currentBill.changeDue, 0, 'f', 2)
    );

    emit paymentCompleted(m_currentBill);
}
