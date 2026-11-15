// Product.cpp
// ----------------------------------------------------------------------------
// Implementation of the Product class.
// Free of <vector>, <map>, <sstream>, <cassert>, and <cctype>.
// ----------------------------------------------------------------------------

#include "Product.h"
#include "DynamicArray.h"
#include <cmath>

namespace ProductBilling {

namespace {
    bool isWhitespaceChar(char c) {
        return c == ' ' || c == '\t' || c == '\r' || c == '\n';
    }

    std::string trim(const std::string& str) {
        if (str.empty()) return "";
        size_t first = 0;
        while (first < str.length() && isWhitespaceChar(str[first])) {
            ++first;
        }
        if (first == str.length()) return "";

        size_t last = str.length() - 1;
        while (last > first && isWhitespaceChar(str[last])) {
            --last;
        }
        return str.substr(first, (last - first + 1));
    }

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

    std::string padRight(const std::string& str, size_t width, char padChar = ' ') {
        if (str.length() >= width) return str;
        return str + std::string(width - str.length(), padChar);
    }

    std::string padLeft(const std::string& str, size_t width, char padChar = ' ') {
        if (str.length() >= width) return str;
        return std::string(width - str.length(), padChar) + str;
    }

    DynamicArray<std::string> splitString(const std::string& str, char delimiter) {
        DynamicArray<std::string> tokens;
        size_t start = 0;
        size_t end = str.find(delimiter);
        while (end != std::string::npos) {
            tokens.push_back(trim(str.substr(start, end - start)));
            start = end + 1;
            end = str.find(delimiter, start);
        }
        tokens.push_back(trim(str.substr(start)));
        return tokens;
    }
}

Product::Product()
    : m_productId("")
    , m_barcode("")
    , m_acronym("")
    , m_name("")
    , m_measurementUnit("count")
    , m_price(0.0)
    , m_taxAmount(0.0)
    , m_discountPerUnit(0.0)
    , m_availableStock(0)
    , m_barcodeFilePath("")
    , m_category("")
    , m_description("")
{
}

Product::Product(const std::string& productId,
                 const std::string& barcode,
                 const std::string& acronym,
                 const std::string& name,
                 const std::string& measurementUnit,
                 double price,
                 double taxAmount,
                 double discountPerUnit,
                 int availableStock,
                 const std::string& barcodeFilePath,
                 const std::string& category,
                 const std::string& description)
    : m_productId(productId)
    , m_barcode(barcode)
    , m_acronym(acronym.length() <= 8 ? acronym : acronym.substr(0, 8))
    , m_name(name)
    , m_measurementUnit(measurementUnit.empty() ? "count" : measurementUnit)
    , m_price(price >= 0.0 ? price : 0.0)
    , m_taxAmount(taxAmount >= 0.0 ? taxAmount : 0.0)
    , m_discountPerUnit(discountPerUnit >= 0.0 ? discountPerUnit : 0.0)
    , m_availableStock(availableStock >= 0 ? availableStock : 0)
    , m_barcodeFilePath(barcodeFilePath)
    , m_category(category)
    , m_description(description.empty() ? name : description)
{
}

// Backward-compatible constructor
Product::Product(const std::string& barcode,
                 const std::string& productId,
                 const std::string& name,
                 const std::string& description,
                 const std::string& category,
                 double price,
                 int availableStock,
                 double taxRate)
    : m_productId(productId)
    , m_barcode(barcode)
    , m_acronym(productId.length() <= 8 ? productId : productId.substr(0, 8))
    , m_name(name)
    , m_measurementUnit((category == "Bakery" || category == "Pantry") ? "weight" : "count")
    , m_price(price >= 0.0 ? price : 0.0)
    , m_taxAmount(price * (taxRate >= 0.0 ? taxRate / 100.0 : 0.0))
    , m_discountPerUnit(0.0)
    , m_availableStock(availableStock >= 0 ? availableStock : 0)
    , m_barcodeFilePath("barcodes/barcode_" + barcode + ".png")
    , m_category(category)
    , m_description(description.empty() ? name : description)
{
}

// Getters
const std::string& Product::getProductId() const       { return m_productId; }
const std::string& Product::getBarcode() const         { return m_barcode; }
const std::string& Product::getAcronym() const         { return m_acronym; }
const std::string& Product::getName() const            { return m_name; }
const std::string& Product::getMeasurementUnit() const { return m_measurementUnit; }
double             Product::getPrice() const           { return m_price; }
double             Product::getTaxAmount() const       { return m_taxAmount; }
double             Product::getDiscountPerUnit() const { return m_discountPerUnit; }
int                Product::getAvailableStock() const  { return m_availableStock; }
const std::string& Product::getBarcodeFilePath() const { return m_barcodeFilePath; }
const std::string& Product::getCategory() const        { return m_category; }
const std::string& Product::getDescription() const     { return m_description; }

double Product::getTaxRate() const {
    if (m_price <= 0.0) return 0.0;
    return (m_taxAmount / m_price) * 100.0;
}

double Product::getTaxRateFraction() const {
    if (m_price <= 0.0) return 0.0;
    return (m_taxAmount / m_price);
}

// Setters
void Product::setProductId(const std::string& productId) { m_productId = productId; }
void Product::setBarcode(const std::string& barcode)     { m_barcode = barcode; }
void Product::setAcronym(const std::string& acronym) {
    m_acronym = (acronym.length() <= 8 ? acronym : acronym.substr(0, 8));
}
void Product::setName(const std::string& name)                       { m_name = name; }
void Product::setMeasurementUnit(const std::string& unit)           { m_measurementUnit = unit; }
void Product::setPrice(double price)                                 { m_price = (price >= 0.0 ? price : 0.0); }
void Product::setTaxAmount(double taxAmount)                         { m_taxAmount = (taxAmount >= 0.0 ? taxAmount : 0.0); }
void Product::setDiscountPerUnit(double discount)                   { m_discountPerUnit = (discount >= 0.0 ? discount : 0.0); }
void Product::setAvailableStock(int stock)                           { m_availableStock = (stock >= 0 ? stock : 0); }
void Product::setBarcodeFilePath(const std::string& path)           { m_barcodeFilePath = path; }
void Product::setCategory(const std::string& category)               { m_category = category; }
void Product::setDescription(const std::string& description)         { m_description = description; }
void Product::setTaxRate(double taxRate) {
    m_taxAmount = m_price * (taxRate >= 0.0 ? taxRate / 100.0 : 0.0);
}

bool Product::hasSufficientStock(int quantity) const {
    return (quantity > 0 && quantity <= m_availableStock);
}

bool Product::deductStock(int quantity) {
    if (!hasSufficientStock(quantity)) {
        return false;
    }
    m_availableStock -= quantity;
    return true;
}

bool Product::restock(int quantity) {
    if (quantity <= 0) {
        return false;
    }
    m_availableStock += quantity;
    return true;
}

bool Product::isValid() const {
    return (!m_barcode.empty() &&
            !m_productId.empty() &&
            !m_name.empty() &&
            m_price >= 0.0 &&
            m_availableStock >= 0);
}

std::string Product::getFormattedDetails() const {
    std::string out = "Product Specifications:\n";
    out += "----------------------------------------\n";
    out += "  Product ID   : " + m_productId + "\n";
    out += "  Barcode      : " + m_barcode + "\n";
    out += "  Acronym      : " + m_acronym + "\n";
    out += "  Name         : " + m_name + "\n";
    out += "  Unit         : " + m_measurementUnit + "\n";
    out += "  Unit Price   : PKR " + formatDouble(m_price, 2) + "\n";
    out += "  Tax (PKR)    : PKR " + formatDouble(m_taxAmount, 2) + "\n";
    out += "  Discount/Unit: PKR " + formatDouble(m_discountPerUnit, 2) + "\n";
    out += "  Available Qty: " + std::to_string(m_availableStock) + " units\n";
    out += "  Barcode File : " + m_barcodeFilePath + "\n";
    return out;
}

std::string Product::toTableRow() const {
    std::string b   = padRight(m_barcode, 10);
    std::string id  = padRight(m_productId, 10);
    std::string n   = padRight(m_name, 28);
    std::string u   = padRight(m_measurementUnit, 8);
    std::string p   = padLeft(formatDouble(m_price, 2), 9);
    std::string t   = padLeft(formatDouble(m_taxAmount, 2), 8);
    std::string s   = padLeft(std::to_string(m_availableStock), 7);
    return b + " | " + id + " | " + n + " | " + u + " | " + p + " | " + t + " | " + s;
}

std::string Product::toFileRecord() const {
    return m_productId + " " +
           m_barcode + " " +
           m_acronym + " " +
           m_name + " " +
           m_measurementUnit + " " +
           formatDouble(m_price, 2) + " " +
           formatDouble(m_taxAmount, 2) + " " +
           formatDouble(m_discountPerUnit, 2) + " " +
           std::to_string(m_availableStock) + " " +
           m_barcodeFilePath + " #";
}

std::string Product::toDelimitedString(char delimiter) const {
    if (delimiter == ' ') {
        return toFileRecord();
    }
    std::string delim(1, delimiter);
    return m_barcode + delim +
           m_productId + delim +
           m_name + delim +
           (m_category.empty() ? m_measurementUnit : m_category) + delim +
           formatDouble(m_price, 2) + delim +
           std::to_string(m_availableStock) + delim +
           formatDouble(getTaxRate(), 2) + delim +
           m_description;
}

Product Product::fromDelimitedString(const std::string& line,
                                     char /*delimiter*/,
                                     std::string* errorMsg)
{
    std::string trimmedLine = trim(line);
    if (trimmedLine.empty()) {
        if (errorMsg) *errorMsg = "Empty line.";
        return Product();
    }

    // Comment lines starting with '#' without '# ' or header
    if (trimmedLine[0] == '#' && trimmedLine.back() != '#') {
        if (errorMsg) *errorMsg = "Comment line.";
        return Product();
    }
    if (trimmedLine.rfind("//", 0) == 0) {
        if (errorMsg) *errorMsg = "Comment line.";
        return Product();
    }

    // Check for required '#' end marker
    if (trimmedLine.back() == '#') {
        std::string content = trim(trimmedLine.substr(0, trimmedLine.length() - 1));
        if (content.empty()) {
            if (errorMsg) *errorMsg = "Record contains only '#' marker.";
            return Product();
        }

        // Tokenize by whitespace
        DynamicArray<std::string> tokens;
        size_t pos = 0;
        while (pos < content.length()) {
            while (pos < content.length() && isWhitespaceChar(content[pos])) {
                ++pos;
            }
            if (pos >= content.length()) break;
            size_t tokenStart = pos;
            while (pos < content.length() && !isWhitespaceChar(content[pos])) {
                ++pos;
            }
            tokens.push_back(content.substr(tokenStart, pos - tokenStart));
        }

        if (tokens.size() < 10) {
            if (errorMsg) {
                *errorMsg = "Insufficient tokens in record (expected at least 10 before '#'): " + trimmedLine;
            }
            return Product();
        }

        std::string productId = tokens[0];
        std::string barcode   = tokens[1];
        std::string acronym   = tokens[2];

        // Search for unit index: token[i] followed by price, tax, discount, quantity
        int unitIdx = -1;
        for (int i = 3; i <= static_cast<int>(tokens.size()) - 6; ++i) {
            try {
                double testPrice = std::stod(tokens[i + 1]);
                double testTax   = std::stod(tokens[i + 2]);
                double testDisc  = std::stod(tokens[i + 3]);
                int testQty      = std::stoi(tokens[i + 4]);
                if (testPrice >= 0.0 && testTax >= 0.0 && testDisc >= 0.0 && testQty >= 0) {
                    unitIdx = i;
                    break;
                }
            } catch (...) {
                continue;
            }
        }

        if (unitIdx < 3) {
            if (errorMsg) {
                *errorMsg = "Failed to parse product name and numeric fields: " + trimmedLine;
            }
            return Product();
        }

        std::string name = tokens[3];
        for (int i = 4; i < unitIdx; ++i) {
            name += " " + tokens[i];
        }

        std::string measurementUnit = tokens[unitIdx];
        try {
            double price = std::stod(tokens[unitIdx + 1]);
            double taxAmount = std::stod(tokens[unitIdx + 2]);
            double discount = std::stod(tokens[unitIdx + 3]);
            int quantity = std::stoi(tokens[unitIdx + 4]);

            std::string barcodePath = tokens[unitIdx + 5];
            for (size_t i = unitIdx + 6; i < tokens.size(); ++i) {
                barcodePath += " " + tokens[i];
            }

            if (errorMsg) *errorMsg = "";
            return Product(productId, barcode, acronym, name, measurementUnit,
                           price, taxAmount, discount, quantity, barcodePath);
        } catch (...) {
            if (errorMsg) *errorMsg = "Failed to parse numeric fields in record: " + trimmedLine;
            return Product();
        }
    }

    // Backward-compatible fallback for pipe-separated format
    if (trimmedLine.find('|') != std::string::npos) {
        DynamicArray<std::string> tokens = splitString(trimmedLine, '|');
        if (tokens.size() >= 6) {
            try {
                std::string barcode   = tokens[0];
                std::string productId = tokens[1];
                std::string name      = tokens[2];
                std::string category  = tokens[3];
                double price = std::stod(tokens[4]);
                int stock    = std::stoi(tokens[5]);
                double taxRate = (tokens.size() >= 7 && !tokens[6].empty()) ? std::stod(tokens[6]) : 0.0;
                std::string description = (tokens.size() >= 8) ? tokens[7] : name;
                double taxAmount = price * (taxRate / 100.0);
                std::string unit = (category == "Bakery" || category == "Pantry") ? "weight" : "count";
                std::string acronym = (productId.length() <= 8 ? productId : productId.substr(0, 8));
                std::string path = "barcodes/barcode_" + barcode + ".png";
                if (errorMsg) *errorMsg = "";
                return Product(productId, barcode, acronym, name, unit, price, taxAmount, 0.0, stock, path, category, description);
            } catch (...) {
                if (errorMsg) *errorMsg = "Malformed numeric field in pipe-delimited record: " + trimmedLine;
                return Product();
            }
        }
    }

    if (errorMsg) {
        *errorMsg = "Record does not match required format (missing '#' end marker): " + trimmedLine;
    }
    return Product();
}

} // namespace ProductBilling
