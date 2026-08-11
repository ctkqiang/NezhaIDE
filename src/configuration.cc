//
// Created by 钟智强 on 2026/8/10.
//

#include "configuration.h"
#include "utilities/logger.h"
#include <QDir>

namespace NezhaIDE {

namespace {
    auto& log() { return Utilities::Logger::instance(); }
}
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
        log().log(Utilities::LogLevel::Info, __FILE__, __LINE__, __func__,
            "主题切换: {}", static_cast<int>(theme));
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
        log().log(Utilities::LogLevel::Info, __FILE__, __LINE__, __func__,
            "清除敏感数据");
        settings_.remove(QStringLiteral("llm/token"));
        settings_.remove(QStringLiteral("user/name"));
        settings_.sync();
    }

    QString Configuration::project_root() const {
        return settings_.value(QStringLiteral("project/root"), QDir::currentPath()).toString();
    }

    void Configuration::set_project_root(const QString &path) {
        settings_.setValue(QStringLiteral("project/root"), path);
    }

    void Configuration::open_documentation() {
    }
}
