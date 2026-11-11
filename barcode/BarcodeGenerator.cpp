// BarcodeGenerator.cpp
// -------------------------------------------------------
// Implementation of validation and Code 39 encoding.
// -------------------------------------------------------

#include "BarcodeGenerator.h"
#include <cctype>    // std::isdigit
#include <stdexcept> // std::invalid_argument

// -------------------------------------------------------
// Code 39 character table
// -------------------------------------------------------
// Each entry is a 9-character string.
// Position: 0 2 4 6 8 → bars  (N=narrow, W=wide)
//           1 3 5 7   → inter-element spaces (N=narrow, W=wide)
//
// Source: ISO/IEC 16388 Code 39 specification.
// -------------------------------------------------------
//
// Encoding key (positions within the 9-char pattern):
//   Index 0 → 1st bar
//   Index 1 → 1st space
//   Index 2 → 2nd bar
//   Index 3 → 2nd space
//   Index 4 → 3rd bar
//   Index 5 → 3rd space
//   Index 6 → 4th bar
//   Index 7 → 4th space
//   Index 8 → 5th bar
// -------------------------------------------------------

std::string BarcodeGenerator::getPattern(char c) {
    // Patterns from the official Code 39 standard.
    switch (c) {
        // ---- Digits ----
        // All patterns verified against ISO/IEC 16388 Code 39 standard.
        // Each pattern has exactly 3 wide (W) elements — the standard requires this.
        case '0': return "NNNWWNWNN"; // 0  → wides at positions 3,4,6
        case '1': return "WNNWNNNNW"; // 1  → wides at positions 0,3,8
        case '2': return "NNWWNNNNW"; // 2  → wides at positions 2,3,8
        case '3': return "WNWWNNNNN"; // 3  → wides at positions 0,2,3
        case '4': return "NNNWWNNWN"; // 4  → wides at positions 3,4,7
        case '5': return "WNNWWNNNN"; // 5  → wides at positions 0,3,4
        case '6': return "NNWWWNNNN"; // 6  → wides at positions 2,3,4
        case '7': return "NNNWNNWNW"; // 7  → wides at positions 3,6,8
        case '8': return "WNNWNNWNN"; // 8  → wides at positions 0,3,6
        case '9': return "NNWWNNWNN"; // 9  → wides at positions 2,3,6

        // ---- Start / Stop character ----
        case '*': return "NWNNWNWNN"; // *  → wides at positions 1,4,6

        default:
            throw std::invalid_argument(
                std::string("Character '") + c + "' is not supported in Code 39.");
    }
}

// -------------------------------------------------------
// validate()
// -------------------------------------------------------
ValidationResult BarcodeGenerator::validate(const std::string& input) {
    // Rule 1: must not be empty
    if (input.empty()) {
        return { false, "Please enter a 6-digit code." };
    }

    // Rule 2: must be exactly 6 characters long
    if (input.length() != 6) {
        return { false,
            "Code must be exactly 6 digits long. "
            "You entered " + std::to_string(input.length()) + " character(s)." };
    }

    // Rule 3: every character must be a digit 0-9
    for (char ch : input) {
        if (!std::isdigit(static_cast<unsigned char>(ch))) {
            return { false,
                "Only numeric digits (0-9) are allowed. "
                "Found invalid character: '" + std::string(1, ch) + "'." };
        }
    }

    // Rule 4: the first digit must be non-zero
    if (input[0] == '0') {
        return { false,
            "The first digit must not be zero. "
            "Codes like 012345 or 000000 are not allowed." };
    }

    return { true, "Valid input." };
}

// -------------------------------------------------------
// encode()
// -------------------------------------------------------
// Builds the full barcode strip:
//   * (start)  →  digit 1 … digit 6  →  * (stop)
// Between each character there is a narrow inter-character gap (white space, 1 unit).
// -------------------------------------------------------
std::vector<BarElement> BarcodeGenerator::encode(const std::string& sixDigitCode) {
    std::vector<BarElement> elements;

    // Helper: convert a 9-char pattern string into BarElements and append.
    auto appendPattern = [&](char c) {
        std::string pattern = getPattern(c);
        for (int i = 0; i < 9; ++i) {
            bool isBar  = (i % 2 == 0);     // even index → bar, odd → space
            int  width  = (pattern[i] == 'W') ? 3 : 1;
            elements.push_back({ isBar, width });
        }
    };

    // Helper: append the narrow inter-character gap (white space, 1 unit).
    auto appendGap = [&]() {
        elements.push_back({ false, 1 }); // white space
    };

    // Build: *  <gap>  d0 <gap> d1 <gap> … d5 <gap>  *
    appendPattern('*');   // start character

    for (char digit : sixDigitCode) {
        appendGap();          // inter-character gap before each data digit
        appendPattern(digit);
    }

    appendGap();              // inter-character gap before stop character
    appendPattern('*');       // stop character

    return elements;
}
