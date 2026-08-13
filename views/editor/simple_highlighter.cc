#include "simple_highlighter.h"
#include "src/services/theme_service.h"

namespace NezhaIDE::Editor {

SimpleHighlighter::SimpleHighlighter(const LanguageDefinition &def, QTextDocument *document, QObject *parent)
    : QSyntaxHighlighter(document), def_(def)
{
    Q_UNUSED(parent);
    stringDouble_ = QRegularExpression(QStringLiteral(R"("[^"\\]*(\\.[^"\\]*)*")"));
    stringSingle_ = QRegularExpression(QStringLiteral("'[^'\\\\]*(\\\\.[^'\\\\]*)*'"));

    if (!def_.lineCommentPrefix.isEmpty()) {
        commentLine_ = QRegularExpression(
            QRegularExpression::escape(def_.lineCommentPrefix) + QStringLiteral("[^\n]*"));
    }
    if (!def_.blockCommentStart.isEmpty()) {
        commentBlockStart_ = QRegularExpression(
            QRegularExpression::escape(def_.blockCommentStart));
        commentBlockEnd_ = QRegularExpression(
            QRegularExpression::escape(def_.blockCommentEnd));
    }

    setTokenColors(NezhaIDE::Services::ThemeService::instance().syntaxColors());
}

void SimpleHighlighter::setTokenColors(const QHash<QString, QColor> &tokens)
{
    const QColor fallback = tokens.value(
        QStringLiteral("syntax.editor.foreground"),
        NezhaIDE::Services::ThemeService::instance().qcolor(QStringLiteral("syntax.editor.foreground")));
    auto setFmt = [&](QTextCharFormat &fmt, const QString &key) {
        fmt.setForeground(tokens.value(key, fallback));
    };
    setFmt(fmt_keyword_, QStringLiteral("syntax.keyword"));
    fmt_keyword_.setFontWeight(QFont::Bold);
    setFmt(fmt_type_, QStringLiteral("syntax.type"));
    setFmt(fmt_string_, QStringLiteral("syntax.string"));
    setFmt(fmt_comment_, QStringLiteral("syntax.comment"));
    fmt_comment_.setFontItalic(true);
    setFmt(fmt_number_, QStringLiteral("syntax.number"));
    setFmt(fmt_preprocessor_, QStringLiteral("syntax.preprocessor"));

    buildRules();
    rehighlight();
}

void SimpleHighlighter::buildRules()
{
    rules_.clear();

    if (!def_.keywords.isEmpty()) {
        const auto kw = QStringLiteral("\\b(") + def_.keywords.join("|") + QStringLiteral(")\\b");
        rules_.append({QRegularExpression(kw), fmt_keyword_});
    }
    if (!def_.types.isEmpty()) {
        const auto tp = QStringLiteral("\\b(") + def_.types.join("|") + QStringLiteral(")\\b");
        rules_.append({QRegularExpression(tp), fmt_type_});
    }
    if (def_.hasNumberHighlight) {
        rules_.append({QRegularExpression(QStringLiteral(
            "\\b(0[bB][01]+|0[oO]?[0-7]+|0[xX][0-9a-fA-F]+|[0-9]+\\.[0-9]*[fF]?|\\.[0-9]+[fF]?|[0-9]+[fF]?)\\b")),
            fmt_number_});
    }
    if (def_.hasHeaderHighlight) {
        headerRx_ = QRegularExpression(QStringLiteral(R"(^[ \t]*[A-Za-z][A-Za-z0-9\-]*:)"));
    } else {
        headerRx_ = QRegularExpression();
    }
}

void SimpleHighlighter::highlightBlock(const QString &text)
{
    highlightBlockComment(text);

    for (const auto &rule : rules_) {
        auto it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            const auto m = it.next();
            setFormat(m.capturedStart(), m.capturedLength(), rule.format);
        }
    }

    if (headerRx_.isValid() && !headerRx_.pattern().isEmpty()) {
        auto it = headerRx_.globalMatch(text);
        while (it.hasNext()) {
            const auto m = it.next();
            setFormat(m.capturedStart(), m.capturedLength(), fmt_type_);
        }
    }

    if (commentLine_.isValid() && !commentLine_.pattern().isEmpty()) {
        auto it = commentLine_.globalMatch(text);
        while (it.hasNext()) {
            const auto m = it.next();
            setFormat(m.capturedStart(), m.capturedLength(), fmt_comment_);
        }
    }

    if (stringDouble_.isValid()) {
        auto it = stringDouble_.globalMatch(text);
        while (it.hasNext()) {
            const auto m = it.next();
            setFormat(m.capturedStart(), m.capturedLength(), fmt_string_);
        }
    }
    if (stringSingle_.isValid()) {
        auto it = stringSingle_.globalMatch(text);
        while (it.hasNext()) {
            const auto m = it.next();
            setFormat(m.capturedStart(), m.capturedLength(), fmt_string_);
        }
    }
}

void SimpleHighlighter::highlightBlockComment(const QString &text)
{
    if (!commentBlockStart_.isValid() || commentBlockStart_.pattern().isEmpty()
        || !commentBlockEnd_.isValid() || commentBlockEnd_.pattern().isEmpty()) {
        return;
    }

    const auto prevState = previousBlockState();
    setCurrentBlockState(0);

    int scanFrom = 0;

    if (prevState == 1) {
        const int endIdx = text.indexOf(commentBlockEnd_, scanFrom);
        if (endIdx >= 0) {
            const int len = endIdx + commentBlockEnd_.pattern().length();
            setFormat(0, len, fmt_comment_);
            scanFrom = len;
        } else {
            setCurrentBlockState(1);
            setFormat(0, text.length(), fmt_comment_);
            return;
        }
    }

    while (scanFrom < text.length()) {
        const int startIdx = text.indexOf(commentBlockStart_, scanFrom);
        if (startIdx < 0) break;

        const int endIdx = text.indexOf(commentBlockEnd_, startIdx + commentBlockStart_.pattern().length());
        if (endIdx >= 0) {
            const int len = endIdx - startIdx + commentBlockEnd_.pattern().length();
            setFormat(startIdx, len, fmt_comment_);
            scanFrom = endIdx + commentBlockEnd_.pattern().length();
        } else {
            setCurrentBlockState(1);
            setFormat(startIdx, text.length() - startIdx, fmt_comment_);
            break;
        }
    }
}

LanguageDefinition languageJson() {
    LanguageDefinition def;
    def.name = "JSON";
    def.keywords = {"true", "false", "null"};
    def.hasNumberHighlight = true;
    return def;
}

LanguageDefinition languageXml() {
    LanguageDefinition def;
    def.name = "XML";
    def.blockCommentStart = "<!--";
    def.blockCommentEnd = "-->";
    def.hasNumberHighlight = false;
    return def;
}

LanguageDefinition languagePython() {
    LanguageDefinition def;
    def.name = "Python";
    def.keywords = {
        "False", "None", "True", "and", "as", "assert", "async", "await",
        "break", "class", "continue", "def", "del", "elif", "else", "except",
        "finally", "for", "from", "global", "if", "import", "in", "is",
        "lambda", "nonlocal", "not", "or", "pass", "raise", "return",
        "try", "while", "with", "yield", "match", "case"
    };
    def.types = {
        "int", "float", "str", "bytes", "bool", "list", "dict", "tuple",
        "set", "frozenset", "None", "type", "object", "range", "slice",
        "self", "cls"
    };
    def.lineCommentPrefix = "#";
    def.blockCommentStart = "\"\"\"";
    def.blockCommentEnd = "\"\"\"";
    def.hasNumberHighlight = true;
    return def;
}

LanguageDefinition languageRuby() {
    LanguageDefinition def;
    def.name = "Ruby";
    def.keywords = {
        "BEGIN", "END", "alias", "and", "begin", "break", "case", "class",
        "def", "defined?", "do", "else", "elsif", "end", "ensure", "false",
        "for", "if", "in", "module", "next", "nil", "not", "or", "redo",
        "rescue", "retry", "return", "self", "super", "then", "true",
        "undef", "unless", "until", "when", "while", "yield", "__FILE__", "__LINE__"
    };
    def.types = {
        "String", "Integer", "Float", "Array", "Hash", "Symbol", "Proc",
        "Lambda", "Range", "Regexp", "NilClass", "TrueClass", "FalseClass",
        "Numeric", "Object", "Module", "Class"
    };
    def.lineCommentPrefix = "#";
    def.blockCommentStart = "=begin";
    def.blockCommentEnd = "=end";
    def.hasNumberHighlight = true;
    return def;
}

LanguageDefinition languageDart() {
    LanguageDefinition def;
    def.name = "Dart";
    def.keywords = {
        "abstract", "as", "assert", "async", "await", "break", "case",
        "catch", "class", "const", "continue", "covariant", "default",
        "deferred", "do", "dynamic", "else", "enum", "export", "extends",
        "extension", "external", "factory", "false", "final", "finally",
        "for", "Function", "get", "hide", "if", "implements", "import",
        "in", "interface", "is", "late", "library", "mixin", "new", "null",
        "on", "operator", "part", "required", "rethrow", "return", "set",
        "show", "static", "super", "switch", "sync", "this", "throw", "true",
        "try", "typedef", "var", "void", "while", "with", "yield"
    };
    def.types = {
        "int", "double", "num", "bool", "String", "List", "Set", "Map",
        "Object", "dynamic", "void", "Future", "Stream", "Iterable",
        "Widget", "BuildContext", "State", "StatelessWidget", "StatefulWidget"
    };
    def.lineCommentPrefix = "//";
    def.blockCommentStart = "/*";
    def.blockCommentEnd = "*/";
    def.hasNumberHighlight = true;
    return def;
}

LanguageDefinition languageRust() {
    LanguageDefinition def;
    def.name = "Rust";
    def.keywords = {
        "as", "async", "await", "break", "const", "continue", "crate",
        "dyn", "else", "enum", "extern", "false", "fn", "for", "if",
        "impl", "in", "let", "loop", "match", "mod", "move", "mut", "pub",
        "ref", "return", "self", "Self", "static", "struct", "super",
        "trait", "true", "type", "unsafe", "use", "where", "while", "yield"
    };
    def.types = {
        "i8", "i16", "i32", "i64", "i128", "isize",
        "u8", "u16", "u32", "u64", "u128", "usize",
        "f32", "f64", "bool", "char", "str", "String",
        "Vec", "Option", "Result", "Box", "Rc", "Arc",
        "HashMap", "HashSet", "BTreeMap", "Cow", "Cell", "RefCell",
        "&str", "&[u8]", "Self"
    };
    def.lineCommentPrefix = "//";
    def.blockCommentStart = "/*";
    def.blockCommentEnd = "*/";
    def.hasNumberHighlight = true;
    return def;
}

LanguageDefinition languageGo() {
    LanguageDefinition def;
    def.name = "Go";
    def.keywords = {
        "break", "case", "chan", "const", "continue", "default", "defer",
        "else", "fallthrough", "for", "func", "go", "goto", "if", "import",
        "interface", "map", "package", "range", "return", "select", "struct",
        "switch", "type", "var"
    };
    def.types = {
        "bool", "byte", "complex64", "complex128", "error", "float32",
        "float64", "int", "int8", "int16", "int32", "int64",
        "rune", "string", "uint", "uint8", "uint16", "uint32", "uint64",
        "uintptr", "nil", "true", "false", "iota",
        "any", "comparable"
    };
    def.lineCommentPrefix = "//";
    def.blockCommentStart = "/*";
    def.blockCommentEnd = "*/";
    def.hasNumberHighlight = true;
    return def;
}

LanguageDefinition languageC() {
    LanguageDefinition def;
    def.name = "C";
    def.keywords = {
        "auto", "break", "case", "const", "continue", "default", "do",
        "else", "enum", "extern", "for", "goto", "if", "inline", "register",
        "restrict", "return", "sizeof", "static", "struct", "switch",
        "typedef", "union", "volatile", "while", "_Alignas", "_Alignof",
        "_Atomic", "_Bool", "_Complex", "_Generic", "_Imaginary",
        "_Noreturn", "_Static_assert", "_Thread_local"
    };
    def.types = {
        "char", "double", "float", "int", "long", "short", "signed",
        "unsigned", "void", "size_t", "ssize_t", "ptrdiff_t",
        "int8_t", "int16_t", "int32_t", "int64_t",
        "uint8_t", "uint16_t", "uint32_t", "uint64_t",
        "NULL", "FILE", "time_t"
    };
    def.lineCommentPrefix = "//";
    def.blockCommentStart = "/*";
    def.blockCommentEnd = "*/";
    def.hasNumberHighlight = true;
    return def;
}

LanguageDefinition languageCpp() {
    LanguageDefinition def;
    def.name = "C++";
    def.keywords = {
        "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand",
        "bitor", "bool", "break", "case", "catch", "char", "char8_t",
        "char16_t", "char32_t", "class", "compl", "concept", "const",
        "consteval", "constexpr", "constinit", "const_cast", "continue",
        "co_await", "co_return", "co_yield", "decltype", "default", "delete",
        "do", "double", "dynamic_cast", "else", "enum", "explicit", "export",
        "extern", "false", "float", "for", "friend", "goto", "if", "inline",
        "int", "long", "mutable", "namespace", "new", "noexcept", "not",
        "not_eq", "nullptr", "operator", "or", "or_eq", "override",
        "private", "protected", "public", "register", "reinterpret_cast",
        "requires", "return", "short", "signed", "sizeof", "static",
        "static_assert", "static_cast", "struct", "switch", "template",
        "this", "thread_local", "throw", "true", "try", "typedef", "typeid",
        "typename", "union", "unsigned", "using", "virtual", "void",
        "volatile", "wchar_t", "while", "xor", "xor_eq",
        "include", "define", "ifdef", "ifndef", "endif", "pragma", "error"
    };
    def.types = {
        "bool", "char", "char8_t", "char16_t", "char32_t",
        "double", "float", "int", "long", "short", "signed",
        "unsigned", "void", "wchar_t", "size_t", "ssize_t", "ptrdiff_t",
        "int8_t", "int16_t", "int32_t", "int64_t",
        "uint8_t", "uint16_t", "uint32_t", "uint64_t",
        "nullptr_t", "max_align_t", "byte",
        "QString", "QList", "QHash", "QMap", "QSet", "QVector",
        "QObject", "QWidget", "QStringView", "QByteArray",
        "std::string", "std::vector", "std::array", "std::shared_ptr",
        "std::unique_ptr", "std::optional", "std::expected"
    };
    def.lineCommentPrefix = "//";
    def.blockCommentStart = "/*";
    def.blockCommentEnd = "*/";
    def.hasNumberHighlight = true;
    return def;
}

LanguageDefinition languageCmake() {
    LanguageDefinition def;
    def.name = "CMake";
    def.keywords = {
        "add_executable", "add_library", "target_link_libraries",
        "target_include_directories", "target_compile_definitions",
        "find_package", "set", "if", "else", "elseif", "endif",
        "foreach", "endforeach", "while", "endwhile", "function",
        "endfunction", "macro", "endmacro", "include", "message",
        "cmake_minimum_required", "project", "option", "list", "string",
        "file", "configure_file", "install", "export", "add_subdirectory",
        "enable_testing", "add_test", "set_target_properties"
    };
    def.types = {
        "ON", "OFF", "TRUE", "FALSE", "CACHE", "INTERNAL", "PUBLIC",
        "PRIVATE", "INTERFACE", "SHARED", "STATIC", "MODULE"
    };
    def.lineCommentPrefix = "#";
    def.blockCommentStart = "#[[";
    def.blockCommentEnd = "]]";
    def.hasNumberHighlight = true;
    return def;
}

LanguageDefinition languageMarkdown() {
    LanguageDefinition def;
    def.name = "Markdown";
    def.keywords = {};
    def.types = {};
    def.lineCommentPrefix = {};
    def.hasNumberHighlight = false;
    return def;
}

LanguageDefinition languageYaml() {
    LanguageDefinition def;
    def.name = "YAML";
    def.keywords = {"true", "false", "yes", "no", "on", "off", "null", "~"};
    def.lineCommentPrefix = "#";
    def.hasNumberHighlight = true;
    return def;
}

LanguageDefinition languageJavascript() {
    LanguageDefinition def;
    def.name = "JavaScript";
    def.keywords = {
        "async", "await", "break", "case", "catch", "class", "const",
        "continue", "debugger", "default", "delete", "do", "else", "export",
        "extends", "finally", "for", "function", "if", "import", "in",
        "instanceof", "let", "new", "of", "return", "super", "switch",
        "this", "throw", "try", "typeof", "var", "void", "while", "with",
        "yield", "true", "false", "null", "undefined", "NaN", "Infinity"
    };
    def.types = {
        "Number", "String", "Boolean", "Array", "Object", "Function",
        "Map", "Set", "Promise", "RegExp", "Error", "Symbol", "BigInt",
        "console", "document", "window", "JSON", "Math", "Date"
    };
    def.lineCommentPrefix = "//";
    def.blockCommentStart = "/*";
    def.blockCommentEnd = "*/";
    def.hasNumberHighlight = true;
    return def;
}

LanguageDefinition languageTypescript() {
    LanguageDefinition def = languageJavascript();
    def.name = "TypeScript";
    def.keywords.append({
        "type", "interface", "enum", "implements", "abstract", "as",
        "declare", "keyof", "module", "namespace", "readonly", "static",
        "private", "protected", "public", "any", "unknown", "never"
    });
    def.types.append({
        "string", "number", "boolean", "void", "any", "unknown", "never",
        "Promise", "ReadonlyArray", "Partial", "Required", "Record"
    });
    return def;
}

LanguageDefinition languageCss() {
    LanguageDefinition def;
    def.name = "CSS";
    def.keywords = {
        "!important", "inherit", "initial", "unset", "revert"
    };
    def.blockCommentStart = "/*";
    def.blockCommentEnd = "*/";
    def.hasNumberHighlight = true;
    return def;
}

LanguageDefinition languageHtml() {
    LanguageDefinition def;
    def.name = "HTML";
    def.blockCommentStart = "<!--";
    def.blockCommentEnd = "-->";
    def.hasNumberHighlight = false;
    return def;
}

LanguageDefinition languageShell() {
    LanguageDefinition def;
    def.name = "Shell";
    def.keywords = {
        "if", "then", "else", "elif", "fi", "case", "esac", "for",
        "while", "until", "do", "done", "in", "function", "return",
        "exit", "break", "continue", "export", "local", "readonly",
        "unset", "source", "alias", "eval", "exec", "trap", "test"
    };
    def.lineCommentPrefix = "#";
    def.hasNumberHighlight = true;
    return def;
}

LanguageDefinition languageSql() {
    LanguageDefinition def;
    def.name = "SQL";
    def.keywords = {
        "SELECT", "FROM", "WHERE", "INSERT", "UPDATE", "DELETE", "CREATE",
        "DROP", "ALTER", "TABLE", "INDEX", "VIEW", "INTO", "VALUES",
        "SET", "JOIN", "INNER", "LEFT", "RIGHT", "OUTER", "ON", "AND",
        "OR", "NOT", "IN", "EXISTS", "BETWEEN", "LIKE", "IS", "NULL",
        "ORDER", "BY", "ASC", "DESC", "GROUP", "HAVING", "LIMIT",
        "OFFSET", "UNION", "ALL", "DISTINCT", "AS", "CASE", "WHEN",
        "THEN", "ELSE", "END", "BEGIN", "COMMIT", "ROLLBACK", "PRIMARY",
        "KEY", "FOREIGN", "REFERENCES", "CONSTRAINT", "DEFAULT", "CHECK",
        "UNIQUE", "CASCADE", "TRIGGER", "PROCEDURE", "FUNCTION", "INT",
        "VARCHAR", "TEXT", "BOOLEAN", "INTEGER", "FLOAT", "DOUBLE",
        "BIGINT", "SMALLINT", "TIMESTAMP", "DATE", "BLOB", "DECIMAL"
    };
    def.lineCommentPrefix = "--";
    def.blockCommentStart = "/*";
    def.blockCommentEnd = "*/";
    def.hasNumberHighlight = true;
    return def;
}

LanguageDefinition languageLua() {
    LanguageDefinition def;
    def.name = "Lua";
    def.keywords = {
        "and", "break", "do", "else", "elseif", "end", "false", "for",
        "function", "goto", "if", "in", "local", "nil", "not", "or",
        "repeat", "return", "then", "true", "until", "while"
    };
    def.types = {
        "string", "number", "boolean", "table", "function", "thread",
        "nil", "userdata"
    };
    def.lineCommentPrefix = "--";
    def.blockCommentStart = "--[[";
    def.blockCommentEnd = "]]";
    def.hasNumberHighlight = true;
    return def;
}

LanguageDefinition languageKotlin() {
    LanguageDefinition def;
    def.name = "Kotlin";
    def.keywords = {
        "abstract", "actual", "annotation", "as", "break", "case", "catch",
        "class", "companion", "const", "constructor", "continue", "data",
        "do", "else", "enum", "expect", "external", "false", "final",
        "finally", "for", "fun", "if", "import", "in", "infix", "init",
        "inline", "inner", "interface", "internal", "is", "lateinit", "null",
        "object", "open", "operator", "out", "override", "package",
        "private", "protected", "public", "reified", "return", "sealed",
        "super", "suspend", "switch", "this", "throw", "true", "try",
        "typealias", "val", "var", "vararg", "when", "while", "by"
    };
    def.types = {
        "Int", "Long", "Float", "Double", "Boolean", "Char", "String",
        "Byte", "Short", "Unit", "Nothing", "Any", "Array", "List",
        "Set", "Map", "MutableList", "MutableSet", "MutableMap",
        "Sequence", "Pair", "Triple"
    };
    def.lineCommentPrefix = "//";
    def.blockCommentStart = "/*";
    def.blockCommentEnd = "*/";
    def.hasNumberHighlight = true;
    return def;
}

LanguageDefinition languageSwift() {
    LanguageDefinition def;
    def.name = "Swift";
    def.keywords = {
        "as", "associatedtype", "async", "await", "break", "case", "catch",
        "class", "continue", "default", "defer", "deinit", "do", "else",
        "enum", "extension", "fallthrough", "false", "fileprivate", "for",
        "func", "guard", "if", "import", "in", "init", "inout", "internal",
        "is", "let", "mutating", "nil", "nonisolated", "open", "operator",
        "override", "precedencegroup", "private", "protocol", "public",
        "repeat", "rethrows", "return", "self", "Self", "static", "struct",
        "subscript", "super", "switch", "throw", "throws", "true", "try",
        "typealias", "var", "where", "while"
    };
    def.types = {
        "Int", "Float", "Double", "Bool", "String", "Character", "Void",
        "Array", "Dictionary", "Set", "Optional", "Result", "Error",
        "Data", "URL", "Date", "Any", "AnyObject", "Never",
        "UIView", "UIViewController", "SwiftUI"
    };
    def.lineCommentPrefix = "//";
    def.blockCommentStart = "/*";
    def.blockCommentEnd = "*/";
    def.hasNumberHighlight = true;
    return def;
}

LanguageDefinition languageHttp() {
    LanguageDefinition def;
    def.name = "HTTP";
    def.keywords = {"GET", "POST", "PUT", "PATCH", "DELETE", "HEAD", "OPTIONS", "TRACE", "CONNECT"};
    def.types = {"HTTP/1.0", "HTTP/1.1", "HTTP/2", "HTTP/3"};
    def.hasHeaderHighlight = true;
    def.hasNumberHighlight = true;
    return def;
}

} // namespace NezhaIDE::Editor

