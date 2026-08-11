#pragma once

#include <QSyntaxHighlighter>
#include <QHash>
#include <QRegularExpression>
#include <QTextCharFormat>

namespace NezhaIDE::Editor {

class CppHighlighter final : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit CppHighlighter(QTextDocument *document, QObject *parent = nullptr);
    void setTokenColors(const QHash<QString, QColor> &tokens);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    void buildRules();

    QTextCharFormat fmt_keyword_;
    QTextCharFormat fmt_type_;
    QTextCharFormat fmt_function_;
    QTextCharFormat fmt_string_;
    QTextCharFormat fmt_comment_;
    QTextCharFormat fmt_number_;
    QTextCharFormat fmt_operator_;
    QTextCharFormat fmt_preprocessor_;

    QList<HighlightRule> rules_;
};

} // namespace NezhaIDE::Editor
