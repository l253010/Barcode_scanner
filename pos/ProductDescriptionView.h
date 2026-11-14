#pragma once

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QFrame>
#include "POSInterfaces.h"
#include "BarcodeGenerator.h"

// ----------------------------------------------------------------------------
// BarcodeDisplayWidget
// Custom cross-platform widget that draws Code 39 bars using Qt's QPainter
// and Increment 1's pure C++ BarcodeGenerator::encode logic!
// ----------------------------------------------------------------------------
class BarcodeDisplayWidget : public QWidget {
    Q_OBJECT

public:
    explicit BarcodeDisplayWidget(QWidget* parent = nullptr);
    void setBarcode(const std::string& code);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    std::string             m_code;
    std::vector<BarElement> m_elements;
    bool                    m_hasData{false};
};

// ----------------------------------------------------------------------------
// ProductDescriptionView (Screen C)
// Dedicated screen displaying complete product details and barcode lookup.
// ----------------------------------------------------------------------------
class ProductDescriptionView : public QWidget {
    Q_OBJECT

public:
    explicit ProductDescriptionView(IProductService* productService, QWidget* parent = nullptr);
    void displayProduct(const std::string& barcode);

signals:
    void addProductToCart(const ProductInfo& product);
    void backToCashierRequested();

private slots:
    void onLookupClicked();
    void onProductDropdownChanged(int index);
    void onAddToCartClicked();

private:
    IProductService*      m_productService;
    ProductInfo           m_currentProduct;

    // UI Widgets
    QLineEdit*            m_lookupInput{nullptr};
    QComboBox*            m_productCombo{nullptr};
    QLabel*               m_lookupStatusLabel{nullptr};

    // Product detail labels
    QLabel*               m_idLabel{nullptr};
    QLabel*               m_barcodeLabel{nullptr};
    QLabel*               m_acronymLabel{nullptr};
    QLabel*               m_nameLabel{nullptr};
    QLabel*               m_unitLabel{nullptr};
    QLabel*               m_priceLabel{nullptr};
    QLabel*               m_taxLabel{nullptr};
    QLabel*               m_discountLabel{nullptr};
    QLabel*               m_stockLabel{nullptr};

    BarcodeDisplayWidget* m_barcodeWidget{nullptr};
    QPushButton*          m_addToCartBtn{nullptr};

    void setupUi();
    void updateFields(const ProductInfo& prod);
    void populateDropdown();
};
