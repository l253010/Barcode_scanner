// camera_scanner.cpp
// -------------------------------------------------------
// Implementation of Reusable Camera Barcode Scanning Module.
// -------------------------------------------------------

#include "camera_scanner.h"
#include "BarcodeDecoder.h"

#include <chrono>
#include <iostream>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4127 4244 5054 4819 4996)
#endif

#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

// Helper: safely release camera and preview window
static void closeCamera(cv::VideoCapture& cap, const ScanOptions& options) {
    if (cap.isOpened()) {
        cap.release();
    }
    if (options.showPreview) {
        cv::destroyWindow(options.windowTitle);
        cv::waitKey(1); // Flush Win32 window message loop
    }
}

// Helper: render visual guide box and status messages on camera preview frame
static void drawOverlay(cv::Mat& frame,
                        const std::string& candidateCode,
                        int matches,
                        int requiredMatches,
                        int secondsRemaining,
                        const std::string& warningMsg,
                        bool isConfirmed)
{
    int frameW = frame.cols;
    int frameH = frame.rows;

    // Viewfinder target box in center of frame
    int boxW = static_cast<int>(frameW * 0.72);
    int boxH = static_cast<int>(frameH * 0.38);
    int boxX = (frameW - boxW) / 2;
    int boxY = (frameH - boxH) / 2;

    cv::Scalar boxColor;
    if (isConfirmed) {
        boxColor = cv::Scalar(0, 255, 0); // Green: Confirmed
    } else if (matches > 0) {
        boxColor = cv::Scalar(0, 230, 255); // Yellow: Confirming
    } else {
        boxColor = cv::Scalar(255, 180, 0); // Cyan/Blue: Searching
    }

    cv::Rect roi(boxX, boxY, boxW, boxH);
    cv::rectangle(frame, roi, boxColor, 2);

    // Corner targeting brackets for enhanced visibility
    int cornerLen = 18;
    int thick = 3;
    // Top-left
    cv::line(frame, cv::Point(boxX, boxY), cv::Point(boxX + cornerLen, boxY), boxColor, thick);
    cv::line(frame, cv::Point(boxX, boxY), cv::Point(boxX, boxY + cornerLen), boxColor, thick);
    // Top-right
    cv::line(frame, cv::Point(boxX + boxW, boxY), cv::Point(boxX + boxW - cornerLen, boxY), boxColor, thick);
    cv::line(frame, cv::Point(boxX + boxW, boxY), cv::Point(boxX + boxW, boxY + cornerLen), boxColor, thick);
    // Bottom-left
    cv::line(frame, cv::Point(boxX, boxY + boxH), cv::Point(boxX + cornerLen, boxY + boxH), boxColor, thick);
    cv::line(frame, cv::Point(boxX, boxY + boxH), cv::Point(boxX, boxY + boxH - cornerLen), boxColor, thick);
    // Bottom-right
    cv::line(frame, cv::Point(boxX + boxW, boxY + boxH), cv::Point(boxX + boxW - cornerLen, boxY + boxH), boxColor, thick);
    cv::line(frame, cv::Point(boxX + boxW, boxY + boxH), cv::Point(boxX + boxW, boxY + boxH - cornerLen), boxColor, thick);

    // Semi-transparent header bar
    cv::Mat overlay = frame.clone();
    cv::rectangle(overlay, cv::Rect(0, 0, frameW, 36), cv::Scalar(30, 30, 30), cv::FILLED);
    cv::rectangle(overlay, cv::Rect(0, frameH - 32, frameW, 32), cv::Scalar(30, 30, 30), cv::FILLED);
    cv::addWeighted(overlay, 0.65, frame, 0.35, 0, frame);

    // Status text in top banner
    if (isConfirmed) {
        std::string text = "CONFIRMED: " + candidateCode;
        cv::putText(frame, text, cv::Point(16, 25), cv::FONT_HERSHEY_SIMPLEX, 0.70, cv::Scalar(0, 255, 0), 2);
    } else if (matches > 0) {
        std::string text = "Verifying: " + candidateCode + " (" +
                           std::to_string(matches) + "/" + std::to_string(requiredMatches) + ")";
        cv::putText(frame, text, cv::Point(16, 25), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 230, 255), 2);
    } else if (!warningMsg.empty()) {
        cv::putText(frame, warningMsg, cv::Point(16, 25), cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(0, 0, 255), 2);
    } else {
        cv::putText(frame, "Align Code 39 Barcode in Guide Box", cv::Point(16, 25),
                    cv::FONT_HERSHEY_SIMPLEX, 0.60, cv::Scalar(240, 240, 240), 1);
    }

    // Bottom banner info (ESC instructions and timeout)
    std::string footer = "Press [ESC] to Cancel";
    if (secondsRemaining >= 0) {
        footer += "  |  Timeout in " + std::to_string(secondsRemaining) + "s";
    }
    cv::putText(frame, footer, cv::Point(16, frameH - 10), cv::FONT_HERSHEY_SIMPLEX, 0.50, cv::Scalar(200, 200, 200), 1);
}

// -------------------------------------------------------
// scanBarcodeFromCamera()
// -------------------------------------------------------
ScanOutcome scanBarcodeFromCamera(const ScanOptions& options) {
    // 1. Open webcam using cv::CAP_DSHOW for direct DirectShow capture on Windows
    cv::VideoCapture cap(options.cameraIndex, cv::CAP_DSHOW);
    if (!cap.isOpened()) {
        // Fallback attempt with default backend
        cap.open(options.cameraIndex);
    }

    if (!cap.isOpened()) {
        return { false, "",
            "Camera Error: Could not open camera device (Index " +
            std::to_string(options.cameraIndex) + "). Check camera connection, privacy permissions, or close other apps using the webcam." };
    }

    // Set standard 640x480 resolution for optimal scanning speed
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

    if (options.showPreview) {
        cv::namedWindow(options.windowTitle, cv::WINDOW_AUTOSIZE);
    }

    const auto startTime = std::chrono::steady_clock::now();
    const int targetMatches = (options.confirmationFrames > 0) ? options.confirmationFrames : 1;

    std::string candidateCode;
    int consecutiveMatches = 0;
    int emptyFrameCount = 0;
    std::string lastValidationError;

    cv::Mat frame;
    cv::Mat gray;

    while (true) {
        // Check timeout
        const auto now = std::chrono::steady_clock::now();
        const double elapsed = std::chrono::duration<double>(now - startTime).count();

        if (options.timeoutSeconds > 0 && elapsed >= options.timeoutSeconds) {
            closeCamera(cap, options);
            if (!lastValidationError.empty()) {
                return { false, "", "Validation Error: Decoded barcode is invalid (" + lastValidationError + ")." };
            }
            return { false, "",
                "Timeout Error: Scanning timed out after " +
                std::to_string(options.timeoutSeconds) + " seconds without a confirmed barcode." };
        }

        // Grab next camera frame
        if (!cap.read(frame) || frame.empty()) {
            emptyFrameCount++;
            if (emptyFrameCount > 45) { // Consecutive frame capture failures
                closeCamera(cap, options);
                return { false, "", "Camera Error: Failed to capture video frames from camera device." };
            }
            if (cv::waitKey(15) == 27) {
                closeCamera(cap, options);
                return { false, "", "Scan Cancelled: Scanning was aborted by user." };
            }
            continue;
        }
        emptyFrameCount = 0;

        // Convert captured BGR frame to 8-bit Grayscale
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

        // Attempt Code 39 decoding using decodeGrayscale
        DecodeResult dec = BarcodeDecoder::decodeGrayscale(
            gray.data, gray.cols, gray.rows, static_cast<int>(gray.step));

        if (dec.success) {
            if (dec.code == candidateCode) {
                consecutiveMatches++;
            } else {
                candidateCode = dec.code;
                consecutiveMatches = 1;
            }

            // Check if confirmation threshold is reached
            if (consecutiveMatches >= targetMatches) {
                if (options.showPreview) {
                    drawOverlay(frame, candidateCode, consecutiveMatches, targetMatches, -1, "", true);
                    cv::imshow(options.windowTitle, frame);
                    cv::waitKey(250); // Visual confirmation flash
                }
                closeCamera(cap, options);
                return { true, candidateCode, "" };
            }
        } else {
            if (!dec.errorMessage.empty() && dec.errorMessage.find("Validation Error") != std::string::npos) {
                lastValidationError = dec.errorMessage;
            }
            consecutiveMatches = 0;
        }

        // Display live preview window
        if (options.showPreview) {
            int secondsRemaining = (options.timeoutSeconds > 0) ?
                static_cast<int>(options.timeoutSeconds - elapsed) : -1;
            drawOverlay(frame, candidateCode, consecutiveMatches, targetMatches, secondsRemaining, lastValidationError, false);
            cv::imshow(options.windowTitle, frame);

            int key = cv::waitKey(15);
            if (key == 27) { // 27 = ESC key
                closeCamera(cap, options);
                return { false, "", "Scan Cancelled: Scanning was aborted by user." };
            }
        } else {
            int key = cv::waitKey(1);
            if (key == 27) {
                closeCamera(cap, options);
                return { false, "", "Scan Cancelled: Scanning was aborted by user." };
            }
        }
    }
}
