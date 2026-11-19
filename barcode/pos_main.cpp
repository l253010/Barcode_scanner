// pos_main.cpp
// -------------------------------------------------------
// Point of Sale (POS) Console Application with Code 39 Barcode Scanning.
//
// Modes:
//   Mode 1: Product Description Lookup (scans barcode and shows details & price)
//   Mode 2: Order Recording (multi-item scanning, quantity count, running total, end summary)
//
// Both modes share the unified acquireCode() helper:
//   - Attempts camera scanning via scanBarcodeFromCamera().
//   - If scan fails or cancels, offers:
//       [1] Retry camera scan
//       [2] Enter barcode manually (validated against Code 39 rules)
//       [3] Cancel
// -------------------------------------------------------

#include "camera_scanner.h"
#include "BarcodeGenerator.h"

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>

// -------------------------------------------------------
// Product Specification Structure
// -------------------------------------------------------
struct ProductItem {
    std::string id;
    std::string barcode;
    std::string name;
    std::string unit;
    double price = 0.0;
    int stock = 0;
};

// -------------------------------------------------------
// Product Catalog Database
// -------------------------------------------------------
class ProductCatalog {
private:
    std::unordered_map<std::string, ProductItem> m_database;
    std::string m_catalogFilePath;

public:
    void add(const ProductItem& item) {
        m_database[item.barcode] = item;
    }

    const ProductItem* find(const std::string& barcode) const {
        auto it = m_database.find(barcode);
        if (it != m_database.end()) {
            return &(it->second);
        }
        return nullptr;
    }

    size_t count() const {
        return m_database.size();
    }

    std::string getLoadedPath() const {
        return m_catalogFilePath;
    }

    bool deductStockAndSave(const std::vector<struct OrderLineItem>& cart, std::string* outMsg);

    void load() {
        // 1. Seed with standard catalog and known sample barcodes
        add({ "PRD-101", "100001", "Organic Whole Milk 1L", "count", 3.49, 50 });
        add({ "PRD-102", "100002", "Whole Grain Wheat Bread", "count", 2.89, 40 });
        add({ "PRD-103", "100003", "Farm Fresh Large Eggs 12pk", "count", 4.25, 30 });
        add({ "PRD-104", "123456", "Arabica Dark Roast Coffee 250g", "weight", 7.99, 25 });
        add({ "PRD-105", "200001", "Natural Spring Water 500ml", "count", 1.20, 100 });
        add({ "PRD-106", "200002", "Crunchy Peanut Butter 500g", "count", 4.50, 20 });
        add({ "PRD-107", "300001", "Dark Chocolate Bar 100g", "count", 2.50, 60 });
        add({ "PRD-108", "300002", "Extra Virgin Olive Oil 750ml", "count", 11.99, 15 });
        add({ "PRD-109", "400001", "Basmati Long Grain Rice 2kg", "weight", 8.75, 35 });
        add({ "PRD-110", "500001", "Sparkling Lemonade 330ml", "count", 1.75, 8 });
        add({ "PRD-111", "100000", "Store Rewards Gift Card $10", "count", 10.00, 100 });
        add({ "PRD-112", "904571", "Ergonomic Optical Mouse", "count", 19.99, 18 });
        add({ "PRD-113", "999999", "Premium Wireless Headset", "count", 59.99, 12 });

        // 2. Try loading Product.txt from possible relative and executable paths
        std::vector<std::string> paths = {
            "Product.txt",
            "product_module/Product.txt",
            "../product_module/Product.txt",
            "../../product_module/Product.txt",
            "D:/barcode_scanner/product_module/Product.txt",
            "D:/barcode_scanner/barcode/Product.txt"
        };
#ifdef _WIN32
        char exeBuf[MAX_PATH];
        if (GetModuleFileNameA(NULL, exeBuf, MAX_PATH) > 0) {
            std::string exeStr(exeBuf);
            size_t slash = exeStr.find_last_of("\\/");
            if (slash != std::string::npos) {
                std::string baseDir = exeStr.substr(0, slash);
                paths.insert(paths.begin(), baseDir + "\\Product.txt");
                paths.insert(paths.begin() + 1, baseDir + "\\..\\product_module\\Product.txt");
            }
        }
#endif

        for (const auto& path : paths) {
            std::ifstream file(path);
            if (!file.is_open()) continue;
            m_catalogFilePath = path;

            std::string line;
            while (std::getline(file, line)) {
                if (line.empty() || line[0] == '#') continue;

                // Format: PRD-101 100001 MILK-1L Organic Whole Milk 1L count 3.49 0.1745 0.00 50 barcodes/... #
                std::istringstream iss(line);
                std::string pid, barcode, acronym;
                if (!(iss >> pid >> barcode >> acronym)) continue;

                std::vector<std::string> remainingTokens;
                std::string token;
                while (iss >> token) {
                    if (token == "#") break;
                    remainingTokens.push_back(token);
                }

                if (remainingTokens.size() >= 5) {
                    // Last 4 tokens before '#' are: price, tax, discount, stock, (optional path)
                    // Find unit: "count" or "weight"
                    int unitIdx = -1;
                    for (int i = 0; i < static_cast<int>(remainingTokens.size()); ++i) {
                        if (remainingTokens[static_cast<size_t>(i)] == "count" ||
                            remainingTokens[static_cast<size_t>(i)] == "weight") {
                            unitIdx = i;
                            break;
                        }
                    }

                    if (unitIdx > 0 && unitIdx + 1 < static_cast<int>(remainingTokens.size())) {
                        std::string name;
                        for (int i = 0; i < unitIdx; ++i) {
                            if (i > 0) name += " ";
                            name += remainingTokens[static_cast<size_t>(i)];
                        }
                        std::string unit = remainingTokens[static_cast<size_t>(unitIdx)];
                        double price = 0.0;
                        int stock = 0;
                        try {
                            price = std::stod(remainingTokens[static_cast<size_t>(unitIdx + 1)]);
                            if (unitIdx + 4 < static_cast<int>(remainingTokens.size())) {
                                stock = std::stoi(remainingTokens[static_cast<size_t>(unitIdx + 4)]);
                            }
                        } catch (...) {}

                        add({ pid, barcode, name, unit, price, stock });
                    }
                }
            }
            break;
        }
    }
};

// -------------------------------------------------------
// Unified acquireCode() Helper
// -------------------------------------------------------
// Prompts for barcode input via camera scan.
// If scan fails or is cancelled, offers:
//   1. Retry camera scan
//   2. Manual code entry (with Code 39 validation)
//   3. Cancel
// Returns true with valid 6-digit code in outCode, or false if cancelled.
// -------------------------------------------------------
static bool acquireCode(std::string& outCode) {
    while (true) {
        std::cout << "\n============================================\n";
        std::cout << "          BARCODE ACQUISITION               \n";
        std::cout << "============================================\n";
        std::cout << "Starting camera scanner...\n";
        std::cout << "Position the barcode in front of the camera.\n";
        std::cout << "Press [ESC] on the preview window to cancel.\n";

        ScanOptions opts;
        opts.cameraIndex = 0;
        opts.timeoutSeconds = 15;
        opts.confirmationFrames = 3;
        opts.showPreview = true;
        opts.windowTitle = "POS Scanner - Center Barcode (ESC to Cancel)";

        ScanOutcome outcome = scanBarcodeFromCamera(opts);

        if (outcome.success) {
            std::cout << "\n[SUCCESS] Barcode decoded: [" << outcome.code << "]\n";
            outCode = outcome.code;
            return true;
        }

        // Camera scan was unsuccessful or cancelled
        std::cout << "\n[SCAN FAILED] " << outcome.error << "\n";
        std::cout << "--------------------------------------------\n";
        std::cout << "Options:\n";
        std::cout << "  [1] Retry camera scan\n";
        std::cout << "  [2] Enter barcode manually\n";
        std::cout << "  [3] Cancel\n";
        std::cout << "Enter choice (1-3): ";

        std::string choice;
        if (!std::getline(std::cin, choice)) {
            return false;
        }

        // Trim whitespace
        while (!choice.empty() && (choice.back() == '\r' || choice.back() == ' ')) choice.pop_back();

        if (choice == "1" || choice == "r" || choice == "R") {
            continue; // Loop back and retry camera
        } else if (choice == "2" || choice == "m" || choice == "M") {
            while (true) {
                std::cout << "\nEnter 6-digit barcode (or 'c' to cancel manual entry): ";
                std::string manualInput;
                if (!std::getline(std::cin, manualInput)) return false;
                while (!manualInput.empty() && (manualInput.back() == '\r' || manualInput.back() == ' ')) manualInput.pop_back();

                if (manualInput == "c" || manualInput == "C") {
                    break; // Back to scan options menu
                }

                ValidationResult val = BarcodeGenerator::validate(manualInput);
                if (val.valid) {
                    std::cout << "[SUCCESS] Manual barcode accepted: [" << manualInput << "]\n";
                    outCode = manualInput;
                    return true;
                } else {
                    std::cout << "[INVALID] " << val.message << "\n";
                    std::cout << "Please try again or enter 'c' to return.\n";
                }
            }
        } else if (choice == "3" || choice == "c" || choice == "C") {
            std::cout << "[CANCELLED] Barcode acquisition cancelled by user.\n";
            return false;
        } else {
            std::cout << "Invalid choice. Please select 1, 2, or 3.\n";
        }
    }
}

// -------------------------------------------------------
// Mode 1: Product Description Mode
// -------------------------------------------------------
static void runProductDescriptionMode(const ProductCatalog& catalog) {
    std::cout << "\n============================================\n";
    std::cout << "      MODE 1: PRODUCT DESCRIPTION LOOKUP    \n";
    std::cout << "============================================\n";

    while (true) {
        std::string code;
        if (!acquireCode(code)) {
            std::cout << "\nReturning to Main Menu...\n";
            break;
        }

        const ProductItem* product = catalog.find(code);
        std::cout << "\n--------------------------------------------\n";
        std::cout << "           PRODUCT INFORMATION              \n";
        std::cout << "--------------------------------------------\n";
        if (product) {
            std::cout << "  Barcode      : " << product->barcode << "\n";
            std::cout << "  Product ID   : " << product->id << "\n";
            std::cout << "  Product Name : " << product->name << "\n";
            std::cout << "  Unit Price   : $" << std::fixed << std::setprecision(2) << product->price << "\n";
            std::cout << "  Unit Type    : " << product->unit << "\n";
            std::cout << "  Stock In Hand: " << product->stock << " units\n";
        } else {
            std::cout << "  Barcode      : " << code << "\n";
            std::cout << "  Status       : Product not found in database.\n";
        }
        std::cout << "--------------------------------------------\n";

        std::cout << "\nScan another product? ([Y]es / [N]o): ";
        std::string answer;
        if (!std::getline(std::cin, answer)) break;
        if (!answer.empty() && (answer[0] == 'n' || answer[0] == 'N')) {
            break;
        }
    }
}

// -------------------------------------------------------
// Order Line Item Structure
// -------------------------------------------------------
struct OrderLineItem {
    ProductItem product;
    int quantity = 0;
    double lineTotal = 0.0;
};

// -------------------------------------------------------
// ProductCatalog::deductStockAndSave()
// Deducts purchased quantities from in-memory catalog AND
// persists updated available stock directly to Product.txt.
// -------------------------------------------------------
bool ProductCatalog::deductStockAndSave(const std::vector<OrderLineItem>& cart, std::string* outMsg) {
    // 1. Update in-memory stock
    for (const auto& item : cart) {
        auto it = m_database.find(item.product.barcode);
        if (it != m_database.end()) {
            it->second.stock = (it->second.stock >= item.quantity) ? (it->second.stock - item.quantity) : 0;
        }
    }

    // 2. Determine target file path
    std::string targetPath = m_catalogFilePath;
    if (targetPath.empty()) {
        std::vector<std::string> paths = {
            "Product.txt",
            "product_module/Product.txt",
            "../product_module/Product.txt",
            "../../product_module/Product.txt",
            "D:/barcode_scanner/product_module/Product.txt",
            "D:/barcode_scanner/barcode/Product.txt"
        };
#ifdef _WIN32
        char exeBuf[MAX_PATH];
        if (GetModuleFileNameA(NULL, exeBuf, MAX_PATH) > 0) {
            std::string exeStr(exeBuf);
            size_t slash = exeStr.find_last_of("\\/");
            if (slash != std::string::npos) {
                std::string baseDir = exeStr.substr(0, slash);
                paths.insert(paths.begin(), baseDir + "\\Product.txt");
                paths.insert(paths.begin() + 1, baseDir + "\\..\\product_module\\Product.txt");
            }
        }
#endif
        for (const auto& p : paths) {
            std::ifstream test(p);
            if (test.good()) {
                targetPath = p;
                break;
            }
        }
    }
    if (targetPath.empty()) {
        targetPath = "Product.txt";
    }

    std::ifstream inFile(targetPath);
    if (!inFile.is_open()) {
        if (outMsg) *outMsg = "Product.txt not found on disk. In-memory stock was updated.";
        return false;
    }

    std::vector<std::string> lines;
    std::string rawLine;
    while (std::getline(inFile, rawLine)) {
        lines.push_back(rawLine);
    }
    inFile.close();

    // 3. Update stock field in matching lines
    for (auto& line : lines) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string pid, barcode, acronym;
        if (!(iss >> pid >> barcode >> acronym)) continue;

        int qtySold = 0;
        for (const auto& item : cart) {
            if (item.product.barcode == barcode) {
                qtySold += item.quantity;
            }
        }
        if (qtySold == 0) continue;

        std::vector<std::string> tokens;
        std::istringstream tokStream(line);
        std::string t;
        while (tokStream >> t) {
            tokens.push_back(t);
        }

        int hashIdx = -1;
        for (int i = 0; i < static_cast<int>(tokens.size()); ++i) {
            if (tokens[static_cast<size_t>(i)] == "#") {
                hashIdx = i;
                break;
            }
        }
        if (hashIdx >= 3) {
            int stockIdx = hashIdx - 2;
            try {
                int currentStock = std::stoi(tokens[static_cast<size_t>(stockIdx)]);
                int newStock = (currentStock >= qtySold) ? (currentStock - qtySold) : 0;
                tokens[static_cast<size_t>(stockIdx)] = std::to_string(newStock);

                std::ostringstream rebuilt;
                for (size_t i = 0; i < tokens.size(); ++i) {
                    if (i > 0) rebuilt << " ";
                    rebuilt << tokens[i];
                }
                line = rebuilt.str();
            } catch (...) {}
        }
    }

    // 4. Save to targetPath
    std::ofstream outFile(targetPath, std::ios::trunc);
    if (!outFile.is_open()) {
        if (outMsg) *outMsg = "Failed to open " + targetPath + " for writing.";
        return false;
    }
    for (const auto& l : lines) {
        outFile << l << "\n";
    }
    outFile.close();

    // 5. Also sync to other candidate locations if they exist
    const std::string syncPaths[] = {
        "Product.txt",
        "product_module/Product.txt",
        "../product_module/Product.txt",
        "../../product_module/Product.txt"
    };
    for (const auto& sp : syncPaths) {
        if (sp != targetPath) {
            std::ifstream test(sp);
            if (test.good()) {
                test.close();
                std::ofstream syncOut(sp, std::ios::trunc);
                if (syncOut.is_open()) {
                    for (const auto& l : lines) {
                        syncOut << l << "\n";
                    }
                }
            }
        }
    }

    if (outMsg) *outMsg = "Product.txt inventory successfully updated on disk.";
    return true;
}

// -------------------------------------------------------
// Mode 2: Order Recording Mode
// -------------------------------------------------------
static void runOrderRecordingMode(ProductCatalog& catalog) {
    std::cout << "\n============================================\n";
    std::cout << "          MODE 2: ORDER RECORDING           \n";
    std::cout << "============================================\n";
    std::cout << "New order started. Scan items to add them to order.\n";

    std::vector<OrderLineItem> cart;
    double runningTotal = 0.0;
    int totalItemsCount = 0;

    while (true) {
        std::cout << "\n--------------------------------------------\n";
        std::cout << "CURRENT RUNNING TOTAL: $" << std::fixed << std::setprecision(2)
                  << runningTotal << " (" << totalItemsCount << " items)\n";
        std::cout << "--------------------------------------------\n";
        std::cout << "Order Actions:\n";
        std::cout << "  [1] Scan / Add next item\n";
        std::cout << "  [2] Complete order & print summary\n";
        std::cout << "  [3] Cancel entire order\n";
        std::cout << "Select action (1-3, default 1): ";

        std::string choice;
        if (!std::getline(std::cin, choice)) break;
        while (!choice.empty() && (choice.back() == '\r' || choice.back() == ' ')) choice.pop_back();

        if (choice.empty() || choice == "1") {
            std::string code;
            if (!acquireCode(code)) {
                // Acquisition cancelled; return to order menu without discarding cart
                std::cout << "Item not added. Returning to order session.\n";
                continue;
            }

            const ProductItem* product = catalog.find(code);
            if (!product) {
                std::cout << "\n[ERROR] Barcode [" << code << "] does not match any product in catalog.\n";
                continue;
            }

            std::cout << "\nProduct Found: " << product->name << " (Unit Price: $"
                      << std::fixed << std::setprecision(2) << product->price << ")\n";
            std::cout << "Enter quantity (default 1): ";
            std::string qtyInput;
            int qty = 1;
            if (std::getline(std::cin, qtyInput)) {
                while (!qtyInput.empty() && (qtyInput.back() == '\r' || qtyInput.back() == ' ')) qtyInput.pop_back();
                if (!qtyInput.empty()) {
                    try {
                        int q = std::stoi(qtyInput);
                        if (q > 0) {
                            qty = q;
                        } else {
                            std::cout << "Quantity must be greater than 0. Defaulting to 1.\n";
                        }
                    } catch (...) {
                        std::cout << "Invalid quantity number. Defaulting to 1.\n";
                    }
                }
            }

            // Check if product is already in cart
            bool alreadyInCart = false;
            for (auto& line : cart) {
                if (line.product.barcode == code) {
                    line.quantity += qty;
                    line.lineTotal = line.quantity * line.product.price;
                    alreadyInCart = true;
                    break;
                }
            }

            if (!alreadyInCart) {
                OrderLineItem newItem;
                newItem.product = *product;
                newItem.quantity = qty;
                newItem.lineTotal = qty * product->price;
                cart.push_back(newItem);
            }

            totalItemsCount += qty;
            runningTotal += qty * product->price;

            std::cout << ">> ADDED: " << qty << "x " << product->name << "\n";
            std::cout << ">> Line Subtotal: $" << std::fixed << std::setprecision(2) << (qty * product->price) << "\n";
            std::cout << ">> New Running Total: $" << std::fixed << std::setprecision(2) << runningTotal << "\n";

        } else if (choice == "2") {
            // Checkout
            break;
        } else if (choice == "3") {
            std::cout << "Are you sure you want to discard and cancel this order? (y/n): ";
            std::string confirm;
            if (std::getline(std::cin, confirm) && (confirm == "y" || confirm == "Y")) {
                std::cout << "[ORDER DISCARDED] Order session cancelled.\n";
                return;
            }
        } else {
            std::cout << "Invalid choice. Please select 1, 2, or 3.\n";
        }
    }

    if (cart.empty()) {
        std::cout << "\n[EMPTY ORDER] No items were added to the order.\n";
        return;
    }

    // Deduct stock from Product.txt on disk
    std::string stockUpdateMsg;
    catalog.deductStockAndSave(cart, &stockUpdateMsg);
    std::cout << "\n>> [INVENTORY UPDATE] " << stockUpdateMsg << "\n";

    // -------------------------------------------------------
    // End of Order Summary Receipt
    // -------------------------------------------------------
    std::cout << "\n";
    std::cout << "=================================================================\n";
    std::cout << "                     FINAL ORDER SUMMARY                         \n";
    std::cout << "=================================================================\n";
    std::cout << std::left  << std::setw(10) << "Barcode"
              << std::left  << std::setw(30) << "Product Name"
              << std::right << std::setw(6)  << "Qty"
              << std::right << std::setw(11) << "Unit Price"
              << std::right << std::setw(12) << "Line Total" << "\n";
    std::cout << "-----------------------------------------------------------------\n";

    for (const auto& item : cart) {
        std::string displayName = item.product.name;
        if (displayName.length() > 28) {
            displayName = displayName.substr(0, 25) + "...";
        }

        std::ostringstream ssUnit, ssLine;
        ssUnit << "$" << std::fixed << std::setprecision(2) << item.product.price;
        ssLine << "$" << std::fixed << std::setprecision(2) << item.lineTotal;

        std::cout << std::left  << std::setw(10) << item.product.barcode
                  << std::left  << std::setw(30) << displayName
                  << std::right << std::setw(6)  << item.quantity
                  << std::right << std::setw(11) << ssUnit.str()
                  << std::right << std::setw(12) << ssLine.str() << "\n";
    }

    std::ostringstream ssGrand;
    ssGrand << "$" << std::fixed << std::setprecision(2) << runningTotal;

    std::cout << "-----------------------------------------------------------------\n";
    std::cout << std::left  << std::setw(40) << "TOTAL QUANTITY:"
              << std::right << std::setw(6)  << totalItemsCount << "\n";
    std::cout << std::left  << std::setw(46) << "GRAND TOTAL:"
              << std::right << std::setw(23) << ssGrand.str() << "\n";
    std::cout << "=================================================================\n";
    std::cout << "Thank you for shopping with us! Transaction complete.\n\n";
}

// -------------------------------------------------------
// Application Entry Point
// -------------------------------------------------------
int main() {
    ProductCatalog catalog;
    catalog.load();

    std::cout << "======================================================\n";
    std::cout << "       Point of Sale (POS) Barcode Scanner App        \n";
    std::cout << "          MSVC C++17 | Code 39 Camera Engine          \n";
    std::cout << "======================================================\n";
    std::cout << "Catalog loaded: " << catalog.count() << " items available.\n";

    while (true) {
        std::cout << "\n============================================\n";
        std::cout << "                 MAIN MENU                  \n";
        std::cout << "============================================\n";
        std::cout << "  1. Product Description Mode (Lookup Item)\n";
        std::cout << "  2. Order Recording Mode (Checkout Session)\n";
        std::cout << "  3. Exit POS Application\n";
        std::cout << "--------------------------------------------\n";
        std::cout << "Select mode (1-3): ";

        std::string choice;
        if (!std::getline(std::cin, choice)) break;
        while (!choice.empty() && (choice.back() == '\r' || choice.back() == ' ')) choice.pop_back();

        if (choice == "1") {
            runProductDescriptionMode(catalog);
        } else if (choice == "2") {
            runOrderRecordingMode(catalog);
        } else if (choice == "3" || choice == "exit" || choice == "q" || choice == "Q") {
            std::cout << "\nExiting POS system. Have a great day!\n";
            break;
        } else {
            std::cout << "Invalid choice. Please select 1, 2, or 3.\n";
        }
    }

    return 0;
}
