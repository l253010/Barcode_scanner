#pragma once
#include <string>

namespace billing {

/**
 * Utility class that generates a unique 9‑digit Bill ID.
 * The implementation simply increments a counter persisted in a
 * small text file (BillIdCounter.txt) located next to Bill.txt.
 * If the file does not exist, it starts at 100000000.
 */
class BillIdGenerator {
public:
    /**
     * Returns the next unique 9‑digit ID as a string.
     * Thread‑safe for single‑process usage (mutex protects the file).
     */
    static std::string nextId();
};

} // namespace billing
