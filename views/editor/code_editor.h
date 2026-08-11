#pragma once

#include <QPlainTextEdit>
#include <QWidget>

class QSyntaxHighlighter;

namespace NezhaIDE::Editor {

class CodeEditor final : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit CodeEditor(const QString &filePath, QWidget *parent = nullptr);
    ~CodeEditor() override;

    [[nodiscard]] QString filePath() const;
    [[nodiscard]] bool load();
    [[nodiscard]] bool save();
    [[nodiscard]] bool isModified() const;
    void applyTheme();

signals:
    void titleChanged(const QString &title);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    class LineNumberArea : public QWidget {
    public:
        explicit LineNumberArea(CodeEditor *editor) : QWidget(editor), editor_(editor) {}
        [[nodiscard]] QSize sizeHint() const override {
            return {editor_->lineNumberAreaWidth(), 0};
        }
    protected:
        void paintEvent(QPaintEvent *event) override { editor_->lineNumberAreaPaintEvent(event); }
    private:
        CodeEditor *editor_;
    };

    friend class LineNumberArea;

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    [[nodiscard]] int lineNumberAreaWidth() const;
    void updateLineNumberAreaWidth(int newBlockCount = 0);
    void updateLineNumberArea(const QRect &rect, int dy);
    void highlightCurrentLine();

    QWidget *line_number_area_{};
    QSyntaxHighlighter *highlighter_{};
    QString file_path_;
};

} // namespace NezhaIDE::Editor
