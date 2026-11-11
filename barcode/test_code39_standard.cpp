// test_code39_standard.cpp
// Checks the decoder against the OFFICIAL Code 39 table
// (written out by hand, NOT copied from our own code).

#include "BarcodeDecoder.h"
#include "BarcodeGenerator.h"

#include <iostream>
#include <string>

static int passed = 0;
static int failed = 0;

static void check(const std::string& name, bool ok) {
    if (ok) { ++passed; std::cout << "  [PASS] " << name << "\n"; }
    else    { ++failed; std::cout << "  [FAIL] " << name << "\n"; }
}

struct Official { char ch; const char* pattern; };
static const Official kOfficial[] = {
    {'0', "NNNWWNWNN"}, {'1', "WNNWNNNNW"}, {'2', "NNWWNNNNW"},
    {'3', "WNWWNNNNN"}, {'4', "NNNWWNNNW"}, {'5', "WNNWWNNNN"},
    {'6', "NNWWWNNNN"}, {'7', "NNNWNNWNW"}, {'8', "WNNWNNWNN"},
    {'9', "NNWWNNWNN"}, {'*', "NWNNWNWNN"}
};

int main() {
    std::cout << "\n--- Official Code 39 standard tests ---\n\n";

    // Test 1: every official pattern decodes to the right character
    for (const Official& o : kOfficial) {
        std::string name = std::string("Official pattern for '") + o.ch + "' decodes correctly";
        check(name, BarcodeDecoder::decodePattern(o.pattern) == o.ch);
    }

    // Test 2: a full standard barcode for 123456 decodes to "123456"
    std::string full[8] = {
        "NWNNWNWNN", "WNNWNNNNW", "NNWWNNNNW", "WNWWNNNNN",
        "NNNWWNNNW", "WNNWWNNNN", "NNWWWNNNN", "NWNNWNWNN"
    };
    DecodeResult r = BarcodeDecoder::decodePatterns(full, 8);
    check("Official 123456 barcode decodes to \"123456\"", r.success && r.code == "123456");

    // Test 3: generator output matches the standard
    auto elements = BarcodeGenerator::encode("123456");
    bool generatorOk = true;
    const char* order = "*123456*";
    for (int c = 0; c < 8; ++c) {
        const char* expected = nullptr;
        for (const Official& o : kOfficial)
            if (o.ch == order[c]) expected = o.pattern;
        for (int e = 0; e < 9; ++e) {
            size_t idx = (size_t)c * 10 + (size_t)e;
            if (idx >= elements.size()) { generatorOk = false; break; }
            int expectedWidth = (expected[e] == 'W') ? 3 : 1;
            if (elements[idx].width != expectedWidth) generatorOk = false;
        }
    }
    check("Generator output matches the official table for 123456", generatorOk);

    // Test 4: the old wrong pattern for '4' must NOT decode as '4'
    check("Old wrong pattern for '4' is rejected",
          BarcodeDecoder::decodePattern("NNNWNNWNW") != '4');

    std::cout << "\nResults: " << passed << " passed, " << failed << " failed\n\n";
    return (failed == 0) ? 0 : 1;
}
