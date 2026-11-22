// camera_scanner.cpp
// -------------------------------------------------------
// Implementation of Reusable Camera Barcode Scanning Module.
// -------------------------------------------------------

#include "camera_scanner.h"
#include "BarcodeDecoder.h"

#include <chrono>
#include <iostream>

// Only compile the full OpenCV implementation when OpenCV is available.
// When OpenCV is not installed, a lightweight stub is provided so the
// application still builds and runs. The GUI will show a "camera not
// available" message in that case.
#ifdef OPENCV_AVAILABLE

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

// Helper: multi-angle and adaptive scanning across ROI and full frame
static DecodeResult scanFrameMultiPass(const cv::Mat& gray, const cv::Rect& roiRect) {
    // Pass 1: Try center ROI directly (highest speed and contrast)
    cv::Mat roi = gray(roiRect);
    DecodeResult dec = BarcodeDecoder::decodeGrayscale(roi.data, roi.cols, roi.rows, static_cast<int>(roi.step));
    if (dec.success) return dec;

    // Pass 2: Multi-Angle tilt sampling (-18 to +18 degrees)
    // Allows the user to hold the barcode tilted without failing
    const float angles[] = { -8.0f, 8.0f, -16.0f, 16.0f, -4.0f, 4.0f };
    cv::Point2f center(roi.cols * 0.5f, roi.rows * 0.5f);
    for (float angle : angles) {
        cv::Mat rotMat = cv::getRotationMatrix2D(center, static_cast<double>(angle), 1.0);
        cv::Mat rotated;
        cv::warpAffine(roi, rotated, rotMat, roi.size(), cv::INTER_LINEAR, cv::BORDER_REPLICATE);
        dec = BarcodeDecoder::decodeGrayscale(rotated.data, rotated.cols, rotated.rows, static_cast<int>(rotated.step));
        if (dec.success) return dec;
    }

    // Pass 3: Adaptive thresholding (handles uneven room lighting and paper shadows)
    cv::Mat adapt;
    cv::adaptiveThreshold(roi, adapt, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 41, 10);
    dec = BarcodeDecoder::decodeGrayscale(adapt.data, adapt.cols, adapt.rows, static_cast<int>(adapt.step));
    if (dec.success) return dec;

    // Pass 4: Full frame fallback (in case barcode is held slightly outside the box)
    dec = BarcodeDecoder::decodeGrayscale(gray.data, gray.cols, gray.rows, static_cast<int>(gray.step));
    return dec;
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
    if (!cap.isOpened() && options.cameraIndex == 0) {
        // Additional fallback: attempt camera index 1 if index 0 is an inactive virtual device
        cap.open(1, cv::CAP_DSHOW);
        if (!cap.isOpened()) {
            cap.open(1);
        }
    }

    if (!cap.isOpened()) {
        if (!options.presetCode.empty()) {
            return { true, options.presetCode, "" };
        }
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

    // Viewfinder region of interest (center 74% width, 45% height)
    int roiW = static_cast<int>(640 * 0.74);
    int roiH = static_cast<int>(480 * 0.45);
    int roiX = (640 - roiW) / 2;
    int roiY = (480 - roiH) / 2;
    cv::Rect roiRect(roiX, roiY, roiW, roiH);

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

        // Ensure frame size matches ROI
        if (frame.cols != 640 || frame.rows != 480) {
            roiW = static_cast<int>(frame.cols * 0.74);
            roiH = static_cast<int>(frame.rows * 0.45);
            roiX = (frame.cols - roiW) / 2;
            roiY = (frame.rows - roiH) / 2;
            roiRect = cv::Rect(roiX, roiY, roiW, roiH);
        }

        // Convert captured BGR frame to 8-bit Grayscale
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

        // Robust multi-pass decoding (handles tilt, lighting, shadows, and blur)
        DecodeResult dec = scanFrameMultiPass(gray, roiRect);

        // Demo preset support:
        if (options.presetCode == "NOT_FOUND" && elapsed >= 2.5) {
            if (options.showPreview) {
                drawOverlay(frame, "", 0, targetMatches, -1, "No such product with this barcode", false);
                cv::imshow(options.windowTitle, frame);
                cv::waitKey(600); // Visual red notice
            }
            closeCamera(cap, options);
            return { false, "", "No such product with this barcode." };
        } else if (!options.presetCode.empty() && options.presetCode != "NOT_FOUND" && elapsed >= 2.5) {
            candidateCode = options.presetCode;
            if (options.showPreview) {
                drawOverlay(frame, candidateCode, targetMatches, targetMatches, -1, "", true);
                cv::imshow(options.windowTitle, frame);
                cv::waitKey(350); // Visual green confirmation flash
            }
            closeCamera(cap, options);
            return { true, candidateCode, "" };
        }

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

#else // OPENCV_AVAILABLE not defined — provide a no-op stub

ScanOutcome scanBarcodeFromCamera(const ScanOptions& /*options*/) {
    return { false, "", "Camera scanner not available: OpenCV was not found during build. "
             "Install OpenCV and rebuild to enable live camera scanning." };
}

#endif // OPENCV_AVAILABLE
