#include "DiscountEngine.h"
#include "Bill.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>

namespace billing {

static std::time_t dateStringToTimeT(const std::string &dateStr) {
    // Expected format: YYYY-MM-DD
    std::tm tm = {};
    std::istringstream ss(dateStr);
    ss >> std::get_time(&tm, "%Y-%m-%d");
    if (ss.fail()) {
        return 0; // fallback – treat as epoch start (won't match)
    }
    tm.tm_hour = 0; tm.tm_min = 0; tm.tm_sec = 0;
    // Assume local time (Windows default). Convert to time_t.
    return std::mktime(&tm);
}

void DiscountEngine::applySpecialDiscount(Bill &bill, double specialPercent) {
    // Clamp to [0, 8]
    if (specialPercent < 0) specialPercent = 0;
    if (specialPercent > 8) specialPercent = 8;
    // The special discount is applied on the subtotal after product discounts.
    double subtotal = bill.totalWithoutTax - bill.totalProductDiscounts;
    if (subtotal < 0) subtotal = 0;
    bill.specialDiscount = subtotal * (specialPercent / 100.0);
    bill.computeTotals();
}

std::vector<Season> DiscountEngine::loadSeasons() {
    std::vector<Season> seasons;
    std::ifstream in("Seasons.txt");
    if (!in) return seasons; // no file – return empty list
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::istringstream ss(line);
        std::string start, end, discountStr, activeStr;
        if (!std::getline(ss, start, ',')) continue;
        if (!std::getline(ss, end, ',')) continue;
        if (!std::getline(ss, discountStr, ',')) continue;
        if (!std::getline(ss, activeStr, ',')) continue;
        Season s;
        s.startDate = start;
        s.endDate = end;
        try {
            s.discountPercent = std::stod(discountStr);
        } catch (...) { s.discountPercent = 0.0; }
        s.active = (activeStr == "1" || activeStr == "true");
        seasons.push_back(std::move(s));
    }
    return seasons;
}

double DiscountEngine::seasonalDiscountFor(const std::vector<Season> &seasons,
                                          std::time_t timestamp) {
    for (const auto &s : seasons) {
        if (!s.active) continue;
        std::time_t start = dateStringToTimeT(s.startDate);
        std::time_t end   = dateStringToTimeT(s.endDate);
        if (start == 0 || end == 0) continue;
        // inclusive range
        if (timestamp >= start && timestamp <= end) {
            return s.discountPercent;
        }
    }
    return 0.0;
}

void DiscountEngine::applySeasonalDiscount(Bill &bill,
                                          const std::vector<Season> &seasons) {
    double percent = seasonalDiscountFor(seasons, bill.billTimestamp);
    if (percent <= 0.0) {
        bill.seasonalDiscount = 0.0;
        bill.computeTotals();
        return;
    }
    // Seasonal discount is a percentage of the subtotal after product discounts.
    double subtotal = bill.totalWithoutTax - bill.totalProductDiscounts;
    if (subtotal < 0) subtotal = 0;
    bill.seasonalDiscount = subtotal * (percent / 100.0);
    bill.computeTotals();
}

} // namespace billing
