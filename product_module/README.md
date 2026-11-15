# Product Management and Billing Calculation Module

A modular, reusable, and pure C++17 engine for product catalog management, barcode-based lookup, inventory stock validation, and billing/tax/discount calculations for the Barcode Scanner project.

---

## Zero External & Zero Prohibited Headers Policy

This module strictly eliminates the following 5 C++ standard library headers:
1. **`<vector>`** — Replaced with [`DynamicArray<T>`](file:///C:/Users/EMAN/Documents/GitHub/Barcode_scanner/Barcode_scanner/product_module/DynamicArray.h), a custom, completely header-free dynamic array with exponential capacity doubling, Rule of Five memory management, manual move/forward semantics (eliminating `<cstddef>` and `<utility>`), and complete self-referential push_back safety.
2. **`<map>`** — Replaced with an array-based contiguous catalog structure inside `ProductManager` that preserves insertion order and eliminates tree/node allocation overhead.
3. **`<sstream>`** — Replaced with custom string manipulation routines (`splitString`, `formatDouble`, `padLeft`, `padRight`) operating directly with basic `std::string`.
4. **`<cassert>`** — Replaced with test framework assertions and structured condition checks (`recordTest`).
5. **`<cctype>`** — Replaced with manual character classification (`isWhitespaceChar`).

Zero external libraries, packages, or frameworks are required.

---

## File Structure

```text
product_module/
├── DynamicArray.h            # Custom lightweight dynamic array template (replaces std::vector)
├── Product.h                 # Product entity definition & interface
├── Product.cpp               # Product specification & string formatting implementation
├── ProductManager.h          # Catalog file handling & inventory search interface
├── ProductManager.cpp        # File loading, parsing, and stock management implementation
├── Billing.h                 # BillItem, BillDiscount, and Bill class definitions
├── Billing.cpp               # Financial calculation, discount, tax, and receipt engine
├── Product.txt               # Sample product catalog text database
├── test_product_module.cpp   # Comprehensive automated test suite (88 test assertions)
├── build_and_run_tests.bat   # Windows MSVC automated build & test script
└── README.md                 # Technical module documentation
```

---

## Core Capabilities

### 1. Product File Handling (`Product.txt`)
- Reads and parses delimited records with the format:
  `Barcode|ProductID|Name|Category|Price|Stock|TaxRate|Description`
- Also accepts comma-separated (CSV) records.
- Automatically ignores blank lines and comments starting with `#`.
- Tolerant against corrupted or malformed lines, logging warnings while preserving valid records.

### 2. Barcode Searching
- Exact, sanitized string matching to avoid partial or incorrect barcode collisions.
- Search directly through contiguous dynamic array memory.
- Returns structured `ProductSearchResult` for direct GUI feedback.

### 3. Complete Specifications
Retrieves:
- **Barcode** (6-digit Code 39 format, e.g. `100001`, `123456`)
- **Product ID** (SKU, e.g. `PRD-101`)
- **Product Name**
- **Description**
- **Category** (e.g. `Dairy`, `Beverages`, `Bakery`)
- **Price** (Unit price)
- **Available Stock** (Quantity in inventory)
- **Tax Rate** (Percentage, e.g. `5.0` for 5%, `0.0` for tax-exempt)

### 4. Stock Management
- Validates purchase quantities before adding to bill.
- Rejects zero or negative quantities.
- Prevents orders exceeding available stock.
- Calculates and projects remaining stock accurately.
- `finalizeAndCommitStock()` performs an atomic two-phase commit: pre-validates all bill items before deducting inventory.

### 5. Billing Calculation Engine
- **Item Subtotal:** `unitPrice * quantity`
- **Line-item Discounts:** Optional fixed discount per line item.
- **Bill-level Discounts:** Supports percentage (`%`) or fixed currency (`$`) discounts with automatic clamping to avoid negative balances.
- **Tax Calculations:** Correctly computes tax on the net discounted taxable amount per item, supporting tax-exempt items (`0%`) alongside taxable goods.
- **Final Payable Amount:** `(Subtotal - Total Discount) + Total Tax`.

### 6. GUI-Ready Structured Outputs
- `ProductSearchResult`: Formatted lookup result with status and error message.
- `StockValidationResult`: Detailed quantity verification and remaining stock projection.
- `BillSummary`: Financial summary DTO (distinct items, units, raw subtotal, discounts, taxable base, tax, total payable).
- `Bill::generateReceipt()`: Produces a fully formatted text receipt for display in text boxes or printer spoolers.

---

## Compilation and Testing

### Using the Automated Batch Script (Windows)
Run from the command prompt inside `product_module/`:
```cmd
build_and_run_tests.bat
```

### Manual Compilation with MSVC
```cmd
cl /nologo /EHsc /std:c++17 /W4 Product.cpp ProductManager.cpp Billing.cpp test_product_module.cpp /Fe:test_product_module.exe
test_product_module.exe
```

---

## Test Suite Coverage
The included `test_product_module.cpp` executes **88 automated test assertions** across 8 test suites without using `<cassert>`:
1. **File Handling:** Missing file handling, empty paths, parsing valid records, skipping malformed lines.
2. **Product Searching:** Existing barcode lookup, Code 39 6-digit barcode lookup, nonexistent barcode handling, partial match rejection.
3. **Product Specifications:** Retrieval and validation of all 8 core product attributes.
4. **Stock Management:** Quantity validation, insufficient stock rejection, zero/negative rejection, in-memory reduction, restock.
5. **Billing Calculations:** Single-item, multi-item, item-level discounts, bill-level percentage and fixed discounts, multi-tier tax computations.
6. **Inventory Commitment:** Atomic multi-item checkout stock deduction.
7. **GUI Data Structures:** `BillSummary` DTO, tabulated item rows, complete receipt generation.
8. **DynamicArray Lifecycle & Memory Safety:** Default construction, insertion and resizing, self-referential push_back at full capacity, copy construction and deep-copy independence, copy self-assignment, move construction, move assignment, self-move assignment, empty-array operations, clear() and pop_back() resource cleanup, boundary erasures (first, middle, last), large-scale 1,200 element resizing, and destruction of moved-from objects.
