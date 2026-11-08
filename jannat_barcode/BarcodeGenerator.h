#pragma once
// BarcodeGenerator.hpp
// -------------------------------------------------------
// Responsible for:
//   1. Validating a user-entered 6-digit code.
//   2. Encoding that code as a Code 39 barcode pattern.
//
// Code 39 basics:
//   - Every character maps to 9 elements (5 bars + 4 spaces).
//   - Each element is either NARROW (1 unit wide) or WIDE (3 units wide).
//   - Bars are drawn in black, spaces are drawn in white.
//   - A barcode always starts and ends with the special '*' character.
//   - Neighbouring characters are separated by a narrow inter-character gap.
// -------------------------------------------------------

#include <string>
#include <vector>

// -------------------------------------------------------
// ValidationResult
// Holds the outcome of input validation.
// -------------------------------------------------------
struct ValidationResult {
    bool  valid;    // true  → input is acceptable
    std::string message; // human-readable feedback for the user
};

// -------------------------------------------------------
// BarElement
// A single bar or space in the barcode strip.
// -------------------------------------------------------
struct BarElement {
    bool isBar;   // true = black bar, false = white space
    int  width;   // 1 = narrow, 3 = wide  (in logical units)
};

// -------------------------------------------------------
// BarcodeGenerator
// Pure C++ class — no GUI code here at all.
// -------------------------------------------------------
class BarcodeGenerator {
public:
    // ---- Validation ----

    // Returns a ValidationResult explaining whether `input` is a
    // valid 6-digit numeric code.
    static ValidationResult validate(const std::string& input);

    // ---- Encoding ----

    // Takes a validated 6-digit string and returns the full sequence
    // of BarElements that represent the Code 39 barcode, including
    // the mandatory '*' start and stop characters.
    static std::vector<BarElement> encode(const std::string& sixDigitCode);

private:
    // Returns the 9-element Code 39 pattern for a single character.
    // The pattern is a string of 9 chars: 'N' (narrow) or 'W' (wide).
    // Odd indices (0,2,4,6,8) are bars; even indices (1,3,5,7) are spaces.
    static std::string getPattern(char c);
};
