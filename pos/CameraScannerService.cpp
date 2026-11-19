// CameraScannerService.cpp
// ----------------------------------------------------------------------------
// Real Camera Scanner Service Implementation for POS System.
// ----------------------------------------------------------------------------

#include "CameraScannerService.h"
#include "camera_scanner.h"

CameraScannerService::CameraScannerService() = default;
CameraScannerService::~CameraScannerService() = default;

bool CameraScannerService::isCameraAvailable() const {
    return true;
}

bool CameraScannerService::startCamera() {
    m_scanning = true;
    return true;
}

void CameraScannerService::stopCamera() {
    m_scanning = false;
}

bool CameraScannerService::isScanning() const {
    return m_scanning;
}

BarcodeScanResult CameraScannerService::captureAndScan() {
    m_scanning = true;
    ScanOptions options;
    options.cameraIndex = 0;
    options.timeoutSeconds = 20;
    options.confirmationFrames = 2; // Fast 2-frame confirmation
    options.showPreview = true;
    options.windowTitle = "POS Optical Scanner - Live Barcode Camera (ESC to Cancel)";

    ScanOutcome outcome = scanBarcodeFromCamera(options);
    m_scanning = false;

    BarcodeScanResult result;
    result.success = outcome.success;
    result.barcode = outcome.code;
    result.errorMessage = outcome.error;
    return result;
}
