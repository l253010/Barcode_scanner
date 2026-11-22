#pragma once

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTabWidget>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QDateEdit>
#include <QCheckBox>
#include <QGroupBox>
#include <QFrame>
#include <QTextEdit>
#include "POSInterfaces.h"

// ManagerView – Increment 3
// Tabbed Manager Dashboard covering:
//   Tab 0: Category Management
//   Tab 1: Product Management
//   Tab 2: Inventory Management
//   Tab 3: Discount Management
//   Tab 4: Reports
class ManagerView : public QWidget {
    Q_OBJECT

public:
    explicit ManagerView(IProductService* productService, QWidget* parent = nullptr);
    void refreshCatalog();

signals:
    void inspectProductRequested(const std::string& barcode);
    void backToDashboardRequested();

private slots:
    // Category tab
    void onAddCategoryClicked();
    void onEditCategoryClicked();
    void onCategoryTableSelectionChanged();

    // Product tab
    void onAddProductClicked();
    void onEditProductClicked();
    void onProductSearchChanged(const QString& text);
    void onProductTableSelectionChanged();

    // Inventory tab
    void onUpdateStockClicked();
    void onInspectClicked();
    void onInventorySearchChanged(const QString& text);

    // Discount tab
    void onAddSeasonClicked();
    void onEditSeasonClicked();
    void onToggleSeasonClicked();
    void onApplySpecialDiscountClicked();
    void onSeasonTableSelectionChanged();

    // Reports tab
    void onGenerateReportClicked();
    void onCustomDateRangeClicked();

private:
    IProductService* m_productService;

    QTabWidget* m_tabs{nullptr};

    // ---- Category Tab ----
    QTableWidget* m_categoryTable{nullptr};
    QLineEdit*    m_catIdInput{nullptr};
    QLineEdit*    m_catNameInput{nullptr};
    QLineEdit*    m_catAcrInput{nullptr};
    QPushButton*  m_addCatBtn{nullptr};
    QPushButton*  m_editCatBtn{nullptr};
    QLabel*       m_catStatusLabel{nullptr};

    // ---- Product Tab ----
    QLineEdit*    m_productSearchInput{nullptr};
    QTableWidget* m_productTable{nullptr};
    QLabel*       m_productSummaryLabel{nullptr};
    QPushButton*  m_addProductBtn{nullptr};
    QPushButton*  m_editProductBtn{nullptr};

    // ---- Inventory Tab ----
    QLineEdit*    m_inventorySearchInput{nullptr};
    QTableWidget* m_inventoryTable{nullptr};
    QLabel*       m_inventorySummaryLabel{nullptr};

    // ---- Discount Tab ----
    QTableWidget* m_seasonTable{nullptr};
    QLabel*       m_discountStatusLabel{nullptr};
    QLabel*       m_specialDiscountLabel{nullptr};
    QDoubleSpinBox* m_specialDiscountSpin{nullptr};
    QPushButton*  m_applySpecialBtn{nullptr};
    QPushButton*  m_addSeasonBtn{nullptr};
    QPushButton*  m_editSeasonBtn{nullptr};
    QPushButton*  m_toggleSeasonBtn{nullptr};
    double        m_currentSpecialDiscount{0.0};

    // ---- Reports Tab ----
    QComboBox*    m_reportTypeCombo{nullptr};
    QDateEdit*    m_fromDateEdit{nullptr};
    QDateEdit*    m_toDateEdit{nullptr};
    QTableWidget* m_reportTable{nullptr};
    QLabel*       m_reportSummaryLabel{nullptr};

    struct SeasonRecord {
        QString name;
        QString startDate;
        QString endDate;
        double  discountPercent{0.0};
        bool    active{false};
    };
    std::vector<CategoryInfo>   m_categories;
    std::vector<SeasonRecord>   m_seasons;

    // Helper methods
    void setupUi();
    QWidget* buildCategoryTab();
    QWidget* buildProductTab();
    QWidget* buildInventoryTab();
    QWidget* buildDiscountTab();
    QWidget* buildReportsTab();

    void refreshCategoryTable();
    void refreshProductTable(const std::vector<ProductInfo>& products);
    void refreshInventoryTable(const std::vector<ProductInfo>& products);
    void refreshSeasonTable();

    void generateReport(int type, const QString& from = QString(), const QString& to = QString());
    void showProductDialog(bool isEdit, const ProductInfo& existing = ProductInfo{});
    void showCategoryDialog(bool isEdit, int index = -1);
    void showSeasonDialog(bool isEdit, int index = -1);

    QPushButton* makeButton(const QString& text, const QString& color, QWidget* parent);
};
