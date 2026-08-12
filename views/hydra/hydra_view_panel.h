//
// Created by 钟智强 on 2026/8/12.
//
#pragma once

#ifndef NEZHAIDE_HYDRA_VIEW_PANEL_H
#define NEZHAIDE_HYDRA_VIEW_PANEL_H

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QHash>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QWidget>
#include "src/tools/hydra.h"

namespace NezhaIDE::Views {

/**
 * Hydra 通用工具编辑器面板，在编辑器工作区内以标签页形式打开。
 *
 * 布局与 HTTP 客户端一致：目标/服务选择区（服务选择器可搜索，按
 * 本机 hydra 探测结果标记未编译模块）、模块参数区（按工具数据库
 * 中的 ToolParameter 动态重建）、用户名区、密码来源区（GitHub/
 * Random/Custom 单选互斥，按选择条件显示子控件）、执行选项与
 * Run/Stop 控制，底部为打码后的运行日志。
 * 状态标签跟随 HydraState 状态机显示 13 种状态之一。
 *
 * @see NezhaIDE::Tools::HydraService
 */
class HydraViewPanel final : public QWidget {
    Q_OBJECT

public:
    explicit HydraViewPanel(QWidget *parent = nullptr);
    ~HydraViewPanel() override;

private:
    void setupUI();
    QWidget *buildTargetSection();
    QWidget *buildUsernameSection();
    QWidget *buildPasswordSection();
    QWidget *buildModuleOptionsSection();
    QWidget *buildOptionsSection();
    QWidget *buildLogSection();

    void populateServices();
    void onServiceTextEdited();
    void onServiceChanged();
    void rebuildServiceOptions();
    [[nodiscard]] bool hasValidService() const;

    void onBrowseUsernames();
    void onLoadUsernames(const QString &path);
    void onBrowsePasswords();
    void onLoadPasswords(const QString &path);
    void onPasswordSourceChanged();
    void onGithubImportClicked();
    void onRandomGenerateClicked();
    void onRunClicked();

    void applyStyles();
    void setState(NezhaIDE::Tools::HydraState state);
    void recomputeState();
    void updateRunButton();
    NezhaIDE::Tools::HydraConfig collectConfig() const;
    void appendLog(const QString &line);

    QLineEdit *host_input_{};
    QSpinBox *port_spin_{};
    QComboBox *service_combo_{};
    QFrame *module_options_frame_{};
    QFormLayout *param_form_{};
    QHash<QString, QWidget *> param_widgets_;
    QLineEdit *username_path_input_{};
    QLabel *username_hint_{};
    QRadioButton *github_radio_{};
    QRadioButton *random_radio_{};
    QRadioButton *custom_radio_{};
    QStackedWidget *source_stack_{};
    QLineEdit *github_url_input_{};
    QLabel *github_hint_{};
    QSpinBox *random_count_spin_{};
    QSpinBox *random_min_spin_{};
    QSpinBox *random_max_spin_{};
    QLineEdit *custom_path_input_{};
    QLabel *custom_hint_{};
    QLabel *password_hint_{};
    QSpinBox *threads_spin_{};
    QSpinBox *timeout_spin_{};
    QComboBox *try_mode_combo_{};
    QCheckBox *exit_first_check_{};
    QCheckBox *verbose_check_{};
    QLineEdit *extra_args_input_{};
    QLabel *status_label_{};
    QPushButton *run_btn_{};
    QPlainTextEdit *log_view_{};

    NezhaIDE::Tools::HydraState state_{NezhaIDE::Tools::HydraState::NoTargetConfigured};
    bool probed_{false};
    bool username_loaded_{false};
    bool username_invalid_{false};
    bool password_loaded_{false};
    bool running_{false};
};

} // namespace NezhaIDE::Views

#endif // NEZHAIDE_HYDRA_VIEW_PANEL_H
