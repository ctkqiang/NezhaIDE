#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>

namespace NezhaIDE::Views {

enum class ActivityBarItem {
    Explorer = 0,
    Git = 1,
    HttpClient = 2,
    Hydra = 3,
    Terminal = 4,
    Preferences = 5
};

class ActivityBar final : public QWidget {
    Q_OBJECT

public:
    explicit ActivityBar(QWidget *parent = nullptr);

    void setActive(ActivityBarItem item);
    void applyStyles();
    void setBadge(ActivityBarItem item, int count);

signals:
    void itemSelected(ActivityBarItem item);

private:
    struct ActivityButton {
        QPushButton *button;
        QLabel *badge;
    };

    ActivityButton addButton(const QString &iconPath, const QString &tooltip, ActivityBarItem id);

    ActivityButton btn_explorer_;
    ActivityButton btn_git_;
    ActivityButton btn_http_;
    ActivityButton btn_hydra_;
    ActivityButton btn_terminal_;
    ActivityButton btn_prefs_;
    ActivityBarItem active_{ActivityBarItem::Explorer};
    QList<ActivityButton> buttons_;
};

} // namespace NezhaIDE::Views
