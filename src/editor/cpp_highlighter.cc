#include "cpp_highlighter.h"
#include "src/services/theme_service.h"

namespace NezhaIDE::Editor {

static const QStringList kCppKeywords = {
    "alignas", "alignof", "and", "and_eq", "asm", "auto",
    "bitand", "bitor", "bool", "break", "case", "catch",
    "char", "char8_t", "char16_t", "char32_t", "class",
    "compl", "concept", "const", "consteval", "constexpr",
    "constinit", "const_cast", "continue", "co_await",
    "co_return", "co_yield", "decltype", "default", "delete",
    "do", "double", "dynamic_cast", "else", "enum", "explicit",
    "export", "extern", "false", "float", "for", "friend",
    "goto", "if", "inline", "int", "long", "mutable",
    "namespace", "new", "noexcept", "not", "not_eq", "nullptr",
    "operator", "or", "or_eq", "override", "private", "protected",
    "public", "register", "reinterpret_cast", "requires", "return",
    "short", "signed", "sizeof", "static", "static_assert",
    "static_cast", "struct", "switch", "template", "this",
    "thread_local", "throw", "true", "try", "typedef", "typeid",
    "typename", "union", "unsigned", "using", "virtual", "void",
    "volatile", "wchar_t", "while", "xor", "xor_eq",
};

static const QStringList kCppTypes = {
    "bool", "char", "char8_t", "char16_t", "char32_t",
    "double", "float", "int", "long", "short", "signed",
    "unsigned", "void", "wchar_t",
    "size_t", "ssize_t", "ptrdiff_t", "int8_t", "int16_t",
    "int32_t", "int64_t", "uint8_t", "uint16_t", "uint32_t",
    "uint64_t", "nullptr_t", "max_align_t", "byte",
    "QString", "QList", "QHash", "QMap", "QSet", "QVector",
    "QObject", "QWidget", "QStringView", "QByteArray",
    "std::string", "std::wstring", "std::u8string",
    "std::vector", "std::array", "std::map", "std::unordered_map",
    "std::set", "std::shared_ptr", "std::unique_ptr",
    "std::optional", "std::expected", "std::string_view",
};

CppHighlighter::CppHighlighter(QTextDocument *document, QObject *parent)
    : QSyntaxHighlighter(document)
{
    Q_UNUSED(parent);
    setTokenColors(NezhaIDE::Services::ThemeService::instance().syntaxColors());
}

void CppHighlighter::setTokenColors(const QHash<QString, QColor> &tokens)
{
    auto setFmt = [&](QTextCharFormat &fmt, const QString &key) {
        fmt.setForeground(tokens.value(key, QColor("#000000")));
    };

    setFmt(fmt_keyword_, QStringLiteral("syntax.keyword"));
    fmt_keyword_.setFontWeight(QFont::Bold);
    setFmt(fmt_type_, QStringLiteral("syntax.type"));
    setFmt(fmt_function_, QStringLiteral("syntax.function"));
    setFmt(fmt_string_, QStringLiteral("syntax.string"));
    setFmt(fmt_comment_, QStringLiteral("syntax.comment"));
    fmt_comment_.setFontItalic(true);
    setFmt(fmt_number_, QStringLiteral("syntax.number"));
    setFmt(fmt_operator_, QStringLiteral("syntax.operator"));
    setFmt(fmt_preprocessor_, QStringLiteral("syntax.preprocessor"));

    buildRules();
    rehighlight();
}

void CppHighlighter::buildRules()
{
    rules_.clear();

    const auto kwPattern = QStringLiteral("\\b(") + kCppKeywords.join("|") + QStringLiteral(")\\b");
    rules_.append({QRegularExpression(kwPattern), fmt_keyword_});

    const auto typePattern = QStringLiteral("\\b(") + kCppTypes.join("|") + QStringLiteral(")\\b");
    rules_.append({QRegularExpression(typePattern), fmt_type_});

    rules_.append({QRegularExpression(QStringLiteral(R"(\b[A-Za-z_]\w*(?=\s*\())")), fmt_function_});

    rules_.append({QRegularExpression(QStringLiteral(R"("[^"\\]*(\\.[^"\\]*)*")")), fmt_string_});
    rules_.append({QRegularExpression(QStringLiteral("'[^'\\\\]*(\\\\.[^'\\\\]*)*'")), fmt_string_});

    rules_.append({QRegularExpression(QStringLiteral("//[^\n]*")), fmt_comment_});

    rules_.append({QRegularExpression(QStringLiteral("\\b(0[bB][01]+|0[oO]?[0-7]+|0[xX][0-9a-fA-F]+|[0-9]+\\.[0-9]*[fF]?|\\.[0-9]+[fF]?|[0-9]+[fF]?)\\b")), fmt_number_});

    rules_.append({QRegularExpression(QStringLiteral(R"((->|\+\+|--|\+=|-=|\*=|/=|%=|&=|\|=|\^=|<<=|>>=|->\*|::|<<|>>|<=>|<=|>=|==|!=|&&|\|\||[+\-*/%&|^~!<>]=?))")), fmt_operator_});

    rules_.append({QRegularExpression(QStringLiteral("^\\s*#.*")), fmt_preprocessor_});
}

void CppHighlighter::highlightBlock(const QString &text)
{
    const auto prevState = previousBlockState();
    setCurrentBlockState(0);

    int blockCommentStart = 0;
    int blockCommentLen = 0;

    if (prevState == 1) {
        const int end = text.indexOf(QStringLiteral("*/"));
        if (end >= 0) {
            blockCommentLen = end + 2;
        } else {
            setCurrentBlockState(1);
            setFormat(0, text.length(), fmt_comment_);
            return;
        }
    }

    const int openComment = text.indexOf(QStringLiteral("/*"), blockCommentLen);
    if (openComment >= 0) {
        const int closeComment = text.indexOf(QStringLiteral("*/"), openComment + 2);
        if (closeComment >= 0) {
            setFormat(openComment, closeComment - openComment + 2, fmt_comment_);
        } else {
            setCurrentBlockState(1);
            setFormat(openComment, text.length() - openComment, fmt_comment_);
        }
    }

    for (const auto &rule : rules_) {
        auto it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            const auto match = it.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}

} // namespace NezhaIDE::Editor
