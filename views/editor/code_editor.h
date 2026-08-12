#pragma once

#include <QPlainTextEdit>
#include <QWidget>

class QSyntaxHighlighter;

/**
 * 代码编辑器命名空间。
 */
namespace NezhaIDE::Editor {

/**
 * 源代码文本编辑器，基于 QPlainTextEdit。
 *
 * 自带行号侧栏，根据 LanguageRegistry 自动创建语法高亮器。
 * 文件读写使用 UTF-8 编码。不支持二进制文件编辑。
 *
 * @note 声明为 final，不能被子类化。
 */
class CodeEditor final : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit CodeEditor(const QString &filePath, QWidget *parent = nullptr);
    ~CodeEditor() override;

    [[nodiscard]] QString filePath() const;

    /**
     * 加载文件内容到编辑器。
     *
     * @return true 表示读取成功。
     */
    [[nodiscard]] bool load();

    /**
     * 保存编辑器内容到文件。
     *
     * @return true 表示写入成功。
     */
    [[nodiscard]] bool save();
    [[nodiscard]] bool isModified() const;

    /**
     * 根据当前主题重新应用颜色（QSS + 语法高亮）。
     */
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
