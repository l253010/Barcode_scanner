// BarcodeDecoder.cpp
#include "BarcodeDecoder.h"
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "user32.lib")
#endif

static inline int min_val(int a, int b) { return (a < b) ? a : b; }
static inline int max_val(int a, int b) { return (a > b) ? a : b; }
static inline bool is_digit_char(char c) { return (c >= '0' && c <= '9'); }

static bool pattern_equals(const char* a, const char* b) {
    for (int i = 0; i < 9; ++i) {
        if (a[i] == '\0' || b[i] == '\0' || a[i] != b[i]) return false;
    }
    return (a[9] == '\0' && b[9] == '\0');
}
#ifdef _WIN32
struct GdiplusInitScope {
    ULONG_PTR token;
    bool      ok;

    GdiplusInitScope() {
        Gdiplus::GdiplusStartupInput input;
        ok = (Gdiplus::GdiplusStartup(&token, &input, NULL) == Gdiplus::Ok);
    }

    ~GdiplusInitScope() {
        if (ok) {
            Gdiplus::GdiplusShutdown(token);
        }
    }
};
#endif
struct Run {
    bool isBar; // true = black bar, false = white space
    int  width; // width in pixels
};
struct ScanlineBuffer {
    int*  luminance;
    bool* isBar;
    Run*  runs;
    Run*  reverseRuns;

    ScanlineBuffer(UINT width) {
        luminance   = new int[width];
        isBar       = new bool[width];
        runs        = new Run[width];
        reverseRuns = new Run[width];
    }

    ~ScanlineBuffer() {
        delete[] luminance;
        delete[] isBar;
        delete[] runs;
        delete[] reverseRuns;
    }
    ScanlineBuffer(const ScanlineBuffer&) = delete;
    ScanlineBuffer& operator=(const ScanlineBuffer&) = delete;
};

char BarcodeDecoder::decodePattern(const char* pattern9) {
    if (!pattern9) return '?';

    int len = 0;
    while (pattern9[len] != '\0') {
        if (++len > 9) return '?';
    }
    if (len != 9) return '?';

    // Official ISO/IEC 16388 Code 39 standard patterns
    if (pattern_equals(pattern9, "NNNWWNWNN")) return '0';
    if (pattern_equals(pattern9, "WNNWNNNNW")) return '1';
    if (pattern_equals(pattern9, "NNWWNNNNW")) return '2';
    if (pattern_equals(pattern9, "WNWWNNNNN")) return '3';
    if (pattern_equals(pattern9, "NNNWWNNWN")) return '4';
    if (pattern_equals(pattern9, "WNNWWNNNN")) return '5';
    if (pattern_equals(pattern9, "NNWWWNNNN")) return '6';
    if (pattern_equals(pattern9, "NNNWNNWNW")) return '7';
    if (pattern_equals(pattern9, "WNNWNNWNN")) return '8';
    if (pattern_equals(pattern9, "NNWWNNWNN")) return '9';
    if (pattern_equals(pattern9, "NWNNWNWNN")) return '*'; // Start / Stop delimiter

    return '?';
}

char BarcodeDecoder::decodePattern(const std::string& pattern9) {
    return decodePattern(pattern9.c_str());
}

// -------------------------------------------------------
// decodePatterns()
// Decodes an array of 9-character pattern strings.
// Expects: '*' + 6 digits + '*' (total 8 character patterns).
// -------------------------------------------------------
DecodeResult BarcodeDecoder::decodePatterns(const char* patterns[], int count) {
    if (count != 8 || !patterns) {
        return { false, "",
            "Decoding Error: Expected 8 characters (start '*', 6 digits, stop '*'), but found " +
            std::to_string(count) + "." };
    }

    // Verify start delimiter
    if (decodePattern(patterns[0]) != '*') {
        return { false, "", "Decoding Error: Missing or invalid Code 39 start '*' delimiter." };
    }

    // Verify stop delimiter
    if (decodePattern(patterns[7]) != '*') {
        return { false, "", "Decoding Error: Missing or invalid Code 39 stop '*' delimiter." };
    }

    // Decode 6 data digits
    std::string decodedDigits = "";
    for (int i = 1; i <= 6; ++i) {
        char ch = decodePattern(patterns[i]);
        if (ch == '?' || !is_digit_char(ch)) {
            return { false, "",
                "Decoding Error: Character at position " + std::to_string(i) +
                " is not a supported Code 39 numeric digit (pattern: " + std::string(patterns[i]) + ")." };
        }
        decodedDigits += ch;
    }

    // Business validation: first digit must not be '0'
    if (decodedDigits[0] == '0') {
        return { false, "",
            "Validation Error: The first digit must not be zero. Codes like " +
            decodedDigits + " are not allowed." };
    }

    return { true, decodedDigits, "" };
}

DecodeResult BarcodeDecoder::decodePatterns(const std::string patterns[], int count) {
    if (count != 8 || !patterns) {
        return { false, "",
            "Decoding Error: Expected 8 characters (start '*', 6 digits, stop '*'), but found " +
            std::to_string(count) + "." };
    }

    const char* ptrs[8];
    for (int i = 0; i < 8; ++i) {
        ptrs[i] = patterns[i].c_str();
    }
    return decodePatterns(ptrs, 8);
}

// -------------------------------------------------------
// Helper: Try decoding from an array of Runs
// -------------------------------------------------------
static bool TryDecodeFromRuns(const Run* runs, int runCount, DecodeResult& outResult) {
    const int totalExpectedRuns = 79;
    if (runCount < totalExpectedRuns) {
        return false;
    }

    int maxStart = runCount - totalExpectedRuns;

    for (int startIdx = 0; startIdx <= maxStart; ++startIdx) {
        // Start must begin with a black bar
        if (!runs[startIdx].isBar) {
            continue;
        }

        // Start '*' pattern is "NWNNWNWNN":
        // Narrow elements: 0, 2, 3, 5, 7, 8 (6 elements)
        // Wide elements:   1, 4, 6          (3 elements)
        int narrowMax = runs[startIdx + 0].width;
        narrowMax = max_val(narrowMax, runs[startIdx + 2].width);
        narrowMax = max_val(narrowMax, runs[startIdx + 3].width);
        narrowMax = max_val(narrowMax, runs[startIdx + 5].width);
        narrowMax = max_val(narrowMax, runs[startIdx + 7].width);
        narrowMax = max_val(narrowMax, runs[startIdx + 8].width);

        int wideMin = runs[startIdx + 1].width;
        wideMin = min_val(wideMin, runs[startIdx + 4].width);
        wideMin = min_val(wideMin, runs[startIdx + 6].width);

        // Wide elements must be noticeably wider than narrow elements
        if (wideMin <= narrowMax) {
            continue;
        }

        double threshold = (narrowMax + wideMin) / 2.0;

        // Gap after start character must be a space of narrow width
        if (runs[startIdx + 9].isBar || runs[startIdx + 9].width > threshold * 1.8) {
            continue;
        }

        // Decode 8 characters (start '*', 6 digits, stop '*')
        char patternBuffers[8][10];
        bool patternParseOk = true;

        for (int charNum = 0; charNum < 8; ++charNum) {
            int charStart = startIdx + charNum * 10;
            int wideCount   = 0;
            int narrowCount = 0;

            for (int el = 0; el < 9; ++el) {
                bool expectedBar = (el % 2 == 0);
                if (runs[charStart + el].isBar != expectedBar) {
                    patternParseOk = false;
                    break;
                }

                if (runs[charStart + el].width > threshold) {
                    patternBuffers[charNum][el] = 'W';
                    wideCount++;
                } else {
                    patternBuffers[charNum][el] = 'N';
                    narrowCount++;
                }
            }
            patternBuffers[charNum][9] = '\0';

            if (!patternParseOk) break;

            if (wideCount != 3 || narrowCount != 6) {
                patternParseOk = false;
                break;
            }

            // Verify inter-character gap (for characters 0 to 6)
            if (charNum < 7) {
                int gapIdx = charStart + 9;
                if (runs[gapIdx].isBar || runs[gapIdx].width > threshold * 1.8) {
                    patternParseOk = false;
                    break;
                }
            }
        }

        if (!patternParseOk) {
            continue;
        }

        const char* patternPtrs[8];
        for (int p = 0; p < 8; ++p) {
            patternPtrs[p] = patternBuffers[p];
        }

        DecodeResult res = BarcodeDecoder::decodePatterns(patternPtrs, 8);
        if (res.success) {
            outResult = res;
            return true;
        } else if (!res.errorMessage.empty()) {
            outResult = res;
            return false;
        }
    }

    return false;
}

// -------------------------------------------------------
// decodeImage()
// Main public API function to load an image and decode it.
// -------------------------------------------------------
DecodeResult BarcodeDecoder::decodeImage(const std::string& imagePath) {
    if (imagePath.empty()) {
        return { false, "", "Error: Image file path is empty." };
    }

#ifdef _WIN32
    // Step 1: Check file existence using native Windows API (zero <fstream> needed)
    DWORD attrs = GetFileAttributesA(imagePath.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES || (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        return { false, "", "Error: Image file not found or could not be opened: \"" + imagePath + "\"." };
    }

    // Step 2: Initialize GDI+ for image loading
    GdiplusInitScope gdiScope;
    if (!gdiScope.ok) {
        return { false, "", "Error: Failed to initialize Windows GDI+ graphics library." };
    }

    // Convert path to wide string for GDI+
    int wLen = MultiByteToWideChar(CP_UTF8, 0, imagePath.c_str(), -1, NULL, 0);
    if (wLen <= 1) {
        return { false, "", "Error: Invalid image file path." };
    }

    WCHAR* wPath = new WCHAR[wLen];
    MultiByteToWideChar(CP_UTF8, 0, imagePath.c_str(), -1, wPath, wLen);

    Gdiplus::Bitmap bitmap(wPath);
    delete[] wPath;

    if (bitmap.GetLastStatus() != Gdiplus::Ok) {
        return { false, "", "Error: Corrupted image file or unsupported image format: \"" + imagePath + "\"." };
    }

    UINT width  = bitmap.GetWidth();
    UINT height = bitmap.GetHeight();

    // Step 3: Validate image boundaries and dimension limits
    if (width < 30 || height < 10) {
        return { false, "", "Error: Image dimensions (" + std::to_string(width) + "x" +
                            std::to_string(height) + ") are too small to contain a readable barcode." };
    }

    if (width > 10000 || height > 10000) {
        return { false, "", "Error: Image dimensions (" + std::to_string(width) + "x" +
                            std::to_string(height) + ") exceed maximum supported resolution." };
    }

    // Step 4: Dynamically allocate memory sized to the exact image width (zero <vector>)
    // RAII guarantees all memory is released cleanly on any return path.
    ScanlineBuffer buffer(width);

    // Step 5: Sample horizontal scanlines across multiple image heights
    const double scanFractions[] = { 0.50, 0.40, 0.60, 0.30, 0.70 };
    const int numFractions = 5;

    bool hasAnyContrast = false;
    DecodeResult validationFailureResult = { false, "", "" };

    for (int s = 0; s < numFractions; ++s) {
        UINT y = static_cast<UINT>(height * scanFractions[s]);
        if (y >= height) y = height - 1;

        int minLum = 255;
        int maxLum = 0;

        for (UINT x = 0; x < width; ++x) {
            Gdiplus::Color color;
            bitmap.GetPixel(x, y, &color);
            // Grayscale luminance formula: Y = 0.299*R + 0.587*G + 0.114*B
            int lum = static_cast<int>(0.299 * color.GetR() + 0.587 * color.GetG() + 0.114 * color.GetB());
            buffer.luminance[x] = lum;
            if (lum < minLum) minLum = lum;
            if (lum > maxLum) maxLum = lum;
        }

        // Check for sufficient contrast along this scanline
        if (maxLum - minLum < 35) {
            continue;
        }
        hasAnyContrast = true;

        // Binarization threshold
        int threshold = (minLum + maxLum) / 2;

        for (UINT x = 0; x < width; ++x) {
            buffer.isBar[x] = (buffer.luminance[x] < threshold);
        }

        // Measure consecutive pixel run lengths
        int runCount = 0;
        bool curBar = buffer.isBar[0];
        int  curLen = 1;

        for (UINT x = 1; x < width; ++x) {
            if (buffer.isBar[x] == curBar) {
                curLen++;
            } else {
                buffer.runs[runCount].isBar = curBar;
                buffer.runs[runCount].width = curLen;
                runCount++;
                curBar = buffer.isBar[x];
                curLen = 1;
            }
        }
        buffer.runs[runCount].isBar = curBar;
        buffer.runs[runCount].width = curLen;
        runCount++;

        // Forward decoding attempt
        DecodeResult res;
        if (TryDecodeFromRuns(buffer.runs, runCount, res)) {
            return res; // Successfully decoded!
        }
        if (!res.errorMessage.empty() && validationFailureResult.errorMessage.empty()) {
            validationFailureResult = res;
        }

        // Reverse decoding attempt (for upside-down 180° rotated barcodes)
        for (int r = 0; r < runCount; ++r) {
            buffer.reverseRuns[r] = buffer.runs[runCount - 1 - r];
        }

        if (TryDecodeFromRuns(buffer.reverseRuns, runCount, res)) {
            return res;
        }
    }

    if (!hasAnyContrast) {
        return { false, "", "Error: No barcode detected in image (insufficient contrast or blank image)." };
    }

    if (!validationFailureResult.errorMessage.empty()) {
        return validationFailureResult;
    }

    return { false, "", "Error: Unreadable or corrupted barcode. Could not detect valid Code 39 start/stop delimiters." };

#else
    return { false, "", "Error: Windows GDI+ is required for image reading." };
#endif
}
