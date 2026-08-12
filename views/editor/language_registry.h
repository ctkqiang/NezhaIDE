#pragma once

#include <QHash>
#include <QString>
#include <QStringList>
#include <functional>

class QSyntaxHighlighter;
class QTextDocument;

namespace NezhaIDE::Editor {

struct LanguageDefinition;

/**
 * 文件扩展名 → 语法高亮器工厂注册表。
 *
 * 根据文件后缀分配对应的 QSyntaxHighlighter 实现。
 * 未知扩展名返回 nullptr（不高亮）。
 */
class LanguageRegistry final {
public:
    /**
     * 获取全局注册表实例。
     *
     * @return 单例引用。
     */
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
