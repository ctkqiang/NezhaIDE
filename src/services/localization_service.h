#pragma once

#include <QObject>
#include <QHash>
#include <QString>

namespace NezhaIDE {
    enum class IDELanguage;
}

/**
 * 多语言本地化服务。
 */
namespace NezhaIDE::Services {

/**
 * 多语言本地化服务单例，基于 XML 文件加载翻译字符串。
 *
 * 支持中文、英文、德文三种语言。通过 LOC() 宏便捷获取翻译文本。
 * 语言切换会发出 languageChanged 信号以通知 UI 刷新。
 *
 * @see LOC(key) 宏定义在文件末尾。
 */
class LocalizationService final : public QObject {
    Q_OBJECT

public:
    static LocalizationService &instance();

    LocalizationService(const LocalizationService &) = delete;
    LocalizationService &operator=(const LocalizationService &) = delete;
    LocalizationService(LocalizationService &&) = delete;
    LocalizationService &operator=(LocalizationService &&) = delete;

    /**
     * 初始化语言环境并加载对应 XML 翻译文件。
     *
     * @param language 初始语言。
     */
    void initialize(IDELanguage language);

    /**
     * 运行时切换语言并持久化到配置。
     *
     * @param language 目标语言。
     *
     * @note 发出 languageChanged 信号，UI 需重新加载翻译字符串。
     */
    void switchLanguage(IDELanguage language);

    /**
     * 根据 key 获取当前语言的翻译文本。
     *
     * @param key 翻译键。
     * @return 翻译后的字符串，若 key 不存在则原样返回 key。
     */
    [[nodiscard]]
    QString translate(const QString &key) const;

    /**
     * 获取当前活动语言。
     *
     * @return 当前语言枚举值。
     */
    [[nodiscard]]
    IDELanguage currentLanguage() const noexcept;

signals:
    void languageChanged(IDELanguage language);

private:
    LocalizationService() = default;
    ~LocalizationService() override = default;

    void loadXml(const QString &path);
    [[nodiscard]]
    static QString xmlPath(IDELanguage language);

    QHash<QString, QString> strings_;
    IDELanguage current_language_{};
};

} // namespace NezhaIDE::Services

#define LOC(key) NezhaIDE::Services::LocalizationService::instance().translate(key)
