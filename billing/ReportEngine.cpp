#include "ReportEngine.h"
#include "Bill.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <map>
#include <iomanip>

namespace billing {

static BillRecord parseBillLine(const std::string &line) {
    BillRecord rec{};
    std::istringstream ss(line);
    std::string token;
    // Expect 12 fields separated by '|'
    std::getline(ss, rec.billId, '|');
    std::getline(ss, rec.customerName, '|');
    std::getline(ss, rec.orderId, '|');
    std::getline(ss, token, '|'); rec.totalWithoutTax = std::stod(token);
    std::getline(ss, token, '|'); rec.totalTax = std::stod(token);
    std::getline(ss, token, '|'); rec.totalProductDiscounts = std::stod(token);
    std::getline(ss, token, '|'); rec.seasonalDiscount = std::stod(token);
    std::getline(ss, token, '|'); rec.specialDiscount = std::stod(token);
    std::getline(ss, token, '|'); rec.totalPayable = std::stod(token);
    std::getline(ss, token, '|'); rec.amountReceived = std::stod(token);
    std::getline(ss, token, '|'); rec.balanceReturned = std::stod(token);
    std::getline(ss, rec.paymentStatus, '|');
    std::getline(ss, token, '|'); rec.timestamp = static_cast<std::time_t>(std::stoll(token));
    return rec;
}

std::vector<BillRecord> ReportEngine::loadBills(const std::string &path) {
    std::vector<BillRecord> records;
    std::ifstream in(path);
    if (!in) return records;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        records.push_back(parseBillLine(line));
    }
    return records;
}

std::vector<std::pair<std::string, int>> ReportEngine::loadBillDetails(const std::string &path) {
    std::vector<std::pair<std::string, int>> details;
    std::ifstream in(path);
    if (!in) return details;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        // Expected: BillID|productCode|unitPrice|quantity|discountPercent
        std::istringstream ss(line);
        std::string billId, productCode, unitPriceStr, quantityStr, discountStr;
        std::getline(ss, billId, '|');
        std::getline(ss, productCode, '|');
        std::getline(ss, unitPriceStr, '|');
        std::getline(ss, quantityStr, '|');
        std::getline(ss, discountStr, '|');
        int qty = std::stoi(quantityStr);
        details.emplace_back(productCode, qty);
    }
    return details;
}

static std::string formatMoney(double v) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << v;
    return oss.str();
}

std::string ReportEngine::generateReport(std::time_t start, std::time_t end) {
    auto bills = loadBills();
    double cashSales = 0.0;
    double totalDiscounts = 0.0;
    double totalTax = 0.0;

    for (const auto &b : bills) {
        if (b.timestamp >= start && b.timestamp <= end) {
            cashSales += b.totalPayable;
            totalDiscounts += b.totalProductDiscounts + b.seasonalDiscount + b.specialDiscount;
            totalTax += b.totalTax;
        }
    }

    std::ostringstream out;
    out << "=== Cash Sales Report ===\n";
    out << "Period: " << std::asctime(std::localtime(&start))
        << "  to  " << std::asctime(std::localtime(&end));
    out << "Total cash sales (payable): $" << formatMoney(cashSales) << "\n";
    out << "Total tax collected: $" << formatMoney(totalTax) << "\n";
    out << "Total discounts applied: $" << formatMoney(totalDiscounts) << "\n";
    out << "Number of bills: " << std::count_if(bills.begin(), bills.end(), [&](const BillRecord &b){
        return b.timestamp >= start && b.timestamp <= end; }) << "\n";
    return out.str();
}

std::string ReportEngine::dailyReport(const std::tm &date) {
    std::tm start = date;
    start.tm_hour = start.tm_min = start.tm_sec = 0;
    std::tm end = start;
    end.tm_hour = 23; end.tm_min = 59; end.tm_sec = 59;
    return generateReport(std::mktime(&start), std::mktime(&end));
}

static std::tm mondayOfWeek(const std::tm &any) {
    std::tm copy = any;
    // Compute offset to Monday (C++ tm: Sunday==0)
    int wday = copy.tm_wday; // 0=Sun,1=Mon,...
    int offset = (wday == 0) ? -6 : 1 - wday; // move back to Monday
    copy.tm_mday += offset;
    std::mktime(&copy); // normalise
    copy.tm_hour = copy.tm_min = copy.tm_sec = 0;
    return copy;
}

std::string ReportEngine::weeklyReport(const std::tm &date) {
    std::tm monday = mondayOfWeek(date);
    std::tm sunday = monday;
    sunday.tm_mday += 6;
    std::mktime(&sunday);
    sunday.tm_hour = 23; sunday.tm_min = 59; sunday.tm_sec = 59;
    return generateReport(std::mktime(&monday), std::mktime(&sunday));
}

std::string ReportEngine::monthlyReport(int year, int month) {
    std::tm start = {};
    start.tm_year = year - 1900;
    start.tm_mon = month - 1; // 0‑based
    start.tm_mday = 1;
    start.tm_hour = start.tm_min = start.tm_sec = 0;
    std::tm end = start;
    end.tm_mon += 1; // next month
    std::mktime(&end);
    end.tm_mday -= 1; // last day of original month
    end.tm_hour = 23; end.tm_min = 59; end.tm_sec = 59;
    return generateReport(std::mktime(&start), std::mktime(&end));
}

std::string ReportEngine::yearlyReport(int year) {
    std::tm start = {};
    start.tm_year = year - 1900;
    start.tm_mon = 0; // Jan
    start.tm_mday = 1;
    start.tm_hour = start.tm_min = start.tm_sec = 0;
    std::tm end = start;
    end.tm_year += 1; // start of next year
    std::mktime(&end);
    end.tm_mday -= 1; // last day of the year
    end.tm_hour = 23; end.tm_min = 59; end.tm_sec = 59;
    return generateReport(std::mktime(&start), std::mktime(&end));
}

std::string ReportEngine::customReport(std::time_t start, std::time_t end) {
    return generateReport(start, end);
}

std::string ReportEngine::top10SoldProducts() {
    auto details = loadBillDetails();
    std::map<std::string, int> qtyMap;
    for (const auto &p : details) {
        qtyMap[p.first] += p.second;
    }
    // Create a vector of pairs and sort descending by quantity
    std::vector<std::pair<std::string, int>> vec(qtyMap.begin(), qtyMap.end());
    std::sort(vec.begin(), vec.end(), [](auto &a, auto &b){ return a.second > b.second; });
    std::ostringstream out;
    out << "=== Top 10 Most‑Sold Products (by quantity) ===\n";
    for (size_t i = 0; i < std::min<size_t>(10, vec.size()); ++i) {
        out << i+1 << ". " << vec[i].first << " – " << vec[i].second << " units\n";
    }
    return out.str();
}

static int parseStockFromProductLine(const std::string &line) {
    // Split on whitespace; the token before the "barcodes/..." part is assumed to be stock.
    std::istringstream ss(line);
    std::vector<std::string> tokens;
    std::string tok;
    while (ss >> tok) tokens.push_back(tok);
    if (tokens.size() < 2) return 0;
    // Find token that ends with ".png" – the preceding token is the stock number.
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i].find(".png") != std::string::npos && i > 0) {
            try {
                return std::stoi(tokens[i-1]);
            } catch (...) { return 0; }
        }
    }
    return 0;
}

std::string ReportEngine::top10StockProducts() {
    std::ifstream in("product_module/Product.txt");
    if (!in) return "Unable to open product_module/Product.txt\n";
    std::map<std::string, int> stockMap; // productCode -> stock
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        // Expected format: CODE ... STOCK barcodes/... #
        std::istringstream ss(line);
        std::string code; ss >> code; // first token is product code (e.g., PRD-101)
        int stock = parseStockFromProductLine(line);
        stockMap[code] = stock;
    }
    // Sort descending by stock
    std::vector<std::pair<std::string, int>> vec(stockMap.begin(), stockMap.end());
    std::sort(vec.begin(), vec.end(), [](auto &a, auto &b){ return a.second > b.second; });
    std::ostringstream out;
    out << "=== Top 10 Products by Remaining Stock ===\n";
    for (size_t i = 0; i < std::min<size_t>(10, vec.size()); ++i) {
        out << i+1 << ". " << vec[i].first << " – " << vec[i].second << " units\n";
    }
    return out.str();
}

} // namespace billing
