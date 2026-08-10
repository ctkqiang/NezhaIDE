#pragma once

#include <QSyntaxHighlighter>
#include <QHash>
#include <QRegularExpression>
#include <QTextCharFormat>
#include <QStringList>

namespace NezhaIDE::Editor {

struct LanguageDefinition {
    QString name;
    QStringList keywords;
    QStringList types;
    QString lineCommentPrefix;   // "//" or "#"
    QString blockCommentStart;   // "/*" or "\"\"\"" or empty
    QString blockCommentEnd;     // "*/" or "\"\"\"" or empty
    bool hasNumberHighlight = true;
};

class SimpleHighlighter final : public QSyntaxHighlighter {
    Q_OBJECT

public:
    SimpleHighlighter(const LanguageDefinition &def, QTextDocument *document, QObject *parent = nullptr);
    void setTokenColors(const QHash<QString, QColor> &tokens);

protected:
    void highlightBlock(const QString &text) override;

private:
    void buildRules();
    void highlightSingleLineComment(const QString &text);
    void highlightBlockComment(const QString &text);

    LanguageDefinition def_;
    QTextCharFormat fmt_keyword_;
    QTextCharFormat fmt_type_;
    QTextCharFormat fmt_string_;
    QTextCharFormat fmt_comment_;
    QTextCharFormat fmt_number_;
    QTextCharFormat fmt_preprocessor_;

    struct Rule { QRegularExpression pattern; QTextCharFormat format; };
    QList<Rule> rules_;
    QRegularExpression stringDouble_;
    QRegularExpression stringSingle_;
    QRegularExpression commentLine_;
    QRegularExpression commentBlockStart_;
    QRegularExpression commentBlockEnd_;
};

LanguageDefinition languageJson();
LanguageDefinition languageXml();
LanguageDefinition languagePython();
LanguageDefinition languageRuby();
LanguageDefinition languageDart();
LanguageDefinition languageRust();
LanguageDefinition languageGo();
LanguageDefinition languageC();
LanguageDefinition languageCpp();
LanguageDefinition languageCmake();
LanguageDefinition languageMarkdown();
LanguageDefinition languageYaml();

} // namespace NezhaIDE::Editor
