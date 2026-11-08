// BarcodeDecoder.h
// -------------------------------------------------------
// Header file for Code 39 barcode image decoding.
//
// Responsible for:
//   1. Representing the outcome of a barcode decoding attempt (DecodeResult).
//   2. Declaring the BarcodeDecoder class to load images, detect bars,
//      decode Code 39 patterns, and validate the 6-digit numeric code.
//
// Uses only std::string for the result and path interface (Option A).
// Zero STL containers (<vector> completely removed).
// Zero third-party or external barcode libraries.
// -------------------------------------------------------

#pragma once

#include <string>

// -------------------------------------------------------
// DecodeResult
// Holds the final outcome of an image decoding operation.
// -------------------------------------------------------
struct DecodeResult {
    bool        success;       // true if a valid 6-digit barcode was decoded
    std::string code;          // the decoded 6-digit string (e.g., "123456")
    std::string errorMessage;  // user-friendly explanation when success == false
};

// -------------------------------------------------------
// BarcodeDecoder
// Class providing static methods to decode Code 39 barcodes.
// -------------------------------------------------------
class BarcodeDecoder {
public:
    // ---- Primary Public API ----

    // Loads a barcode image from disk (PNG, BMP, JPG),
    // detects the barcode, decodes the Code 39 elements, and
    // validates the resulting 6-digit numeric code.
    static DecodeResult decodeImage(const std::string& imagePath);

    // ---- Helper API ----

    // Decodes a single 9-character Code 39 pattern string (e.g., "WNNWNNNNW" -> '1').
    // Returns the decoded character, or '?' if the pattern is unknown.
    static char decodePattern(const char* pattern9);
    static char decodePattern(const std::string& pattern9);

    // Decodes an array of character patterns: '*' + 6 digits + '*'.
    // Accepts a standard C array of pattern string pointers (count = 8).
    static DecodeResult decodePatterns(const char* patterns[], int count);
    static DecodeResult decodePatterns(const std::string patterns[], int count);
};
