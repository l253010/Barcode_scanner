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
    // Manual character whitespace check without <cctype>
    bool isWhitespaceChar(char c) {
        return c == ' ' || c == '\t' || c == '\r' || c == '\n';
    }

    // Manual string trimming without <cctype>
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

    // Formats a double to fixed decimal representation without <sstream>
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

    // Right-pads a string with padChar up to width
    std::string padRight(const std::string& str, size_t width, char padChar = ' ') {
        if (str.length() >= width) return str;
        return str + std::string(width - str.length(), padChar);
    }

    // Left-pads a string with padChar up to width
    std::string padLeft(const std::string& str, size_t width, char padChar = ' ') {
        if (str.length() >= width) return str;
        return std::string(width - str.length(), padChar) + str;
    }

    // String splitting using DynamicArray and basic string find without <sstream> or <vector>
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
    : m_barcode("")
    , m_productId("")
    , m_name("")
    , m_description("")
    , m_category("")
    , m_price(0.0)
    , m_availableStock(0)
    , m_taxRate(0.0)
{
}

Product::Product(const std::string& barcode,
                 const std::string& productId,
                 const std::string& name,
                 const std::string& description,
                 const std::string& category,
                 double price,
                 int availableStock,
                 double taxRate)
    : m_barcode(trim(barcode))
    , m_productId(trim(productId))
    , m_name(trim(name))
    , m_description(trim(description))
    , m_category(trim(category))
    , m_price(price >= 0.0 ? price : 0.0)
    , m_availableStock(availableStock >= 0 ? availableStock : 0)
    , m_taxRate(taxRate >= 0.0 ? taxRate : 0.0)
{
}

// Getters
const std::string& Product::getBarcode() const        { return m_barcode; }
const std::string& Product::getProductId() const      { return m_productId; }
const std::string& Product::getName() const           { return m_name; }
const std::string& Product::getDescription() const    { return m_description; }
const std::string& Product::getCategory() const       { return m_category; }
double             Product::getPrice() const          { return m_price; }
int                Product::getAvailableStock() const { return m_availableStock; }
double             Product::getTaxRate() const        { return m_taxRate; }
double             Product::getTaxRateFraction() const { return m_taxRate / 100.0; }

// Setters
void Product::setBarcode(const std::string& barcode)        { m_barcode = trim(barcode); }
void Product::setProductId(const std::string& productId)    { m_productId = trim(productId); }
void Product::setName(const std::string& name)              { m_name = trim(name); }
void Product::setDescription(const std::string& description){ m_description = trim(description); }
void Product::setCategory(const std::string& category)      { m_category = trim(category); }
void Product::setPrice(double price)                        { m_price = (price >= 0.0 ? price : 0.0); }
void Product::setAvailableStock(int stock)                  { m_availableStock = (stock >= 0 ? stock : 0); }
void Product::setTaxRate(double taxRate)                    { m_taxRate = (taxRate >= 0.0 ? taxRate : 0.0); }

// Stock Operations
bool Product::hasSufficientStock(int quantity) const {
    if (quantity <= 0) {
        return false;
    }
    return m_availableStock >= quantity;
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
    return (!m_barcode.empty() && !m_name.empty() && m_price >= 0.0 && m_availableStock >= 0);
}

std::string Product::getFormattedDetails() const {
    std::string out;
    out += "----------------------------------------\n";
    out += " Product Information\n";
    out += "----------------------------------------\n";
    out += " Barcode        : " + m_barcode + "\n";
    out += " Product ID     : " + (m_productId.empty() ? "N/A" : m_productId) + "\n";
    out += " Name           : " + m_name + "\n";
    out += " Category       : " + (m_category.empty() ? "General" : m_category) + "\n";
    out += " Unit Price     : $" + formatDouble(m_price, 2) + "\n";
    out += " Available Stock: " + std::to_string(m_availableStock) + " units\n";
    out += " Tax Rate       : " + formatDouble(m_taxRate, 2) + "%\n";
    out += " Description    : " + (m_description.empty() ? "None" : m_description) + "\n";
    out += "----------------------------------------";
    return out;
}

std::string Product::toTableRow() const {
    std::string displayName = m_name;
    if (displayName.length() > 28) {
        displayName = displayName.substr(0, 25) + "...";
    }

    std::string row = padRight(m_barcode, 10) + " | " +
                      padRight(m_productId.empty() ? "N/A" : m_productId, 8) + " | " +
                      padRight(displayName, 28) + " | " +
                      padRight(m_category, 14) + " | $" +
                      padRight(formatDouble(m_price, 2), 7) + " | " +
                      padRight(std::to_string(m_availableStock), 6) + " | " +
                      padRight(formatDouble(m_taxRate, 2) + "%", 6);
    return row;
}

std::string Product::toDelimitedString(char delimiter) const {
    std::string delim(1, delimiter);
    return m_barcode + delim +
           m_productId + delim +
           m_name + delim +
           m_category + delim +
           formatDouble(m_price, 2) + delim +
           std::to_string(m_availableStock) + delim +
           formatDouble(m_taxRate, 2) + delim +
           m_description;
}

Product Product::fromDelimitedString(const std::string& line,
                                     char delimiter,
                                     std::string* errorMsg)
{
    std::string trimmedLine = trim(line);
    if (trimmedLine.empty() || trimmedLine[0] == '#') {
        if (errorMsg) *errorMsg = "Empty line or comment.";
        return Product();
    }

    // Auto-detect delimiter if requested delimiter not found but alternate is present
    char activeDelimiter = delimiter;
    if (trimmedLine.find(delimiter) == std::string::npos) {
        if (trimmedLine.find('|') != std::string::npos) {
            activeDelimiter = '|';
        } else if (trimmedLine.find(',') != std::string::npos) {
            activeDelimiter = ',';
        } else if (trimmedLine.find('\t') != std::string::npos) {
            activeDelimiter = '\t';
        }
    }

    DynamicArray<std::string> tokens = splitString(trimmedLine, activeDelimiter);

    // Expected format:
    // Barcode|ProductID|Name|Category|Price|Stock|TaxRate|Description (8 fields)
    // Or minimum 6 fields: Barcode|ProductID|Name|Category|Price|Stock
    if (tokens.size() < 6) {
        if (errorMsg) {
            *errorMsg = "Insufficient fields in record (found " +
                        std::to_string(tokens.size()) + ", expected >= 6): " + trimmedLine;
        }
        return Product();
    }

    std::string barcode     = tokens[0];
    std::string productId   = tokens[1];
    std::string name        = tokens[2];
    std::string category    = tokens[3];
    double price = 0.0;
    int stock = 0;
    double taxRate = 0.0;
    std::string description = "";

    try {
        price = std::stod(tokens[4]);
    } catch (...) {
        if (errorMsg) *errorMsg = "Invalid price format: " + tokens[4];
        return Product();
    }

    try {
        stock = std::stoi(tokens[5]);
    } catch (...) {
        if (errorMsg) *errorMsg = "Invalid stock format: " + tokens[5];
        return Product();
    }

    if (tokens.size() >= 7 && !tokens[6].empty()) {
        try {
            taxRate = std::stod(tokens[6]);
        } catch (...) {
            taxRate = 0.0;
        }
    }

    if (tokens.size() >= 8) {
        description = tokens[7];
    }

    if (barcode.empty()) {
        if (errorMsg) *errorMsg = "Missing barcode in record.";
        return Product();
    }

    if (price < 0.0) {
        if (errorMsg) *errorMsg = "Negative price is not allowed: " + std::to_string(price);
        return Product();
    }

    if (stock < 0) {
        if (errorMsg) *errorMsg = "Negative stock is not allowed: " + std::to_string(stock);
        return Product();
    }

    if (errorMsg) *errorMsg = "";
    return Product(barcode, productId, name, description, category, price, stock, taxRate);
}

} // namespace ProductBilling
