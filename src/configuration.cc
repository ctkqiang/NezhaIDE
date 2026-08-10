//
// Created by 钟智强 on 2026/8/10.
//

#include "configuration.h"

namespace NezhaIDE {
    Configuration::Configuration()
        : settings_(QString::fromUtf8(Constants::OrganisationName.data(),
                                      static_cast<qsizetype>(Constants::OrganisationName.size())),
                    QString::fromUtf8(Constants::ApplicationName.data(),
                                      static_cast<qsizetype>(Constants::ApplicationName.size()))) {
    }

    Configuration &Configuration::instance() {
        static Configuration inst;
        return inst;
    }

    void Configuration::save() {
        settings_.sync();
    }

    IDETheme Configuration::theme() const noexcept {
        return static_cast<IDETheme>(
            settings_.value(QStringLiteral("theme"), static_cast<int>(IDETheme::Auto)).toInt());
    }

    void Configuration::set_theme(const IDETheme theme) {
        settings_.setValue(QStringLiteral("theme"), static_cast<int>(theme));
    }

    IDELLM Configuration::llm() const noexcept {
        return static_cast<IDELLM>(
            settings_.value(QStringLiteral("llm"), static_cast<int>(IDELLM::DeepSeek)).toInt());
    }

    void Configuration::set_llm(const IDELLM llm) {
        settings_.setValue(QStringLiteral("llm"), static_cast<int>(llm));
    }

    IDELanguage Configuration::language() const noexcept {
        return static_cast<IDELanguage>(
            settings_.value(QStringLiteral("language"), static_cast<int>(IDELanguage::Chinese)).toInt());
    }

    void Configuration::set_language(const IDELanguage language) {
        settings_.setValue(QStringLiteral("language"), static_cast<int>(language));
    }

    void Configuration::set_user_name(const QString &name) {
        settings_.setValue(QStringLiteral("user/name"), name);
    }

    QString Configuration::user_name() const {
        return settings_.value(QStringLiteral("user/name")).toString();
    }

    void Configuration::set_llm_api_token(const QString &token) {
        settings_.setValue(QStringLiteral("llm/token"), token);
    }

    QString Configuration::llm_api_token() const {
        return settings_.value(QStringLiteral("llm/token")).toString();
    }

    void Configuration::clear_memory() {
        settings_.remove(QStringLiteral("llm/token"));
        settings_.remove(QStringLiteral("user/name"));
        settings_.sync();
    }

    void Configuration::open_documentation() {
    }
}
