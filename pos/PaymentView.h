#pragma once

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QButtonGroup>
#include <QStackedWidget>
#include <QFrame>
#include "POSInterfaces.h"

class PaymentView : public QWidget {
    Q_OBJECT

public:
    explicit PaymentView(IOrderStorageService* storageService, QWidget* parent = nullptr);
    void loadBill(const BillInfo& bill);

signals:
    void paymentCompleted(const BillInfo& updatedBill);
    void backToBillRequested(const BillInfo& currentBill);

private slots:
    void onPaymentModeChanged(int id);
    void onTenderedAmountChanged();
    void onQuickCashClicked(double amount);
    void onConfirmPaymentClicked();

private:
    IOrderStorageService* m_storageService;
    BillInfo              m_currentBill;

    // Header & Summary
    QLabel*         m_billIdLabel{nullptr};
    QLabel*         m_customerLabel{nullptr};
    QLabel*         m_amountDueLabel{nullptr};

    // Mode Selection
    QRadioButton*   m_cashRadio{nullptr};
    QRadioButton*   m_cardRadio{nullptr};
    QRadioButton*   m_walletRadio{nullptr};
    QButtonGroup*   m_modeGroup{nullptr};

    // Dynamic Stacked Panels
    QStackedWidget* m_detailsStack{nullptr};

    // Cash panel
    QLineEdit*      m_tenderedInput{nullptr};
    QLabel*         m_changeDueLabel{nullptr};

    // Card panel
    QLineEdit*      m_cardMerchantInput{nullptr};
    QLineEdit*      m_cardAuthCodeInput{nullptr};

    // Wallet panel
    QLineEdit*      m_walletMerchantInput{nullptr};
    QLineEdit*      m_walletTxnIdInput{nullptr};

    void setupUi();
    QWidget* createCashPanel();
    QWidget* createCardPanel();
    QWidget* createWalletPanel();
    void updateChangeCalculation();
};
