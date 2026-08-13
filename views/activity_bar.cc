#include "activity_bar.h"
#include "preferences_dialog.h"
#include "src/services/design_tokens.h"
#include "src/services/localization_service.h"
#include "src/services/theme_service.h"
#include <QIcon>
#include <QStyle>
#include <QToolTip>

namespace NezhaIDE::Views {

ActivityBar::ActivityBar(QWidget *parent)
    : QWidget(parent)
{
    setFixedWidth(NezhaIDE::Services::Tokens::kActivityBarWidth);
    setObjectName(QStringLiteral("activityBar"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->setSpacing(2);

    btn_explorer_ = addButton(QStringLiteral("explorer"),
        LOC("activity.explorer") + QStringLiteral("  \\u2318\\u21E7E"),
        ActivityBarItem::Explorer);
    btn_git_ = addButton(QStringLiteral("git"),
        LOC("activity.git") + QStringLiteral("  \\u2318\\u21E7G"),
        ActivityBarItem::Git);
    btn_http_ = addButton(QStringLiteral("http"),
        LOC("activity.http_client") + QStringLiteral("  \\u2318\\u21E7H"),
        ActivityBarItem::HttpClient);
    btn_hydra_ = addButton(QStringLiteral("hydra"),
        LOC("activity.hydra") + QStringLiteral("  \\u2318\\u21E7Y"),
        ActivityBarItem::Hydra);
    btn_terminal_ = addButton(QStringLiteral("terminal"),
        LOC("activity.terminal") + QStringLiteral("  \\u2318`"),
        ActivityBarItem::Terminal);
    layout->addStretch();
    btn_prefs_ = addButton(QStringLiteral("preferences"),
        LOC("activity.preferences") + QStringLiteral("  \\u2318,"),
        ActivityBarItem::Preferences);

    setActive(ActivityBarItem::Explorer);
    applyStyles();

    connect(&NezhaIDE::Services::ThemeService::instance(),
            &NezhaIDE::Services::ThemeService::themeChanged,
            this, [this](NezhaIDE::IDETheme) {
                applyStyles();
                setActive(active_);
            });
}

ActivityBar::ActivityButton ActivityBar::addButton(
    const QString &iconPath, const QString &tooltip, ActivityBarItem id)
{
    ActivityButton ab;

    const auto s = NezhaIDE::Services::Tokens::kActivityBtnSize;

    auto *container = new QWidget(this);
    container->setFixedSize(s, s);

    ab.button = new QPushButton(container);
    ab.button->setFixedSize(s, s);
    ab.button->setIconSize({24, 24});
    ab.button->setIcon(QIcon(QStringLiteral(":/vectors/%1.svg").arg(iconPath)));
    ab.button->setToolTip(tooltip);
    ab.button->setCursor(Qt::PointingHandCursor);
    ab.button->setFlat(true);
    ab.button->setObjectName(QStringLiteral("activityBarBtn"));

    ab.badge = new QLabel(container);
    ab.badge->setObjectName(QStringLiteral("activityBadge"));
    ab.badge->setFixedHeight(16);
    ab.badge->setAlignment(Qt::AlignCenter);
    ab.badge->hide();

    connect(ab.button, &QPushButton::clicked, this, [this, id] {
        if (id == ActivityBarItem::Preferences) {
            PreferencesDialog dlg(window());
            dlg.exec();
            return;
        }
        setActive(id);
        emit itemSelected(id);
    });

    layout()->addWidget(container);
    buttons_.append(ab);
    return ab;
}

void ActivityBar::setActive(ActivityBarItem item)
{
    active_ = item;

    auto applyBtn = [](ActivityButton &ab, const QString &iconPath, bool active) {
        ab.button->setIcon(QIcon(QStringLiteral(":/vectors/%1%2.svg")
            .arg(iconPath, active ? QStringLiteral("_active") : QString())));
        ab.button->setProperty("active", active);
        ab.button->style()->unpolish(ab.button);
        ab.button->style()->polish(ab.button);
    };

    applyBtn(btn_explorer_, QStringLiteral("explorer"), item == ActivityBarItem::Explorer);
    applyBtn(btn_git_, QStringLiteral("git"), item == ActivityBarItem::Git);
    applyBtn(btn_http_, QStringLiteral("http"), item == ActivityBarItem::HttpClient);
    applyBtn(btn_hydra_, QStringLiteral("hydra"), item == ActivityBarItem::Hydra);
    applyBtn(btn_terminal_, QStringLiteral("terminal"), item == ActivityBarItem::Terminal);
    applyBtn(btn_prefs_, QStringLiteral("preferences"), false);
}

void ActivityBar::setBadge(ActivityBarItem item, int count)
{
    ActivityButton *target = nullptr;
    switch (item) {
    case ActivityBarItem::Explorer: target = &btn_explorer_; break;
    case ActivityBarItem::Git: target = &btn_git_; break;
    case ActivityBarItem::HttpClient: target = &btn_http_; break;
    case ActivityBarItem::Hydra: target = &btn_hydra_; break;
    case ActivityBarItem::Terminal: target = &btn_terminal_; break;
    default: return;
    }

    if (count <= 0) {
        target->badge->hide();
        return;
    }

    target->badge->setText(count > 99 ? QStringLiteral("99+") : QString::number(count));
    target->badge->adjustSize();
    target->badge->setFixedWidth(qMax(16, target->badge->width() + 6));
    target->badge->move(28, 4);
    target->badge->show();
    target->badge->raise();
}

void ActivityBar::applyStyles()
{
    auto &ts = NezhaIDE::Services::ThemeService::instance();
    setStyleSheet(ts.qss(QStringLiteral("style.activity_bar")));
    for (auto &ab : buttons_) {
        ab.button->setStyleSheet(ts.qss(QStringLiteral("style.activity_bar_btn")));
        ab.badge->setStyleSheet(ts.qss(QStringLiteral("style.badge")));
    }
}

} // namespace NezhaIDE::Views
