#include "BillIdGenerator.h"
#include <fstream>
#include <mutex>
#include <iomanip>
#include <sstream>

namespace billing {
static std::mutex mtx;
static const std::string counterFile = "BillIdCounter.txt";

std::string BillIdGenerator::nextId() {
    std::lock_guard<std::mutex> lock(mtx);
    long long counter = 100000000; // start at 9‑digit minimum
    // Try to read existing counter
    std::ifstream in(counterFile);
    if (in) {
        in >> counter;
        in.close();
    }
    // Increment and write back
    ++counter;
    std::ofstream out(counterFile, std::ios::trunc);
    out << counter;
    out.close();
    // Format as 9‑digit string with leading zeros if needed
    std::ostringstream oss;
    oss << std::setw(9) << std::setfill('0') << counter;
    return oss.str();
}
} // namespace billing
