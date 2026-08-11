#pragma once

#include <QSyntaxHighlighter>
#include <QHash>
#include <QRegularExpression>
#include <QTextCharFormat>
#include <QStringList>

/**
 * 通用语法高亮器组件。
 */
namespace NezhaIDE::Editor {

/**
 * 语言定义结构体，描述一种编程语言的词法规则。
 *
 * 用于 SimpleHighlighter 的构造。支持关键词、类型、
 * 行注释、块注释、数字高亮和 HTTP header 高亮的配置。
 */
struct LanguageDefinition {
    QString name;
    QStringList keywords;
    QStringList types;
    QString lineCommentPrefix;   // "//" or "#"
    QString blockCommentStart;   // "/*" or "\"\"\"" or empty
    QString blockCommentEnd;     // "*/" or "\"\"\"" or empty
    bool hasNumberHighlight = true;
    bool hasHeaderHighlight = false;
};

/**
 * 基于正则表达式的通用语法高亮器。
 *
 * 根据 LanguageDefinition 配置规则集，通过 ThemeService::syntaxColors()
 * 获取主题颜色。支持关键词、类型名称、字符串、注释、数字和 HTTP header 的高亮。
 */
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
    QRegularExpression headerRx_;
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
LanguageDefinition languageJavascript();
LanguageDefinition languageTypescript();
LanguageDefinition languageCss();
LanguageDefinition languageHtml();
LanguageDefinition languageShell();
LanguageDefinition languageSql();
LanguageDefinition languageLua();
LanguageDefinition languageKotlin();
LanguageDefinition languageSwift();
LanguageDefinition languageHttp();

} // namespace NezhaIDE::Editor
