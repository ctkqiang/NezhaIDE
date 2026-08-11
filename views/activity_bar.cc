#include "activity_bar.h"
#include "preferences_dialog.h"
#include "src/services/theme_service.h"
#include <QToolTip>

namespace NezhaIDE::Views {

ActivityBar::ActivityBar(QWidget *parent)
    : QWidget(parent)
{
    setFixedWidth(36);
    setObjectName(QStringLiteral("activityBar"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->setSpacing(2);

    btn_explorer_ = addButton(QStringLiteral("▣"), "Explorer", ActivityBarItem::Explorer);
    btn_git_ = addButton(QStringLiteral("⎇"), "Git", ActivityBarItem::Git);
    btn_http_ = addButton(QStringLiteral("⟐"), "HTTP Client", ActivityBarItem::HttpClient);
    btn_terminal_ = addButton(QStringLiteral("▸_"), "Terminal", ActivityBarItem::Terminal);
    layout->addStretch();
    btn_prefs_ = addButton(QStringLiteral("⚙"), "Preferences", ActivityBarItem::Preferences);

    setActive(ActivityBarItem::Explorer);
    applyStyles();

    connect(&NezhaIDE::Services::ThemeService::instance(),
            &NezhaIDE::Services::ThemeService::themeChanged,
            this, [this](NezhaIDE::IDETheme) {
                applyStyles();
                setActive(active_);
            });
}

QPushButton *ActivityBar::addButton(const QString &text, const QString &tooltip, ActivityBarItem id)
{
    auto *btn = new QPushButton(text, this);
    btn->setFixedSize(32, 32);
    btn->setToolTip(tooltip);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setFlat(true);
    connect(btn, &QPushButton::clicked, this, [this, id] {
        if (id == ActivityBarItem::Preferences) {
            PreferencesDialog dlg(window());
            dlg.exec();
            return;
        }
        setActive(id);
        emit itemSelected(id);
    });
    layout()->addWidget(btn);
    return btn;
}

void ActivityBar::setActive(ActivityBarItem item)
{
    active_ = item;
    auto &ts = NezhaIDE::Services::ThemeService::instance();

    auto applyBtn = [&](QPushButton *btn, bool active) {
        if (active) {
            btn->setStyleSheet(
                QStringLiteral("QPushButton { background: transparent; color: %1;"
                "border: none; border-left: 2px solid %2;"
                "font-size: 16px; padding: 0; border-radius: 0; }")
                .arg(ts.color(QStringLiteral("accent")),
                     ts.color(QStringLiteral("accent"))));
        } else {
            btn->setStyleSheet(
                QStringLiteral("QPushButton { background: transparent; color: %1;"
                "border: none; border-left: 2px solid transparent;"
                "font-size: 16px; padding: 0; border-radius: 0; }"
                "QPushButton:hover { color: %2; }")
                .arg(ts.color(QStringLiteral("text.tertiary")),
                     ts.color(QStringLiteral("text.primary"))));
        }
    };

    applyBtn(btn_explorer_, item == ActivityBarItem::Explorer);
    applyBtn(btn_git_, item == ActivityBarItem::Git);
    applyBtn(btn_http_, item == ActivityBarItem::HttpClient);
    applyBtn(btn_terminal_, item == ActivityBarItem::Terminal);
    applyBtn(btn_prefs_, false);
}

void ActivityBar::applyStyles()
{
    setStyleSheet(
        QStringLiteral("QWidget#activityBar { background: %1; }")
        .arg(NezhaIDE::Services::ThemeService::instance().color(QStringLiteral("bg.secondary"))));
}

} // namespace NezhaIDE::Views
