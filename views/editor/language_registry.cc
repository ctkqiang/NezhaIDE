#include "language_registry.h"
#include "cpp_highlighter.h"
#include "simple_highlighter.h"
#include <QFileInfo>

namespace NezhaIDE::Editor {

LanguageRegistry &LanguageRegistry::instance()
{
    static LanguageRegistry reg;
    return reg;
}

LanguageRegistry::LanguageRegistry()
{
    registerCpp();
    registerAll();
}

void LanguageRegistry::registerCpp()
{
    static const QStringList cpp_extensions = {
        "cpp", "cc", "cxx", "c++", "c",
        "h", "hpp", "hxx", "h++", "hh",
        "inl", "ipp",
    };

    for (const auto &ext : cpp_extensions) {
        factories_[ext] = [](QTextDocument *doc, QObject *parent) -> QSyntaxHighlighter * {
            return new CppHighlighter(doc, parent);
        };
    }
}

void LanguageRegistry::registerLanguage(const QStringList &extensions, const LanguageDefinition &def)
{
    for (const auto &ext : extensions) {
        factories_[ext] = [def](QTextDocument *doc, QObject *parent) -> QSyntaxHighlighter * {
            return new SimpleHighlighter(def, doc, parent);
        };
    }
}

void LanguageRegistry::registerAll()
{
    registerLanguage({"json"}, languageJson());
    registerLanguage({"xml", "svg", "xaml", "ui", "qrc", "tsx", "jsx"}, languageXml());
    registerLanguage({"py", "pyw", "pyi"}, languagePython());
    registerLanguage({"rb", "rake", "gemspec"}, languageRuby());
    registerLanguage({"dart"}, languageDart());
    registerLanguage({"rs"}, languageRust());
    registerLanguage({"go"}, languageGo());
    registerLanguage({"cmake", "cmake.in"}, languageCmake());
    registerLanguage({"md", "markdown"}, languageMarkdown());
    registerLanguage({"yaml", "yml"}, languageYaml());

    registerLanguage({"js", "mjs", "cjs"}, languageJavascript());
    registerLanguage({"ts"}, languageTypescript());
    registerLanguage({"css", "scss", "less"}, languageCss());
    registerLanguage({"html", "htm"}, languageHtml());
    registerLanguage({"sh", "bash", "zsh", "fish"}, languageShell());
    registerLanguage({"sql"}, languageSql());
    registerLanguage({"lua"}, languageLua());
    registerLanguage({"kt", "kts"}, languageKotlin());
    registerLanguage({"swift"}, languageSwift());
}

QSyntaxHighlighter *LanguageRegistry::createHighlighter(
    const QString &filePath, QTextDocument *document, QObject *parent) const
{
    const QFileInfo info(filePath);
    const auto suffix = info.suffix().toLower();
    if (const auto it = factories_.find(suffix); it != factories_.end()) {
        return it.value()(document, parent);
    }
    return nullptr;
}

} // namespace NezhaIDE::Editor
