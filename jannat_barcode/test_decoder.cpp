// test_decoder.cpp
// -------------------------------------------------------
// Automated Test Suite for BarcodeDecoder.
//
// Tests all required scenarios:
//   1. Successful decoding of valid six-digit barcodes.
//   2. Real PNG barcode image decoding.
//   3. Real BMP barcode image decoding.
//   4. Real JPG barcode image decoding.
//   5. Leading-zero rejection in pattern and image ("012345").
//   6. Rotated barcode decoding (180 deg upside-down scan).
//   7. Invalid image handling (empty path, dimensions too small).
//   8. Corrupted barcode image handling (erased bars).
//   9. Unsupported image handling (non-image / text file).
//  10. Appropriate, descriptive error messages for all failure modes.
//
// Standalone execution:
//   cl /nologo /EHsc /std:c++17 /W4 test_decoder.cpp BarcodeDecoder.cpp BarcodeGenerator.cpp /Fe:test_decoder.exe
//   test_decoder.exe
// -------------------------------------------------------

#include "BarcodeDecoder.h"
#include "BarcodeGenerator.h"

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#endif

#include <iostream>
#include <string>

// ANSI terminal colors for clear test status output
#define GREEN  "\033[32m"
#define RED    "\033[31m"
#define RESET  "\033[0m"
#define BOLD   "\033[1m"
#define CYAN   "\033[36m"

static int passedCount = 0;
static int failedCount = 0;

// -------------------------------------------------------
// Helper: Report PASS or FAIL with optional details
// -------------------------------------------------------
static void report(const std::string& testName, bool success, const std::string& details) {
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
// Helper: Find GDI+ encoder CLSID for image format
// -------------------------------------------------------
#ifdef _WIN32
static int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT num = 0, size = 0;
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;

    auto* pCodec = (Gdiplus::ImageCodecInfo*)(malloc(size));
    if (!pCodec) return -1;

    Gdiplus::GetImageEncoders(num, size, pCodec);
    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pCodec[j].MimeType, format) == 0) {
            *pClsid = pCodec[j].Clsid;
            free(pCodec);
            return static_cast<int>(j);
        }
    }
    free(pCodec);
    return -1;
}

// -------------------------------------------------------
// Helper: Programmatically generate a test barcode image
// -------------------------------------------------------
static bool CreateTestBarcodeImage(const std::string& code,
                                  const std::wstring& outPath,
                                  const WCHAR* mimeType,
                                  bool corruptBars = false)
{
    auto elements = BarcodeGenerator::encode(code);

    const int imgW      = 600;
    const int imgH      = 200;
    const int paddingPx = 20;

    int totalUnits = 0;
    for (const auto& el : elements) totalUnits += el.width;
    if (totalUnits <= 0) return false;

    Gdiplus::Bitmap bmp(imgW, imgH, PixelFormat24bppRGB);
    Gdiplus::Graphics g(&bmp);
    g.SetSmoothingMode(Gdiplus::SmoothingModeNone);
    g.Clear(Gdiplus::Color(255, 255, 255)); // White background

    double barcodeW = imgW - 2.0 * paddingPx;
    double unitPx   = barcodeW / static_cast<double>(totalUnits);
    double barcodeH = imgH - 2.0 * paddingPx;
    double x        = paddingPx;

    Gdiplus::SolidBrush blackBrush(Gdiplus::Color(0, 0, 0));
    for (const auto& el : elements) {
        double elW = unitPx * static_cast<double>(el.width);
        if (el.isBar) {
            g.FillRectangle(&blackBrush,
                            static_cast<Gdiplus::REAL>(x),
                            static_cast<Gdiplus::REAL>(paddingPx),
                            static_cast<Gdiplus::REAL>(elW),
                            static_cast<Gdiplus::REAL>(barcodeH));
        }
        x += elW;
    }

    // Corrupt the barcode by erasing a vertical strip across all rows
    if (corruptBars) {
        Gdiplus::SolidBrush whiteBrush(Gdiplus::Color(255, 255, 255));
        g.FillRectangle(&whiteBrush, 20.0f, 0.0f, 150.0f, 200.0f);
    }

    CLSID clsid;
    if (GetEncoderClsid(mimeType, &clsid) < 0) return false;

    return (bmp.Save(outPath.c_str(), &clsid, NULL) == Gdiplus::Ok);
}

// -------------------------------------------------------
// Helper: Programmatically generate a blank solid image
// -------------------------------------------------------
static bool CreateBlankTestImage(const std::wstring& outPath) {
    Gdiplus::Bitmap bmp(400, 200, PixelFormat24bppRGB);
    Gdiplus::Graphics g(&bmp);
    g.Clear(Gdiplus::Color(255, 255, 255)); // Pure white

    CLSID clsid;
    if (GetEncoderClsid(L"image/png", &clsid) < 0) return false;
    return (bmp.Save(outPath.c_str(), &clsid, NULL) == Gdiplus::Ok);
}
#endif

// -------------------------------------------------------
// Helper: Decode and print result for a single image file
// -------------------------------------------------------
static void DecodeSingleImage(const std::string& rawPath) {
    std::string path = rawPath;
    if (path.size() >= 2 && path.front() == '"' && path.back() == '"') {
        path = path.substr(1, path.size() - 2);
    }

    std::cout << "\n" << BOLD << "======================================================\n"
              << "       BarcodeDecoder — Single Image Decoding         \n"
              << "======================================================\n" << RESET;
    std::cout << "  Input File: " << path << "\n\n";

    DecodeResult result = BarcodeDecoder::decodeImage(path);

    if (result.success) {
        std::cout << GREEN << BOLD << "  [SUCCESS] Barcode successfully decoded!\n" << RESET;
        std::cout << "  Decoded 6-digit Code : " << BOLD << GREEN << result.code << RESET << "\n";
    } else {
        std::cout << RED << BOLD << "  [FAILED] Could not decode barcode image!\n" << RESET;
        std::cout << "  Error Reason         : " << RED << result.errorMessage << RESET << "\n";
    }
    std::cout << BOLD << "======================================================\n\n" << RESET;
}

// -------------------------------------------------------
// main() — Runs all automated tests and exits cleanly
// -------------------------------------------------------
int main(int argc, char* argv[]) {
    // If a specific image file was passed as an argument, decode that file directly
    if (argc >= 2 && std::string(argv[1]) != "--all" && std::string(argv[1]) != "--test") {
        DecodeSingleImage(argv[1]);
        return 0;
    }

    std::cout << BOLD << "\n======================================================\n"
              << "       BarcodeDecoder — Automated Test Suite          \n"
              << "======================================================\n\n" << RESET;

#ifdef _WIN32
    Gdiplus::GdiplusStartupInput gdiInput;
    ULONG_PTR gdiToken;
    Gdiplus::GdiplusStartup(&gdiToken, &gdiInput, NULL);

    // ========================================================
    // SECTION 1: Code 39 Pattern Unit Tests
    // ========================================================
    std::cout << CYAN << BOLD << "[Section 1] Code 39 Pattern Unit Tests\n" << RESET;

    report("Pattern decoding: '0'", BarcodeDecoder::decodePattern("NNNWWNWNN") == '0', "Expected '0'");
    report("Pattern decoding: '1'", BarcodeDecoder::decodePattern("WNNWNNNNW") == '1', "Expected '1'");
    report("Pattern decoding: '5'", BarcodeDecoder::decodePattern("WNNWWNNNN") == '5', "Expected '5'");
    report("Pattern decoding: '9'", BarcodeDecoder::decodePattern("NNWWNNWNN") == '9', "Expected '9'");
    report("Pattern decoding: '*'", BarcodeDecoder::decodePattern("NWNNWNWNN") == '*', "Expected '*'");
    report("Pattern decoding: invalid pattern rejected", BarcodeDecoder::decodePattern("WWWWWWWWW") == '?', "Expected '?'");

    // Valid sequence decoding: '*' + 6 digits + '*'
    const char* validPatterns[8] = {
        "NWNNWNWNN", // *
        "WNNWNNNNW", // 1
        "NNWWNNNNW", // 2
        "WNWWNNNNN", // 3
        "NNNWWNNWN", // 4
        "WNNWWNNNN", // 5
        "NNWWWNNNN", // 6
        "NWNNWNWNN"  // *
    };
    DecodeResult resPatterns = BarcodeDecoder::decodePatterns(validPatterns, 8);
    report("Valid pattern sequence (\"123456\")",
           resPatterns.success && resPatterns.code == "123456",
           "Result: \"" + resPatterns.code + "\"");

    // Leading zero rejection: first digit must not be '0'
    const char* leadingZeroPatterns[8] = {
        "NWNNWNWNN", // *
        "NNNWWNWNN", // 0
        "WNNWNNNNW", // 1
        "NNWWNNNNW", // 2
        "WNWWNNNNN", // 3
        "NNNWWNNWN", // 4
        "WNNWWNNNN", // 5
        "NWNNWNWNN"  // *
    };
    DecodeResult resZero = BarcodeDecoder::decodePatterns(leadingZeroPatterns, 8);
    report("Leading zero rejection (\"012345\")",
           !resZero.success && resZero.errorMessage.find("zero") != std::string::npos,
           "Correctly rejected with: " + resZero.errorMessage);

    // Wrong digit count rejection: 5 digits instead of 6
    const char* shortPatterns[7] = {
        "NWNNWNWNN", // *
        "WNNWNNNNW", // 1
        "NNWWNNNNW", // 2
        "WNWWNNNNN", // 3
        "NNNWWNNWN", // 4
        "WNNWWNNNN", // 5
        "NWNNWNWNN"  // *
    };
    DecodeResult resShort = BarcodeDecoder::decodePatterns(shortPatterns, 7);
    report("Wrong digit count rejection (5 digits)",
           !resShort.success && resShort.errorMessage.find("Expected 8") != std::string::npos,
           "Error message: " + resShort.errorMessage);

    // Unsupported character pattern rejection
    const char* invalidCharPatterns[8] = {
        "NWNNWNWNN", // *
        "WNNWNNNNW", // 1
        "NNWWNNNNW", // 2
        "WWWWWWWWW", // Invalid pattern (not a Code 39 digit)
        "NNNWWNNWN", // 4
        "WNNWWNNNN", // 5
        "NNWWWNNNN", // 6
        "NWNNWNWNN"  // *
    };
    DecodeResult resInvalidChar = BarcodeDecoder::decodePatterns(invalidCharPatterns, 8);
    report("Unsupported character in pattern sequence",
           !resInvalidChar.success && resInvalidChar.errorMessage.find("not a supported") != std::string::npos,
           "Error message: " + resInvalidChar.errorMessage);

    // ========================================================
    // SECTION 2: Real Barcode Image File Decoding Tests
    // ========================================================
    std::cout << "\n" << CYAN << BOLD << "[Section 2] Real Barcode Image File Decoding Tests\n" << RESET;

    // Test 1: Real PNG format
    std::wstring pngPath = L"test_sample_123456.png";
    CreateTestBarcodeImage("123456", pngPath, L"image/png");
    DecodeResult rPng = BarcodeDecoder::decodeImage("test_sample_123456.png");
    report("Decode PNG barcode (\"123456\")",
           rPng.success && rPng.code == "123456",
           "Decoded code: \"" + rPng.code + "\"");

    // Test 2: Real BMP format
    std::wstring bmpPath = L"test_sample_904571.bmp";
    CreateTestBarcodeImage("904571", bmpPath, L"image/bmp");
    DecodeResult rBmp = BarcodeDecoder::decodeImage("test_sample_904571.bmp");
    report("Decode BMP barcode (\"904571\")",
           rBmp.success && rBmp.code == "904571",
           "Decoded code: \"" + rBmp.code + "\"");

    // Test 3: Real JPG format
    std::wstring jpgPath = L"test_sample_999999.jpg";
    CreateTestBarcodeImage("999999", jpgPath, L"image/jpeg");
    DecodeResult rJpg = BarcodeDecoder::decodeImage("test_sample_999999.jpg");
    report("Decode JPG barcode (\"999999\")",
           rJpg.success && rJpg.code == "999999",
           "Decoded code: \"" + rJpg.code + "\"");

    // ========================================================
    // SECTION 3: Edge Cases, Error Handling & Failure Scenarios
    // ========================================================
    std::cout << "\n" << CYAN << BOLD << "[Section 3] Error Handling & Edge Cases\n" << RESET;

    // Test 4: Empty file path handling
    DecodeResult rEmpty = BarcodeDecoder::decodeImage("");
    report("Empty file path handling",
           !rEmpty.success && rEmpty.errorMessage.find("empty") != std::string::npos,
           "Error message: " + rEmpty.errorMessage);

    // Test 5: Non-existent image file handling
    DecodeResult rMissing = BarcodeDecoder::decodeImage("non_existent_file_xyz123.png");
    report("Non-existent file handling",
           !rMissing.success && rMissing.errorMessage.find("not found") != std::string::npos,
           "Error message: " + rMissing.errorMessage);

    // Test 6: Unsupported / non-image file handling
    std::string txtPath = "test_unsupported_file.txt";
    HANDLE hTxt = CreateFileA(txtPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hTxt != INVALID_HANDLE_VALUE) {
        const char* txtContent = "This is a plain text file, not an image.";
        DWORD written = 0;
        WriteFile(hTxt, txtContent, static_cast<DWORD>(strlen(txtContent)), &written, NULL);
        CloseHandle(hTxt);
    }
    DecodeResult rUnsupported = BarcodeDecoder::decodeImage(txtPath);
    report("Unsupported / non-image file handling",
           !rUnsupported.success && (rUnsupported.errorMessage.find("Corrupted") != std::string::npos ||
                                     rUnsupported.errorMessage.find("unsupported") != std::string::npos),
           "Error message: " + rUnsupported.errorMessage);
    DeleteFileA(txtPath.c_str());

    // Test 7: Blank image containing no barcode
    std::wstring blankPath = L"test_blank_image.png";
    CreateBlankTestImage(blankPath);
    DecodeResult rBlank = BarcodeDecoder::decodeImage("test_blank_image.png");
    report("Blank image / No barcode detected",
           !rBlank.success && rBlank.errorMessage.find("No barcode") != std::string::npos,
           "Error message: " + rBlank.errorMessage);

    // Test 8: Corrupted / damaged barcode image
    std::wstring corruptPath = L"test_corrupted_barcode.png";
    CreateTestBarcodeImage("123456", corruptPath, L"image/png", true); // corruptBars = true
    DecodeResult rCorrupt = BarcodeDecoder::decodeImage("test_corrupted_barcode.png");
    report("Corrupted barcode image handling",
           !rCorrupt.success,
           "Error message: " + rCorrupt.errorMessage);

    // Test 9: Image containing leading-zero code ("012345")
    std::wstring zeroPath = L"test_leading_zero.png";
    CreateTestBarcodeImage("012345", zeroPath, L"image/png");
    DecodeResult rLeadZero = BarcodeDecoder::decodeImage("test_leading_zero.png");
    report("Image with leading zero rejected (\"012345\")",
           !rLeadZero.success && rLeadZero.errorMessage.find("zero") != std::string::npos,
           "Error message: " + rLeadZero.errorMessage);

    // Test 10: Image dimensions too small (<30px width / <10px height)
    std::wstring smallPath = L"test_too_small.png";
    {
        Gdiplus::Bitmap smallBmp(20, 5, PixelFormat24bppRGB);
        CLSID pngClsid;
        GetEncoderClsid(L"image/png", &pngClsid);
        smallBmp.Save(smallPath.c_str(), &pngClsid, NULL);
    }
    DecodeResult rSmall = BarcodeDecoder::decodeImage("test_too_small.png");
    report("Image dimensions too small handling",
           !rSmall.success && rSmall.errorMessage.find("too small") != std::string::npos,
           "Error message: " + rSmall.errorMessage);
    DeleteFileW(smallPath.c_str());

    // Test 11: Upside-down / inverted barcode image (rotated 180 degrees)
    std::wstring rotPath = L"test_sample_rotated.png";
    CreateTestBarcodeImage("123456", rotPath, L"image/png");
    {
        Gdiplus::Bitmap rotBmp(rotPath.c_str());
        rotBmp.RotateFlip(Gdiplus::Rotate180FlipNone);
        CLSID pngClsid;
        GetEncoderClsid(L"image/png", &pngClsid);
        rotBmp.Save(rotPath.c_str(), &pngClsid, NULL);
    }
    DecodeResult rRot = BarcodeDecoder::decodeImage("test_sample_rotated.png");
    report("Decode upside-down barcode (180 deg rotated)",
           rRot.success && rRot.code == "123456",
           "Decoded code: \"" + rRot.code + "\"");

    // Clean up all programmatically generated temporary test image files
    DeleteFileW(pngPath.c_str());
    DeleteFileW(bmpPath.c_str());
    DeleteFileW(jpgPath.c_str());
    DeleteFileW(blankPath.c_str());
    DeleteFileW(corruptPath.c_str());
    DeleteFileW(zeroPath.c_str());
    DeleteFileW(rotPath.c_str());

    Gdiplus::GdiplusShutdown(gdiToken);
#endif

    // ========================================================
    // SUMMARY
    // ========================================================
    std::cout << BOLD << "\n======================================================\n"
              << "  Results: " << GREEN << passedCount << " passed" << RESET << ", "
              << (failedCount > 0 ? RED : GREEN) << failedCount << " failed\n"
              << BOLD << "======================================================\n\n" << RESET;

    return (failedCount == 0) ? 0 : 1;
}
