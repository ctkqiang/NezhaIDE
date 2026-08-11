#pragma once

#include <QHash>
#include <QString>
#include <QStringList>
#include <functional>

class QSyntaxHighlighter;
class QTextDocument;

namespace NezhaIDE::Editor {

struct LanguageDefinition;

class LanguageRegistry final {
public:
    static LanguageRegistry &instance();

    LanguageRegistry(const LanguageRegistry &) = delete;
    LanguageRegistry &operator=(const LanguageRegistry &) = delete;

    [[nodiscard]] QSyntaxHighlighter *createHighlighter(
        const QString &filePath, QTextDocument *document, QObject *parent = nullptr) const;

private:
    LanguageRegistry();
    void registerCpp();
    void registerAll();
    void registerLanguage(const QStringList &extensions, const LanguageDefinition &def);

    QHash<QString, std::function<QSyntaxHighlighter *(QTextDocument *, QObject *)>> factories_;
};

} // namespace NezhaIDE::Editor
