#pragma once

#include <QHash>
#include <QString>
#include <functional>

class QSyntaxHighlighter;
class QTextDocument;

namespace NezhaIDE::Editor {

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

    QHash<QString, std::function<QSyntaxHighlighter *(QTextDocument *, QObject *)>> factories_;
};

} // namespace NezhaIDE::Editor
