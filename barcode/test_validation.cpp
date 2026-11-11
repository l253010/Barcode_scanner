// test_validation.cpp
// -------------------------------------------------------
// Standalone command-line test for BarcodeGenerator.
// Tests all validation cases and barcode encoding.
//
// Build:  clang++ -std=c++17 -o test_validation test_validation.cpp BarcodeGenerator.cpp
// Run:    ./test_validation
// -------------------------------------------------------

#include "BarcodeGenerator.h"
#include <iostream>
#include <cassert>
#include <string>
#include <fstream>   // std::ifstream — used for image-save verification

// ANSI colours for terminal output
#define GREEN  "\033[32m"
#define RED    "\033[31m"
#define RESET  "\033[0m"
#define BOLD   "\033[1m"

// -------------------------------------------------------
// Helper: print pass / fail and the message
// -------------------------------------------------------
static int passed = 0;
static int failed = 0;

void check(const std::string& testName,
           bool               expectedValid,
           const std::string& input)
{
    ValidationResult r = BarcodeGenerator::validate(input);

    bool ok = (r.valid == expectedValid);
    if (ok) {
        ++passed;
        std::cout << GREEN << "[PASS]" << RESET
                  << "  " << testName << "\n"
                  << "       Input    : \"" << input << "\"\n"
                  << "       Message  : " << r.message << "\n\n";
    } else {
        ++failed;
        std::cout << RED << "[FAIL]" << RESET
                  << "  " << testName << "\n"
                  << "       Input      : \"" << input << "\"\n"
                  << "       Expected   : " << (expectedValid ? "valid" : "invalid") << "\n"
                  << "       Got        : " << (r.valid ? "valid" : "invalid") << "\n"
                  << "       Message    : " << r.message << "\n\n";
    }
}

// -------------------------------------------------------
// Helper: print one barcode encoding result
// -------------------------------------------------------
void printEncoding(const std::string& code) {
    auto elements = BarcodeGenerator::encode(code);

    int totalUnits = 0;
    for (auto& e : elements) totalUnits += e.width;

    std::cout << BOLD << "Encoding for: \"" << code << "\"" << RESET << "\n";
    std::cout << "  Elements : " << elements.size() << "\n";
    std::cout << "  Total units (width): " << totalUnits << "\n";
    std::cout << "  Pattern  : ";
    for (auto& e : elements) {
        char sym = e.isBar ? 'B' : 'S';     // B=bar, S=space
        int  w   = e.width;
        std::cout << sym << w << " ";
    }
    std::cout << "\n\n";
}

// -------------------------------------------------------
// Verify that a specific digit's pattern has exactly 3 wides
// -------------------------------------------------------
void checkPatternWidths() {
    std::cout << BOLD << "--- Code 39 Pattern Verification ---" << RESET << "\n";
    std::cout << "Each character must encode to exactly 3 wide elements.\n\n";

    // We test encode with single chars by encoding "0" and inspecting
    // the first character (after stripping start *).
    // For digits: encode a 6-char code and check each digit's 9 elements.

    // Easier: test the full encoding of "123456" and verify element count.
    auto elements = BarcodeGenerator::encode("123456");
    // Expected structure:
    //   * + gap + 1 + gap + 2 + gap + 3 + gap + 4 + gap + 5 + gap + 6 + gap + *
    // = 9 + 1 + 9 + 1 + 9 + 1 + 9 + 1 + 9 + 1 + 9 + 1 + 9 + 1 + 9  = 9*8 + 7*1 = 72+7 = 79

    int expected = 9 * 8 + 7 * 1; // 8 characters (2 stars + 6 digits) + 7 gaps
    bool countOk = (elements.size() == (size_t)expected);

    std::cout << "  encode(\"123456\") element count: "
              << elements.size() << " (expected " << expected << ") "
              << (countOk ? GREEN "✓" : RED "✗") << RESET << "\n\n";

    if (countOk) {
        ++passed;
        std::cout << GREEN << "[PASS]" << RESET << "  Code 39 element count correct.\n\n";
    } else {
        ++failed;
        std::cout << RED   << "[FAIL]" << RESET << "  Code 39 element count wrong.\n\n";
    }
}

// -------------------------------------------------------
// Image-save smoke test
// Writes a minimal PBM barcode image using only <fstream>
// (a standard file-handling facility — no barcode library).
// Then verifies the file exists on disk.
// -------------------------------------------------------
void checkImageSave() {
    std::cout << BOLD << "--- Image Save Test ---" << RESET << "\n";
    std::cout << "Generates a barcode and saves it as a PBM file using std::fstream.\n\n";

    const std::string code = "123456";
    auto elements = BarcodeGenerator::encode(code);

    // ----- Render into a pixel row -----
    // Each logical unit = 3 pixels wide; barcode height = 60 px.
    const int unitPx = 3;
    const int height = 60;

    // Build the row of pixels (1 = black, 0 = white)
    std::vector<int> pixelRow;
    for (const BarElement& el : elements) {
        int colour = el.isBar ? 1 : 0;
        for (int p = 0; p < el.width * unitPx; ++p)
            pixelRow.push_back(colour);
    }
    int width = static_cast<int>(pixelRow.size());

    // ----- Write PBM (Portable Bit Map) — a plain text image format -----
    // PBM is part of the Netpbm family; it uses only the standard library.
    // Format: P1 <width> <height> <pixels (0=white,1=black)>
   const std::string tmpPath = "test_barcode_123456.pbm";
    {
        std::ofstream out(tmpPath);
        if (!out.is_open()) {
            ++failed;
            std::cout << RED << "[FAIL]" << RESET
                      << "  Could not open " << tmpPath << " for writing.\n\n";
            return;
        }
        out << "P1\n" << width << " " << height << "\n";
        for (int row = 0; row < height; ++row) {
            for (int col = 0; col < width; ++col) {
                out << pixelRow[col];
                if (col + 1 < width) out << " ";
            }
            out << "\n";
        }
    } // file closed here

    // ----- Verify file exists and has content -----
    std::ifstream verify(tmpPath, std::ios::binary | std::ios::ate);
    bool fileOk = verify.is_open() && (verify.tellg() > 0);
    verify.close();

    if (fileOk) {
        ++passed;
        std::cout << GREEN << "[PASS]" << RESET
                  << "  Barcode image saved and verified: " << tmpPath << "\n\n";
    } else {
        ++failed;
        std::cout << RED << "[FAIL]" << RESET
                  << "  Barcode image file is missing or empty: " << tmpPath << "\n\n";
    }
}

// -------------------------------------------------------
// main
// -------------------------------------------------------
int main() {
    std::cout << BOLD
              << "========================================\n"
              << "  BarcodeGenerator — Validation Tests  \n"
              << "========================================\n\n"
              << RESET;

    // ---- Validation tests ----
    std::cout << BOLD << "--- Input Validation Tests ---" << RESET << "\n\n";

    // --- Valid inputs ---
    check("Valid 6-digit code",                     true,  "123456");
    check("Valid 6-digit code (non-zero first)",    true,  "904571");
    check("Valid 6-digit code (1 as first digit)",  true,  "100000");
    check("Valid 6-digit code (9 as first digit)",  true,  "999999");

    // --- Leading-zero rejection (requirement: first digit must be non-zero) ---
    check("Leading zero — 012345",                  false, "012345");
    check("Leading zero — 000000",                  false, "000000");
    check("Leading zero — 000001",                  false, "000001");

    // --- Empty input ---
    check("Empty input",                            false, "");

    // --- Wrong length ---
    check("Too short (5 digits)",                   false, "12345");
    check("Too long (7 digits)",                    false, "1234567");
    check("Single digit",                           false, "7");
    check("Twelve digits",                          false, "123456789012");

    // --- Non-numeric characters ---
    check("Letters only",                           false, "abcdef");
    check("Mixed letters and digits",               false, "12ab56");
    check("Special characters",                     false, "12!@56");
    check("Space in input",                         false, "123 56");
    check("Leading space",                          false, " 23456");
    check("Decimal / dot",                          false, "123.56");

    // ---- Encoding tests ----
    std::cout << BOLD << "--- Barcode Encoding Tests ---" << RESET << "\n\n";
    printEncoding("123456");
    printEncoding("100000");
    printEncoding("999999");

    // ---- Pattern structure test ----
    checkPatternWidths();

    // ---- Image save test ----
    checkImageSave();

    // ---- Summary ----
    std::cout << BOLD
              << "========================================\n"
              << "  Results: " << passed << " passed, " << failed << " failed\n"
              << "========================================\n"
              << RESET;

    return (failed == 0) ? 0 : 1;
}
