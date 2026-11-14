#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QFrame>
#include "POSInterfaces.h"

class BillPreviewView : public QWidget {
    Q_OBJECT

public:
    explicit BillPreviewView(IOrderStorageService* storageService, QWidget* parent = nullptr);
    void loadBill(const BillInfo& bill);
    const BillInfo& getCurrentBill() const { return m_currentBill; }

signals:
    void proceedToPaymentRequested(const BillInfo& bill);
    void backToCashierRequested();

private slots:
    void onProceedToPaymentClicked();
    void onPrintOrSaveClicked();

private:
    IOrderStorageService* m_storageService;
    BillInfo              m_currentBill;

    // Header labels
    QLabel*       m_billIdLabel{nullptr};
    QLabel*       m_orderIdLabel{nullptr};
    QLabel*       m_customerNameLabel{nullptr};
    QLabel*       m_customerContactLabel{nullptr};
    QLabel*       m_timestampLabel{nullptr};
    QLabel*       m_paymentStatusBadge{nullptr};

    // Table
    QTableWidget* m_itemsTable{nullptr};

    // Summary labels
    QLabel*       m_subtotalLabel{nullptr};
    QLabel*       m_discountLabel{nullptr};
    QLabel*       m_taxLabel{nullptr};
    QLabel*       m_totalPayableLabel{nullptr};

    QPushButton*  m_proceedPaymentBtn{nullptr};

    void setupUi();
    void populateBillData();
};
