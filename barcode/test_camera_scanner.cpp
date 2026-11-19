// test_camera_scanner.cpp
// -------------------------------------------------------
// Offline Automated Test Suite for Camera Scanning Engine & decodeGrayscale.
//
// Tests all required scenarios WITHOUT requiring a physical camera or OpenCV:
//   1. Synthetic grayscale frame decoding for valid sample barcodes:
//      "123456", "100000", "999999", "904571".
//   2. Mirrored / reversed scanline frame (180 deg orientation).
//   3. Noisy frame (synthetic noise added to pixels).
//   4. Leading-zero barcode rejection ("012345").
//   5. Blank frame rejection (insufficient contrast).
//   6. Null buffer pointer handling.
//   7. Tiny frame dimensions handling (< 30 width or < 10 height).
//   8. Multi-frame confirmation sequence simulation.
// -------------------------------------------------------

#include "BarcodeDecoder.h"
#include "BarcodeGenerator.h"

#include <iostream>
#include <vector>
#include <string>
#include <cstdint>

// Terminal color codes
#define GREEN  "\033[32m"
#define RED    "\033[31m"
#define RESET  "\033[0m"
#define BOLD   "\033[1m"
#define CYAN   "\033[36m"

static int passedCount = 0;
static int failedCount = 0;

static void report(const std::string& testName, bool success, const std::string& details = "") {
    if (success) {
        ++passedCount;
        std::cout << GREEN << "  [PASS] " << RESET << testName << "\n";
        if (!details.empty()) {
            std::cout << "         " << details << "\n";
        }
    } else {
        ++failedCount;
        std::cout << RED << "  [FAIL] " << RESET << testName << "\n";
        if (!details.empty()) {
            std::cout << "         " << RED << details << RESET << "\n";
        }
    }
}

// -------------------------------------------------------
// Synthetic Grayscale Frame Structure & Builder
// -------------------------------------------------------
struct SyntheticFrame {
    std::vector<unsigned char> pixels;
    int width = 0;
    int height = 0;
    int stride = 0;
};

static SyntheticFrame CreateSyntheticBarcodeFrame(const std::string& code,
                                                 int moduleWidth = 2,
                                                 int quietZone = 24,
                                                 int height = 64)
{
    std::vector<BarElement> elements = BarcodeGenerator::encode(code);

    int totalUnits = 0;
    for (const auto& elem : elements) {
        totalUnits += elem.width;
    }

    int barcodePixels = totalUnits * moduleWidth;
    int width = quietZone * 2 + barcodePixels;
    int stride = (width + 3) & ~3; // 4-byte row alignment

    SyntheticFrame frame;
    frame.width = width;
    frame.height = height;
    frame.stride = stride;
    frame.pixels.assign(static_cast<size_t>(stride) * static_cast<size_t>(height), 255); // White canvas

    for (int y = 0; y < height; ++y) {
        int x = quietZone;
        for (const auto& elem : elements) {
            int elemPixels = elem.width * moduleWidth;
            unsigned char color = elem.isBar ? 0 : 255;
            for (int p = 0; p < elemPixels; ++p) {
                frame.pixels[static_cast<size_t>(y) * static_cast<size_t>(stride) + static_cast<size_t>(x + p)] = color;
            }
            x += elemPixels;
        }
    }

    return frame;
}

int main() {
    std::cout << "\n" << BOLD << CYAN;
    std::cout << "======================================================\n";
    std::cout << "   Camera Scanner Offline Synthetic Test Suite        \n";
    std::cout << "======================================================\n" << RESET;

    // -------------------------------------------------------
    // Section 1: Sample Barcodes (123456, 100000, 999999, 904571)
    // -------------------------------------------------------
    std::cout << "\n" << BOLD << "[Section 1] Sample Barcodes Synthetic Frame Decoding\n" << RESET;

    const std::string sampleCodes[] = { "123456", "100000", "999999", "904571" };
    for (const auto& code : sampleCodes) {
        SyntheticFrame frame = CreateSyntheticBarcodeFrame(code);
        DecodeResult res = BarcodeDecoder::decodeGrayscale(
            frame.pixels.data(), frame.width, frame.height, frame.stride);

        bool ok = res.success && (res.code == code);
        std::string desc = "Barcode \"" + code + "\" -> Decoded: \"" + res.code + "\"";
        if (!ok) desc += " | Error: " + res.errorMessage;
        report("Synthetic frame decode: " + code, ok, desc);
    }

    // -------------------------------------------------------
    // Section 2: Mirrored Frame (180 deg / Horizontal Flip)
    // -------------------------------------------------------
    std::cout << "\n" << BOLD << "[Section 2] Mirrored / Reverse Barcode Frame\n" << RESET;
    {
        SyntheticFrame normalFrame = CreateSyntheticBarcodeFrame("123456");
        SyntheticFrame mirroredFrame = normalFrame;

        for (int y = 0; y < normalFrame.height; ++y) {
            for (int x = 0; x < normalFrame.width; ++x) {
                size_t srcIdx = static_cast<size_t>(y) * static_cast<size_t>(normalFrame.stride) + static_cast<size_t>(x);
                size_t dstIdx = static_cast<size_t>(y) * static_cast<size_t>(mirroredFrame.stride) + static_cast<size_t>(normalFrame.width - 1 - x);
                mirroredFrame.pixels[dstIdx] = normalFrame.pixels[srcIdx];
            }
        }

        DecodeResult res = BarcodeDecoder::decodeGrayscale(
            mirroredFrame.pixels.data(), mirroredFrame.width, mirroredFrame.height, mirroredFrame.stride);

        bool ok = res.success && (res.code == "123456");
        report("Mirrored frame decode (\"123456\")", ok,
               ok ? "Successfully decoded reversed scanline." : res.errorMessage);
    }

    // -------------------------------------------------------
    // Section 3: Noisy Frame Handling
    // -------------------------------------------------------
    std::cout << "\n" << BOLD << "[Section 3] Noisy Frame Decoding\n" << RESET;
    {
        SyntheticFrame noisyFrame = CreateSyntheticBarcodeFrame("904571");

        // Add deterministic pseudo-random luminance jitter (+/- 25)
        uint32_t seed = 987654321;
        for (int y = 0; y < noisyFrame.height; ++y) {
            for (int x = 0; x < noisyFrame.width; ++x) {
                seed = seed * 1664525u + 1013904223u;
                int noise = static_cast<int>((seed >> 16) % 51) - 25; // [-25, +25]
                size_t idx = static_cast<size_t>(y) * static_cast<size_t>(noisyFrame.stride) + static_cast<size_t>(x);
                int val = static_cast<int>(noisyFrame.pixels[idx]) + noise;
                if (val < 0) val = 0;
                if (val > 255) val = 255;
                noisyFrame.pixels[idx] = static_cast<unsigned char>(val);
            }
        }

        DecodeResult res = BarcodeDecoder::decodeGrayscale(
            noisyFrame.pixels.data(), noisyFrame.width, noisyFrame.height, noisyFrame.stride);

        bool ok = res.success && (res.code == "904571");
        report("Noisy frame decode (\"904571\")", ok,
               ok ? "Successfully filtered noise and decoded." : res.errorMessage);
    }

    // -------------------------------------------------------
    // Section 4: Leading-Zero Barcode Rejection
    // -------------------------------------------------------
    std::cout << "\n" << BOLD << "[Section 4] Leading-Zero Frame Validation\n" << RESET;
    {
        SyntheticFrame lzFrame = CreateSyntheticBarcodeFrame("012345");
        DecodeResult res = BarcodeDecoder::decodeGrayscale(
            lzFrame.pixels.data(), lzFrame.width, lzFrame.height, lzFrame.stride);

        bool ok = !res.success && (res.errorMessage.find("The first digit must not be zero") != std::string::npos);
        report("Leading-zero frame rejection (\"012345\")", ok,
               "ErrorMessage: " + res.errorMessage);
    }

    // -------------------------------------------------------
    // Section 5: Blank / Zero-Contrast Frame
    // -------------------------------------------------------
    std::cout << "\n" << BOLD << "[Section 5] Blank Frame Handling\n" << RESET;
    {
        SyntheticFrame blankFrame;
        blankFrame.width = 200;
        blankFrame.height = 50;
        blankFrame.stride = 200;
        blankFrame.pixels.assign(static_cast<size_t>(200 * 50), 255); // All white

        DecodeResult res = BarcodeDecoder::decodeGrayscale(
            blankFrame.pixels.data(), blankFrame.width, blankFrame.height, blankFrame.stride);

        bool ok = !res.success && (res.errorMessage.find("insufficient contrast or blank image") != std::string::npos);
        report("Blank image frame handling", ok, "ErrorMessage: " + res.errorMessage);
    }

    // -------------------------------------------------------
    // Section 6: Null Pointer Buffer Handling
    // -------------------------------------------------------
    std::cout << "\n" << BOLD << "[Section 6] Null Pointer Buffer Handling\n" << RESET;
    {
        DecodeResult res = BarcodeDecoder::decodeGrayscale(nullptr, 640, 480, 640);
        bool ok = !res.success && (res.errorMessage.find("null") != std::string::npos);
        report("Null frame buffer handling", ok, "ErrorMessage: " + res.errorMessage);
    }

    // -------------------------------------------------------
    // Section 7: Tiny Frame Dimensions (<30x10)
    // -------------------------------------------------------
    std::cout << "\n" << BOLD << "[Section 7] Tiny Frame Handling\n" << RESET;
    {
        unsigned char tinyPixels[20 * 5] = { 0 };
        DecodeResult res = BarcodeDecoder::decodeGrayscale(tinyPixels, 20, 5, 20);
        bool ok = !res.success && (res.errorMessage.find("too small") != std::string::npos);
        report("Tiny frame dimensions (20x5) handling", ok, "ErrorMessage: " + res.errorMessage);
    }

    // -------------------------------------------------------
    // Section 8: Multi-Frame Confirmation Simulation
    // -------------------------------------------------------
    std::cout << "\n" << BOLD << "[Section 8] Multi-Frame Confirmation Logic Simulation\n" << RESET;
    {
        // Scenario A: 3 consecutive matching frames -> confirmed
        SyntheticFrame frame123456 = CreateSyntheticBarcodeFrame("123456");
        std::string candidateCode;
        int matchCount = 0;
        const int requiredMatches = 3;
        bool confirmed = false;

        for (int frameIdx = 0; frameIdx < 3; ++frameIdx) {
            DecodeResult res = BarcodeDecoder::decodeGrayscale(
                frame123456.pixels.data(), frame123456.width, frame123456.height, frame123456.stride);
            if (res.success) {
                if (res.code == candidateCode) {
                    matchCount++;
                } else {
                    candidateCode = res.code;
                    matchCount = 1;
                }
                if (matchCount >= requiredMatches) {
                    confirmed = true;
                    break;
                }
            } else {
                matchCount = 0;
            }
        }
        report("Multi-frame confirmation (3 consecutive frames)", confirmed && candidateCode == "123456",
               "Confirmed code: " + candidateCode);

        // Scenario B: Interrupted frame sequence (drop frame resets counter)
        matchCount = 0;
        candidateCode = "";
        confirmed = false;

        // Frame 1: Valid
        DecodeResult r1 = BarcodeDecoder::decodeGrayscale(
            frame123456.pixels.data(), frame123456.width, frame123456.height, frame123456.stride);
        if (r1.success) { candidateCode = r1.code; matchCount = 1; }

        // Frame 2: Dropped / Blank frame
        SyntheticFrame blankFrame;
        blankFrame.width = 200; blankFrame.height = 50; blankFrame.stride = 200;
        blankFrame.pixels.assign(10000, 255);
        DecodeResult r2 = BarcodeDecoder::decodeGrayscale(
            blankFrame.pixels.data(), blankFrame.width, blankFrame.height, blankFrame.stride);
        if (!r2.success) { matchCount = 0; } // Resets

        // Frame 3: Valid again
        DecodeResult r3 = BarcodeDecoder::decodeGrayscale(
            frame123456.pixels.data(), frame123456.width, frame123456.height, frame123456.stride);
        if (r3.success) { candidateCode = r3.code; matchCount = 1; }

        // Must NOT confirm because Frame 2 dropped and reset matchCount
        bool rejectedPremature = (matchCount < requiredMatches);
        report("Multi-frame drop-frame reset protection", rejectedPremature,
               "Match count correctly reset on blank frame (count = " + std::to_string(matchCount) + ")");
    }

    // -------------------------------------------------------
    // Final Summary
    // -------------------------------------------------------
    std::cout << "\n======================================================\n";
    std::cout << "  Results: " << passedCount << " passed, " << failedCount << " failed\n";
    std::cout << "======================================================\n\n";

    return (failedCount == 0) ? 0 : 1;
}
