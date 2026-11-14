#pragma once

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include "POSInterfaces.h"

class ManagerView : public QWidget {
    Q_OBJECT

public:
    explicit ManagerView(IProductService* productService, QWidget* parent = nullptr);
    void refreshCatalog();

signals:
    void inspectProductRequested(const std::string& barcode);
    void backToDashboardRequested();

private slots:
    void onSearchTextChanged(const QString& text);
    void onInspectClicked();

private:
    IProductService* m_productService;

    QLineEdit*    m_searchInput{nullptr};
    QTableWidget* m_catalogTable{nullptr};
    QLabel*       m_summaryLabel{nullptr};

    void setupUi();
    void populateTable(const std::vector<ProductInfo>& products);
};
