#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>

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

signals:
    void itemSelected(ActivityBarItem item);

private:
    QPushButton *addButton(const QString &iconPath, const QString &tooltip, ActivityBarItem id);

    QPushButton *btn_explorer_{};
    QPushButton *btn_git_{};
    QPushButton *btn_http_{};
    QPushButton *btn_hydra_{};
    QPushButton *btn_terminal_{};
    QPushButton *btn_prefs_{};
    ActivityBarItem active_ = ActivityBarItem::Explorer;
};

} // namespace NezhaIDE::Views
