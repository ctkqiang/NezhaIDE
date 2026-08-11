#pragma once

#include <QObject>
#include <QHash>
#include <QString>

namespace NezhaIDE {
    enum class IDELanguage;
}

namespace NezhaIDE::Services {

class LocalizationService final : public QObject {
    Q_OBJECT

public:
    static LocalizationService &instance();

    LocalizationService(const LocalizationService &) = delete;
    LocalizationService &operator=(const LocalizationService &) = delete;
    LocalizationService(LocalizationService &&) = delete;
    LocalizationService &operator=(LocalizationService &&) = delete;

    void initialize(IDELanguage language);
    void switchLanguage(IDELanguage language);

    [[nodiscard]]
    QString translate(const QString &key) const;

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
