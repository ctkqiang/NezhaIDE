#include "activity_bar.h"
#include "preferences_dialog.h"
#include "src/services/theme_service.h"
#include <QIcon>
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

    btn_explorer_ = addButton(QStringLiteral("explorer"), "Explorer", ActivityBarItem::Explorer);
    btn_git_ = addButton(QStringLiteral("git"), "Git", ActivityBarItem::Git);
    btn_http_ = addButton(QStringLiteral("http"), "HTTP Client", ActivityBarItem::HttpClient);
    btn_hydra_ = addButton(QStringLiteral("hydra"), "Hydra", ActivityBarItem::Hydra);
    btn_terminal_ = addButton(QStringLiteral("terminal"), "Terminal", ActivityBarItem::Terminal);
    layout->addStretch();
    btn_prefs_ = addButton(QStringLiteral("preferences"), "Preferences", ActivityBarItem::Preferences);

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
    btn->setFixedSize(32, 32);
    btn->setIconSize({18, 18});
    btn->setIcon(QIcon(QStringLiteral(":/vectors/%1.svg").arg(iconPath)));
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

    auto applyBtn = [&](QPushButton *btn, const QString &iconPath, bool active) {
        btn->setIcon(QIcon(QStringLiteral(":/vectors/%1%2.svg")
                               .arg(iconPath, active ? QStringLiteral("_active") : QString())));
        if (active) {
            btn->setStyleSheet(
                QStringLiteral("QPushButton { background: transparent;"
                "border: none; border-left: 2px solid %1;"
                "padding: 0; border-radius: 0; }")
                .arg(ts.color(QStringLiteral("accent"))));
        } else {
            btn->setStyleSheet(
                QStringLiteral("QPushButton { background: transparent;"
                "border: none; border-left: 2px solid transparent;"
                "padding: 0; border-radius: 0; }"));
        }
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
    setStyleSheet(
        QStringLiteral("QWidget#activityBar { background: %1; }")
        .arg(NezhaIDE::Services::ThemeService::instance().color(QStringLiteral("bg.secondary"))));
}

} // namespace NezhaIDE::Views
