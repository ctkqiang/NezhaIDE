#include "activity_bar.h"
#include "preferences_dialog.h"
#include "src/services/localization_service.h"
#include "src/services/theme_service.h"
#include <QIcon>
#include <QStyle>
#include <QToolTip>

namespace NezhaIDE::Views {

ActivityBar::ActivityBar(QWidget *parent)
    : QWidget(parent)
{
    setFixedWidth(48);
    setObjectName(QStringLiteral("activityBar"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 6, 0, 0);
    layout->setSpacing(0);

    btn_explorer_ = addButton(QStringLiteral("explorer"), LOC("activity.explorer") + QStringLiteral("  ⌘⇧E"), ActivityBarItem::Explorer);
    btn_git_ = addButton(QStringLiteral("git"), LOC("activity.git") + QStringLiteral("  ⌘⇧G"), ActivityBarItem::Git);
    btn_http_ = addButton(QStringLiteral("http"), QStringLiteral("HTTP Client  ⌘⇧H"), ActivityBarItem::HttpClient);
    btn_hydra_ = addButton(QStringLiteral("hydra"), QStringLiteral("Hydra  ⌘⇧Y"), ActivityBarItem::Hydra);
    btn_terminal_ = addButton(QStringLiteral("terminal"), LOC("activity.terminal") + QStringLiteral("  ⌘`"), ActivityBarItem::Terminal);
    layout->addStretch();
    btn_prefs_ = addButton(QStringLiteral("preferences"), LOC("activity.preferences") + QStringLiteral("  ⌘,"), ActivityBarItem::Preferences);

    setActive(ActivityBarItem::Explorer);
    applyStyles();

    connect(&NezhaIDE::Services::ThemeService::instance(),
            &NezhaIDE::Services::ThemeService::themeChanged,
            this, [this](NezhaIDE::IDETheme) {
                applyStyles();
                setActive(active_);
            });
}

QPushButton *ActivityBar::addButton(const QString &iconPath, const QString &tooltip, ActivityBarItem id)
{
    auto *btn = new QPushButton(this);
    btn->setFixedSize(40, 40);
    btn->setIconSize({22, 22});
    btn->setIcon(QIcon(QStringLiteral(":/vectors/%1.svg").arg(iconPath)));
    btn->setToolTip(tooltip);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setFlat(true);
    btn->setObjectName(QStringLiteral("activityBarBtn"));
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

    auto applyBtn = [&](QPushButton *btn, const QString &iconPath, bool active) {
        btn->setIcon(QIcon(QStringLiteral(":/vectors/%1%2.svg")
                               .arg(iconPath, active ? QStringLiteral("_active") : QString())));
        btn->setProperty("active", active);
        btn->style()->unpolish(btn);
        btn->style()->polish(btn);
    };

    applyBtn(btn_explorer_, QStringLiteral("explorer"), item == ActivityBarItem::Explorer);
    applyBtn(btn_git_, QStringLiteral("git"), item == ActivityBarItem::Git);
    applyBtn(btn_http_, QStringLiteral("http"), item == ActivityBarItem::HttpClient);
    applyBtn(btn_hydra_, QStringLiteral("hydra"), item == ActivityBarItem::Hydra);
    applyBtn(btn_terminal_, QStringLiteral("terminal"), item == ActivityBarItem::Terminal);
    applyBtn(btn_prefs_, QStringLiteral("preferences"), false);
}

void ActivityBar::applyStyles()
{
    auto &ts = NezhaIDE::Services::ThemeService::instance();
    setStyleSheet(ts.qss(QStringLiteral("style.activity_bar")));
    for (auto *btn : findChildren<QPushButton*>(QStringLiteral("activityBarBtn"))) {
        btn->setStyleSheet(ts.qss(QStringLiteral("style.activity_bar_btn")));
    }
}

} // namespace NezhaIDE::Views
