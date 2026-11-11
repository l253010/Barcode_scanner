// main.mm
// -------------------------------------------------------
// Barcode Generator — single Objective-C++ source file.
//
// Contains (top to bottom):
//   1. BarcodeView   — custom NSView that draws the barcode.
//   2. AppDelegate   — window, UI controls, button handlers,
//                      and PNG image saving.
//   3. main()        — application entry point.
//
// This file must remain .mm (Objective-C++) because it uses
// macOS Cocoa APIs (NSView, NSWindow, NSBitmapImageRep, …).
// Pure barcode logic lives in BarcodeGenerator.cpp / .h.
// -------------------------------------------------------

#import <Cocoa/Cocoa.h>
#include "BarcodeGenerator.h"
#include <vector>

// ═══════════════════════════════════════════════════════
// SECTION 1 — BarcodeView
// A custom NSView that draws Code 39 bars as black/white
// rectangles.  No barcode logic here — it only renders
// the BarElement list it is given.
// ═══════════════════════════════════════════════════════

@interface BarcodeView : NSView {
    // The sequence of bar/space elements to draw.
    // Empty means "no barcode yet".
    std::vector<BarElement> _elements;

    // Whether there is a barcode ready to draw.
    BOOL _hasBarcodeData;
}

// Call this to load a new barcode (or clear the view).
- (void)setElements:(const std::vector<BarElement>&)elements;
- (void)clearBarcode;

@end

@implementation BarcodeView

// initWithFrame: — called when the view is first created.
- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        _hasBarcodeData = NO;
    }
    return self;
}

// setElements: — store bar data and trigger a redraw.
- (void)setElements:(const std::vector<BarElement>&)elements {
    _elements       = elements;   // copy the vector
    _hasBarcodeData = YES;
    [self setNeedsDisplay:YES];   // ask macOS to call drawRect:
}

// clearBarcode — remove the current barcode.
- (void)clearBarcode {
    _elements.clear();
    _hasBarcodeData = NO;
    [self setNeedsDisplay:YES];
}

// drawRect: — called by macOS whenever the view needs to be painted.
- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];

    NSRect bounds = [self bounds];

    // ---- Draw background ----
    [[NSColor whiteColor] setFill];
    NSRectFill(bounds);

    // ---- Draw a subtle border around the view ----
    [[NSColor lightGrayColor] setStroke];
    NSBezierPath* border = [NSBezierPath bezierPathWithRect:
        NSInsetRect(bounds, 0.5, 0.5)];
    [border setLineWidth:1.0];
    [border stroke];

    // ---- If no barcode yet, show a placeholder message ----
    if (!_hasBarcodeData || _elements.empty()) {
        NSString* placeholder = @"Barcode will appear here";
        NSDictionary* attrs = @{
            NSFontAttributeName:            [NSFont systemFontOfSize:13],
            NSForegroundColorAttributeName: [NSColor lightGrayColor]
        };
        NSSize textSize = [placeholder sizeWithAttributes:attrs];
        NSPoint textOrigin = NSMakePoint(
            (bounds.size.width  - textSize.width)  / 2.0,
            (bounds.size.height - textSize.height) / 2.0
        );
        [placeholder drawAtPoint:textOrigin withAttributes:attrs];
        return;
    }

    // ---- Calculate total width in logical units ----
    int totalUnits = 0;
    for (const BarElement& el : _elements) {
        totalUnits += el.width;
    }
    if (totalUnits == 0) return;

    // ---- Drawing constants ----
    // Leave 20 px padding on each side and top/bottom.
    CGFloat padding    = 20.0;
    CGFloat barcodeH   = bounds.size.height - 2.0 * padding;
    CGFloat barcodeW   = bounds.size.width  - 2.0 * padding;
    CGFloat unitPixels = barcodeW / (CGFloat)totalUnits;

    // ---- Draw each element left-to-right ----
    CGFloat x = padding;

    for (const BarElement& el : _elements) {
        CGFloat elementW = unitPixels * (CGFloat)el.width;

        if (el.isBar) {
            [[NSColor blackColor] setFill];
            NSRect barRect = NSMakeRect(x, padding, elementW, barcodeH);
            NSRectFill(barRect);
        }
        // White spaces: background is already white — nothing to draw.

        x += elementW;
    }
}

@end


// ═══════════════════════════════════════════════════════
// SECTION 2 — AppDelegate
// Builds the window, wires up UI controls, and handles:
//   • Generate button → validate → encode → display → save PNG
//   • Clear button    → reset everything
// ═══════════════════════════════════════════════════════

//   ┌──────────────────────────────────────────────────┐
//   │             🔢 Barcode Generator                 │
//   │     Enter a 6-digit numeric code and press       │
//   │     Generate.                                    │
//   │                                                  │
//   │  ┌───────────────────────────┐ [Generate][Clear] │
//   │  │   e.g. 123456             │                   │
//   │  └───────────────────────────┘                   │
//   │                                                  │
//   │  ┌──────────────────────────────────────────┐    │
//   │  │       (Barcode drawn here — grows)       │    │
//   │  └──────────────────────────────────────────┘    │
//   │                                                  │
//   │  Status: "Please enter a 6-digit code."          │
//   └──────────────────────────────────────────────────┘

@interface AppDelegate : NSObject <NSApplicationDelegate> {
    NSWindow*     _window;
    NSTextField*  _inputField;   // where the user types
    NSTextField*  _statusLabel;  // shows validation messages
    BarcodeView*  _barcodeView;  // draws the barcode
}

- (void)onGenerateClicked:(id)sender;
- (void)onClearClicked:(id)sender;

@end

@implementation AppDelegate

// applicationDidFinishLaunching: — entry point after launch
- (void)applicationDidFinishLaunching:(NSNotification*)notification {

    // ---- Window setup ----
    // 900 × 650 initial size; resizable so the barcode area can grow.
    NSRect windowRect = NSMakeRect(0, 0, 900, 650);
    _window = [[NSWindow alloc]
        initWithContentRect: windowRect
                  styleMask: NSWindowStyleMaskTitled
                           | NSWindowStyleMaskClosable
                           | NSWindowStyleMaskMiniaturizable
                           | NSWindowStyleMaskResizable
                    backing: NSBackingStoreBuffered
                      defer: NO];

    [_window setTitle:@"Barcode Generator — Code 39"];
    [_window center];
    [_window setReleasedWhenClosed:NO];
    [_window setMinSize:NSMakeSize(600, 450)];

    NSView* contentView = [_window contentView];

    // ---- Title label ----
    NSTextField* titleLabel = [NSTextField labelWithString:@"🔢 Barcode Generator"];
    titleLabel.font      = [NSFont boldSystemFontOfSize:22];
    titleLabel.textColor = [NSColor labelColor];
    titleLabel.translatesAutoresizingMaskIntoConstraints = NO;
    [contentView addSubview:titleLabel];

    // ---- Subtitle / instructions ----
    NSTextField* subtitle = [NSTextField
        labelWithString:@"Enter a 6-digit numeric code and press Generate."];
    subtitle.font      = [NSFont systemFontOfSize:14];
    subtitle.textColor = [NSColor secondaryLabelColor];
    subtitle.translatesAutoresizingMaskIntoConstraints = NO;
    [contentView addSubview:subtitle];

    // ---- Input text field ----
    _inputField = [[NSTextField alloc] init];
    _inputField.placeholderString    = @"e.g.  123456";
    _inputField.font                 = [NSFont monospacedSystemFontOfSize:20
                                                                   weight:NSFontWeightRegular];
    _inputField.alignment            = NSTextAlignmentCenter;
    _inputField.translatesAutoresizingMaskIntoConstraints = NO;
    _inputField.maximumNumberOfLines = 1;
    [contentView addSubview:_inputField];

    // ---- Generate button ----
    NSButton* generateBtn = [NSButton buttonWithTitle:@"Generate"
                                               target:self
                                               action:@selector(onGenerateClicked:)];
    generateBtn.bezelStyle    = NSBezelStyleRounded;
    generateBtn.keyEquivalent = @"\r"; // Enter key triggers Generate
    generateBtn.translatesAutoresizingMaskIntoConstraints = NO;
    [contentView addSubview:generateBtn];

    // ---- Clear button ----
    NSButton* clearBtn = [NSButton buttonWithTitle:@"Clear"
                                            target:self
                                            action:@selector(onClearClicked:)];
    clearBtn.bezelStyle = NSBezelStyleRounded;
    clearBtn.translatesAutoresizingMaskIntoConstraints = NO;
    [contentView addSubview:clearBtn];

    // ---- Barcode view (custom drawing area) ----
    _barcodeView = [[BarcodeView alloc] initWithFrame:NSZeroRect];
    _barcodeView.translatesAutoresizingMaskIntoConstraints = NO;
    [contentView addSubview:_barcodeView];

    // ---- Status label (shows errors or success) ----
    _statusLabel = [NSTextField labelWithString:@"Please enter a 6-digit code."];
    _statusLabel.font                 = [NSFont systemFontOfSize:14];
    _statusLabel.textColor            = [NSColor secondaryLabelColor];
    _statusLabel.alignment            = NSTextAlignmentCenter;
    _statusLabel.lineBreakMode        = NSLineBreakByWordWrapping;
    _statusLabel.maximumNumberOfLines = 3;
    _statusLabel.translatesAutoresizingMaskIntoConstraints = NO;
    [contentView addSubview:_statusLabel];

    // ================================================================
    // AUTO LAYOUT
    // ================================================================

    // Row 1: title — centred horizontally, top margin 24
    [NSLayoutConstraint activateConstraints:@[
        [titleLabel.centerXAnchor constraintEqualToAnchor:contentView.centerXAnchor],
        [titleLabel.topAnchor     constraintEqualToAnchor:contentView.topAnchor constant:24],
    ]];

    // Row 2: subtitle — centred, 6 below title
    [NSLayoutConstraint activateConstraints:@[
        [subtitle.centerXAnchor constraintEqualToAnchor:contentView.centerXAnchor],
        [subtitle.topAnchor     constraintEqualToAnchor:titleLabel.bottomAnchor constant:6],
    ]];

    // Row 3: [inputField]  [Generate]  [Clear]
    [NSLayoutConstraint activateConstraints:@[
        [_inputField.topAnchor    constraintEqualToAnchor:subtitle.bottomAnchor constant:20],
        [_inputField.heightAnchor constraintEqualToConstant:36],
        [_inputField.leadingAnchor  constraintEqualToAnchor:contentView.leadingAnchor  constant:40],
        [generateBtn.leadingAnchor  constraintEqualToAnchor:_inputField.trailingAnchor constant:12],
        [clearBtn.leadingAnchor     constraintEqualToAnchor:generateBtn.trailingAnchor constant:8],
        [clearBtn.trailingAnchor    constraintEqualToAnchor:contentView.trailingAnchor constant:-40],
        [generateBtn.widthAnchor    constraintEqualToAnchor:clearBtn.widthAnchor],
        [generateBtn.topAnchor      constraintEqualToAnchor:_inputField.topAnchor],
        [clearBtn.topAnchor         constraintEqualToAnchor:_inputField.topAnchor],
    ]];

    // Row 4: barcode view — stretches to fill available height
    [NSLayoutConstraint activateConstraints:@[
        [_barcodeView.topAnchor      constraintEqualToAnchor:_inputField.bottomAnchor constant:20],
        [_barcodeView.leadingAnchor  constraintEqualToAnchor:contentView.leadingAnchor  constant:40],
        [_barcodeView.trailingAnchor constraintEqualToAnchor:contentView.trailingAnchor constant:-40],
        [_barcodeView.heightAnchor   constraintGreaterThanOrEqualToConstant:200],
    ]];

    // Row 5: status label — pinned below barcode view, above bottom edge
    [NSLayoutConstraint activateConstraints:@[
        [_statusLabel.topAnchor      constraintEqualToAnchor:_barcodeView.bottomAnchor constant:14],
        [_statusLabel.leadingAnchor  constraintEqualToAnchor:contentView.leadingAnchor  constant:40],
        [_statusLabel.trailingAnchor constraintEqualToAnchor:contentView.trailingAnchor constant:-40],
        [_statusLabel.bottomAnchor   constraintLessThanOrEqualToAnchor:contentView.bottomAnchor constant:-20],
    ]];

    [_window makeKeyAndOrderFront:nil];
}

// -------------------------------------------------------
// onGenerateClicked: — user pressed the Generate button
// -------------------------------------------------------
- (void)onGenerateClicked:(id)sender {
    // Get raw text from the input field
    std::string input = [[_inputField stringValue] UTF8String];

    // Validate using our C++ class
    ValidationResult result = BarcodeGenerator::validate(input);

    if (!result.valid) {
        // Show error in red
        _statusLabel.stringValue = [NSString stringWithUTF8String:result.message.c_str()];
        _statusLabel.textColor   = [NSColor systemRedColor];
        [_barcodeView clearBarcode];
        return;
    }

    // Validation passed — encode and display
    std::vector<BarElement> elements = BarcodeGenerator::encode(input);
    [_barcodeView setElements:elements];

    // -------------------------------------------------------
    // Save the barcode as a PNG image on disk.
    //
    // Rendered into an off-screen bitmap with Core Graphics
    // (a macOS system framework — not an external barcode
    // library).  Every pixel is produced by our own Code 39
    // encoder above.
    //
    // Output file: ~/Documents/barcode_<code>.png
    // -------------------------------------------------------
    const int imgW      = 600;
    const int imgH      = 200;
    const int paddingPx = 20;

    int totalUnits = 0;
    for (const BarElement& el : elements) totalUnits += el.width;

    NSString* savedPath = nil;
    if (totalUnits > 0) {
        CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
        CGContextRef ctx = CGBitmapContextCreate(
            nullptr,
            (size_t)imgW, (size_t)imgH,
            8,
            (size_t)imgW * 4,
            cs,
            kCGImageAlphaPremultipliedLast);
        CGColorSpaceRelease(cs);

        if (ctx) {
            // Fill background white
            CGContextSetRGBFillColor(ctx, 1, 1, 1, 1);
            CGContextFillRect(ctx, CGRectMake(0, 0, imgW, imgH));

            // Draw bars
            double barcodeW = imgW - 2.0 * paddingPx;
            double unitPx   = barcodeW / (double)totalUnits;
            double barcodeH = imgH - 2.0 * paddingPx;
            double x        = paddingPx;

            CGContextSetRGBFillColor(ctx, 0, 0, 0, 1); // black
            for (const BarElement& el : elements) {
                double elW = unitPx * el.width;
                if (el.isBar) {
                    CGContextFillRect(ctx, CGRectMake(x, paddingPx, elW, barcodeH));
                }
                x += elW;
            }

            CGImageRef cgImg = CGBitmapContextCreateImage(ctx);
            CGContextRelease(ctx);

            if (cgImg) {
                NSImage* nsImg = [[NSImage alloc] initWithCGImage:cgImg
                                                             size:NSMakeSize(imgW, imgH)];
                CGImageRelease(cgImg);

                NSString* filename = [NSString stringWithFormat:
                    @"barcode_%s.png", input.c_str()];
                NSString* docsDir = [NSSearchPathForDirectoriesInDomains(
                    NSDocumentDirectory, NSUserDomainMask, YES) firstObject];
                NSString* filePath = [docsDir stringByAppendingPathComponent:filename];

                NSBitmapImageRep* rep = [NSBitmapImageRep
                    imageRepWithData:[nsImg TIFFRepresentation]];
                NSData* pngData = [rep representationUsingType:NSBitmapImageFileTypePNG
                                                    properties:@{}];
                if ([pngData writeToFile:filePath atomically:YES]) {
                    savedPath = filePath;
                }
            }
        }
    }

    // Show success message (include save path if available)
    NSString* successMsg;
    if (savedPath) {
        successMsg = [NSString stringWithFormat:
            @"✅  Code 39 barcode generated for: %s  (with * start/stop)\n"
             "💾  Saved to: %@",
            input.c_str(), savedPath];
    } else {
        successMsg = [NSString stringWithFormat:
            @"✅  Code 39 barcode generated for: %s  (with * start/stop)\n"
             "⚠️  Could not save image — check Documents folder permissions.",
            input.c_str()];
    }
    _statusLabel.stringValue = successMsg;
    _statusLabel.textColor   = [NSColor systemGreenColor];
}

// -------------------------------------------------------
// onClearClicked: — user pressed the Clear button
// -------------------------------------------------------
- (void)onClearClicked:(id)sender {
    [_inputField setStringValue:@""];
    [_barcodeView clearBarcode];
    _statusLabel.stringValue = @"Please enter a 6-digit code.";
    _statusLabel.textColor   = [NSColor secondaryLabelColor];
    [_window makeFirstResponder:_inputField];
}

// Close the app when the window is closed
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
    return YES;
}

@end


// ═══════════════════════════════════════════════════════
// SECTION 3 — main()
// Creates the NSApplication, assigns the delegate, and
// starts the macOS event loop.
// ═══════════════════════════════════════════════════════

int main(int /*argc*/, const char* /*argv*/[]) {
    NSApplication* app = [NSApplication sharedApplication];
    [app setActivationPolicy:NSApplicationActivationPolicyRegular];

    AppDelegate* delegate = [[AppDelegate alloc] init];
    [app setDelegate:delegate];

    [app activateIgnoringOtherApps:YES];
    [app run];

    return 0;
}
