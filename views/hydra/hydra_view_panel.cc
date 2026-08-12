//
// Created by 钟智强 on 2026/8/12.
//

#include "hydra_view_panel.h"
#include "src/services/localization_service.h"
#include "src/services/theme_service.h"
#include <QCompleter>
#include <QFileDialog>
#include <QFontDatabase>
#include <QFrame>
#include <QHBoxLayout>
#include <QSplitter>
#include <QVBoxLayout>

namespace NezhaIDE::Views {

namespace {

QString stateText(NezhaIDE::Tools::HydraState state)
{
    switch (state) {
    case NezhaIDE::Tools::HydraState::NoTargetConfigured:   return LOC("hydra.status.no_target");
    case NezhaIDE::Tools::HydraState::NoUsernameFile:        return LOC("hydra.status.no_username");
    case NezhaIDE::Tools::HydraState::UsernameLoaded:        return LOC("hydra.status.username_loaded");
    case NezhaIDE::Tools::HydraState::InvalidUsernameFile:   return LOC("hydra.status.invalid_username");
    case NezhaIDE::Tools::HydraState::SelectService:         return LOC("hydra.status.select_service");
    case NezhaIDE::Tools::HydraState::SelectPasswordSource:  return LOC("hydra.status.select_source");
    case NezhaIDE::Tools::HydraState::CustomPasswordRequired: return LOC("hydra.status.custom_required");
    case NezhaIDE::Tools::HydraState::PasswordLoaded:        return LOC("hydra.status.password_loaded");
    case NezhaIDE::Tools::HydraState::Ready:                 return LOC("hydra.status.ready");
    case NezhaIDE::Tools::HydraState::Running:               return LOC("hydra.status.running");
    case NezhaIDE::Tools::HydraState::Stopped:               return LOC("hydra.status.stopped");
    case NezhaIDE::Tools::HydraState::Completed:             return LOC("hydra.status.completed");
    case NezhaIDE::Tools::HydraState::Error:                 return LOC("hydra.status.error");
    }
    return {};
}

const char *stateColorKey(NezhaIDE::Tools::HydraState state)
{
    using NezhaIDE::Tools::HydraState;
    switch (state) {
    case HydraState::NoTargetConfigured:
    case HydraState::NoUsernameFile:
    case HydraState::SelectService:
    case HydraState::SelectPasswordSource:
    case HydraState::Stopped:
        return "text.secondary";
    case HydraState::UsernameLoaded:
    case HydraState::PasswordLoaded:
    case HydraState::Running:
        return "accent";
    case HydraState::InvalidUsernameFile:
    case HydraState::Error:
        return "git.deleted";
    case HydraState::CustomPasswordRequired:
        return "git.modified";
    case HydraState::Ready:
    case HydraState::Completed:
        return "git.added";
    }
    return "text.secondary";
}

} // namespace

HydraViewPanel::HydraViewPanel(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("hydraRoot"));
    setupUI();
    applyStyles();
    populateServices();
    recomputeState();

    connect(&NezhaIDE::Services::ThemeService::instance(), &NezhaIDE::Services::ThemeService::themeChanged,
            this, [this] { applyStyles(); });

    auto &svc = NezhaIDE::Tools::HydraService::instance();
    connect(&svc, &NezhaIDE::Tools::HydraService::modulesProbed,
            this, [this] {
        probed_ = true;
        if (!NezhaIDE::Tools::HydraService::instance().hasInstalledModules()) {
            // hydra 未安装：无可选服务，明确提示并禁用运行
            service_combo_->clear();
            service_combo_->addItem(LOC("hydra.not_installed"), QVariant());
            service_combo_->setEnabled(false);
            setState(NezhaIDE::Tools::HydraState::Error);
            status_label_->setText(LOC("hydra.not_installed"));
            return;
        }
        populateServices();
        recomputeState();
    });
    connect(&svc, &NezhaIDE::Tools::HydraService::usernameDatasetChanged,
            this, [this](int count) {
        username_loaded_ = true;
        username_invalid_ = false;
        if (username_single_radio_->isChecked()) {
            username_hint_->setText(
                LOC("hydra.username_single_loaded").arg(username_single_input_->text().trimmed()));
        } else {
            username_hint_->setText(LOC("hydra.username_hint_loaded").arg(count));
        }
        recomputeState();
    });
    connect(&svc, &NezhaIDE::Tools::HydraService::passwordDatasetChanged,
            this, [this](int count, const QString &provenance) {
        password_loaded_ = true;
        password_hint_->setText(LOC("hydra.password_hint_loaded").arg(count).arg(provenance));
        recomputeState();
    });
    connect(&svc, &NezhaIDE::Tools::HydraService::githubImportFinished,
            this, [this](int count, const QString &error) {
        if (!error.isEmpty()) {
            password_hint_->setText(LOC("hydra.error.github_fetch").arg(error));
            setState(NezhaIDE::Tools::HydraState::Error);
            return;
        }
        password_hint_->setText(LOC("hydra.password_hint_loaded").arg(count).arg(github_url_input_->text()));
        recomputeState();
    });
    connect(&svc, &NezhaIDE::Tools::HydraService::runStateChanged,
            this, [this](NezhaIDE::Tools::HydraState state) {
        running_ = (state == NezhaIDE::Tools::HydraState::Running);
        setState(state);
    });
    connect(&svc, &NezhaIDE::Tools::HydraService::runFinished,
            this, [this](bool, int, int, const QString &) {
        running_ = false;
        updateRunButton();
    });
    connect(&svc, &NezhaIDE::Tools::HydraService::logLine,
            this, &HydraViewPanel::appendLog);
}

HydraViewPanel::~HydraViewPanel() = default;

void HydraViewPanel::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto *splitter = new QSplitter(Qt::Vertical, this);
    splitter->setHandleWidth(1);
    splitter->setChildrenCollapsible(false);

    auto *configContainer = new QWidget(splitter);
    auto *configLayout = new QVBoxLayout(configContainer);
    configLayout->setContentsMargins(12, 12, 12, 8);
    configLayout->setSpacing(10);

    configLayout->addWidget(buildTargetSection());
    configLayout->addWidget(buildModuleOptionsSection());
    configLayout->addWidget(buildUsernameSection());
    configLayout->addWidget(buildPasswordSection());
    configLayout->addWidget(buildOptionsSection());

    auto *controlRow = new QHBoxLayout();
    controlRow->setSpacing(8);
    run_btn_ = new QPushButton(LOC("hydra.run"), configContainer);
    run_btn_->setObjectName(QStringLiteral("hydraRunButton"));
    run_btn_->setCursor(Qt::PointingHandCursor);
    run_btn_->setMinimumWidth(96);
    run_btn_->setMinimumHeight(34);
    connect(run_btn_, &QPushButton::clicked, this, &HydraViewPanel::onRunClicked);

    status_label_ = new QLabel(configContainer);
    status_label_->setObjectName(QStringLiteral("hydraStatusLabel"));
    status_label_->setAlignment(Qt::AlignCenter);
    status_label_->setMinimumHeight(28);

    controlRow->addWidget(run_btn_);
    controlRow->addWidget(status_label_, 1);
    configLayout->addLayout(controlRow);
    configLayout->addStretch();

    splitter->addWidget(configContainer);
    splitter->addWidget(buildLogSection());
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);
    splitter->setSizes({420, 360});

    mainLayout->addWidget(splitter);
}

QWidget *HydraViewPanel::buildTargetSection()
{
    auto *frame = new QFrame(this);
    frame->setObjectName(QStringLiteral("hydraTargetBar"));
    auto *layout = new QHBoxLayout(frame);
    layout->setContentsMargins(10, 6, 10, 6);
    layout->setSpacing(6);

    auto *serviceLabel = new QLabel(LOC("hydra.service"), frame);
    serviceLabel->setObjectName(QStringLiteral("hydraSectionLabel"));

    service_combo_ = new QComboBox(frame);
    service_combo_->setObjectName(QStringLiteral("hydraServiceCombo"));
    service_combo_->setEditable(true);
    service_combo_->setInsertPolicy(QComboBox::NoInsert);
    service_combo_->setMinimumHeight(32);
    service_combo_->setCursor(Qt::PointingHandCursor);
    service_combo_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    auto *completer = service_combo_->completer();
    if (completer) {
        completer->setCompletionMode(QCompleter::PopupCompletion);
        completer->setFilterMode(Qt::MatchContains);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
    }
    connect(service_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this] { onServiceChanged(); });
    connect(service_combo_, &QComboBox::editTextChanged,
            this, &HydraViewPanel::onServiceTextEdited);

    auto *targetLabel = new QLabel(LOC("hydra.target"), frame);
    targetLabel->setObjectName(QStringLiteral("hydraSectionLabel"));

    host_input_ = new QLineEdit(frame);
    host_input_->setObjectName(QStringLiteral("hydraHostInput"));
    host_input_->setPlaceholderText(QStringLiteral("192.168.1.1"));
    host_input_->setMinimumHeight(32);
    connect(host_input_, &QLineEdit::textChanged, this, [this] { recomputeState(); });

    port_spin_ = new QSpinBox(frame);
    port_spin_->setObjectName(QStringLiteral("hydraPortSpin"));
    port_spin_->setRange(1, 65535);
    port_spin_->setValue(22);
    port_spin_->setMinimumHeight(32);
    port_spin_->setFixedWidth(96);

    layout->addWidget(serviceLabel);
    layout->addWidget(service_combo_);
    layout->addWidget(targetLabel);
    layout->addWidget(host_input_, 1);
    layout->addWidget(port_spin_);
    return frame;
}

QWidget *HydraViewPanel::buildModuleOptionsSection()
{
    module_options_frame_ = new QFrame(this);
    module_options_frame_->setObjectName(QStringLiteral("hydraSection"));
    auto *layout = new QVBoxLayout(module_options_frame_);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(6);

    auto *label = new QLabel(LOC("hydra.module_options"), module_options_frame_);
    label->setObjectName(QStringLiteral("hydraSectionLabel"));

    param_form_ = new QFormLayout();
    param_form_->setContentsMargins(0, 0, 0, 0);
    param_form_->setSpacing(6);
    param_form_->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    layout->addWidget(label);
    layout->addLayout(param_form_);
    return module_options_frame_;
}

QWidget *HydraViewPanel::buildUsernameSection()
{
    auto *frame = new QFrame(this);
    frame->setObjectName(QStringLiteral("hydraSection"));
    auto *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(6);

    auto *label = new QLabel(LOC("hydra.usernames"), frame);
    label->setObjectName(QStringLiteral("hydraSectionLabel"));

    auto *radioRow = new QHBoxLayout();
    radioRow->setSpacing(16);
    username_file_radio_ = new QRadioButton(LOC("hydra.username_file"), frame);
    username_file_radio_->setObjectName(QStringLiteral("hydraUsernameSourceFile"));
    username_single_radio_ = new QRadioButton(LOC("hydra.username_single"), frame);
    username_single_radio_->setObjectName(QStringLiteral("hydraUsernameSourceSingle"));
    username_file_radio_->setChecked(true);
    radioRow->addWidget(username_file_radio_);
    radioRow->addWidget(username_single_radio_);
    layout->addLayout(radioRow);

    username_file_row_ = new QWidget(frame);
    auto *fileLayout = new QHBoxLayout(username_file_row_);
    fileLayout->setContentsMargins(0, 0, 0, 0);
    fileLayout->setSpacing(6);
    username_path_input_ = new QLineEdit(username_file_row_);
    username_path_input_->setObjectName(QStringLiteral("hydraUsernamePath"));
    username_path_input_->setPlaceholderText(LOC("hydra.username_placeholder"));
    username_path_input_->setMinimumHeight(30);
    connect(username_path_input_, &QLineEdit::returnPressed, this,
            [this] { onLoadUsernames(username_path_input_->text()); });
    auto *browseBtn = new QPushButton(LOC("hydra.browse"), username_file_row_);
    browseBtn->setObjectName(QStringLiteral("hydraGhostButton"));
    browseBtn->setCursor(Qt::PointingHandCursor);
    browseBtn->setMinimumHeight(30);
    connect(browseBtn, &QPushButton::clicked, this, &HydraViewPanel::onBrowseUsernames);
    fileLayout->addWidget(username_path_input_, 1);
    fileLayout->addWidget(browseBtn);
    layout->addWidget(username_file_row_);

    username_single_row_ = new QWidget(frame);
    auto *singleLayout = new QHBoxLayout(username_single_row_);
    singleLayout->setContentsMargins(0, 0, 0, 0);
    singleLayout->setSpacing(6);
    username_single_input_ = new QLineEdit(username_single_row_);
    username_single_input_->setObjectName(QStringLiteral("hydraUsernameSingle"));
    username_single_input_->setPlaceholderText(LOC("hydra.username_single_placeholder"));
    username_single_input_->setMinimumHeight(30);
    connect(username_single_input_, &QLineEdit::returnPressed, this,
            [this] { onLoadSingleUsername(); });
    singleLayout->addWidget(username_single_input_, 1);
    username_single_row_->setVisible(false);
    layout->addWidget(username_single_row_);

    username_hint_ = new QLabel(LOC("hydra.username_hint_none"), frame);
    username_hint_->setObjectName(QStringLiteral("hydraHintLabel"));

    connect(username_file_radio_, &QRadioButton::toggled, this, [this](const bool checked) {
        username_file_row_->setVisible(checked);
        username_single_row_->setVisible(!checked);
        if (checked && username_path_input_->text().trimmed().isEmpty()) {
            username_loaded_ = false;
            username_invalid_ = false;
            username_hint_->setText(LOC("hydra.username_hint_none"));
            recomputeState();
        }
    });

    layout->addWidget(username_hint_);
    return frame;
}

QWidget *HydraViewPanel::buildPasswordSection()
{
    auto *frame = new QFrame(this);
    frame->setObjectName(QStringLiteral("hydraSection"));
    auto *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(6);

    auto *label = new QLabel(LOC("hydra.password_source"), frame);
    label->setObjectName(QStringLiteral("hydraSectionLabel"));

    auto *radioRow = new QHBoxLayout();
    radioRow->setSpacing(16);
    github_radio_ = new QRadioButton(LOC("hydra.github"), frame);
    random_radio_ = new QRadioButton(LOC("hydra.random"), frame);
    custom_radio_ = new QRadioButton(LOC("hydra.custom"), frame);
    github_radio_->setObjectName(QStringLiteral("hydraSourceGithub"));
    random_radio_->setObjectName(QStringLiteral("hydraSourceRandom"));
    custom_radio_->setObjectName(QStringLiteral("hydraSourceCustom"));
    for (auto *radio : {github_radio_, random_radio_, custom_radio_}) {
        radio->setCursor(Qt::PointingHandCursor);
        radioRow->addWidget(radio);
    }
    radioRow->addStretch();

    source_stack_ = new QStackedWidget(frame);
    source_stack_->setObjectName(QStringLiteral("hydraSourceStack"));

    auto *githubPage = new QWidget(source_stack_);
    auto *githubLayout = new QVBoxLayout(githubPage);
    githubLayout->setContentsMargins(0, 4, 0, 0);
    githubLayout->setSpacing(4);
    auto *githubRow = new QHBoxLayout();
    githubRow->setSpacing(6);
    github_url_input_ = new QLineEdit(githubPage);
    github_url_input_->setObjectName(QStringLiteral("hydraGithubUrl"));
    github_url_input_->setPlaceholderText(LOC("hydra.github_placeholder"));
    github_url_input_->setMinimumHeight(30);
    auto *importBtn = new QPushButton(LOC("hydra.github_import"), githubPage);
    importBtn->setObjectName(QStringLiteral("hydraGithubImportButton"));
    importBtn->setCursor(Qt::PointingHandCursor);
    importBtn->setMinimumHeight(30);
    connect(importBtn, &QPushButton::clicked, this, &HydraViewPanel::onGithubImportClicked);
    githubRow->addWidget(github_url_input_, 1);
    githubRow->addWidget(importBtn);
    github_hint_ = new QLabel(LOC("hydra.github_hint"), githubPage);
    github_hint_->setObjectName(QStringLiteral("hydraHintLabel"));
    github_hint_->setWordWrap(true);
    githubLayout->addLayout(githubRow);
    githubLayout->addWidget(github_hint_);
    source_stack_->addWidget(githubPage);

    auto *randomPage = new QWidget(source_stack_);
    auto *randomLayout = new QHBoxLayout(randomPage);
    randomLayout->setContentsMargins(0, 4, 0, 0);
    randomLayout->setSpacing(6);
    auto *countLabel = new QLabel(LOC("hydra.random_count"), randomPage);
    countLabel->setObjectName(QStringLiteral("hydraSectionLabel"));
    random_count_spin_ = new QSpinBox(randomPage);
    random_count_spin_->setObjectName(QStringLiteral("hydraRandomCount"));
    random_count_spin_->setRange(1, 100000);
    random_count_spin_->setValue(100);
    random_count_spin_->setMinimumHeight(30);
    auto *minLabel = new QLabel(LOC("hydra.random_min"), randomPage);
    minLabel->setObjectName(QStringLiteral("hydraSectionLabel"));
    random_min_spin_ = new QSpinBox(randomPage);
    random_min_spin_->setObjectName(QStringLiteral("hydraRandomMin"));
    random_min_spin_->setRange(1, 128);
    random_min_spin_->setValue(8);
    random_min_spin_->setMinimumHeight(30);
    auto *maxLabel = new QLabel(LOC("hydra.random_max"), randomPage);
    maxLabel->setObjectName(QStringLiteral("hydraSectionLabel"));
    random_max_spin_ = new QSpinBox(randomPage);
    random_max_spin_->setObjectName(QStringLiteral("hydraRandomMax"));
    random_max_spin_->setRange(1, 128);
    random_max_spin_->setValue(16);
    random_max_spin_->setMinimumHeight(30);
    auto *generateBtn = new QPushButton(LOC("hydra.generate"), randomPage);
    generateBtn->setObjectName(QStringLiteral("hydraGenerateButton"));
    generateBtn->setCursor(Qt::PointingHandCursor);
    generateBtn->setMinimumHeight(30);
    connect(generateBtn, &QPushButton::clicked, this, &HydraViewPanel::onRandomGenerateClicked);
    randomLayout->addWidget(countLabel);
    randomLayout->addWidget(random_count_spin_);
    randomLayout->addWidget(minLabel);
    randomLayout->addWidget(random_min_spin_);
    randomLayout->addWidget(maxLabel);
    randomLayout->addWidget(random_max_spin_);
    randomLayout->addWidget(generateBtn);
    randomLayout->addStretch();
    source_stack_->addWidget(randomPage);

    auto *customPage = new QWidget(source_stack_);
    auto *customLayout = new QVBoxLayout(customPage);
    customLayout->setContentsMargins(0, 4, 0, 0);
    customLayout->setSpacing(4);
    auto *customRow = new QHBoxLayout();
    customRow->setSpacing(6);
    custom_path_input_ = new QLineEdit(customPage);
    custom_path_input_->setObjectName(QStringLiteral("hydraCustomPath"));
    custom_path_input_->setPlaceholderText(LOC("hydra.custom_placeholder"));
    custom_path_input_->setMinimumHeight(30);
    connect(custom_path_input_, &QLineEdit::returnPressed, this,
            [this] { onLoadPasswords(custom_path_input_->text()); });
    auto *customBrowse = new QPushButton(LOC("hydra.browse"), customPage);
    customBrowse->setObjectName(QStringLiteral("hydraGhostButton"));
    customBrowse->setCursor(Qt::PointingHandCursor);
    customBrowse->setMinimumHeight(30);
    connect(customBrowse, &QPushButton::clicked, this, &HydraViewPanel::onBrowsePasswords);
    custom_hint_ = new QLabel(LOC("hydra.custom_hint"), customPage);
    custom_hint_->setObjectName(QStringLiteral("hydraHintLabel"));
    customRow->addWidget(custom_path_input_, 1);
    customRow->addWidget(customBrowse);
    customLayout->addLayout(customRow);
    customLayout->addWidget(custom_hint_);
    source_stack_->addWidget(customPage);

    password_hint_ = new QLabel(frame);
    password_hint_->setObjectName(QStringLiteral("hydraHintLabel"));
    password_hint_->setText(LOC("hydra.password_hint_none"));

    connect(github_radio_, &QRadioButton::toggled, this,
            [this](bool checked) {
        if (checked) source_stack_->setCurrentIndex(0);
        onPasswordSourceChanged();
    });
    connect(random_radio_, &QRadioButton::toggled, this,
            [this](bool checked) {
        if (checked) source_stack_->setCurrentIndex(1);
        onPasswordSourceChanged();
    });
    connect(custom_radio_, &QRadioButton::toggled, this,
            [this](bool checked) {
        if (checked) source_stack_->setCurrentIndex(2);
        onPasswordSourceChanged();
    });

    layout->addWidget(label);
    layout->addLayout(radioRow);
    layout->addWidget(source_stack_);
    layout->addWidget(password_hint_);
    return frame;
}

QWidget *HydraViewPanel::buildOptionsSection()
{
    auto *frame = new QFrame(this);
    frame->setObjectName(QStringLiteral("hydraSection"));
    auto *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(6);

    auto *row1 = new QHBoxLayout();
    row1->setSpacing(6);

    auto *threadsLabel = new QLabel(LOC("hydra.threads"), frame);
    threadsLabel->setObjectName(QStringLiteral("hydraSectionLabel"));
    threads_spin_ = new QSpinBox(frame);
    threads_spin_->setObjectName(QStringLiteral("hydraThreads"));
    threads_spin_->setRange(1, 64);
    threads_spin_->setValue(4);
    threads_spin_->setMinimumHeight(30);

    auto *timeoutLabel = new QLabel(LOC("hydra.timeout"), frame);
    timeoutLabel->setObjectName(QStringLiteral("hydraSectionLabel"));
    timeout_spin_ = new QSpinBox(frame);
    timeout_spin_->setObjectName(QStringLiteral("hydraTimeout"));
    timeout_spin_->setRange(1, 600);
    timeout_spin_->setValue(30);
    timeout_spin_->setMinimumHeight(30);

    auto *tryLabel = new QLabel(LOC("hydra.try_mode"), frame);
    tryLabel->setObjectName(QStringLiteral("hydraSectionLabel"));
    try_mode_combo_ = new QComboBox(frame);
    try_mode_combo_->setObjectName(QStringLiteral("hydraTryMode"));
    try_mode_combo_->setMinimumHeight(30);
    try_mode_combo_->setCursor(Qt::PointingHandCursor);
    try_mode_combo_->addItem(LOC("hydra.try_none"), QString());
    try_mode_combo_->addItem(LOC("hydra.try_null"), QStringLiteral("n"));
    try_mode_combo_->addItem(LOC("hydra.try_same"), QStringLiteral("s"));
    try_mode_combo_->addItem(LOC("hydra.try_reverse"), QStringLiteral("r"));
    try_mode_combo_->addItem(LOC("hydra.try_all"), QStringLiteral("nsr"));

    row1->addWidget(threadsLabel);
    row1->addWidget(threads_spin_);
    row1->addSpacing(8);
    row1->addWidget(timeoutLabel);
    row1->addWidget(timeout_spin_);
    row1->addSpacing(8);
    row1->addWidget(tryLabel);
    row1->addWidget(try_mode_combo_);
    row1->addStretch();

    auto *row2 = new QHBoxLayout();
    row2->setSpacing(12);
    exit_first_check_ = new QCheckBox(LOC("hydra.exit_first"), frame);
    exit_first_check_->setObjectName(QStringLiteral("hydraExitFirst"));
    exit_first_check_->setCursor(Qt::PointingHandCursor);
    verbose_check_ = new QCheckBox(LOC("hydra.verbose"), frame);
    verbose_check_->setObjectName(QStringLiteral("hydraVerbose"));
    verbose_check_->setCursor(Qt::PointingHandCursor);
    auto *extraLabel = new QLabel(LOC("hydra.extra_args"), frame);
    extraLabel->setObjectName(QStringLiteral("hydraSectionLabel"));
    extra_args_input_ = new QLineEdit(frame);
    extra_args_input_->setObjectName(QStringLiteral("hydraExtraArgs"));
    extra_args_input_->setPlaceholderText(QStringLiteral("-f"));
    extra_args_input_->setMinimumHeight(30);

    row2->addWidget(exit_first_check_);
    row2->addWidget(verbose_check_);
    row2->addSpacing(8);
    row2->addWidget(extraLabel);
    row2->addWidget(extra_args_input_, 1);

    layout->addLayout(row1);
    layout->addLayout(row2);
    return frame;
}

QWidget *HydraViewPanel::buildLogSection()
{
    auto *container = new QWidget(this);
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(12, 8, 12, 12);
    layout->setSpacing(6);

    auto *label = new QLabel(LOC("hydra.log_title"), container);
    label->setObjectName(QStringLiteral("hydraSectionLabel"));

    log_view_ = new QPlainTextEdit(container);
    log_view_->setObjectName(QStringLiteral("hydraLogView"));
    log_view_->setReadOnly(true);
    log_view_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    log_view_->setPlaceholderText(LOC("hydra.log_hint"));

    layout->addWidget(label);
    layout->addWidget(log_view_, 1);
    return container;
}

void HydraViewPanel::populateServices()
{
    const auto previous = service_combo_->currentData().toString();
    service_combo_->blockSignals(true);
    service_combo_->clear();
    const auto modules = NezhaIDE::Tools::HydraService::instance().knownModules();
    for (const auto &mod : modules) {
        service_combo_->addItem(mod.display, QVariant(mod.name));
    }
    service_combo_->blockSignals(false);

    const auto suffix = LOC("hydra.unavailable_suffix");
    int selected = -1;
    for (int i = 0; i < service_combo_->count(); ++i) {
        const auto name = service_combo_->itemData(i).toString();
        if (probed_ && !NezhaIDE::Tools::HydraService::instance().isModuleInstalled(name)) {
            service_combo_->setItemText(i, service_combo_->itemText(i) + QLatin1Char(' ') + suffix);
            service_combo_->setItemData(i, false, Qt::UserRole - 1);
        }
        if (name == previous || (selected < 0 && service_combo_->itemData(i, Qt::UserRole - 1).toBool())) {
            selected = i;
        }
    }
    service_combo_->setCurrentIndex(selected);
    onServiceChanged();
}

void HydraViewPanel::onServiceTextEdited()
{
    const auto text = service_combo_->currentText().trimmed();
    if (service_combo_->currentData().isValid() || text.isEmpty()) {
        recomputeState();
        return;
    }
    const auto modules = NezhaIDE::Tools::HydraService::instance().knownModules();
    for (int i = 0; i < modules.size(); ++i) {
        if (modules[i].name.compare(text, Qt::CaseInsensitive) == 0) {
            // 吸附到匹配项，后续由 currentIndexChanged 走完整更新
            service_combo_->setCurrentIndex(i);
            return;
        }
    }
    recomputeState();
}

void HydraViewPanel::onServiceChanged()
{
    const auto name = service_combo_->currentData().toString();
    for (const auto &mod : NezhaIDE::Tools::HydraService::instance().knownModules()) {
        if (mod.name == name) {
            port_spin_->setValue(mod.port);
            break;
        }
    }
    rebuildServiceOptions();
    recomputeState();
}

void HydraViewPanel::rebuildServiceOptions()
{
    // Qt 6.4+ 的 removeRow 会直接销毁行内 widget，若再对其 deleteLater
    // 会向已销毁对象投递事件导致崩溃；takeRow 只解绑不销毁
    while (param_form_->rowCount() > 0) {
        const auto taken = param_form_->takeRow(0);
        for (auto *item : {taken.labelItem, taken.fieldItem}) {
            if (item && item->widget()) {
                item->widget()->deleteLater();
            }
        }
    }
    param_widgets_.clear();

    const auto params = NezhaIDE::Tools::HydraService::instance()
                            .moduleParams(service_combo_->currentData().toString());
    module_options_frame_->setVisible(!params.isEmpty());
    for (const auto &param : params) {
        QWidget *widget = nullptr;
        if (param.type == QLatin1String("boolean")) {
            auto *check = new QCheckBox(param.display, module_options_frame_);
            check->setChecked(param.default_value == QLatin1String("1"));
            widget = check;
        } else if (param.type == QLatin1String("integer")) {
            auto *spin = new QSpinBox(module_options_frame_);
            spin->setRange(0, 100000);
            spin->setValue(param.default_value.toInt());
            spin->setMinimumHeight(30);
            widget = spin;
        } else {
            auto *edit = new QLineEdit(module_options_frame_);
            edit->setText(param.default_value);
            edit->setPlaceholderText(param.help);
            edit->setMinimumHeight(30);
            widget = edit;
        }
        widget->setObjectName(QStringLiteral("hydraModuleParam_%1").arg(param.name));
        param_form_->addRow(param.display, widget);
        param_widgets_.insert(param.name, widget);
    }
    applyStyles();
}

bool HydraViewPanel::hasValidService() const
{
    if (service_combo_->currentData().isValid()) {
        return true;
    }
    const auto text = service_combo_->currentText().trimmed();
    if (text.isEmpty()) {
        return false;
    }
    const auto modules = NezhaIDE::Tools::HydraService::instance().knownModules();
    for (const auto &mod : modules) {
        if (mod.name.compare(text, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

void HydraViewPanel::onBrowseUsernames()
{
    const auto path = QFileDialog::getOpenFileName(this, LOC("hydra.username_file"),
        QString(), QStringLiteral("Text Files (*.txt);;All Files (*)"));
    if (path.isEmpty()) return;
    username_path_input_->setText(path);
    onLoadUsernames(path);
}

void HydraViewPanel::onLoadUsernames(const QString &path)
{
    if (path.trimmed().isEmpty()) {
        username_loaded_ = false;
        username_invalid_ = false;
        username_hint_->setText(LOC("hydra.username_hint_none"));
        recomputeState();
        return;
    }
    const auto result = NezhaIDE::Tools::HydraService::instance().loadUsernameFile(path);
    if (!result.ok()) {
        username_loaded_ = false;
        username_invalid_ = true;
        username_hint_->setText(LOC("hydra.error.username_invalid"));
        setState(NezhaIDE::Tools::HydraState::InvalidUsernameFile);
        return;
    }
    username_hint_->setText(LOC("hydra.username_hint_loaded").arg(result.entry_count));
    username_loaded_ = true;
    username_invalid_ = false;
    recomputeState();
}

void HydraViewPanel::onLoadSingleUsername()
{
    const auto name = username_single_input_->text().trimmed();
    if (name.isEmpty()) {
        username_loaded_ = false;
        username_invalid_ = false;
        username_hint_->setText(LOC("hydra.username_hint_none"));
        recomputeState();
        return;
    }
    const auto result = NezhaIDE::Tools::HydraService::instance().loadSingleUsername(name);
    if (!result.ok()) {
        username_loaded_ = false;
        username_invalid_ = true;
        username_hint_->setText(LOC("hydra.error.username_invalid"));
        setState(NezhaIDE::Tools::HydraState::InvalidUsernameFile);
        return;
    }
    username_hint_->setText(LOC("hydra.username_single_loaded").arg(name));
    username_loaded_ = true;
    username_invalid_ = false;
    recomputeState();
}

void HydraViewPanel::onBrowsePasswords()
{
    const auto path = QFileDialog::getOpenFileName(this, LOC("hydra.custom_file"),
        QString(), QStringLiteral("Text Files (*.txt);;All Files (*)"));
    if (path.isEmpty()) return;
    custom_path_input_->setText(path);
    onLoadPasswords(path);
}

void HydraViewPanel::onLoadPasswords(const QString &path)
{
    if (path.trimmed().isEmpty()) {
        password_loaded_ = false;
        password_hint_->setText(LOC("hydra.password_hint_none"));
        recomputeState();
        return;
    }
    const auto result = NezhaIDE::Tools::HydraService::instance().loadPasswordFile(path);
    if (!result.ok()) {
        password_loaded_ = false;
        password_hint_->setText(LOC("hydra.error.password_invalid"));
        setState(NezhaIDE::Tools::HydraState::Error);
        return;
    }
    password_hint_->setText(LOC("hydra.password_hint_loaded").arg(result.entry_count).arg(path));
    password_loaded_ = true;
    recomputeState();
}

void HydraViewPanel::onPasswordSourceChanged()
{
    if (github_radio_->isChecked()) {
        source_stack_->setCurrentIndex(0);
    } else if (random_radio_->isChecked()) {
        source_stack_->setCurrentIndex(1);
    } else if (custom_radio_->isChecked()) {
        source_stack_->setCurrentIndex(2);
    }
    recomputeState();
}

void HydraViewPanel::onGithubImportClicked()
{
    const auto url = github_url_input_->text().trimmed();
    if (url.isEmpty()) {
        password_hint_->setText(LOC("hydra.error.github_url"));
        setState(NezhaIDE::Tools::HydraState::Error);
        return;
    }
    NezhaIDE::Tools::HydraService::instance().importGithubPasswords(url);
    github_hint_->setText(LOC("hydra.github_importing"));
}

void HydraViewPanel::onRandomGenerateClicked()
{
    const auto result = NezhaIDE::Tools::HydraService::instance().generateRandomPasswords(
        random_count_spin_->value(), random_min_spin_->value(), random_max_spin_->value());
    if (!result.ok()) {
        password_hint_->setText(LOC("hydra.error.password_invalid"));
        setState(NezhaIDE::Tools::HydraState::Error);
        return;
    }
    password_hint_->setText(LOC("hydra.password_hint_loaded").arg(result.entry_count).arg(LOC("hydra.random")));
    password_loaded_ = true;
    recomputeState();
}

void HydraViewPanel::onRunClicked()
{
    if (running_) {
        NezhaIDE::Tools::HydraService::instance().stop();
        return;
    }
    const auto config = collectConfig();
    if (config.target_host.trimmed().isEmpty()) {
        appendLog(LOC("hydra.error.host_required"));
        setState(NezhaIDE::Tools::HydraState::Error);
        return;
    }
    NezhaIDE::Tools::HydraService::instance().run(config);
}

NezhaIDE::Tools::HydraConfig HydraViewPanel::collectConfig() const
{
    NezhaIDE::Tools::HydraConfig config;
    config.target_host = host_input_->text().trimmed();
    config.target_port = port_spin_->value();
    config.service = service_combo_->currentData().isValid()
                         ? service_combo_->currentData().toString()
                         : service_combo_->currentText().trimmed();
    config.username_file = username_path_input_->text().trimmed();
    if (github_radio_->isChecked()) {
        config.password_source = NezhaIDE::Tools::PasswordSource::GitHub;
        config.github_url = github_url_input_->text().trimmed();
    } else if (random_radio_->isChecked()) {
        config.password_source = NezhaIDE::Tools::PasswordSource::Random;
        config.random_count = random_count_spin_->value();
        config.random_min_length = random_min_spin_->value();
        config.random_max_length = random_max_spin_->value();
    } else if (custom_radio_->isChecked()) {
        config.password_source = NezhaIDE::Tools::PasswordSource::Custom;
        config.custom_password_file = custom_path_input_->text().trimmed();
    }
    config.threads = threads_spin_->value();
    config.timeout_seconds = timeout_spin_->value();
    config.try_mode = try_mode_combo_->currentData().toString();
    config.exit_on_first = exit_first_check_->isChecked();
    config.verbose = verbose_check_->isChecked();
    config.extra_args = extra_args_input_->text().trimmed();
    for (auto it = param_widgets_.cbegin(); it != param_widgets_.cend(); ++it) {
        if (auto *edit = qobject_cast<QLineEdit *>(it.value())) {
            config.module_params.insert(it.key(), edit->text().trimmed());
        } else if (auto *spin = qobject_cast<QSpinBox *>(it.value())) {
            config.module_params.insert(it.key(), QString::number(spin->value()));
        } else if (auto *check = qobject_cast<QCheckBox *>(it.value())) {
            config.module_params.insert(it.key(),
                check->isChecked() ? QStringLiteral("1") : QStringLiteral("0"));
        }
    }
    return config;
}

void HydraViewPanel::recomputeState()
{
    if (running_) return;
    if (username_invalid_) {
        setState(NezhaIDE::Tools::HydraState::InvalidUsernameFile);
        return;
    }
    if (!hasValidService()) {
        setState(NezhaIDE::Tools::HydraState::SelectService);
        return;
    }
    if (host_input_->text().trimmed().isEmpty()) {
        setState(NezhaIDE::Tools::HydraState::NoTargetConfigured);
        return;
    }
    if (!username_loaded_) {
        setState(NezhaIDE::Tools::HydraState::NoUsernameFile);
        return;
    }
    if (!password_loaded_) {
        if (custom_radio_->isChecked()) {
            setState(NezhaIDE::Tools::HydraState::CustomPasswordRequired);
        } else {
            setState(NezhaIDE::Tools::HydraState::SelectPasswordSource);
        }
        return;
    }
    setState(NezhaIDE::Tools::HydraState::Ready);
}

void HydraViewPanel::setState(NezhaIDE::Tools::HydraState state)
{
    state_ = state;
    const auto &ts = NezhaIDE::Services::ThemeService::instance();
    const auto bg = ts.color(QString::fromUtf8(stateColorKey(state)));
    status_label_->setStyleSheet(
        QStringLiteral("QLabel#hydraStatusLabel { background: %1; color: %2;"
                       "border-radius: 11px; padding: 4px 14px;"
                       "font-size: 12px; font-weight: bold; }")
            .arg(bg, ts.color(QStringLiteral("button.text"))));
    status_label_->setText(stateText(state));
    updateRunButton();
}

void HydraViewPanel::updateRunButton()
{
    if (running_) {
        run_btn_->setText(LOC("hydra.stop"));
        run_btn_->setStyleSheet(NezhaIDE::Services::ThemeService::instance()
                                    .qss(QStringLiteral("style.http_cancel_button")));
        return;
    }
    run_btn_->setText(LOC("hydra.run"));
    run_btn_->setStyleSheet(NezhaIDE::Services::ThemeService::instance()
                                .qss(QStringLiteral("style.primary_button")));
    run_btn_->setEnabled(state_ == NezhaIDE::Tools::HydraState::Ready);
}

void HydraViewPanel::appendLog(const QString &line)
{
    log_view_->appendPlainText(line);
    auto cursor = log_view_->textCursor();
    cursor.movePosition(QTextCursor::End);
    log_view_->setTextCursor(cursor);
}

void HydraViewPanel::applyStyles()
{
    auto &ts = NezhaIDE::Services::ThemeService::instance();
    setStyleSheet(ts.qss(QStringLiteral("style.hydra_panel")));
    host_input_->setStyleSheet(ts.qss(QStringLiteral("style.http_url_input")));
    username_path_input_->setStyleSheet(ts.qss(QStringLiteral("style.http_url_input")));
    github_url_input_->setStyleSheet(ts.qss(QStringLiteral("style.http_url_input")));
    custom_path_input_->setStyleSheet(ts.qss(QStringLiteral("style.http_url_input")));
    extra_args_input_->setStyleSheet(ts.qss(QStringLiteral("style.http_url_input")));
    service_combo_->setStyleSheet(ts.qss(QStringLiteral("style.http_combo")));
    try_mode_combo_->setStyleSheet(ts.qss(QStringLiteral("style.http_combo")));
    log_view_->setStyleSheet(ts.qss(QStringLiteral("style.git_diff")));
    for (auto *spin : {port_spin_, random_count_spin_, random_min_spin_, random_max_spin_,
                        threads_spin_, timeout_spin_}) {
        spin->setStyleSheet(ts.qss(QStringLiteral("style.hydra_spin")));
    }
    for (auto it = param_widgets_.cbegin(); it != param_widgets_.cend(); ++it) {
        if (auto *edit = qobject_cast<QLineEdit *>(it.value())) {
            edit->setStyleSheet(ts.qss(QStringLiteral("style.http_url_input")));
        } else if (auto *spin = qobject_cast<QSpinBox *>(it.value())) {
            spin->setStyleSheet(ts.qss(QStringLiteral("style.hydra_spin")));
        }
    }
    updateRunButton();
}

} // namespace NezhaIDE::Views
