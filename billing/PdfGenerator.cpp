#include "PdfGenerator.h"
#include "Bill.h"
#include <QPdfWriter>
#include <QPainter>
#include <QFont>
#include <QDateTime>

namespace billing {

bool PdfGenerator::createPdf(const Bill &bill, const QString &outputPath) {
    // Initialise PDF writer
    QPdfWriter writer(outputPath);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(300);
    QPainter painter(&writer);
    if (!painter.isActive()) {
        return false; // cannot open file
    }

    // Simple layout constants
    const int margin = 30;
    int y = margin;
    QFont titleFont("Arial", 18, QFont::Bold);
    QFont normalFont("Arial", 10);
    QFont headerFont("Arial", 12, QFont::Bold);

    // Title
    painter.setFont(titleFont);
    painter.drawText(margin, y, QString("Bill ID: %1").arg(QString::fromStdString(bill.billId)));
    y += 40;

    // Header info (customer, order, date)
    painter.setFont(normalFont);
    QDateTime dt = QDateTime::fromSecsSinceEpoch(bill.billTimestamp);
    painter.drawText(margin, y, QString("Customer: %1").arg(QString::fromStdString(bill.customerName)));
    y += 20;
    painter.drawText(margin, y, QString("Order ID: %1").arg(QString::fromStdString(bill.orderId)));
    y += 20;
    painter.drawText(margin, y, QString("Date: %1").arg(dt.toString(Qt::ISODate)));
    y += 30;

    // Table header for items
    painter.setFont(headerFont);
    painter.drawText(margin, y, "Product");
    painter.drawText(margin + 150, y, "Unit Price");
    painter.drawText(margin + 250, y, "Qty");
    painter.drawText(margin + 300, y, "Discount %");
    painter.drawText(margin + 380, y, "Line Total");
    y += 20;
    painter.drawLine(margin, y, writer.width() - margin, y);
    y += 10;

    // Items rows
    painter.setFont(normalFont);
    for (const auto &item : bill.items) {
        double line = item.unitPriceWithoutTax * item.quantity;
        double lineAfterDiscount = line * (1.0 - item.discountPercent / 100.0);
        painter.drawText(margin, y, QString::fromStdString(item.productCode));
        painter.drawText(margin + 150, y, QString::asprintf("%.2f", item.unitPriceWithoutTax));
        painter.drawText(margin + 250, y, QString::number(item.quantity));
        painter.drawText(margin + 300, y, QString::asprintf("%.2f", item.discountPercent));
        painter.drawText(margin + 380, y, QString::asprintf("%.2f", lineAfterDiscount));
        y += 20;
        if (y > writer.height() - margin) {
            writer.newPage();
            y = margin;
        }
    }

    y += 10;
    painter.drawLine(margin, y, writer.width() - margin, y);
    y += 20;

    // Totals summary
    painter.setFont(headerFont);
    auto drawPair = [&](const QString &label, double value) {
        painter.drawText(margin + 250, y, label);
        painter.drawText(margin + 380, y, QString::asprintf("%.2f", value));
        y += 20;
    };
    drawPair("Subtotal (w/o tax):", bill.totalWithoutTax);
    drawPair("Product Discounts:", bill.totalProductDiscounts);
    drawPair("Seasonal Discount:", bill.seasonalDiscount);
    drawPair("Special Discount:", bill.specialDiscount);
    drawPair("Tax (10%):", bill.totalTax);
    drawPair("Total Payable:", bill.totalPayable);
    drawPair("Amount Received:", bill.amountReceived);
    drawPair("Balance Returned:", bill.balanceReturned);

    painter.end();
    return true;
}

} // namespace billing
