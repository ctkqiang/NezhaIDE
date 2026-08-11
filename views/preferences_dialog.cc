#include "preferences_dialog.h"
#include "src/configuration.h"
#include "src/services/localization_service.h"
#include "src/services/theme_service.h"
#include "src/utilities/logger.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace NezhaIDE::Views {

PreferencesDialog::PreferencesDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QString::fromUtf8(
        NezhaIDE::Constants::ApplicationName.data(),
        static_cast<int>(NezhaIDE::Constants::ApplicationName.size())) +
        QStringLiteral(" — ") + LOC("prefs.title"));
    setMinimumWidth(420);
    setupUi();
    loadSettings();
}

void PreferencesDialog::setupUi()
{
    auto *main_layout = new QVBoxLayout(this);
    main_layout->setSpacing(12);

    auto *theme_group = new QGroupBox(LOC("prefs.theme"), this);
    auto *theme_layout = new QFormLayout(theme_group);
    theme_combo_ = new QComboBox(this);
    theme_combo_->addItem(LOC("prefs.theme_auto"), static_cast<int>(NezhaIDE::IDETheme::Auto));
    theme_combo_->addItem(LOC("prefs.theme_light"), static_cast<int>(NezhaIDE::IDETheme::Light));
    theme_combo_->addItem(LOC("prefs.theme_dark"), static_cast<int>(NezhaIDE::IDETheme::Dark));
    theme_combo_->addItem(LOC("prefs.theme_github"), static_cast<int>(NezhaIDE::IDETheme::GitHub));
    theme_combo_->addItem(LOC("prefs.theme_xcode"), static_cast<int>(NezhaIDE::IDETheme::Xcode));
    theme_layout->addRow(theme_combo_);
    main_layout->addWidget(theme_group);

    auto *lang_group = new QGroupBox(LOC("prefs.language"), this);
    auto *lang_layout = new QFormLayout(lang_group);
    language_combo_ = new QComboBox(this);
    language_combo_->addItem(LOC("prefs.lang_chinese"), static_cast<int>(NezhaIDE::IDELanguage::Chinese));
    language_combo_->addItem(LOC("prefs.lang_english"), static_cast<int>(NezhaIDE::IDELanguage::English));
    lang_layout->addRow(language_combo_);
    main_layout->addWidget(lang_group);

    auto *llm_group = new QGroupBox(LOC("prefs.llm"), this);
    auto *llm_layout = new QFormLayout(llm_group);
    llm_combo_ = new QComboBox(this);
    llm_combo_->addItem(QStringLiteral("DeepSeek"), static_cast<int>(NezhaIDE::IDELLM::DeepSeek));
    llm_combo_->addItem(QStringLiteral("Kimi"), static_cast<int>(NezhaIDE::IDELLM::Kimi));
    llm_combo_->addItem(QStringLiteral("OpenRouter"), static_cast<int>(NezhaIDE::IDELLM::OpenRouter));
    llm_layout->addRow(LOC("prefs.llm_provider"), llm_combo_);

    api_token_edit_ = new QLineEdit(this);
    api_token_edit_->setEchoMode(QLineEdit::Password);
    api_token_edit_->setPlaceholderText(QStringLiteral("sk-..."));
    llm_layout->addRow(LOC("prefs.llm_token"), api_token_edit_);
    main_layout->addWidget(llm_group);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply, this);
    connect(buttons, &QDialogButtonBox::accepted, this, [this] {
        applySettings();
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons->button(QDialogButtonBox::Apply), &QPushButton::clicked,
            this, &PreferencesDialog::applySettings);
    main_layout->addWidget(buttons);
}

void PreferencesDialog::loadSettings()
{
    auto &cfg = NezhaIDE::Configuration::instance();

    const auto themeIdx = theme_combo_->findData(static_cast<int>(cfg.theme()));
    if (themeIdx >= 0) theme_combo_->setCurrentIndex(themeIdx);

    const auto langIdx = language_combo_->findData(static_cast<int>(cfg.language()));
    if (langIdx >= 0) language_combo_->setCurrentIndex(langIdx);

    const auto llmIdx = llm_combo_->findData(static_cast<int>(cfg.llm()));
    if (llmIdx >= 0) llm_combo_->setCurrentIndex(llmIdx);

    api_token_edit_->setText(cfg.llm_api_token());
}

void PreferencesDialog::applySettings()
{
    auto &cfg = NezhaIDE::Configuration::instance();

    const auto newTheme = static_cast<NezhaIDE::IDETheme>(theme_combo_->currentData().toInt());
    const auto newLang = static_cast<NezhaIDE::IDELanguage>(language_combo_->currentData().toInt());
    const auto newLlm = static_cast<NezhaIDE::IDELLM>(llm_combo_->currentData().toInt());

    bool themeChanged_ = (newTheme != cfg.theme());
    bool langChanged_ = (newLang != cfg.language());

    NezhaIDE::Utilities::Logger::instance().log(
        NezhaIDE::Utilities::LogLevel::Info, __FILE__, __LINE__, __func__,
        "应用设置: theme={} lang={} llm={}", static_cast<int>(newTheme),
        static_cast<int>(newLang), static_cast<int>(newLlm));

    cfg.set_theme(newTheme);
    cfg.set_language(newLang);
    cfg.set_llm(newLlm);
    cfg.set_llm_api_token(api_token_edit_->text());
    cfg.save();

    if (themeChanged_) {
        NezhaIDE::Services::ThemeService::instance().applyTheme(newTheme);
    }
    if (langChanged_) {
        NezhaIDE::Services::LocalizationService::instance().switchLanguage(newLang);
        QMessageBox::information(this, QString::fromUtf8(
            NezhaIDE::Constants::ApplicationName.data(),
            static_cast<int>(NezhaIDE::Constants::ApplicationName.size())),
            LOC("prefs.restart_hint"));
    }
}

} // namespace NezhaIDE::Views
