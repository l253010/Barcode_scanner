#pragma once
#include "Bill.h"
#include <string>
#include <vector>

namespace billing {

/**
 * Simple representation of a seasonal discount period.
 * The file `Seasons.txt` should contain one line per season:
 *   startDate,endDate,discountPercent,active
 * where dates are in ISO format (YYYY-MM-DD) and `active` is 0 or 1.
 */
struct Season {
    std::string startDate;   // e.g. "2024-11-01"
    std::string endDate;     // e.g. "2024-11-30"
    double discountPercent;  // e.g. 5.0 for 5 %
    bool active;             // true if the season is currently enabled
};

/**
 * DiscountEngine – applies the three kinds of discounts described in
 * issue #46:
 *   1️⃣ Product‑level discounts (already stored per‑item in `BillItem`).
 *   2️⃣ A special manager‑controlled discount (max 8 %).
 *   3️⃣ Seasonal discounts read from `Seasons.txt`.
 *
 * The engine does **not** write the final bill – it only mutates the
 * `Bill` object (sets `seasonalDiscount`, `specialDiscount` and updates
 * the totals via `Bill::computeTotals()`).
 */
class DiscountEngine {
public:
    /**
     * Apply a manager‑provided special discount (percentage of the
     * subtotal after product discounts). The discount is capped at 8 %.
     */
    static void applySpecialDiscount(Bill &bill, double specialPercent);

    /**
     * Load all seasons from `Seasons.txt` (located next to the executable).
     */
    static std::vector<Season> loadSeasons();

    /**
     * Determine the seasonal discount that applies for the given
     * timestamp (seconds since epoch). Returns the discount percent
     * (0.0 if no active season matches).
     */
    static double seasonalDiscountFor(const std::vector<Season> &seasons,
                                      std::time_t timestamp);

    /**
     * Apply the seasonal discount to the bill (adds it to `Bill::seasonalDiscount`
     * and recomputes totals). The discount is a *percentage* of the subtotal
     * after product discounts.
     */
    static void applySeasonalDiscount(Bill &bill, const std::vector<Season> &seasons);
};

} // namespace billing
