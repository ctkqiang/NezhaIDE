#include "hex_view.h"
#include "src/services/theme_service.h"
#include <QFontDatabase>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>

namespace NezhaIDE::Views {

HexView::HexView(QWidget *parent)
    : QAbstractScrollArea(parent)
{
    setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    setMouseTracking(true);
    viewport()->setCursor(Qt::IBeamCursor);
}

void HexView::setData(const uint8_t *data, size_t size) {
    data_ = data;
    data_size_ = size;
    sel_start_ = 0;
    sel_end_ = 0;
    updateScrollbars();
    viewport()->update();
}

void HexView::setSelection(const uint64_t start, const uint64_t size) {
    if (!data_ || start >= data_size_) return;
    sel_start_ = start;
    sel_end_ = std::min(start + size, static_cast<uint64_t>(data_size_));
    const auto y = static_cast<int>(start / kBytesPerRow) * rowHeight();
    auto *vs = verticalScrollBar();
    if (y < vs->value()) {
        vs->setValue(y);
    } else if (y + rowHeight() > vs->value() + viewport()->height()) {
        vs->setValue(y + rowHeight() - viewport()->height());
    }
    viewport()->update();
}

void HexView::navigateToOffset(uint64_t offset) {
    if (!data_) return;
    auto row = static_cast<int>(offset / kBytesPerRow);
    verticalScrollBar()->setValue(row * rowHeight());
    sel_start_ = offset;
    sel_end_ = offset + 1;
    viewport()->update();
    emit byteRangeSelected(sel_start_, 1);
}

void HexView::updateScrollbars() {
    auto rows = rowCount();
    auto h = rowHeight();
    verticalScrollBar()->setRange(0, std::max(0, rows * h - viewport()->height()));
    verticalScrollBar()->setSingleStep(h);
    verticalScrollBar()->setPageStep(viewport()->height());
    horizontalScrollBar()->setRange(0, 0);
    horizontalScrollBar()->hide();
}

int HexView::rowCount() const {
    if (!data_ || data_size_ == 0) return 0;
    return static_cast<int>((data_size_ + kBytesPerRow - 1) / kBytesPerRow);
}

int HexView::rowHeight() const {
    return fontMetrics().height() + 2;
}

int HexView::offsetColumnWidth() const {
    return fontMetrics().horizontalAdvance(QStringLiteral("00000000")) + 16;
}

int HexView::hexColumnWidth() const {
    return fontMetrics().horizontalAdvance(
        QStringLiteral("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00")) + 16;
}

int HexView::asciiColumnX() const {
    return offsetColumnWidth() + hexColumnWidth();
}

void HexView::resizeEvent(QResizeEvent *event) {
    QAbstractScrollArea::resizeEvent(event);
    updateScrollbars();
}

int HexView::byteAtPosition(const QPoint &pos) const {
    int x = pos.x() - offsetColumnWidth();
    if (x < 0 || x >= hexColumnWidth()) return -1;
    int byteIdx = x / (fontMetrics().horizontalAdvance(QStringLiteral("00 ")));
    return std::min(byteIdx, kBytesPerRow - 1);
}

uint64_t HexView::byteOffsetAtPosition(const QPoint &pos) const {
    int scrollY = verticalScrollBar()->value();
    int rh = rowHeight();
    int row = (pos.y() + scrollY) / rh;
    int col = byteAtPosition(pos);
    if (col < 0) return UINT64_MAX;
    uint64_t offset = static_cast<uint64_t>(row) * kBytesPerRow + static_cast<uint64_t>(col);
    return offset < data_size_ ? offset : UINT64_MAX;
}

void HexView::mousePressEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton) return;
    auto off = byteOffsetAtPosition(event->pos());
    if (off == UINT64_MAX) return;
    selecting_ = true;
    drag_anchor_ = off;
    sel_start_ = off;
    sel_end_ = off + 1;
    viewport()->update();
    emit byteRangeSelected(sel_start_, 1);
}

void HexView::mouseMoveEvent(QMouseEvent *event) {
    if (!selecting_) return;
    auto off = byteOffsetAtPosition(event->pos());
    if (off == UINT64_MAX) off = data_size_ - 1;
    sel_start_ = std::min(drag_anchor_, off);
    sel_end_ = std::max(drag_anchor_, off) + 1;
    viewport()->update();
    emit byteRangeSelected(sel_start_, sel_end_ - sel_start_);
}

void HexView::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton) return;
    selecting_ = false;
}

void HexView::paintEvent(QPaintEvent *event) {
    QPainter painter(viewport());
    auto &ts = Services::ThemeService::instance();

    auto bg = ts.qcolor(QStringLiteral("bg.primary"));
    painter.fillRect(event->rect(), bg);

    if (!data_ || data_size_ == 0) return;

    auto fgOffset = ts.qcolor(QStringLiteral("text.tertiary"));
    auto fgData = ts.qcolor(QStringLiteral("text.primary"));
    auto fgAscii = ts.qcolor(QStringLiteral("text.secondary"));
    auto selBg = ts.qcolor(QStringLiteral("overlay.selection"));

    int rh = rowHeight();
    int scrollY = verticalScrollBar()->value();
    int firstRow = std::max(0, (event->rect().top() + scrollY) / rh);
    int lastRow = std::min(rowCount(), (event->rect().bottom() + scrollY) / rh + 1);

    int offW = offsetColumnWidth();
    int hexW = hexColumnWidth();
    int asciiX = asciiColumnX();

    int charW = fontMetrics().horizontalAdvance(QStringLiteral("0"));
    int hexCharW = fontMetrics().horizontalAdvance(QStringLiteral("00 "));

    for (int row = firstRow; row < lastRow; ++row) {
        int y = row * rh - scrollY;
        uint64_t rowOffset = static_cast<uint64_t>(row) * kBytesPerRow;

        QString offsetText = QStringLiteral("%1").arg(rowOffset, 8, 16, QLatin1Char('0'));
        painter.setPen(fgOffset);
        painter.drawText(8, y + fontMetrics().ascent(), offsetText);

        for (int col = 0; col < kBytesPerRow; ++col) {
            uint64_t byteOff = rowOffset + col;
            if (byteOff >= data_size_) break;

            int colX = offW + col * hexCharW;
            bool sel = byteOff >= sel_start_ && byteOff < sel_end_;

            if (sel) {
                painter.fillRect(colX, y, hexCharW - 1, rh, selBg);
            }

            QString hex = QStringLiteral("%1").arg(data_[byteOff], 2, 16, QLatin1Char('0'));
            painter.setPen(sel ? fgData : fgData);
            painter.drawText(colX, y + fontMetrics().ascent(), hex);
        }

        int asciiY = y + fontMetrics().ascent();
        for (int col = 0; col < kBytesPerRow; ++col) {
            uint64_t byteOff = rowOffset + col;
            if (byteOff >= data_size_) break;

            int charX = asciiX + col * charW;
            bool sel = byteOff >= sel_start_ && byteOff < sel_end_;
            if (sel) {
                painter.fillRect(charX - 1, y, charW, rh, selBg);
            }

            uint8_t b = data_[byteOff];
            QChar ch = (b >= 0x20 && b <= 0x7E) ? QChar(b) : QChar('.');
            painter.setPen(sel ? fgData : fgAscii);
            painter.drawText(charX, asciiY, QString(ch));
        }
    }
}

} // namespace NezhaIDE::Views
