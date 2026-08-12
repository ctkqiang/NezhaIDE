#include "code_editor.h"
#include "cpp_highlighter.h"
#include "simple_highlighter.h"
#include "language_registry.h"
#include "src/services/localization_service.h"
#include "src/services/theme_service.h"
#include "src/utilities/logger.h"
#include <QFile>
#include <QFileInfo>
#include <QKeySequence>
#include <QMessageBox>
#include <QPainter>
#include <QShortcut>
#include <QTextBlock>

namespace NezhaIDE::Editor {

CodeEditor::CodeEditor(const QString &filePath, QWidget *parent)
    : QPlainTextEdit(parent), file_path_(filePath)
{
    line_number_area_ = new LineNumberArea(this);
    line_number_area_->setObjectName(QStringLiteral("lineNumberArea"));

    connect(this, &QPlainTextEdit::blockCountChanged,
            this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest,
            this, &CodeEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged,
            this, &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);

    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * 4);

    highlighter_ = LanguageRegistry::instance().createHighlighter(
        filePath, document(), this);

    applyTheme();

    connect(&NezhaIDE::Services::ThemeService::instance(),
            &NezhaIDE::Services::ThemeService::themeChanged,
            this, &CodeEditor::applyTheme);

    connect(document(), &QTextDocument::modificationChanged,
            this, [this](bool changed) {
        const auto title = QFileInfo(file_path_).fileName();
        emit titleChanged(changed ? QStringLiteral("* ") + title : title);
    });

    auto *saveShortcut = new QShortcut(QKeySequence::Save, this);
    saveShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(saveShortcut, &QShortcut::activated, this, &CodeEditor::save);
}

CodeEditor::~CodeEditor() = default;

QString CodeEditor::filePath() const { return file_path_; }

bool CodeEditor::load()
{
    QFile file(file_path_);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        NezhaIDE::Utilities::Logger::instance().log(
            NezhaIDE::Utilities::LogLevel::Error, __FILE__, __LINE__, __func__,
            "文件读取失败: {} error={}", file_path_.toStdString(), file.errorString().toStdString());
        QMessageBox::warning(
            const_cast<CodeEditor *>(this),
            LOC("error.title"),
            LOC("editor.file_read_error").arg(file.errorString()));
        return false;
    }

    setPlainText(QString::fromUtf8(file.readAll()));
    document()->setModified(false);
    return true;
}

bool CodeEditor::save()
{
    QFile file(file_path_);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        NezhaIDE::Utilities::Logger::instance().log(
            NezhaIDE::Utilities::LogLevel::Error, __FILE__, __LINE__, __func__,
            "文件保存失败: {} error={}", file_path_.toStdString(), file.errorString().toStdString());
        QMessageBox::warning(
            const_cast<CodeEditor *>(this),
            LOC("error.title"),
            LOC("editor.file_read_error").arg(file.errorString()));
        return false;
    }

    file.write(toPlainText().toUtf8());
    document()->setModified(false);
    return true;
}

bool CodeEditor::isModified() const
{
    return document()->isModified();
}

void CodeEditor::applyTheme()
{
    auto &ts = NezhaIDE::Services::ThemeService::instance();
    setStyleSheet(ts.qss(QStringLiteral("style.editor")));

    const auto tokenColors = ts.syntaxColors();
    if (auto *ch = dynamic_cast<CppHighlighter *>(highlighter_)) {
        ch->setTokenColors(tokenColors);
    } else if (auto *sh = dynamic_cast<SimpleHighlighter *>(highlighter_)) {
        sh->setTokenColors(tokenColors);
    }
}

void CodeEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);

    auto cr = contentsRect();
    cr.setWidth(lineNumberAreaWidth());
    line_number_area_->setGeometry(cr);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(line_number_area_);
    auto &ts = NezhaIDE::Services::ThemeService::instance();
    painter.fillRect(event->rect(), ts.qcolor(QStringLiteral("bg.secondary")));

    auto block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    const auto fgColor = ts.qcolor(QStringLiteral("text.tertiary"));

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            const auto number = QString::number(blockNumber + 1);
            painter.setPen(fgColor);
            painter.drawText(0, top, line_number_area_->width() - 6,
                             fontMetrics().height(),
                             Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

int CodeEditor::lineNumberAreaWidth() const
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) { max /= 10; ++digits; }
    return 12 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void CodeEditor::updateLineNumberAreaWidth(int)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy) {
        line_number_area_->scroll(0, dy);
    } else {
        line_number_area_->update(0, rect.y(), line_number_area_->width(), rect.height());
    }

    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth(0);
    }
}

void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extras;
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection sel;
        sel.format.setBackground(
            NezhaIDE::Services::ThemeService::instance().qcolor(QStringLiteral("overlay.selection")));
        sel.format.setProperty(QTextFormat::FullWidthSelection, true);
        sel.cursor = textCursor();
        sel.cursor.clearSelection();
        extras.append(sel);
    }
    setExtraSelections(extras);
}

} // namespace NezhaIDE::Editor
