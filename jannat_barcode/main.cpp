// main.cpp
// -------------------------------------------------------
// Barcode Generator — Windows C++ implementation.
//
// Matches the UI, behaviour, and functionality of main.mm (macOS):
//   1. BarcodeView   — custom child window that draws the barcode.
//   2. Window & UI   — input box, Generate/Clear buttons, status label.
//   3. PNG Export    — saves barcode_<input>.png into Documents folder.
//   4. Entry point   — supports both wWinMain and main().
//
// Pure barcode logic lives in BarcodeGenerator.cpp / .h.
// -------------------------------------------------------

#ifdef _WIN32

#define NOMINMAX
#include <windows.h>
#include <objbase.h>
#include <commctrl.h>
#include <gdiplus.h>
#include <shlobj.h>

#include "BarcodeGenerator.h"

#include <string>
#include <vector>

#ifdef _MSC_VER
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(linker, "/subsystem:windows /entry:wWinMainCRTStartup")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

// Control identifiers
#define IDC_INPUT_FIELD   101
#define IDC_GENERATE_BTN  102
#define IDC_CLEAR_BTN     103
#define IDC_BARCODE_VIEW  104
#define IDC_STATUS_LABEL  105
#define IDC_TITLE_LABEL   106
#define IDC_SUBTITLE_LBL  107

// Global state
static HINSTANCE g_hInstance = nullptr;
static HWND g_hMainWindow   = nullptr;
static HWND g_hTitleLabel   = nullptr;
static HWND g_hSubtitleLbl  = nullptr;
static HWND g_hInputField   = nullptr;
static HWND g_hGenerateBtn  = nullptr;
static HWND g_hClearBtn     = nullptr;
static HWND g_hBarcodeView  = nullptr;
static HWND g_hStatusLabel  = nullptr;

static HFONT g_hTitleFont       = nullptr;
static HFONT g_hSubtitleFont    = nullptr;
static HFONT g_hInputFont       = nullptr;
static HFONT g_hButtonFont      = nullptr;
static HFONT g_hStatusFont      = nullptr;
static HFONT g_hPlaceholderFont = nullptr;

static HBRUSH g_hWindowBgBrush = nullptr;
static COLORREF g_statusTextColor = RGB(110, 110, 110);

// Barcode data state
static std::vector<BarElement> g_barcodeElements;
static bool g_hasBarcodeData = false;

// -------------------------------------------------------
// Helper: GDI+ PNG Encoder CLSID
// -------------------------------------------------------
static int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT num = 0, size = 0;
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;

    auto* pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
    if (!pImageCodecInfo) return -1;

    Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);
    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return static_cast<int>(j);
        }
    }
    free(pImageCodecInfo);
    return -1;
}

// -------------------------------------------------------
// Helper: Save barcode PNG image into user's Documents folder
// -------------------------------------------------------
static bool SaveBarcodePNG(const std::vector<BarElement>& elements,
                           const std::string& input,
                           std::wstring& outSavedPath)
{
    const int imgW      = 600;
    const int imgH      = 200;
    const int paddingPx = 20;

    int totalUnits = 0;
    for (const BarElement& el : elements) {
        totalUnits += el.width;
    }
    if (totalUnits <= 0) return false;

    WCHAR docsPath[MAX_PATH];
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_MYDOCUMENTS, NULL, 0, docsPath))) {
        return false;
    }

    std::wstring fileName = L"barcode_" + std::wstring(input.begin(), input.end()) + L".png";
    std::wstring fullPath = std::wstring(docsPath) + L"\\" + fileName;

    Gdiplus::Bitmap bmp(imgW, imgH, PixelFormat24bppRGB);
    Gdiplus::Graphics g(&bmp);
    g.SetSmoothingMode(Gdiplus::SmoothingModeNone);
    g.Clear(Gdiplus::Color(255, 255, 255));

    double barcodeW = imgW - 2.0 * paddingPx;
    double unitPx   = barcodeW / static_cast<double>(totalUnits);
    double barcodeH = imgH - 2.0 * paddingPx;
    double x        = paddingPx;

    Gdiplus::SolidBrush blackBrush(Gdiplus::Color(0, 0, 0));
    for (const BarElement& el : elements) {
        double elW = unitPx * static_cast<double>(el.width);
        if (el.isBar) {
            g.FillRectangle(&blackBrush,
                            static_cast<Gdiplus::REAL>(x),
                            static_cast<Gdiplus::REAL>(paddingPx),
                            static_cast<Gdiplus::REAL>(elW),
                            static_cast<Gdiplus::REAL>(barcodeH));
        }
        x += elW;
    }

    CLSID pngClsid;
    if (GetEncoderClsid(L"image/png", &pngClsid) < 0) {
        return false;
    }

    Gdiplus::Status status = bmp.Save(fullPath.c_str(), &pngClsid, NULL);
    if (status == Gdiplus::Ok) {
        outSavedPath = fullPath;
        return true;
    }
    return false;
}

// -------------------------------------------------------
// Custom Window Proc for BarcodeView
// -------------------------------------------------------
static LRESULT CALLBACK BarcodeViewProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_ERASEBKGND:
            return 1; // handled in WM_PAINT with double-buffering

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            RECT bounds;
            GetClientRect(hWnd, &bounds);
            int width  = bounds.right - bounds.left;
            int height = bounds.bottom - bounds.top;

            if (width > 0 && height > 0) {
                // Double buffering
                HDC memDC = CreateCompatibleDC(hdc);
                HBITMAP memBmp = CreateCompatibleBitmap(hdc, width, height);
                HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

                // Background: white
                HBRUSH whiteBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
                FillRect(memDC, &bounds, whiteBrush);

                // Border: 1px subtle light gray
                HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(211, 211, 211));
                HPEN oldPen = (HPEN)SelectObject(memDC, borderPen);
                HBRUSH oldBrush = (HBRUSH)SelectObject(memDC, GetStockObject(NULL_BRUSH));
                Rectangle(memDC, 0, 0, width, height);
                SelectObject(memDC, oldBrush);
                SelectObject(memDC, oldPen);
                DeleteObject(borderPen);

                if (!g_hasBarcodeData || g_barcodeElements.empty()) {
                    // Placeholder message
                    SetBkMode(memDC, TRANSPARENT);
                    SetTextColor(memDC, RGB(160, 160, 160));
                    HFONT oldFont = (HFONT)SelectObject(memDC, g_hPlaceholderFont);
                    DrawTextW(memDC, L"Barcode will appear here", -1, &bounds,
                              DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                    SelectObject(memDC, oldFont);
                } else {
                    int totalUnits = 0;
                    for (const BarElement& el : g_barcodeElements) {
                        totalUnits += el.width;
                    }

                    if (totalUnits > 0) {
                        double padding    = 20.0;
                        double barcodeW   = static_cast<double>(width)  - 2.0 * padding;
                        double barcodeH   = static_cast<double>(height) - 2.0 * padding;
                        double unitPixels = barcodeW / static_cast<double>(totalUnits);
                        double x          = padding;

                        HBRUSH blackBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
                        for (const BarElement& el : g_barcodeElements) {
                            double elW = unitPixels * static_cast<double>(el.width);
                            if (el.isBar) {
                                RECT barRect;
                                barRect.left   = static_cast<LONG>(x + 0.5);
                                barRect.top    = static_cast<LONG>(padding);
                                barRect.right  = static_cast<LONG>(x + elW + 0.5);
                                barRect.bottom = static_cast<LONG>(padding + barcodeH);
                                FillRect(memDC, &barRect, blackBrush);
                            }
                            x += elW;
                        }
                    }
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
// Subclass Edit control to trigger Generate on Enter key
// -------------------------------------------------------
static LRESULT CALLBACK EditSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                                         UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    (void)uIdSubclass;
    (void)dwRefData;

    if (uMsg == WM_KEYDOWN && wParam == VK_RETURN) {
        HWND hParent = GetParent(hWnd);
        SendMessageW(hParent, WM_COMMAND, MAKEWPARAM(IDC_GENERATE_BTN, BN_CLICKED),
                     (LPARAM)GetDlgItem(hParent, IDC_GENERATE_BTN));
        return 0;
    }
    if (uMsg == WM_CHAR && wParam == VK_RETURN) {
        return 0; // prevent standard beep
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

// -------------------------------------------------------
// Button Handlers
// -------------------------------------------------------
static void OnGenerateClicked() {
    int len = GetWindowTextLengthW(g_hInputField);
    std::wstring wInput(len, L'\0');
    if (len > 0) {
        GetWindowTextW(g_hInputField, &wInput[0], len + 1);
    }

    // Convert to UTF-8 std::string
    std::string input;
    if (!wInput.empty()) {
        int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wInput.c_str(), -1, NULL, 0, NULL, NULL);
        if (sizeNeeded > 1) {
            input.resize(sizeNeeded - 1);
            WideCharToMultiByte(CP_UTF8, 0, wInput.c_str(), -1, &input[0], sizeNeeded, NULL, NULL);
        }
    }

    ValidationResult result = BarcodeGenerator::validate(input);

    if (!result.valid) {
        // Show error message in red
        int wLen = MultiByteToWideChar(CP_UTF8, 0, result.message.c_str(), -1, NULL, 0);
        std::wstring wMsg(wLen > 0 ? wLen - 1 : 0, L'\0');
        if (wLen > 1) {
            MultiByteToWideChar(CP_UTF8, 0, result.message.c_str(), -1, &wMsg[0], wLen);
        }

        SetWindowTextW(g_hStatusLabel, wMsg.c_str());
        g_statusTextColor = RGB(210, 30, 30); // Red
        InvalidateRect(g_hStatusLabel, NULL, TRUE);

        g_barcodeElements.clear();
        g_hasBarcodeData = false;
        InvalidateRect(g_hBarcodeView, NULL, TRUE);
        return;
    }

    // Validation passed — encode and display
    g_barcodeElements = BarcodeGenerator::encode(input);
    g_hasBarcodeData  = true;
    InvalidateRect(g_hBarcodeView, NULL, TRUE);

    // Save PNG image to Documents
    std::wstring savedPath;
    bool saved = SaveBarcodePNG(g_barcodeElements, input, savedPath);

    std::wstring successMsg;
    if (saved) {
        successMsg = L"✅  Code 39 barcode generated for: " +
                     std::wstring(input.begin(), input.end()) +
                     L"  (with * start/stop)\r\n💾  Saved to: " + savedPath;
    } else {
        successMsg = L"✅  Code 39 barcode generated for: " +
                     std::wstring(input.begin(), input.end()) +
                     L"  (with * start/stop)\r\n⚠️  Could not save image — check Documents folder permissions.";
    }

    SetWindowTextW(g_hStatusLabel, successMsg.c_str());
    g_statusTextColor = RGB(20, 140, 30); // Green
    InvalidateRect(g_hStatusLabel, NULL, TRUE);
}

static void OnClearClicked() {
    SetWindowTextW(g_hInputField, L"");
    g_barcodeElements.clear();
    g_hasBarcodeData = false;
    InvalidateRect(g_hBarcodeView, NULL, TRUE);

    SetWindowTextW(g_hStatusLabel, L"Please enter a 6-digit code.");
    g_statusTextColor = RGB(110, 110, 110); // Secondary gray
    InvalidateRect(g_hStatusLabel, NULL, TRUE);

    SetFocus(g_hInputField);
}

// -------------------------------------------------------
// Auto-Layout on Window Resize
// -------------------------------------------------------
static void UpdateLayout(int clientW, int clientH) {
    if (!g_hInputField || !g_hBarcodeView || !g_hStatusLabel) return;

    const int margin      = 40;
    const int btnW        = 100;
    const int btnH        = 36;
    const int inputH      = 36;
    const int spacing     = 10;
    const int statusH     = 46;
    const int bottomSpace = 20;

    // Row 1: Title
    MoveWindow(g_hTitleLabel, margin, 24, clientW - 2 * margin, 32, TRUE);

    // Row 2: Subtitle
    MoveWindow(g_hSubtitleLbl, margin, 60, clientW - 2 * margin, 22, TRUE);

    // Row 3: [Input Field]  [Generate]  [Clear]
    int row3Y = 96;
    int inputW = clientW - 2 * margin - 2 * btnW - 2 * spacing;
    if (inputW < 120) inputW = 120;

    int genX   = margin + inputW + spacing;
    int clearX = genX + btnW + spacing;

    MoveWindow(g_hInputField,  margin, row3Y, inputW, inputH, TRUE);
    MoveWindow(g_hGenerateBtn, genX,   row3Y, btnW,   btnH,   TRUE);
    MoveWindow(g_hClearBtn,    clearX, row3Y, btnW,   btnH,   TRUE);

    // Row 4: Barcode View
    int row4Y = row3Y + inputH + 20;
    int barcodeH = clientH - row4Y - statusH - bottomSpace - 14;
    if (barcodeH < 200) barcodeH = 200;

    MoveWindow(g_hBarcodeView, margin, row4Y, clientW - 2 * margin, barcodeH, TRUE);

    // Row 5: Status Label
    int row5Y = row4Y + barcodeH + 14;
    MoveWindow(g_hStatusLabel, margin, row5Y, clientW - 2 * margin, statusH, TRUE);
}

// -------------------------------------------------------
// Main Window Procedure
// -------------------------------------------------------
static LRESULT CALLBACK MainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            InitCommonControls();

            // Create fonts
            g_hTitleFont = CreateFontW(-22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

            g_hSubtitleFont = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                          CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

            g_hInputFont = CreateFontW(-20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                       CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");

            g_hButtonFont = CreateFontW(-14, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

            g_hStatusFont = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

            g_hPlaceholderFont = CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                             CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

            g_hWindowBgBrush = CreateSolidBrush(RGB(246, 247, 249));

            // Title label
            g_hTitleLabel = CreateWindowExW(
                0, L"STATIC", L"🔢 Barcode Generator",
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                0, 0, 0, 0, hWnd, (HMENU)IDC_TITLE_LABEL, g_hInstance, NULL);
            SendMessageW(g_hTitleLabel, WM_SETFONT, (WPARAM)g_hTitleFont, TRUE);

            // Subtitle label
            g_hSubtitleLbl = CreateWindowExW(
                0, L"STATIC", L"Enter a 6-digit numeric code and press Generate.",
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                0, 0, 0, 0, hWnd, (HMENU)IDC_SUBTITLE_LBL, g_hInstance, NULL);
            SendMessageW(g_hSubtitleLbl, WM_SETFONT, (WPARAM)g_hSubtitleFont, TRUE);

            // Input field
            g_hInputField = CreateWindowExW(
                WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | ES_CENTER | ES_AUTOHSCROLL,
                0, 0, 0, 0, hWnd, (HMENU)IDC_INPUT_FIELD, g_hInstance, NULL);
            SendMessageW(g_hInputField, WM_SETFONT, (WPARAM)g_hInputFont, TRUE);
            SendMessageW(g_hInputField, EM_SETCUEBANNER, TRUE, (LPARAM)L"e.g.  123456");
            SetWindowSubclass(g_hInputField, EditSubclassProc, 1, 0);

            // Generate button
            g_hGenerateBtn = CreateWindowExW(
                0, L"BUTTON", L"Generate",
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                0, 0, 0, 0, hWnd, (HMENU)IDC_GENERATE_BTN, g_hInstance, NULL);
            SendMessageW(g_hGenerateBtn, WM_SETFONT, (WPARAM)g_hButtonFont, TRUE);

            // Clear button
            g_hClearBtn = CreateWindowExW(
                0, L"BUTTON", L"Clear",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                0, 0, 0, 0, hWnd, (HMENU)IDC_CLEAR_BTN, g_hInstance, NULL);
            SendMessageW(g_hClearBtn, WM_SETFONT, (WPARAM)g_hButtonFont, TRUE);

            // Barcode view
            g_hBarcodeView = CreateWindowExW(
                0, L"BarcodeViewClass", L"",
                WS_CHILD | WS_VISIBLE,
                0, 0, 0, 0, hWnd, (HMENU)IDC_BARCODE_VIEW, g_hInstance, NULL);

            // Status label
            g_hStatusLabel = CreateWindowExW(
                0, L"STATIC", L"Please enter a 6-digit code.",
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                0, 0, 0, 0, hWnd, (HMENU)IDC_STATUS_LABEL, g_hInstance, NULL);
            SendMessageW(g_hStatusLabel, WM_SETFONT, (WPARAM)g_hStatusFont, TRUE);

            return 0;
        }

        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            if (wmId == IDC_GENERATE_BTN) {
                OnGenerateClicked();
                return 0;
            }
            if (wmId == IDC_CLEAR_BTN) {
                OnClearClicked();
                return 0;
            }
            break;
        }

        case WM_CTLCOLORSTATIC: {
            HDC hdcStatic = (HDC)wParam;
            HWND hCtrl    = (HWND)lParam;
            if (hCtrl == g_hStatusLabel) {
                SetBkMode(hdcStatic, TRANSPARENT);
                SetTextColor(hdcStatic, g_statusTextColor);
                return (INT_PTR)g_hWindowBgBrush;
            }
            if (hCtrl == g_hTitleLabel || hCtrl == g_hSubtitleLbl) {
                SetBkMode(hdcStatic, TRANSPARENT);
                if (hCtrl == g_hSubtitleLbl) {
                    SetTextColor(hdcStatic, RGB(110, 110, 110));
                } else {
                    SetTextColor(hdcStatic, RGB(30, 30, 30));
                }
                return (INT_PTR)g_hWindowBgBrush;
            }
            break;
        }

        case WM_ERASEBKGND: {
            HDC hdc = (HDC)wParam;
            RECT rc;
            GetClientRect(hWnd, &rc);
            FillRect(hdc, &rc, g_hWindowBgBrush);
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
            lpMMI->ptMinTrackSize.x = 600;
            lpMMI->ptMinTrackSize.y = 450;
            return 0;
        }

        case WM_DESTROY: {
            if (g_hTitleFont)       DeleteObject(g_hTitleFont);
            if (g_hSubtitleFont)    DeleteObject(g_hSubtitleFont);
            if (g_hInputFont)       DeleteObject(g_hInputFont);
            if (g_hButtonFont)      DeleteObject(g_hButtonFont);
            if (g_hStatusFont)      DeleteObject(g_hStatusFont);
            if (g_hPlaceholderFont) DeleteObject(g_hPlaceholderFont);
            if (g_hWindowBgBrush)   DeleteObject(g_hWindowBgBrush);

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
static int RunApplication(HINSTANCE hInstance, int nCmdShow) {
    g_hInstance = hInstance;

    // Enable modern visual styling
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);

    // Initialize GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // Register BarcodeView class
    WNDCLASSEXW bvc = {0};
    bvc.cbSize        = sizeof(WNDCLASSEXW);
    bvc.style         = CS_HREDRAW | CS_VREDRAW;
    bvc.lpfnWndProc   = BarcodeViewProc;
    bvc.hInstance     = hInstance;
    bvc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    bvc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    bvc.lpszClassName = L"BarcodeViewClass";
    RegisterClassExW(&bvc);

    // Register Main Window class
    WNDCLASSEXW wc = {0};
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = MainWindowProc;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(246, 247, 249));
    wc.lpszClassName = L"BarcodeGeneratorMainClass";
    RegisterClassExW(&wc);

    // Initial window size: 900 × 650
    const int initW = 900;
    const int initH = 650;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int posX    = (screenW - initW) / 2;
    int posY    = (screenH - initH) / 2;

    g_hMainWindow = CreateWindowExW(
        0,
        L"BarcodeGeneratorMainClass",
        L"Barcode Generator — Code 39",
        WS_OVERLAPPEDWINDOW,
        posX, posY, initW, initH,
        NULL, NULL, hInstance, NULL);

    if (!g_hMainWindow) {
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return 1;
    }

    ShowWindow(g_hMainWindow, nCmdShow);
    UpdateWindow(g_hMainWindow);

    // Message loop
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    Gdiplus::GdiplusShutdown(gdiplusToken);
    return static_cast<int>(msg.wParam);
}

// Standard Windows subsystem entry point
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    return RunApplication(hInstance, nCmdShow);
}

// Fallback for console / standard main entry point
int main(int /*argc*/, char* /*argv*/[]) {
    return RunApplication(GetModuleHandleW(NULL), SW_SHOWDEFAULT);
}

#endif // _WIN32
