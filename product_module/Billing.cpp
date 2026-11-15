// Billing.cpp
// ----------------------------------------------------------------------------
// Implementation of the Billing calculation and order engine.
// Free of <vector>, <map>, <sstream>, <cassert>, and <cctype>.
// ----------------------------------------------------------------------------

#include "Billing.h"
#include <cmath>
#include <algorithm>

namespace ProductBilling {

namespace {
    // Formats double to fixed decimal string without <sstream> or <iomanip>
    std::string formatDouble(double value, int decimals = 2) {
        if (std::isnan(value)) return "0.00";
        if (value < 0.0) return "-" + formatDouble(-value, decimals);

        double factor = 1.0;
        for (int i = 0; i < decimals; ++i) {
            factor *= 10.0;
        }

        long long totalUnits = static_cast<long long>(std::round(value * factor));
        long long whole = totalUnits / static_cast<long long>(factor);
        long long frac = totalUnits % static_cast<long long>(factor);

        std::string fracStr = std::to_string(frac);
        while (static_cast<int>(fracStr.length()) < decimals) {
            fracStr = "0" + fracStr;
        }

        if (decimals == 0) {
            return std::to_string(whole);
        }
        return std::to_string(whole) + "." + fracStr;
    }

    // Pads a string with padChar up to width on the right (left-aligned text)
    std::string padRight(const std::string& str, size_t width, char padChar = ' ') {
        if (str.length() >= width) return str;
        return str + std::string(width - str.length(), padChar);
    }

    // Pads a string with padChar up to width on the left (right-aligned numbers)
    std::string padLeft(const std::string& str, size_t width, char padChar = ' ') {
        if (str.length() >= width) return str;
        return std::string(width - str.length(), padChar) + str;
    }

    double roundCurrency(double val) {
        return std::round(val * 100.0) / 100.0;
    }
}

// ============================================================================
// BillItem Implementation
// ============================================================================

BillItem::BillItem()
    : m_product()
    , m_quantity(0)
    , m_unitPrice(0.0)
    , m_itemDiscount(0.0)
{
}

BillItem::BillItem(const Product& product, int quantity, double itemDiscount)
    : m_product(product)
    , m_quantity(quantity > 0 ? quantity : 0)
    , m_unitPrice(product.getPrice())
    , m_itemDiscount(0.0)
{
    if (itemDiscount > 0.0) {
        m_itemDiscount = itemDiscount;
    } else if (product.getDiscountPerUnit() > 0.0) {
        m_itemDiscount = roundCurrency(product.getDiscountPerUnit() * m_quantity);
    }
    // Clamp item discount to not exceed the subtotal
    double sub = getSubtotal();
    if (m_itemDiscount > sub) {
        m_itemDiscount = sub;
    }
}

const Product& BillItem::getProduct() const        { return m_product; }
int            BillItem::getQuantity() const       { return m_quantity; }
double         BillItem::getUnitPrice() const      { return m_unitPrice; }
double         BillItem::getItemDiscount() const   { return m_itemDiscount; }
double         BillItem::getTaxRate() const        { return m_product.getTaxRate(); }

void BillItem::setQuantity(int quantity) {
    m_quantity = (quantity > 0 ? quantity : 0);
    double sub = getSubtotal();
    if (m_itemDiscount > sub) {
        m_itemDiscount = sub;
    }
}

void BillItem::setItemDiscount(double discount) {
    if (discount < 0.0) {
        m_itemDiscount = 0.0;
    } else {
        double sub = getSubtotal();
        m_itemDiscount = std::min(discount, sub);
    }
}

double BillItem::getSubtotal() const {
    return roundCurrency(m_unitPrice * m_quantity);
}

double BillItem::getTaxableAmount() const {
    double sub = getSubtotal();
    return std::max(0.0, roundCurrency(sub - m_itemDiscount));
}

double BillItem::getTaxAmount() const {
    double taxable = getTaxableAmount();
    return roundCurrency(taxable * m_product.getTaxRateFraction());
}

double BillItem::getLineTotal() const {
    return roundCurrency(getTaxableAmount() + getTaxAmount());
}

std::string BillItem::toFormattedRow() const {
    std::string displayName = m_product.getName();
    if (displayName.length() > 24) {
        displayName = displayName.substr(0, 21) + "...";
    }

    std::string row = padRight(m_product.getBarcode(), 10) + " | " +
                      padRight(displayName, 24) + " | " +
                      padLeft(std::to_string(m_quantity), 4) + " | $" +
                      padLeft(formatDouble(m_unitPrice, 2), 7) + " | $" +
                      padLeft(formatDouble(getSubtotal(), 2), 7) + " | $" +
                      padLeft(formatDouble(m_itemDiscount, 2), 6) + " | $" +
                      padLeft(formatDouble(getTaxAmount(), 2), 6) + " | $" +
                      padLeft(formatDouble(getLineTotal(), 2), 8);
    return row;
}

// ============================================================================
// BillDiscount Implementation
// ============================================================================

double BillDiscount::computeDiscount(double baseSubtotal) const {
    if (baseSubtotal <= 0.0 || value <= 0.0) {
        return 0.0;
    }

    double discountAmt = 0.0;
    switch (type) {
        case DiscountType::PERCENTAGE:
            discountAmt = baseSubtotal * (std::min(100.0, value) / 100.0);
            break;
        case DiscountType::FIXED_AMOUNT:
            discountAmt = std::min(baseSubtotal, value);
            break;
        case DiscountType::NONE:
        default:
            discountAmt = 0.0;
            break;
    }
    return roundCurrency(discountAmt);
}

// ============================================================================
// Bill Implementation
// ============================================================================

Bill::Bill(const std::string& billId)
    : m_billId(billId.empty() ? "BILL-0001" : billId)
    , m_billDiscount(DiscountType::NONE, 0.0)
{
}

const std::string& Bill::getBillId() const { return m_billId; }
void Bill::setBillId(const std::string& billId) { m_billId = billId; }

bool Bill::addItem(const Product& product,
                   int quantity,
                   double itemDiscount,
                   std::string* errorMsg)
{
    if (!product.isValid()) {
        if (errorMsg) *errorMsg = "Invalid product specifications.";
        return false;
    }

    if (quantity <= 0) {
        if (errorMsg) *errorMsg = "Quantity must be greater than zero (received " + std::to_string(quantity) + ").";
        return false;
    }

    // Check if the product is already in the bill
    for (size_t i = 0; i < m_items.size(); ++i) {
        if (m_items[i].getProduct().getBarcode() == product.getBarcode()) {
            int newTotalQty = m_items[i].getQuantity() + quantity;
            if (!product.hasSufficientStock(newTotalQty)) {
                if (errorMsg) {
                    *errorMsg = "Insufficient stock for '" + product.getName() +
                                "'. Already in bill: " + std::to_string(m_items[i].getQuantity()) +
                                ", Adding: " + std::to_string(quantity) +
                                ", Total required: " + std::to_string(newTotalQty) +
                                ", Available: " + std::to_string(product.getAvailableStock());
                }
                return false;
            }
            m_items[i].setQuantity(newTotalQty);
            if (itemDiscount > 0.0) {
                m_items[i].setItemDiscount(m_items[i].getItemDiscount() + itemDiscount);
            }
            if (errorMsg) *errorMsg = "";
            return true;
        }
    }

    // New item in bill
    if (!product.hasSufficientStock(quantity)) {
        if (errorMsg) {
            *errorMsg = "Insufficient stock for '" + product.getName() +
                        "'. Requested: " + std::to_string(quantity) +
                        ", Available: " + std::to_string(product.getAvailableStock());
        }
        return false;
    }

    m_items.emplace_back(product, quantity, itemDiscount);
    if (errorMsg) *errorMsg = "";
    return true;
}

bool Bill::addItemByBarcode(ProductManager& manager,
                            const std::string& barcode,
                            int quantity,
                            double itemDiscount,
                            std::string* errorMsg)
{
    const Product* prod = manager.findProductByBarcode(barcode);
    if (prod == nullptr) {
        if (errorMsg) *errorMsg = "Product with barcode '" + barcode + "' not found in catalog.";
        return false;
    }
    return addItem(*prod, quantity, itemDiscount, errorMsg);
}

bool Bill::removeItem(size_t index, std::string* errorMsg) {
    if (index >= m_items.size()) {
        if (errorMsg) *errorMsg = "Invalid item index: " + std::to_string(index);
        return false;
    }
    m_items.erase(index);
    if (errorMsg) *errorMsg = "";
    return true;
}

bool Bill::updateItemQuantity(size_t index,
                              int newQuantity,
                              const ProductManager* manager,
                              std::string* errorMsg)
{
    if (index >= m_items.size()) {
        if (errorMsg) *errorMsg = "Invalid item index: " + std::to_string(index);
        return false;
    }

    if (newQuantity <= 0) {
        if (errorMsg) *errorMsg = "Quantity must be greater than zero.";
        return false;
    }

    BillItem& item = m_items[index];
    int availableStock = item.getProduct().getAvailableStock();

    if (manager != nullptr) {
        const Product* latest = manager->findProductByBarcode(item.getProduct().getBarcode());
        if (latest) {
            availableStock = latest->getAvailableStock();
        }
    }

    if (newQuantity > availableStock) {
        if (errorMsg) {
            *errorMsg = "Insufficient stock. Requested: " + std::to_string(newQuantity) +
                        ", Available: " + std::to_string(availableStock);
        }
        return false;
    }

    item.setQuantity(newQuantity);
    if (errorMsg) *errorMsg = "";
    return true;
}

void Bill::clear() {
    m_items.clear();
    m_billDiscount = BillDiscount(DiscountType::NONE, 0.0);
}

void Bill::setBillDiscount(DiscountType type, double value) {
    m_billDiscount = BillDiscount(type, value);
}

void Bill::clearBillDiscount() {
    m_billDiscount = BillDiscount(DiscountType::NONE, 0.0);
}

const BillDiscount& Bill::getBillDiscount() const {
    return m_billDiscount;
}

// Financial Calculations
double Bill::calculateSubtotal() const {
    double total = 0.0;
    for (size_t i = 0; i < m_items.size(); ++i) {
        total += m_items[i].getSubtotal();
    }
    return roundCurrency(total);
}

double Bill::calculateItemDiscounts() const {
    double total = 0.0;
    for (size_t i = 0; i < m_items.size(); ++i) {
        total += m_items[i].getItemDiscount();
    }
    return roundCurrency(total);
}

double Bill::calculateBillDiscount() const {
    double sub = calculateSubtotal();
    double itemDisc = calculateItemDiscounts();
    double netSub = std::max(0.0, sub - itemDisc);
    return m_billDiscount.computeDiscount(netSub);
}

double Bill::calculateTotalDiscount() const {
    return roundCurrency(calculateItemDiscounts() + calculateBillDiscount());
}

double Bill::calculateTotalTax() const {
    double subtotal = calculateSubtotal();
    double itemDiscounts = calculateItemDiscounts();
    double netSubtotal = std::max(0.0, subtotal - itemDiscounts);
    double billDiscount = calculateBillDiscount();

    // Discount ratio to proportionally apply bill discount across items
    double billDiscountRatio = (netSubtotal > 0.0) ? (1.0 - (billDiscount / netSubtotal)) : 1.0;
    if (billDiscountRatio < 0.0) billDiscountRatio = 0.0;

    double totalTax = 0.0;
    for (size_t i = 0; i < m_items.size(); ++i) {
        double itemTaxable = m_items[i].getTaxableAmount() * billDiscountRatio;
        double itemTax = itemTaxable * m_items[i].getProduct().getTaxRateFraction();
        totalTax += itemTax;
    }
    return roundCurrency(totalTax);
}

double Bill::calculateFinalPayable() const {
    double subtotal = calculateSubtotal();
    double totalDiscount = calculateTotalDiscount();
    double totalTax = calculateTotalTax();
    double netPayable = std::max(0.0, (subtotal - totalDiscount)) + totalTax;
    return roundCurrency(netPayable);
}

bool Bill::finalizeAndCommitStock(ProductManager& manager, std::string* errorMsg) {
    if (m_items.empty()) {
        if (errorMsg) *errorMsg = "Cannot finalize an empty bill.";
        return false;
    }

    // Step 1: Pre-validation of all items to ensure atomic commit
    for (size_t i = 0; i < m_items.size(); ++i) {
        const std::string& barcode = m_items[i].getProduct().getBarcode();
        int requestedQty = m_items[i].getQuantity();

        StockValidationResult res = manager.validateStock(barcode, requestedQty);
        if (!res.isValid) {
            if (errorMsg) {
                *errorMsg = "Stock commitment failed: " + res.message;
            }
            return false;
        }
    }

    // Step 2: Deduct stock from manager
    for (size_t i = 0; i < m_items.size(); ++i) {
        const std::string& barcode = m_items[i].getProduct().getBarcode();
        int requestedQty = m_items[i].getQuantity();
        manager.reduceStock(barcode, requestedQty);
    }

    if (errorMsg) *errorMsg = "";
    return true;
}

size_t Bill::getItemCount() const {
    return m_items.size();
}

const DynamicArray<BillItem>& Bill::getItems() const {
    return m_items;
}

const BillItem* Bill::getItem(size_t index) const {
    if (index < m_items.size()) {
        return &m_items[index];
    }
    return nullptr;
}

BillSummary Bill::getSummary() const {
    BillSummary summary;
    summary.distinctItemCount = static_cast<int>(m_items.size());
    summary.totalUnitsCount = 0;
    for (size_t i = 0; i < m_items.size(); ++i) {
        summary.totalUnitsCount += m_items[i].getQuantity();
    }
    summary.rawSubtotal = calculateSubtotal();
    summary.itemDiscounts = calculateItemDiscounts();
    summary.billDiscount = calculateBillDiscount();
    summary.totalDiscount = calculateTotalDiscount();
    summary.netTaxableAmount = std::max(0.0, roundCurrency(summary.rawSubtotal - summary.totalDiscount));
    summary.totalTax = calculateTotalTax();
    summary.totalPayable = calculateFinalPayable();
    return summary;
}

std::string Bill::generateReceipt(const std::string& storeName) const {
    BillSummary sum = getSummary();
    std::string out;

    out += "========================================================================================\n";
    out += "                               " + storeName + "\n";
    out += "                             TAX INVOICE / RECEIPT\n";
    out += "========================================================================================\n";
    out += " Invoice ID: " + padRight(m_billId, 24) +
           " Items Count : " + std::to_string(sum.distinctItemCount) +
           " (" + std::to_string(sum.totalUnitsCount) + " units)\n";
    out += "----------------------------------------------------------------------------------------\n";
    out += " Barcode    | Product Description      |  Qty |   Price  | Subtotal |  Disc. |   Tax  |  Total\n";
    out += "----------------------------------------------------------------------------------------\n";

    if (m_items.empty()) {
        out += "                             *** NO ITEMS IN BILL ***\n";
    } else {
        for (size_t i = 0; i < m_items.size(); ++i) {
            out += " " + m_items[i].toFormattedRow() + "\n";
        }
    }

    out += "----------------------------------------------------------------------------------------\n";
    out += " FINANCIAL SUMMARY:\n";
    out += "   Gross Subtotal          : $" + padLeft(formatDouble(sum.rawSubtotal, 2), 10) + "\n";

    if (sum.itemDiscounts > 0.0) {
        out += "   Item-level Discounts    : -$" + padLeft(formatDouble(sum.itemDiscounts, 2), 9) + "\n";
    }
    if (sum.billDiscount > 0.0) {
        out += "   Bill-level Discount     : -$" + padLeft(formatDouble(sum.billDiscount, 2), 9);
        if (m_billDiscount.type == DiscountType::PERCENTAGE) {
            out += " (" + formatDouble(m_billDiscount.value, 2) + "% off)";
        }
        out += "\n";
    }
    if (sum.totalDiscount > 0.0) {
        out += "   Total Discounts Applied : -$" + padLeft(formatDouble(sum.totalDiscount, 2), 9) + "\n";
        out += "   Net Taxable Subtotal    : $" + padLeft(formatDouble(sum.netTaxableAmount, 2), 10) + "\n";
    }

    out += "   Total Tax               : +$" + padLeft(formatDouble(sum.totalTax, 2), 9) + "\n";
    out += "========================================================================================\n";
    out += "   TOTAL PAYABLE AMOUNT    : $" + padLeft(formatDouble(sum.totalPayable, 2), 10) + "\n";
    out += "========================================================================================\n";
    out += "                   Thank you for shopping with us! Please come again!\n";

    return out;
}

} // namespace ProductBilling
