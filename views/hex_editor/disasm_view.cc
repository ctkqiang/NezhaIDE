#include "disasm_view.h"
#include "src/services/theme_service.h"
#include <QPainter>
#include <QScrollBar>
#include <QTextBlock>

namespace NezhaIDE::Views {

// Qt 6.11 中 QPlainTextEdit 的 updateRequest 由 protected virtual 改为信号，
// 滚动/内容变化时经信号驱动行号区重绘
class DisasmView::LineNumberArea final : public QWidget {
public:
    explicit LineNumberArea(DisasmView *editor) : QWidget(editor), editor_(editor) {}

    QSize sizeHint() const override {
        const auto digits = QString::number(editor_->blockCount()).size();
        return {
            static_cast<int>(editor_->fontMetrics().horizontalAdvance(QLatin1Char('9'))
                             * (digits + 1)) + 12,
            0
        };
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        auto &ts = Services::ThemeService::instance();
        QPainter painter(this);
        painter.fillRect(event->rect(), ts.qcolor(QStringLiteral("bg.tertiary")));

        auto block = editor_->firstVisibleBlock();
        int top = static_cast<int>(editor_->blockBoundingGeometry(block)
                                       .translated(editor_->contentOffset()).top());
        int bottom = top + static_cast<int>(editor_->blockBoundingRect(block).height());
        const auto line_height = fontMetrics().height();
        const auto pen = ts.qcolor(QStringLiteral("text.tertiary"));

        while (block.isValid() && top <= event->rect().bottom()) {
            if (block.isVisible() && bottom >= event->rect().top()) {
                painter.setPen(pen);
                painter.drawText(0, top, width() - 6, line_height, Qt::AlignRight,
                                 QString::number(block.blockNumber() + 1));
            }
            block = block.next();
            top = bottom;
            bottom = top + static_cast<int>(editor_->blockBoundingRect(block).height());
        }
    }

private:
    DisasmView *editor_{};
};

DisasmView::DisasmView(QWidget *parent) : QPlainTextEdit(parent) {
    line_number_area_ = new LineNumberArea(this);
    updateLineNumberAreaWidth();

    connect(this, &QPlainTextEdit::updateRequest, this,
            [this](const QRect &rect, const int dy) {
                if (dy != 0) {
                    line_number_area_->scroll(0, dy);
                } else {
                    line_number_area_->update(0, rect.y(),
                                              line_number_area_->width(), rect.height());
                }
                if (rect.contains(viewport()->rect())) {
                    updateLineNumberAreaWidth();
                }
            });
}

void DisasmView::setInstructionHighlights(const QList<QTextEdit::ExtraSelection> &selections) {
    insn_highlights_ = selections;
    updateExtraSelections();
}

void DisasmView::setCurrentLine(const int line) {
    if (current_line_ == line) {
        return;
    }
    current_line_ = line;
    if (line >= 0) {
        const auto block = document()->findBlockByNumber(line);
        if (block.isValid()) {
            setTextCursor(QTextCursor(block));
            centerCursor();
        }
    }
    updateExtraSelections();
}

void DisasmView::refresh() {
    line_number_area_->update();
    updateExtraSelections();
}

void DisasmView::resizeEvent(QResizeEvent *event) {
    QPlainTextEdit::resizeEvent(event);
    line_number_area_->setGeometry(0, 0, line_number_area_->sizeHint().width(), height());
}

void DisasmView::updateLineNumberAreaWidth() {
    const auto width = line_number_area_->sizeHint().width();
    setViewportMargins(width, 0, 0, 0);
    line_number_area_->setGeometry(0, 0, width, height());
}

void DisasmView::updateExtraSelections() {
    auto &ts = Services::ThemeService::instance();
    auto all = insn_highlights_;
    if (current_line_ >= 0) {
        QTextEdit::ExtraSelection sel;
        sel.format.setBackground(ts.qcolor(QStringLiteral("overlay.hover")));
        sel.format.setProperty(QTextFormat::FullWidthSelection, true);
        QTextCursor cursor(document());
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, current_line_);
        sel.cursor = cursor;
        all.append(sel);
    }
    setExtraSelections(all);
}

} // namespace NezhaIDE::Views
