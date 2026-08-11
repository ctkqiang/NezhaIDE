#pragma once

#include <QObject>
#include <QColor>
#include <QHash>
#include <QString>

namespace NezhaIDE {
    enum class IDETheme;
}

/**
 * IDE 主题与样式服务。
 */
namespace NezhaIDE::Services {

/**
 * 主题管理单例，负责从 XML 加载主题颜色并派生 QSS 样式表。
 *
 * 支持 Auto/Light/Dark/GitHub/Xcode 五种主题。通过 $token 替换机制
 * 将颜色值注入预定义的 QSS 模板。主题切换时发出 themeChanged 信号，
 * 并通过 applyGlobalPalette() 设置全局 QPalette。
 *
 * @see kStyleTemplates 定义在 theme_service.cc 中。
 */
class ThemeService final : public QObject {
    Q_OBJECT

public:
    static ThemeService &instance();

    ThemeService(const ThemeService &) = delete;
    ThemeService &operator=(const ThemeService &) = delete;
    ThemeService(ThemeService &&) = delete;
    ThemeService &operator=(ThemeService &&) = delete;

    /**
     * 初始化主题系统，解析 XML 并构建样式表。
     *
     * @param theme 初始主题枚举。
     */
    void initialize(IDETheme theme);

    /**
     * 运行时切换主题，重新加载 XML 并发出 themeChanged 信号。
     *
     * @param theme 目标主题枚举。
     */
    void applyTheme(IDETheme theme);

    /**
     * 获取指定 key 对应的颜色字符串（如 "#3370FF" 或 "rgba(0,0,0,0.04)"）。
     *
     * @param key 颜色标记名，如 "bg.primary"。
     * @return 颜色字符串，若 key 不存在则返回空字符串。
     */
    [[nodiscard]] QString color(const QString &key) const;

    /**
     * 获取指定 key 对应的 QColor 对象。
     *
     * @param key 颜色标记名。
     * @return QColor，若 key 不存在则返回无效颜色。
     */
    [[nodiscard]] QColor qcolor(const QString &key) const;

    /**
     * 获取指定 key 对应的 QSS 样式表，所有 $token 已替换为实际颜色值。
     *
     * @param key 样式模板名，如 "style.menu"。
     * @return 替换后的 QSS 字符串。
     */
    [[nodiscard]] QString qss(const QString &key) const;

    /**
     * 获取所有语法高亮颜色（key 以 "syntax." 开头的颜色项）。
     *
     * @return QColor 哈希表，供 QSyntaxHighlighter 使用。
     */
    [[nodiscard]] QHash<QString, QColor> syntaxColors() const;

    /**
     * 获取当前已解析的活动主题。
     *
     * @return 当前主题枚举值（Auto 已被解析为 Light 或 Dark）。
     */
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
    void applyGlobalPalette();
    static IDETheme resolveTheme(IDETheme theme);

    QHash<QString, QString> colors_;
    QHash<QString, QString> styles_;
    IDETheme current_theme_{};
};

} // namespace NezhaIDE::Services
