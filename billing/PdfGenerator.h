#pragma once
#include "Bill.h"
#include <QString>

namespace billing {

/**
 * Generates a printable PDF for a completed Bill.
 * The PDF is written to the path passed in `outputPath` –
 * typically something like "output/Bill_<ID>.pdf".
 * Uses Qt6's QPdfWriter (part of the QtPdf module) and QPainter.
 */
class PdfGenerator {
public:
    /**
     * Create a PDF file for the given bill.
     * @param bill The fully populated Bill (totals already computed).
     * @param outputPath Full filesystem path where the PDF should be saved.
     * @return true on success, false on failure (e.g., cannot open file).
     */
    static bool createPdf(const Bill &bill, const QString &outputPath);
};

} // namespace billing
