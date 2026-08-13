#pragma once

// NezhaIDE 统一设计 Token 层
// 几何常量（圆角/间距/字号/布局）与主题无关，由 ThemeService::rebuildStyles()
// 以 %token% 占位符替换进 QSS 模板；颜色 token 走 $key 机制（主题 XML）。
// 所有视图层布局魔数必须引用本文件的常量，禁止散落硬编码。

#include <QString>

namespace NezhaIDE::Services::Tokens {

// ---- 圆角 ----
inline constexpr int kRadius     = 6;    // 主圆角：按钮/输入框/列表项/卡片
inline constexpr int kRadiusSm   = 4;    // 小圆角：徽标/小工具按钮/快捷键标签
inline constexpr int kRadiusLg   = 8;    // 大圆角：卡片/面板/菜单
inline constexpr int kRadiusPill = 999;  // 胶囊：状态 pill / 徽章

// ---- 间距（4px 基数体系）----
inline constexpr int kSpaceXs  = 2;
inline constexpr int kSpaceS   = 4;
inline constexpr int kSpaceM   = 8;
inline constexpr int kSpaceL   = 12;
inline constexpr int kSpaceXl  = 16;
inline constexpr int kSpace2xl = 24;

// ---- 字号 ----
inline constexpr int kFontTiny = 10;  // 徽章数字
inline constexpr int kFontSm   = 11;  // 状态栏/侧栏标题/面板 tab
inline constexpr int kFontUi   = 13;  // 正文/编辑器
inline constexpr int kFontLg   = 16;  // 面板大标题
inline constexpr int kFontXl   = 24;  // 欢迎卡片图标

// ---- 布局高度/宽度 ----
inline constexpr int kActivityBarWidth   = 48;
inline constexpr int kSidebarHeaderH     = 36;
inline constexpr int kBottomPanelHeaderH = 32;
inline constexpr int kStatusBarMinH      = 26;
inline constexpr int kIconBtnSize        = 26;  // 侧栏 header 按钮
inline constexpr int kToolBtnSize        = 24;  // 底部面板 □/✕ 按钮
inline constexpr int kActivityBtnSize    = 48;  // 活动栏按钮

// ---- 字体栈（QSS font-family 逗号列表即回退栈；跨平台可用）----
inline constexpr const char *kMonoFontStack =
    "\"SF Mono\", \"Menlo\", \"Consolas\", \"DejaVu Sans Mono\", \"monospace\"";

// ---- QSS %token% 替换表（值与上方常量一一对应）----
inline const QHash<QString, QString> &qssTokenTable()
{
    static const QHash<QString, QString> table = {
        {"radius",     QString::number(kRadius)},
        {"radius-sm",  QString::number(kRadiusSm)},
        {"radius-lg",  QString::number(kRadiusLg)},
        {"radius-pill",QString::number(kRadiusPill)},
        {"space-xs",   QString::number(kSpaceXs)},
        {"space-s",    QString::number(kSpaceS)},
        {"space-m",    QString::number(kSpaceM)},
        {"space-l",    QString::number(kSpaceL)},
        {"space-xl",   QString::number(kSpaceXl)},
        {"space-2xl",  QString::number(kSpace2xl)},
        {"font-tiny",  QString::number(kFontTiny)},
        {"font-sm",    QString::number(kFontSm)},
        {"font-ui",    QString::number(kFontUi)},
        {"font-lg",    QString::number(kFontLg)},
        {"font-xl",    QString::number(kFontXl)},
        {"font-mono",  QString::fromLatin1(kMonoFontStack)},
    };
    return table;
}

} // namespace NezhaIDE::Services::Tokens
