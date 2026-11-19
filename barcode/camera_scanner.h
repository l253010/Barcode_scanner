// camera_scanner.h
// -------------------------------------------------------
// Reusable Camera Barcode Scanning Module.
//
// Powered by OpenCV (cv::VideoCapture with cv::CAP_DSHOW)
// and BarcodeDecoder::decodeGrayscale.
//
// Key features:
//   - Multi-frame confirmation against false/partial reads.
//   - Live preview window with viewfinder overlay and status.
//   - Clean cancellation via ESC key.
//   - Configurable timeout and camera index.
//   - Comprehensive error reporting for all failure modes.
// -------------------------------------------------------

#pragma once

#include <string>

// -------------------------------------------------------
// ScanOptions
// Configuration options for live camera barcode scanning.
// -------------------------------------------------------
struct ScanOptions {
    int cameraIndex = 0;             // Camera device index (0 for default webcam)
    int timeoutSeconds = 15;         // Timeout duration in seconds (<= 0 for unlimited)
    int confirmationFrames = 3;      // Number of consecutive matching frames required to confirm
    bool showPreview = true;         // Whether to show OpenCV live preview window
    std::string windowTitle = "POS Barcode Scanner - Live Camera"; // Window title
};

// -------------------------------------------------------
// ScanOutcome
// Result of a camera scanning attempt.
// -------------------------------------------------------
struct ScanOutcome {
    bool success = false;            // true if a valid Code 39 barcode was confirmed
    std::string code;                // confirmed 6-digit barcode string (e.g., "123456")
    std::string error;               // descriptive error message if success == false
};

// -------------------------------------------------------
// scanBarcodeFromCamera()
// Opens the camera using cv::VideoCapture(cameraIndex, cv::CAP_DSHOW),
// displays a live preview with a viewfinder targeting box,
// decodes frames using BarcodeDecoder::decodeGrayscale,
// confirms reads across multiple consecutive frames to prevent false positives,
// and supports ESC cancellation as well as timeouts.
// -------------------------------------------------------
ScanOutcome scanBarcodeFromCamera(const ScanOptions& options = ScanOptions{});
