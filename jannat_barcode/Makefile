# Makefile — Barcode Generator (jannat_barcode)
# -------------------------------------------------------
# Project structure (5 files):
#   main.mm              — app entry point + AppDelegate + BarcodeView
#   BarcodeGenerator.cpp — Code 39 encoding, validation
#   BarcodeGenerator.h   — class/struct declarations
#   test_validation.cpp  — command-line tests
#   Makefile             — this file
#
# Requirements: macOS with Xcode Command Line Tools installed.
#   Check with: xcode-select -p
#
# Usage:
#   make            → build everything (GUI app + test binary)
#   make app        → build only the GUI application
#   make test       → build and run all tests
#   make run        → build and launch the GUI application
#   make clean      → remove build artifacts
# -------------------------------------------------------

# Compiler and flags
CXX      = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra

# macOS frameworks needed for the GUI
FRAMEWORKS = -framework Cocoa -framework CoreGraphics

# Source files
GUI_SOURCES  = main.mm BarcodeGenerator.cpp
TEST_SOURCES = test_validation.cpp BarcodeGenerator.cpp

# Output names
APP_BINARY  = BarcodeApp
TEST_BINARY = test_validation

# -------------------------------------------------------
# Default target: build both
# -------------------------------------------------------
all: app test_binary

# -------------------------------------------------------
# Build the GUI application
# -------------------------------------------------------
app: $(GUI_SOURCES) BarcodeGenerator.h
	@echo ""
	@echo ">>> Building GUI application: $(APP_BINARY)"
	$(CXX) $(CXXFLAGS) $(FRAMEWORKS) $(GUI_SOURCES) -o $(APP_BINARY)
	@echo ">>> Build complete: ./$(APP_BINARY)"
	@echo ""

# -------------------------------------------------------
# Build the command-line test binary
# -------------------------------------------------------
test_binary: $(TEST_SOURCES) BarcodeGenerator.h
	@echo ""
	@echo ">>> Building test binary: $(TEST_BINARY)"
	$(CXX) $(CXXFLAGS) $(TEST_SOURCES) -o $(TEST_BINARY)
	@echo ">>> Build complete: ./$(TEST_BINARY)"
	@echo ""

# -------------------------------------------------------
# Build and run validation tests
# -------------------------------------------------------
test: test_binary
	@echo ""
	@echo ">>> Running validation tests..."
	@echo ""
	./$(TEST_BINARY)

# -------------------------------------------------------
# Build and launch the GUI application
# -------------------------------------------------------
run: app
	./$(APP_BINARY)

# -------------------------------------------------------
# Remove build artifacts
# -------------------------------------------------------
clean:
	@echo ">>> Cleaning build artifacts..."
	rm -f $(APP_BINARY) $(TEST_BINARY)
	@echo ">>> Done."

.PHONY: all app test_binary test run clean
