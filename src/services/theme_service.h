#pragma once

#include <QObject>
#include <QColor>
#include <QHash>
#include <QString>

namespace NezhaIDE {
    enum class IDETheme;
}

namespace NezhaIDE::Services {

class ThemeService final : public QObject {
    Q_OBJECT

public:
    static ThemeService &instance();

    ThemeService(const ThemeService &) = delete;
    ThemeService &operator=(const ThemeService &) = delete;
    ThemeService(ThemeService &&) = delete;
    ThemeService &operator=(ThemeService &&) = delete;

    void initialize(IDETheme theme);
    void applyTheme(IDETheme theme);

    [[nodiscard]] QString color(const QString &key) const;
    [[nodiscard]] QColor qcolor(const QString &key) const;
    [[nodiscard]] QString qss(const QString &key) const;
    [[nodiscard]] QHash<QString, QColor> syntaxColors() const;
    [[nodiscard]] IDETheme currentTheme() const noexcept;

    [[nodiscard]] static QString themeId(IDETheme theme);
    [[nodiscard]] static QString xmlPath(IDETheme theme);

signals:
    void themeChanged(IDETheme theme);

private:
    ThemeService() = default;
    ~ThemeService() override = default;

    void loadXml(const QString &path);
    void rebuildStyles();
    static IDETheme resolveTheme(IDETheme theme);

    QHash<QString, QString> colors_;
    QHash<QString, QString> styles_;
    IDETheme current_theme_{};
};

} // namespace NezhaIDE::Services
