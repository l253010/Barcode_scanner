// scanner_app.cpp
// -------------------------------------------------------
// Desktop GUI Application for Barcode Image Uploading & Camera Scanning.
//
// Features:
//   1. "Upload Image" — Select a PNG, JPG, or BMP image file via GetOpenFileNameW.
//   2. "Scan Using Camera" — Open laptop webcam via Windows Media Foundation.
//   3. Live camera preview displayed smoothly using StretchDIBits.
//   4. "Capture / Scan" — Captures current frame to unique temporary BMP file,
//      flushes and closes file, calls BarcodeDecoder::decodeImage(), and deletes file.
//   5. "Stop Camera" — Signals thread, waits for termination, and safely cleans up.
//   6. "Clear" — Resets preview and results.
//
// Thread Safety & Architecture:
//   - Camera worker thread initializes and releases COM and Media Foundation locally.
//   - Shared frame buffer protected by CRITICAL_SECTION (pixels, width, height, size).
//   - Worker thread communicates with GUI thread via PostMessageW only.
//   - GUI painting copies frame under lock and paints outside lock.
//   - Zero OpenCV, zero third-party barcode libraries.
// -------------------------------------------------------

#ifdef _WIN32

#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#include <gdiplus.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#include "BarcodeDecoder.h"

#ifdef _MSC_VER
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(linker, "/subsystem:windows /entry:wWinMainCRTStartup")
#endif

// Control identifiers
#define IDC_UPLOAD_BTN     201
#define IDC_CLEAR_BTN      202
#define IDC_PREVIEW_PANEL  203
#define IDC_TITLE_LABEL    204
#define IDC_SUBTITLE_LABEL 205
#define IDC_RESULT_LABEL   206
#define IDC_STATUS_LABEL   207
#define IDC_CAMERA_BTN     208
#define IDC_CAPTURE_BTN    209
#define IDC_STOP_BTN       210

// Custom Windows Messages for Thread-Safe GUI Communication
#define WM_APP_FRAME_READY     (WM_APP + 1)
#define WM_APP_CAMERA_STARTED  (WM_APP + 2)
#define WM_APP_CAMERA_STOPPED  (WM_APP + 3)
#define WM_APP_CAMERA_ERROR    (WM_APP + 4)

// -------------------------------------------------------
// Shared Frame Structure (Protected by Critical Section)
// -------------------------------------------------------
struct SharedFrame {
    BYTE* pixels;
    int   width;
    int   height;
    DWORD byteSize;
};

// Global synchronization and camera thread state
static CRITICAL_SECTION g_frameLock;
static SharedFrame      g_sharedFrame    = { nullptr, 0, 0, 0 };
static volatile LONG    g_cameraRunning  = 0; // 0 = stopped, 1 = running
static HANDLE           g_hCameraThread  = NULL;
static bool             g_isCameraActive = false;

// Global window handles
static HINSTANCE g_hInstance     = nullptr;
static HWND      g_hMainWindow   = nullptr;
static HWND      g_hTitleLabel   = nullptr;
static HWND      g_hSubtitleLbl  = nullptr;
static HWND      g_hUploadBtn    = nullptr;
static HWND      g_hCameraBtn    = nullptr;
static HWND      g_hCaptureBtn   = nullptr;
static HWND      g_hStopBtn      = nullptr;
static HWND      g_hClearBtn     = nullptr;
static HWND      g_hPreviewPanel = nullptr;
static HWND      g_hResultLabel  = nullptr;
static HWND      g_hStatusLabel  = nullptr;

// Fonts and UI resources
static HFONT     g_hTitleFont       = nullptr;
static HFONT     g_hSubtitleFont    = nullptr;
static HFONT     g_hButtonFont      = nullptr;
static HFONT     g_hResultFont      = nullptr;
static HFONT     g_hStatusFont      = nullptr;
static HFONT     g_hPlaceholderFont = nullptr;
static HBRUSH    g_hBgBrush         = nullptr;

// Uploaded image preview state (when not in camera mode)
static Gdiplus::Bitmap* g_pUploadedImage = nullptr;
static COLORREF         g_statusColor    = RGB(100, 100, 100);
static COLORREF         g_resultColor    = RGB(30, 30, 30);

// Simple inline helper
static inline int min_int(int a, int b) { return (a < b) ? a : b; }

// Camera pixel formats
enum CameraPixelFormat {
    CAM_FMT_UNKNOWN = 0,
    CAM_FMT_RGB32,
    CAM_FMT_YUY2,
    CAM_FMT_NV12,
    CAM_FMT_MJPEG
};

// Simple clamp helper
static inline BYTE ClampByte(int val) {
    return (BYTE)(val < 0 ? 0 : (val > 255 ? 255 : val));
}

// Distance helper for resolution selection
static inline int diff_target(int val, int target) {
    int d = val - target;
    return d < 0 ? -d : d;
}

// Priority ranking helper (RGB32 > YUY2 > NV12 > MJPEG)
static int GetFormatPriority(const GUID& subtype) {
    if (subtype == MFVideoFormat_RGB32) return 1;
    if (subtype == MFVideoFormat_YUY2)  return 2;
    if (subtype == MFVideoFormat_NV12)  return 3;
    if (subtype == MFVideoFormat_MJPG)  return 4;
    return 999;
}

// Convert YUY2 (4:2:2) to RGB32 (BGRA) using ITU-R BT.601 integer arithmetic
static void ConvertYUY2ToRGB32(const BYTE* pSrc, LONG srcStride, int width, int height, BYTE* pDst) {
    if (!pSrc || !pDst || width <= 0 || height <= 0 || srcStride < (width * 2)) return;

    for (int y = 0; y < height; ++y) {
        const BYTE* pSrcRow = pSrc + y * srcStride;
        BYTE* pDstRow = pDst + y * (width * 4);
        for (int x = 0; x < width; x += 2) {
            int y0 = pSrcRow[x * 2 + 0];
            int u  = pSrcRow[x * 2 + 1];
            int y1 = pSrcRow[x * 2 + 2];
            int v  = pSrcRow[x * 2 + 3];

            int c0 = y0 - 16;
            int c1 = y1 - 16;
            int d  = u - 128;
            int e  = v - 128;

            int r0 = (298 * c0 + 409 * e + 128) >> 8;
            int g0 = (298 * c0 - 100 * d - 208 * e + 128) >> 8;
            int b0 = (298 * c0 + 516 * d + 128) >> 8;

            pDstRow[x * 4 + 0] = ClampByte(b0);
            pDstRow[x * 4 + 1] = ClampByte(g0);
            pDstRow[x * 4 + 2] = ClampByte(r0);
            pDstRow[x * 4 + 3] = 255;

            if (x + 1 < width) {
                int r1 = (298 * c1 + 409 * e + 128) >> 8;
                int g1 = (298 * c1 - 100 * d - 208 * e + 128) >> 8;
                int b1 = (298 * c1 + 516 * d + 128) >> 8;

                pDstRow[(x + 1) * 4 + 0] = ClampByte(b1);
                pDstRow[(x + 1) * 4 + 1] = ClampByte(g1);
                pDstRow[(x + 1) * 4 + 2] = ClampByte(r1);
                pDstRow[(x + 1) * 4 + 3] = 255;
            }
        }
    }
}

// Convert NV12 (4:2:0 planar) to RGB32 (BGRA) using ITU-R BT.601 integer arithmetic
static void ConvertNV12ToRGB32(const BYTE* pSrc, LONG srcStride, int width, int height, BYTE* pDst) {
    if (!pSrc || !pDst || width <= 0 || height <= 0 || srcStride < width) return;

    const BYTE* pYPlane  = pSrc;
    const BYTE* pUVPlane = pSrc + srcStride * height;

    for (int y = 0; y < height; ++y) {
        const BYTE* pRowY  = pYPlane + y * srcStride;
        const BYTE* pRowUV = pUVPlane + (y / 2) * srcStride;
        BYTE* pDstRow = pDst + y * (width * 4);

        for (int x = 0; x < width; ++x) {
            int yVal = pRowY[x];
            int uvIdx = (x & ~1);
            int uVal = pRowUV[uvIdx + 0];
            int vVal = pRowUV[uvIdx + 1];

            int c = yVal - 16;
            int d = uVal - 128;
            int e = vVal - 128;

            int r = (298 * c + 409 * e + 128) >> 8;
            int g = (298 * c - 100 * d - 208 * e + 128) >> 8;
            int b = (298 * c + 516 * d + 128) >> 8;

            pDstRow[x * 4 + 0] = ClampByte(b);
            pDstRow[x * 4 + 1] = ClampByte(g);
            pDstRow[x * 4 + 2] = ClampByte(r);
            pDstRow[x * 4 + 3] = 255;
        }
    }
}

// Decode compressed MJPEG sample into uncompressed 32-bit RGB buffer via GDI+
static bool DecodeMJPEGToRGB32(const BYTE* pJpegData, DWORD cbJpegLength, BYTE*& pDst, int& curWidth, int& curHeight, DWORD& curByteSize) {
    if (!pJpegData || cbJpegLength < 4) return false;

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, cbJpegLength);
    if (!hMem) return false;

    void* pMem = GlobalLock(hMem);
    if (!pMem) {
        GlobalFree(hMem);
        return false;
    }
    memcpy(pMem, pJpegData, cbJpegLength);
    GlobalUnlock(hMem);

    IStream* pStream = nullptr;
    if (FAILED(CreateStreamOnHGlobal(hMem, TRUE, &pStream))) {
        GlobalFree(hMem);
        return false;
    }

    bool success = false;
    Gdiplus::Bitmap* pBmp = Gdiplus::Bitmap::FromStream(pStream);
    if (pBmp && pBmp->GetLastStatus() == Gdiplus::Ok) {
        UINT bmpW = pBmp->GetWidth();
        UINT bmpH = pBmp->GetHeight();
        if (bmpW > 0 && bmpH > 0 && bmpW <= 4096 && bmpH <= 4096) {
            DWORD neededSize = bmpW * bmpH * 4;
            if (!pDst || curWidth != (int)bmpW || curHeight != (int)bmpH) {
                delete[] pDst;
                pDst = new BYTE[neededSize];
                curWidth = (int)bmpW;
                curHeight = (int)bmpH;
                curByteSize = neededSize;
            }

            Gdiplus::Rect rc(0, 0, bmpW, bmpH);
            Gdiplus::BitmapData bmpData;
            if (pBmp->LockBits(&rc, Gdiplus::ImageLockModeRead, PixelFormat32bppRGB, &bmpData) == Gdiplus::Ok) {
                if (bmpData.Scan0) {
                    const BYTE* pScan0 = (const BYTE*)bmpData.Scan0;
                    for (UINT y = 0; y < bmpH; ++y) {
                        const BYTE* pSrcRow = pScan0 + y * bmpData.Stride;
                        BYTE* pDstRow = pDst + y * (bmpW * 4);
                        memcpy(pDstRow, pSrcRow, bmpW * 4);
                    }
                    success = true;
                }
                pBmp->UnlockBits(&bmpData);
            }
        }
        delete pBmp;
    }

    pStream->Release();
    return success;
}

// Forward declarations
static bool StopCameraWorker();

// -------------------------------------------------------
// Camera Worker Thread Function
//
// Fully owns its COM and Media Foundation initialization and lifetime.
// Never touches GUI controls directly; notifies GUI via PostMessageW.
// -------------------------------------------------------
static DWORD WINAPI CameraWorkerThread(LPVOID lpParam) {
    HWND hNotifyWnd = (HWND)lpParam;

    // 1. Initialize COM on this worker thread
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        PostMessageW(hNotifyWnd, WM_APP_CAMERA_ERROR, 0, (LPARAM)L"Failed to initialize COM on camera thread.");
        return 1;
    }

    // 2. Initialize Media Foundation on this worker thread
    hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) {
        CoUninitialize();
        PostMessageW(hNotifyWnd, WM_APP_CAMERA_ERROR, 0, (LPARAM)L"Failed to initialize Media Foundation.");
        return 1;
    }

    // 3. Enumerate video capture devices
    IMFAttributes* pAttributes = nullptr;
    IMFActivate** ppDevices = nullptr;
    UINT32 deviceCount = 0;
    IMFMediaSource* pMediaSource = nullptr;
    IMFSourceReader* pSourceReader = nullptr;

    hr = MFCreateAttributes(&pAttributes, 1);
    if (SUCCEEDED(hr)) {
        hr = pAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
    }
    if (SUCCEEDED(hr)) {
        hr = MFEnumDeviceSources(pAttributes, &ppDevices, &deviceCount);
    }

    if (FAILED(hr) || deviceCount == 0 || !ppDevices) {
        if (ppDevices) {
            for (UINT32 i = 0; i < deviceCount; ++i) if (ppDevices[i]) ppDevices[i]->Release();
            CoTaskMemFree(ppDevices);
        }
        if (pAttributes) pAttributes->Release();
        MFShutdown();
        CoUninitialize();
        PostMessageW(hNotifyWnd, WM_APP_CAMERA_ERROR, 0, (LPARAM)L"No camera detected on this computer.");
        return 1;
    }

    // Activate the first camera device
    hr = ppDevices[0]->ActivateObject(IID_PPV_ARGS(&pMediaSource));
    for (UINT32 i = 0; i < deviceCount; ++i) {
        if (ppDevices[i]) ppDevices[i]->Release();
    }
    CoTaskMemFree(ppDevices);
    ppDevices = nullptr;

    if (FAILED(hr) || !pMediaSource) {
        if (pAttributes) pAttributes->Release();
        MFShutdown();
        CoUninitialize();
        PostMessageW(hNotifyWnd, WM_APP_CAMERA_ERROR, 0, (LPARAM)L"Could not access camera (it may be in use by another app).");
        return 1;
    }

    if (pAttributes) { pAttributes->Release(); pAttributes = nullptr; }

    // Create Source Reader with video processing enabled
    IMFAttributes* pReaderAttrs = nullptr;
    hr = MFCreateAttributes(&pReaderAttrs, 2);
    if (SUCCEEDED(hr)) {
        pReaderAttrs->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
        // Attempt hardware transforms, ignore if unsupported
        pReaderAttrs->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
    }

    hr = MFCreateSourceReaderFromMediaSource(pMediaSource, pReaderAttrs, &pSourceReader);
    if (pReaderAttrs) { pReaderAttrs->Release(); pReaderAttrs = nullptr; }

    if (FAILED(hr) || !pSourceReader) {
        if (pMediaSource) { pMediaSource->Release(); pMediaSource = nullptr; }
        MFShutdown();
        CoUninitialize();
        PostMessageW(hNotifyWnd, WM_APP_CAMERA_ERROR, 0, (LPARAM)L"Failed to create Media Foundation source reader.");
        return 1;
    }

    // Enumerate native media types and rank candidates (Priority: RGB32 > YUY2 > NV12 > MJPEG)
    DWORD bestIndex = (DWORD)-1;
    int bestPriority = 999;
    UINT32 bestWidth = 0, bestHeight = 0;
    GUID bestSubtype = GUID_NULL;

    for (DWORD i = 0; ; ++i) {
        IMFMediaType* pNativeType = nullptr;
        hr = pSourceReader->GetNativeMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, i, &pNativeType);
        if (FAILED(hr)) break;

        GUID subtype = GUID_NULL;
        pNativeType->GetGUID(MF_MT_SUBTYPE, &subtype);

        UINT32 w = 0, h = 0;
        MFGetAttributeSize(pNativeType, MF_MT_FRAME_SIZE, &w, &h);

        int prio = GetFormatPriority(subtype);
        if (prio < 999 && w >= 320 && h >= 240 && w <= 4096 && h <= 4096) {
            bool isBetter = false;
            if (prio < bestPriority) {
                isBetter = true;
            } else if (prio == bestPriority) {
                // If same format priority, prefer standard scanning resolutions (around 1280x720)
                int currentScore = (int)w * (int)h;
                int bestScore    = (int)bestWidth * (int)bestHeight;
                int diffCur  = diff_target(currentScore, 921600);
                int diffBest = diff_target(bestScore, 921600);
                if (diffCur < diffBest) {
                    isBetter = true;
                }
            }

            if (isBetter) {
                bestIndex = i;
                bestPriority = prio;
                bestWidth = w;
                bestHeight = h;
                bestSubtype = subtype;
            }
        }
        pNativeType->Release();
    }

    if (bestIndex == (DWORD)-1) {
        pSourceReader->Release();
        pMediaSource->Release();
        MFShutdown();
        CoUninitialize();
        PostMessageW(hNotifyWnd, WM_APP_CAMERA_ERROR, 0, (LPARAM)L"No supported camera video format found (RGB32, YUY2, NV12, or MJPEG required).");
        return 1;
    }

    // Output format negotiation:
    CameraPixelFormat activeFormat = CAM_FMT_UNKNOWN;
    bool rgb32Negotiated = false;

    // 1. Attempt to request RGB32 via Media Foundation Video Processing
    IMFMediaType* pTargetType = nullptr;
    hr = MFCreateMediaType(&pTargetType);
    if (SUCCEEDED(hr)) {
        pTargetType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        pTargetType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
        MFSetAttributeSize(pTargetType, MF_MT_FRAME_SIZE, bestWidth, bestHeight);

        hr = pSourceReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, pTargetType);
        if (SUCCEEDED(hr)) {
            // Retrieve actual output type and verify subtype
            IMFMediaType* pActualType = nullptr;
            if (SUCCEEDED(pSourceReader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, &pActualType))) {
                GUID actualSubtype = GUID_NULL;
                if (SUCCEEDED(pActualType->GetGUID(MF_MT_SUBTYPE, &actualSubtype)) && actualSubtype == MFVideoFormat_RGB32) {
                    rgb32Negotiated = true;
                    activeFormat = CAM_FMT_RGB32;
                }
                pActualType->Release();
            }
        }
        pTargetType->Release();
    }

    // 2. If RGB32 could not be negotiated directly, fallback to setting the best native media type
    if (!rgb32Negotiated) {
        IMFMediaType* pNativeType = nullptr;
        hr = pSourceReader->GetNativeMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, bestIndex, &pNativeType);
        if (SUCCEEDED(hr)) {
            hr = pSourceReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, pNativeType);
            if (SUCCEEDED(hr)) {
                IMFMediaType* pActualType = nullptr;
                if (SUCCEEDED(pSourceReader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, &pActualType))) {
                    GUID actualSubtype = GUID_NULL;
                    if (SUCCEEDED(pActualType->GetGUID(MF_MT_SUBTYPE, &actualSubtype))) {
                        if (actualSubtype == MFVideoFormat_RGB32)      activeFormat = CAM_FMT_RGB32;
                        else if (actualSubtype == MFVideoFormat_YUY2) activeFormat = CAM_FMT_YUY2;
                        else if (actualSubtype == MFVideoFormat_NV12) activeFormat = CAM_FMT_NV12;
                        else if (actualSubtype == MFVideoFormat_MJPG) activeFormat = CAM_FMT_MJPEG;
                    }
                    pActualType->Release();
                }
            }
            pNativeType->Release();
        }
    }

    if (activeFormat == CAM_FMT_UNKNOWN) {
        pSourceReader->Release();
        pMediaSource->Release();
        MFShutdown();
        CoUninitialize();
        PostMessageW(hNotifyWnd, WM_APP_CAMERA_ERROR, 0, (LPARAM)L"Failed to configure camera video output format.");
        return 1;
    }

    // Query active video stream dimensions and default stride
    IMFMediaType* pCurrentType = nullptr;
    UINT32 frameWidth = 0, frameHeight = 0;
    LONG defaultStride = 0;
    if (SUCCEEDED(pSourceReader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, &pCurrentType))) {
        MFGetAttributeSize(pCurrentType, MF_MT_FRAME_SIZE, &frameWidth, &frameHeight);
        UINT32 strideVal = 0;
        if (SUCCEEDED(pCurrentType->GetUINT32(MF_MT_DEFAULT_STRIDE, &strideVal))) {
            defaultStride = (LONG)strideVal;
        }
        pCurrentType->Release();
    }

    // Validate dimensions and prevent integer overflow
    if (frameWidth == 0 || frameHeight == 0 || frameWidth > 4096 || frameHeight > 4096) {
        pSourceReader->Release();
        pMediaSource->Release();
        MFShutdown();
        CoUninitialize();
        PostMessageW(hNotifyWnd, WM_APP_CAMERA_ERROR, 0, (LPARAM)L"Invalid camera video frame dimensions.");
        return 1;
    }

    if (defaultStride <= 0) {
        if (activeFormat == CAM_FMT_RGB32)      defaultStride = frameWidth * 4;
        else if (activeFormat == CAM_FMT_YUY2) defaultStride = frameWidth * 2;
        else if (activeFormat == CAM_FMT_NV12) defaultStride = frameWidth;
    }

    DWORD expectedRgbSize = frameWidth * frameHeight * 4;

    // Camera is successfully initialized! Notify GUI thread
    PostMessageW(hNotifyWnd, WM_APP_CAMERA_STARTED, 0, 0);

    // Frame capture loop (reads at the camera's available frame rate)
    while (InterlockedCompareExchange(&g_cameraRunning, 1, 1) == 1) {
        DWORD streamIndex = 0, flags = 0;
        LONGLONG llTimeStamp = 0;
        IMFSample* pSample = nullptr;

        hr = pSourceReader->ReadSample(
            (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
            0,
            &streamIndex,
            &flags,
            &llTimeStamp,
            &pSample
        );

        if (FAILED(hr) || (flags & MF_SOURCE_READERF_ENDOFSTREAM)) {
            break;
        }

        if (pSample) {
            IMFMediaBuffer* pBuffer = nullptr;
            hr = pSample->ConvertToContiguousBuffer(&pBuffer);
            if (SUCCEEDED(hr) && pBuffer) {
                BYTE* pData = nullptr;
                DWORD cbLength = 0;
                hr = pBuffer->Lock(&pData, NULL, &cbLength);
                if (SUCCEEDED(hr) && pData && cbLength > 0) {
                    bool frameReady = false;

                    if (activeFormat == CAM_FMT_MJPEG) {
                        EnterCriticalSection(&g_frameLock);
                        frameReady = DecodeMJPEGToRGB32(pData, cbLength, g_sharedFrame.pixels, g_sharedFrame.width, g_sharedFrame.height, g_sharedFrame.byteSize);
                        LeaveCriticalSection(&g_frameLock);
                    } else {
                        EnterCriticalSection(&g_frameLock);
                        if (!g_sharedFrame.pixels || g_sharedFrame.width != (int)frameWidth || g_sharedFrame.height != (int)frameHeight) {
                            delete[] g_sharedFrame.pixels;
                            g_sharedFrame.pixels = new BYTE[expectedRgbSize];
                            g_sharedFrame.width  = (int)frameWidth;
                            g_sharedFrame.height = (int)frameHeight;
                            g_sharedFrame.byteSize = expectedRgbSize;
                        }

                        if (activeFormat == CAM_FMT_RGB32) {
                            if (cbLength >= (DWORD)(defaultStride * frameHeight)) {
                                if (defaultStride == (LONG)(frameWidth * 4)) {
                                    memcpy(g_sharedFrame.pixels, pData, expectedRgbSize);
                                } else {
                                    for (UINT32 y = 0; y < frameHeight; ++y) {
                                        memcpy(g_sharedFrame.pixels + y * (frameWidth * 4),
                                               pData + y * defaultStride,
                                               frameWidth * 4);
                                    }
                                }
                                frameReady = true;
                            }
                        } else if (activeFormat == CAM_FMT_YUY2) {
                            if (cbLength >= (DWORD)(defaultStride * frameHeight)) {
                                ConvertYUY2ToRGB32(pData, defaultStride, (int)frameWidth, (int)frameHeight, g_sharedFrame.pixels);
                                frameReady = true;
                            }
                        } else if (activeFormat == CAM_FMT_NV12) {
                            DWORD nv12Needed = (DWORD)(defaultStride * frameHeight + defaultStride * (frameHeight / 2));
                            if (cbLength >= nv12Needed) {
                                ConvertNV12ToRGB32(pData, defaultStride, (int)frameWidth, (int)frameHeight, g_sharedFrame.pixels);
                                frameReady = true;
                            }
                        }
                        LeaveCriticalSection(&g_frameLock);
                    }

                    if (frameReady) {
                        // Safely inform GUI thread that a new frame is ready
                        PostMessageW(hNotifyWnd, WM_APP_FRAME_READY, 0, 0);
                    }

                    pBuffer->Unlock();
                }
                pBuffer->Release();
            }
            pSample->Release();
        }

        Sleep(10); // cooperative yield
    }

    // Release all camera objects on this worker thread
    if (pSourceReader) { pSourceReader->Release(); pSourceReader = nullptr; }
    if (pMediaSource)  { pMediaSource->Release();  pMediaSource  = nullptr; }

    MFShutdown();
    CoUninitialize();

    PostMessageW(hNotifyWnd, WM_APP_CAMERA_STOPPED, 0, 0);
    return 0;
}

// -------------------------------------------------------
// Helper: Start the Camera
// -------------------------------------------------------
static void StartCamera() {
    if (g_cameraRunning == 1) return;

    // Reset uploaded preview image
    if (g_pUploadedImage) {
        delete g_pUploadedImage;
        g_pUploadedImage = nullptr;
    }

    SetWindowTextW(g_hStatusLabel, L"Connecting to camera...");
    g_statusColor = RGB(80, 80, 80);
    InvalidateRect(g_hStatusLabel, NULL, TRUE);

    InterlockedExchange(&g_cameraRunning, 1);
    g_hCameraThread = CreateThread(NULL, 0, CameraWorkerThread, g_hMainWindow, 0, NULL);
    if (!g_hCameraThread) {
        InterlockedExchange(&g_cameraRunning, 0);
        SetWindowTextW(g_hStatusLabel, L"❌ Error: Failed to create camera worker thread.");
        g_statusColor = RGB(210, 30, 30);
        InvalidateRect(g_hStatusLabel, NULL, TRUE);
    }
}

// -------------------------------------------------------
// Helper: Stop the Camera (Verified Thread-Safe Shutdown)
// -------------------------------------------------------
static bool StopCameraWorker() {
    if (InterlockedCompareExchange(&g_cameraRunning, 0, 1) == 1) {
        if (g_hCameraThread != NULL) {
            DWORD waitResult = WaitForSingleObject(g_hCameraThread, 3000);
            if (waitResult == WAIT_OBJECT_0) {
                // Thread cleanly terminated
                CloseHandle(g_hCameraThread);
                g_hCameraThread = NULL;
            } else if (waitResult == WAIT_TIMEOUT) {
                // Thread did not finish within timeout; do not delete memory unsafely
                return false;
            } else {
                CloseHandle(g_hCameraThread);
                g_hCameraThread = NULL;
            }
        }
    }

    g_isCameraActive = false;

    // Clear shared frame buffer under lock
    EnterCriticalSection(&g_frameLock);
    delete[] g_sharedFrame.pixels;
    g_sharedFrame.pixels   = nullptr;
    g_sharedFrame.width    = 0;
    g_sharedFrame.height   = 0;
    g_sharedFrame.byteSize = 0;
    LeaveCriticalSection(&g_frameLock);

    // Update UI controls
    EnableWindow(g_hCaptureBtn, FALSE);
    EnableWindow(g_hStopBtn, FALSE);
    EnableWindow(g_hCameraBtn, TRUE);
    EnableWindow(g_hUploadBtn, TRUE);

    SetWindowTextW(g_hStatusLabel, L"Camera stopped. Ready to scan or upload.");
    g_statusColor = RGB(100, 100, 100);

    InvalidateRect(g_hPreviewPanel, NULL, TRUE);
    InvalidateRect(g_hStatusLabel, NULL, TRUE);
    return true;
}

// -------------------------------------------------------
// Helper: Capture Frame, Write Unique BMP, and Decode
// -------------------------------------------------------
static void OnCaptureClicked() {
    // 1. Copy the current frame under the lock into a local buffer
    BYTE* capturePixels = nullptr;
    int capW = 0, capH = 0;
    DWORD capSize = 0;

    EnterCriticalSection(&g_frameLock);
    if (g_sharedFrame.pixels && g_sharedFrame.width > 0 && g_sharedFrame.height > 0) {
        capW = g_sharedFrame.width;
        capH = g_sharedFrame.height;
        capSize = g_sharedFrame.byteSize;
        capturePixels = new BYTE[capSize];
        memcpy(capturePixels, g_sharedFrame.pixels, capSize);
    }
    LeaveCriticalSection(&g_frameLock);

    if (!capturePixels) {
        SetWindowTextW(g_hStatusLabel, L"❌ Error: No camera frame available to capture.");
        g_statusColor = RGB(210, 30, 30);
        InvalidateRect(g_hStatusLabel, NULL, TRUE);
        return;
    }

    // 2. Generate unique temporary filename in the Windows temp directory
    char tempDir[MAX_PATH];
    char tempFilePath[MAX_PATH];
    DWORD pathLen = GetTempPathA(MAX_PATH, tempDir);
    if (pathLen == 0 || pathLen > MAX_PATH) {
        delete[] capturePixels;
        SetWindowTextW(g_hStatusLabel, L"❌ Error: Could not determine Windows temp directory.");
        g_statusColor = RGB(210, 30, 30);
        InvalidateRect(g_hStatusLabel, NULL, TRUE);
        return;
    }

    if (GetTempFileNameA(tempDir, "BAR", 0, tempFilePath) == 0) {
        delete[] capturePixels;
        SetWindowTextW(g_hStatusLabel, L"❌ Error: Could not create unique temporary filename.");
        g_statusColor = RGB(210, 30, 30);
        InvalidateRect(g_hStatusLabel, NULL, TRUE);
        return;
    }

    // 3. Prepare BMP headers
    BITMAPFILEHEADER bmfHeader = {0};
    BITMAPINFOHEADER bmiHeader = {0};

    DWORD fileHeaderSize = sizeof(BITMAPFILEHEADER);
    DWORD infoHeaderSize = sizeof(BITMAPINFOHEADER);
    DWORD totalFileSize  = fileHeaderSize + infoHeaderSize + capSize;

    bmfHeader.bfType      = 0x4D42; // "BM"
    bmfHeader.bfSize      = totalFileSize;
    bmfHeader.bfOffBits   = fileHeaderSize + infoHeaderSize;

    bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    bmiHeader.biWidth         = capW;
    bmiHeader.biHeight        = -capH; // top-down DIB
    bmiHeader.biPlanes        = 1;
    bmiHeader.biBitCount       = 32;
    bmiHeader.biCompression   = BI_RGB;
    bmiHeader.biSizeImage     = capSize;

    // 4. Create and write file, validating EVERY WriteFile call
    HANDLE hFile = CreateFileA(tempFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        delete[] capturePixels;
        DeleteFileA(tempFilePath);
        SetWindowTextW(g_hStatusLabel, L"❌ Error: Could not open temporary file for writing.");
        g_statusColor = RGB(210, 30, 30);
        InvalidateRect(g_hStatusLabel, NULL, TRUE);
        return;
    }

    DWORD written = 0;
    bool writeOk = true;

    if (!WriteFile(hFile, &bmfHeader, fileHeaderSize, &written, NULL) || written != fileHeaderSize) {
        writeOk = false;
    }
    if (writeOk && (!WriteFile(hFile, &bmiHeader, infoHeaderSize, &written, NULL) || written != infoHeaderSize)) {
        writeOk = false;
    }
    if (writeOk && (!WriteFile(hFile, capturePixels, capSize, &written, NULL) || written != capSize)) {
        writeOk = false;
    }

    delete[] capturePixels;

    // Flush OS write buffers and close file handle
    FlushFileBuffers(hFile);
    CloseHandle(hFile);

    if (!writeOk) {
        DeleteFileA(tempFilePath);
        SetWindowTextW(g_hStatusLabel, L"❌ Error: Failed to write complete frame data to file.");
        g_statusColor = RGB(210, 30, 30);
        InvalidateRect(g_hStatusLabel, NULL, TRUE);
        return;
    }

    // 5. Decode the captured image file using existing BarcodeDecoder
    DecodeResult result = BarcodeDecoder::decodeImage(tempFilePath);

    // 6. Delete temporary file ONLY after decoding has finished
    DeleteFileA(tempFilePath);

    // 7. Display result
    if (result.success) {
        WCHAR resultText[64];
        WCHAR wCode[16] = {0};
        MultiByteToWideChar(CP_UTF8, 0, result.code.c_str(), -1, wCode, 16);
        wsprintfW(resultText, L"Decoded Code:   %s", wCode);
        SetWindowTextW(g_hResultLabel, resultText);
        g_resultColor = RGB(22, 145, 45); // Dark Green

        SetWindowTextW(g_hStatusLabel, L"✅ Camera frame successfully decoded!");
        g_statusColor = RGB(22, 145, 45);

        MessageBeep(MB_OK);
    } else {
        SetWindowTextW(g_hResultLabel, L"Decoded Code:   [ — ]");
        g_resultColor = RGB(150, 150, 150);

        WCHAR wErr[256] = {0};
        MultiByteToWideChar(CP_UTF8, 0, result.errorMessage.c_str(), -1, wErr, 256);

        WCHAR statusText[512];
        wsprintfW(statusText, L"❌ %s", wErr);
        SetWindowTextW(g_hStatusLabel, statusText);
        g_statusColor = RGB(210, 30, 30); // Red
    }

    InvalidateRect(g_hResultLabel, NULL, TRUE);
    InvalidateRect(g_hStatusLabel, NULL, TRUE);
}

// -------------------------------------------------------
// Helper: Custom Window Procedure for the Preview Panel
// -------------------------------------------------------
static LRESULT CALLBACK PreviewPanelProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_ERASEBKGND:
            return 1; // Double buffering handled in WM_PAINT

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            RECT rc;
            GetClientRect(hWnd, &rc);
            int width  = rc.right - rc.left;
            int height = rc.bottom - rc.top;

            if (width > 0 && height > 0) {
                // Double buffer DC and Bitmap
                HDC memDC = CreateCompatibleDC(hdc);
                HBITMAP memBmp = CreateCompatibleBitmap(hdc, width, height);
                HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

                // Background: white
                HBRUSH whiteBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
                FillRect(memDC, &rc, whiteBrush);

                // Border: 1px subtle light gray
                HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(215, 218, 224));
                HPEN oldPen = (HPEN)SelectObject(memDC, borderPen);
                HBRUSH oldBrush = (HBRUSH)SelectObject(memDC, GetStockObject(NULL_BRUSH));
                Rectangle(memDC, 0, 0, width, height);
                SelectObject(memDC, oldBrush);
                SelectObject(memDC, oldPen);
                DeleteObject(borderPen);

                if (g_isCameraActive) {
                    // 1. Copy frame data under lock into local buffer
                    BYTE* localFramePixels = nullptr;
                    int fW = 0, fH = 0;
                    DWORD fSize = 0;

                    EnterCriticalSection(&g_frameLock);
                    if (g_sharedFrame.pixels && g_sharedFrame.width > 0 && g_sharedFrame.height > 0) {
                        fW = g_sharedFrame.width;
                        fH = g_sharedFrame.height;
                        fSize = g_sharedFrame.byteSize;
                        localFramePixels = new BYTE[fSize];
                        memcpy(localFramePixels, g_sharedFrame.pixels, fSize);
                    }
                    LeaveCriticalSection(&g_frameLock);

                    // 2. Perform GDI painting outside of the lock
                    if (localFramePixels) {
                        BITMAPINFO bmi = {0};
                        bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
                        bmi.bmiHeader.biWidth       = fW;
                        bmi.bmiHeader.biHeight      = -fH; // top-down
                        bmi.bmiHeader.biPlanes      = 1;
                        bmi.bmiHeader.biBitCount    = 32;
                        bmi.bmiHeader.biCompression = BI_RGB;

                        const int padding = 10;
                        int availW = width  - 2 * padding;
                        int availH = height - 2 * padding;

                        double scaleX = static_cast<double>(availW) / fW;
                        double scaleY = static_cast<double>(availH) / fH;
                        double scale  = (scaleX < scaleY) ? scaleX : scaleY;

                        int destW = static_cast<int>(fW * scale);
                        int destH = static_cast<int>(fH * scale);
                        int destX = padding + (availW - destW) / 2;
                        int destY = padding + (availH - destH) / 2;

                        StretchDIBits(memDC, destX, destY, destW, destH,
                                      0, 0, fW, fH,
                                      localFramePixels, &bmi, DIB_RGB_COLORS, SRCCOPY);

                        delete[] localFramePixels;

                        // Draw central red alignment guideline
                        HPEN guidePen = CreatePen(PS_SOLID, 2, RGB(230, 40, 40));
                        HPEN oldG = (HPEN)SelectObject(memDC, guidePen);
                        int midY = destY + destH / 2;
                        int lineStart = destX + destW / 8;
                        int lineEnd   = destX + (destW * 7) / 8;
                        MoveToEx(memDC, lineStart, midY, NULL);
                        LineTo(memDC, lineEnd, midY);
                        SelectObject(memDC, oldG);
                        DeleteObject(guidePen);
                    }
                } else if (g_pUploadedImage) {
                    // Render uploaded image
                    Gdiplus::Graphics g(memDC);
                    g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

                    UINT imgW = g_pUploadedImage->GetWidth();
                    UINT imgH = g_pUploadedImage->GetHeight();

                    if (imgW > 0 && imgH > 0) {
                        const int padding = 16;
                        int availW = width  - 2 * padding;
                        int availH = height - 2 * padding;

                        double scaleX = static_cast<double>(availW) / imgW;
                        double scaleY = static_cast<double>(availH) / imgH;
                        double scale  = (scaleX < scaleY) ? scaleX : scaleY;

                        int destW = static_cast<int>(imgW * scale);
                        int destH = static_cast<int>(imgH * scale);
                        int destX = padding + (availW - destW) / 2;
                        int destY = padding + (availH - destH) / 2;

                        g.DrawImage(g_pUploadedImage, destX, destY, destW, destH);
                    }
                } else {
                    // Placeholder message
                    SetBkMode(memDC, TRANSPARENT);
                    SetTextColor(memDC, RGB(150, 155, 165));
                    HFONT oldFont = (HFONT)SelectObject(memDC, g_hPlaceholderFont);
                    RECT textRc = rc;
                    DrawTextW(memDC,
                              L"No image or camera active.\nClick \"Upload Image\" or \"Scan Using Camera\".",
                              -1, &textRc, DT_CENTER | DT_VCENTER | DT_NOPREFIX);
                    SelectObject(memDC, oldFont);
                }

                BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);
                SelectObject(memDC, oldBmp);
                DeleteObject(memBmp);
                DeleteDC(memDC);
            }

            EndPaint(hWnd, &ps);
            return 0;
        }

        default:
            return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
}

// -------------------------------------------------------
// Helper: Open file selection dialog and process image
// -------------------------------------------------------
static void OnUploadImageClicked(HWND hWndParent) {
    // If camera is running, stop it first
    if (g_cameraRunning == 1) {
        StopCameraWorker();
    }

    WCHAR filePath[MAX_PATH] = L"";

    OPENFILENAMEW ofn = {0};
    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.hwndOwner   = hWndParent;
    ofn.lpstrFilter = L"Barcode Images (*.png;*.jpg;*.jpeg;*.bmp)\0*.png;*.jpg;*.jpeg;*.bmp\0"
                      L"PNG Images (*.png)\0*.png\0"
                      L"JPEG Images (*.jpg;*.jpeg)\0*.jpg;*.jpeg\0"
                      L"Bitmap Images (*.bmp)\0*.bmp\0"
                      L"All Files (*.*)\0*.*\0";
    ofn.lpstrFile   = filePath;
    ofn.nMaxFile    = MAX_PATH;
    ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_ENABLESIZING;
    ofn.lpstrTitle  = L"Select Barcode Image to Decode";

    if (!GetOpenFileNameW(&ofn)) {
        return; // User canceled
    }

    // Load preview image
    if (g_pUploadedImage) {
        delete g_pUploadedImage;
        g_pUploadedImage = nullptr;
    }
    g_pUploadedImage = new Gdiplus::Bitmap(filePath);

    // Redraw preview panel
    InvalidateRect(g_hPreviewPanel, NULL, TRUE);

    // Convert wide path to UTF-8 char buffer for BarcodeDecoder
    char utf8Path[MAX_PATH * 3] = {0};
    WideCharToMultiByte(CP_UTF8, 0, filePath, -1, utf8Path, sizeof(utf8Path), NULL, NULL);

    // Call decoder
    DecodeResult result = BarcodeDecoder::decodeImage(utf8Path);

    // Extract file name
    const WCHAR* pFileName = wcsrchr(filePath, L'\\');
    if (!pFileName) pFileName = wcsrchr(filePath, L'/');
    pFileName = pFileName ? (pFileName + 1) : filePath;

    if (result.success) {
        WCHAR resultText[64];
        WCHAR wCode[16] = {0};
        MultiByteToWideChar(CP_UTF8, 0, result.code.c_str(), -1, wCode, 16);
        wsprintfW(resultText, L"Decoded Code:   %s", wCode);
        SetWindowTextW(g_hResultLabel, resultText);
        g_resultColor = RGB(22, 145, 45); // Dark Green

        WCHAR statusText[MAX_PATH + 64];
        wsprintfW(statusText, L"✅ Successfully decoded from: \"%s\"", pFileName);
        SetWindowTextW(g_hStatusLabel, statusText);
        g_statusColor = RGB(22, 145, 45);
    } else {
        SetWindowTextW(g_hResultLabel, L"Decoded Code:   [ — ]");
        g_resultColor = RGB(150, 150, 150);

        WCHAR wErr[256] = {0};
        MultiByteToWideChar(CP_UTF8, 0, result.errorMessage.c_str(), -1, wErr, 256);

        WCHAR statusText[512];
        wsprintfW(statusText, L"❌ %s", wErr);
        SetWindowTextW(g_hStatusLabel, statusText);
        g_statusColor = RGB(210, 30, 30); // Red
    }

    InvalidateRect(g_hResultLabel, NULL, TRUE);
    InvalidateRect(g_hStatusLabel, NULL, TRUE);
}

// -------------------------------------------------------
// Helper: Clear view and reset status
// -------------------------------------------------------
static void OnClearClicked() {
    if (g_cameraRunning == 1) {
        StopCameraWorker();
    }

    if (g_pUploadedImage) {
        delete g_pUploadedImage;
        g_pUploadedImage = nullptr;
    }

    SetWindowTextW(g_hResultLabel, L"Decoded Code:   [ — ]");
    g_resultColor = RGB(130, 130, 130);

    SetWindowTextW(g_hStatusLabel, L"Ready. Choose \"Upload Image\" or \"Scan Using Camera\".");
    g_statusColor = RGB(110, 110, 110);

    InvalidateRect(g_hPreviewPanel, NULL, TRUE);
    InvalidateRect(g_hResultLabel, NULL, TRUE);
    InvalidateRect(g_hStatusLabel, NULL, TRUE);
}

// -------------------------------------------------------
// Dynamic layout calculation on window resize
// -------------------------------------------------------
static void UpdateLayout(int clientW, int clientH) {
    if (!g_hUploadBtn || !g_hPreviewPanel || !g_hResultLabel || !g_hStatusLabel) return;

    const int margin       = 30;
    const int btnH         = 38;
    const int spacing      = 10;
    const int resultCardH  = 44;
    const int statusH      = 34;
    const int bottomSpace  = 16;

    // Row 1: Title
    MoveWindow(g_hTitleLabel, margin, 20, clientW - 2 * margin, 30, TRUE);

    // Row 2: Subtitle
    MoveWindow(g_hSubtitleLbl, margin, 52, clientW - 2 * margin, 20, TRUE);

    // Row 3: Action Buttons (Upload, Camera, Capture, Stop, Clear)
    int row3Y = 82;
    int btnW1 = 140; // Upload
    int btnW2 = 160; // Scan Using Camera
    int btnW3 = 130; // Capture
    int btnW4 = 110; // Stop
    int btnW5 = 80;  // Clear

    int curX = margin;
    MoveWindow(g_hUploadBtn,  curX, row3Y, btnW1, btnH, TRUE); curX += btnW1 + spacing;
    MoveWindow(g_hCameraBtn,  curX, row3Y, btnW2, btnH, TRUE); curX += btnW2 + spacing;
    MoveWindow(g_hCaptureBtn, curX, row3Y, btnW3, btnH, TRUE); curX += btnW3 + spacing;
    MoveWindow(g_hStopBtn,    curX, row3Y, btnW4, btnH, TRUE); curX += btnW4 + spacing;
    MoveWindow(g_hClearBtn,   curX, row3Y, btnW5, btnH, TRUE);

    // Row 4: Preview Display Panel
    int row4Y = row3Y + btnH + 14;
    int previewH = clientH - row4Y - resultCardH - statusH - bottomSpace - 20;
    if (previewH < 180) previewH = 180;

    MoveWindow(g_hPreviewPanel, margin, row4Y, clientW - 2 * margin, previewH, TRUE);

    // Row 5: Decoded Result Card
    int row5Y = row4Y + previewH + 12;
    MoveWindow(g_hResultLabel, margin, row5Y, clientW - 2 * margin, resultCardH, TRUE);

    // Row 6: Status / Error Label
    int row6Y = row5Y + resultCardH + 4;
    MoveWindow(g_hStatusLabel, margin, row6Y, clientW - 2 * margin, statusH, TRUE);
}

// -------------------------------------------------------
// Main Window Procedure
// -------------------------------------------------------
static LRESULT CALLBACK MainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            // Create fonts
            g_hTitleFont = CreateFontW(-22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

            g_hSubtitleFont = CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                          CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

            g_hButtonFont = CreateFontW(-13, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

            g_hResultFont = CreateFontW(-22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

            g_hStatusFont = CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

            g_hPlaceholderFont = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                             CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

            g_hBgBrush = CreateSolidBrush(RGB(246, 247, 249));

            // Title label
            g_hTitleLabel = CreateWindowExW(
                0, L"STATIC", L"📷 Barcode Scanner & Decoder",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                0, 0, 0, 0, hWnd, (HMENU)IDC_TITLE_LABEL, g_hInstance, NULL);
            SendMessageW(g_hTitleLabel, WM_SETFONT, (WPARAM)g_hTitleFont, TRUE);

            // Subtitle label
            g_hSubtitleLbl = CreateWindowExW(
                0, L"STATIC", L"Upload an image or scan live with your webcam to decode Code 39 barcodes.",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                0, 0, 0, 0, hWnd, (HMENU)IDC_SUBTITLE_LABEL, g_hInstance, NULL);
            SendMessageW(g_hSubtitleLbl, WM_SETFONT, (WPARAM)g_hSubtitleFont, TRUE);

            // Upload button
            g_hUploadBtn = CreateWindowExW(
                0, L"BUTTON", L"📁  Upload Image",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                0, 0, 0, 0, hWnd, (HMENU)IDC_UPLOAD_BTN, g_hInstance, NULL);
            SendMessageW(g_hUploadBtn, WM_SETFONT, (WPARAM)g_hButtonFont, TRUE);

            // Scan using camera button
            g_hCameraBtn = CreateWindowExW(
                0, L"BUTTON", L"📹  Scan Using Camera",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                0, 0, 0, 0, hWnd, (HMENU)IDC_CAMERA_BTN, g_hInstance, NULL);
            SendMessageW(g_hCameraBtn, WM_SETFONT, (WPARAM)g_hButtonFont, TRUE);

            // Capture button (disabled until camera is running)
            g_hCaptureBtn = CreateWindowExW(
                0, L"BUTTON", L"📸  Capture / Scan",
                WS_CHILD | WS_VISIBLE | WS_DISABLED | BS_DEFPUSHBUTTON,
                0, 0, 0, 0, hWnd, (HMENU)IDC_CAPTURE_BTN, g_hInstance, NULL);
            SendMessageW(g_hCaptureBtn, WM_SETFONT, (WPARAM)g_hButtonFont, TRUE);

            // Stop camera button (disabled until camera is running)
            g_hStopBtn = CreateWindowExW(
                0, L"BUTTON", L"⏹  Stop Camera",
                WS_CHILD | WS_VISIBLE | WS_DISABLED | BS_PUSHBUTTON,
                0, 0, 0, 0, hWnd, (HMENU)IDC_STOP_BTN, g_hInstance, NULL);
            SendMessageW(g_hStopBtn, WM_SETFONT, (WPARAM)g_hButtonFont, TRUE);

            // Clear button
            g_hClearBtn = CreateWindowExW(
                0, L"BUTTON", L"Clear",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                0, 0, 0, 0, hWnd, (HMENU)IDC_CLEAR_BTN, g_hInstance, NULL);
            SendMessageW(g_hClearBtn, WM_SETFONT, (WPARAM)g_hButtonFont, TRUE);

            // Preview Panel (Custom Window Class)
            g_hPreviewPanel = CreateWindowExW(
                0, L"ScannerPreviewClass", L"",
                WS_CHILD | WS_VISIBLE,
                0, 0, 0, 0, hWnd, (HMENU)IDC_PREVIEW_PANEL, g_hInstance, NULL);

            // Decoded Result Card
            g_hResultLabel = CreateWindowExW(
                0, L"STATIC", L"Decoded Code:   [ — ]",
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                0, 0, 0, 0, hWnd, (HMENU)IDC_RESULT_LABEL, g_hInstance, NULL);
            SendMessageW(g_hResultLabel, WM_SETFONT, (WPARAM)g_hResultFont, TRUE);

            // Status / Error label
            g_hStatusLabel = CreateWindowExW(
                0, L"STATIC", L"Ready. Choose \"Upload Image\" or \"Scan Using Camera\".",
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                0, 0, 0, 0, hWnd, (HMENU)IDC_STATUS_LABEL, g_hInstance, NULL);
            SendMessageW(g_hStatusLabel, WM_SETFONT, (WPARAM)g_hStatusFont, TRUE);

            return 0;
        }

        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            if (wmId == IDC_UPLOAD_BTN) {
                OnUploadImageClicked(hWnd);
                return 0;
            }
            if (wmId == IDC_CAMERA_BTN) {
                StartCamera();
                return 0;
            }
            if (wmId == IDC_CAPTURE_BTN) {
                OnCaptureClicked();
                return 0;
            }
            if (wmId == IDC_STOP_BTN) {
                StopCameraWorker();
                return 0;
            }
            if (wmId == IDC_CLEAR_BTN) {
                OnClearClicked();
                return 0;
            }
            break;
        }

        // Camera worker thread notification messages (handled on main GUI thread)
        case WM_APP_FRAME_READY: {
            if (g_isCameraActive) {
                InvalidateRect(g_hPreviewPanel, NULL, FALSE);
            }
            return 0;
        }

        case WM_APP_CAMERA_STARTED: {
            g_isCameraActive = true;
            EnableWindow(g_hCameraBtn, FALSE);
            EnableWindow(g_hCaptureBtn, TRUE);
            EnableWindow(g_hStopBtn, TRUE);

            SetWindowTextW(g_hStatusLabel, L"📹 Camera live. Align the barcode with the red line and click \"Capture / Scan\".");
            g_statusColor = RGB(22, 145, 45);

            InvalidateRect(g_hPreviewPanel, NULL, TRUE);
            InvalidateRect(g_hStatusLabel, NULL, TRUE);
            return 0;
        }

        case WM_APP_CAMERA_STOPPED: {
            g_isCameraActive = false;
            EnableWindow(g_hCaptureBtn, FALSE);
            EnableWindow(g_hStopBtn, FALSE);
            EnableWindow(g_hCameraBtn, TRUE);
            InvalidateRect(g_hPreviewPanel, NULL, TRUE);
            return 0;
        }

        case WM_APP_CAMERA_ERROR: {
            g_isCameraActive = false;
            EnableWindow(g_hCaptureBtn, FALSE);
            EnableWindow(g_hStopBtn, FALSE);
            EnableWindow(g_hCameraBtn, TRUE);

            const WCHAR* errMsg = (const WCHAR*)lParam;
            WCHAR fullErr[512];
            wsprintfW(fullErr, L"❌ %s", errMsg ? errMsg : L"Unknown camera error.");
            SetWindowTextW(g_hStatusLabel, fullErr);
            g_statusColor = RGB(210, 30, 30);

            InvalidateRect(g_hPreviewPanel, NULL, TRUE);
            InvalidateRect(g_hStatusLabel, NULL, TRUE);
            return 0;
        }

        case WM_CTLCOLORSTATIC: {
            HDC hdcStatic = (HDC)wParam;
            HWND hCtrl    = (HWND)lParam;

            if (hCtrl == g_hResultLabel) {
                SetBkMode(hdcStatic, TRANSPARENT);
                SetTextColor(hdcStatic, g_resultColor);
                return (INT_PTR)g_hBgBrush;
            }
            if (hCtrl == g_hStatusLabel) {
                SetBkMode(hdcStatic, TRANSPARENT);
                SetTextColor(hdcStatic, g_statusColor);
                return (INT_PTR)g_hBgBrush;
            }
            if (hCtrl == g_hTitleLabel || hCtrl == g_hSubtitleLbl) {
                SetBkMode(hdcStatic, TRANSPARENT);
                if (hCtrl == g_hSubtitleLbl) {
                    SetTextColor(hdcStatic, RGB(110, 115, 125));
                } else {
                    SetTextColor(hdcStatic, RGB(25, 30, 40));
                }
                return (INT_PTR)g_hBgBrush;
            }
            break;
        }

        case WM_ERASEBKGND: {
            HDC hdc = (HDC)wParam;
            RECT rc;
            GetClientRect(hWnd, &rc);
            FillRect(hdc, &rc, g_hBgBrush);
            return 1;
        }

        case WM_SIZE: {
            int w = LOWORD(lParam);
            int h = HIWORD(lParam);
            UpdateLayout(w, h);
            InvalidateRect(hWnd, NULL, TRUE);
            return 0;
        }

        case WM_GETMINMAXINFO: {
            LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
            lpMMI->ptMinTrackSize.x = 750;
            lpMMI->ptMinTrackSize.y = 520;
            return 0;
        }

        case WM_DESTROY: {
            // Safely stop camera before destroying window
            StopCameraWorker();

            if (g_pUploadedImage) {
                delete g_pUploadedImage;
                g_pUploadedImage = nullptr;
            }
            if (g_hTitleFont)       DeleteObject(g_hTitleFont);
            if (g_hSubtitleFont)    DeleteObject(g_hSubtitleFont);
            if (g_hButtonFont)      DeleteObject(g_hButtonFont);
            if (g_hResultFont)      DeleteObject(g_hResultFont);
            if (g_hStatusFont)      DeleteObject(g_hStatusFont);
            if (g_hPlaceholderFont) DeleteObject(g_hPlaceholderFont);
            if (g_hBgBrush)         DeleteObject(g_hBgBrush);

            PostQuitMessage(0);
            return 0;
        }

        default:
            return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

// -------------------------------------------------------
// Application Entry Point
// -------------------------------------------------------
static int RunScannerApplication(HINSTANCE hInstance, int nCmdShow) {
    g_hInstance = hInstance;

    // Initialize synchronization critical section
    InitializeCriticalSection(&g_frameLock);

    // Initialize GDI+ for image loading
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // Register Preview panel class
    WNDCLASSEXW pvc = {0};
    pvc.cbSize        = sizeof(WNDCLASSEXW);
    pvc.style         = CS_HREDRAW | CS_VREDRAW;
    pvc.lpfnWndProc   = PreviewPanelProc;
    pvc.hInstance     = hInstance;
    pvc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    pvc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    pvc.lpszClassName = L"ScannerPreviewClass";
    RegisterClassExW(&pvc);

    // Register Main Window class
    WNDCLASSEXW wc = {0};
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = MainWindowProc;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(246, 247, 249));
    wc.lpszClassName = L"BarcodeScannerMainClass";
    RegisterClassExW(&wc);

    // Initial window size: 850 × 640, centered on screen
    const int initW = 850;
    const int initH = 640;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int posX    = (screenW - initW) / 2;
    int posY    = (screenH - initH) / 2;

    g_hMainWindow = CreateWindowExW(
        0,
        L"BarcodeScannerMainClass",
        L"Barcode Scanner & Decoder — Code 39",
        WS_OVERLAPPEDWINDOW,
        posX, posY, initW, initH,
        NULL, NULL, hInstance, NULL);

    if (!g_hMainWindow) {
        Gdiplus::GdiplusShutdown(gdiplusToken);
        DeleteCriticalSection(&g_frameLock);
        return 1;
    }

    ShowWindow(g_hMainWindow, nCmdShow);
    UpdateWindow(g_hMainWindow);

    // Main message loop
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // Ensure camera is stopped before exit
    StopCameraWorker();

    Gdiplus::GdiplusShutdown(gdiplusToken);
    DeleteCriticalSection(&g_frameLock);

    return static_cast<int>(msg.wParam);
}

// Windows subsystem entry point
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    return RunScannerApplication(hInstance, nCmdShow);
}

// Fallback console main entry point
int main(int /*argc*/, char* /*argv*/[]) {
    return RunScannerApplication(GetModuleHandleW(NULL), SW_SHOWDEFAULT);
}

#endif // _WIN32
