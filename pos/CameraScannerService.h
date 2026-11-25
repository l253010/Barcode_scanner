// CameraScannerService.h
// ----------------------------------------------------------------------------
// Real Camera Scanner Service Implementation for POS System.
// Integrates OpenCV live camera capture and BarcodeDecoder with the POS GUI.
// ----------------------------------------------------------------------------

#pragma once

#include "POSInterfaces.h"

class CameraScannerService : public ICameraScannerService {
public:
    CameraScannerService();
    ~CameraScannerService() override;

    bool isCameraAvailable() const override;
    bool startCamera() override;
    void stopCamera() override;
    bool isScanning() const override;
    BarcodeScanResult captureAndScan() override;

private:
    bool m_scanning{false};
};
