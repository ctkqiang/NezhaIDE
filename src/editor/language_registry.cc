#include "language_registry.h"
#include "cpp_highlighter.h"
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
