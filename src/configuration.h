//
// Created by 钟智强 on 2026/8/10.
//

#pragma once

#ifndef NEZHAIDE_CONFIGURATION_H
#define NEZHAIDE_CONFIGURATION_H

#if __has_include(<QSettings>)
    #include <QSettings>
    #define HAS_QSETTINGS 1
#else
    #define HAS_QSETTINGS 0
#endif

#include <array>

namespace NezhaIDE {
    struct AuthorMetadata final {
        std::string_view name;
        std::string_view email;
        std::string_view wechat;
    };

    enum class IDELLM {
        DeepSeek,
        Kimi,
        OpenRouter,
    };

    enum class IDETheme {
        Auto,
        Light,
        Dark,
        GitHub,
        Xcode
    };

    enum class IDELanguage {
        Chinese,
        English
    };

    class Constants {
    public:
        Constants() = delete;

        [[ maybe_unused ]]
        static constexpr std::string_view ApplicationName = "哪吒网络安全IDE";

        [[ maybe_unused ]]
        static constexpr std::string_view ApplicationVersion = "v0.0.1";

        [[ maybe_unused ]]
        static constexpr std::string_view DatabaseName = "nezha_ide.db";

        [[ maybe_unused ]]
        static constexpr std::string_view OrganisationName = "哪吒网络安全";

        [[ maybe_unused ]]
        static constexpr std::string_view FileExtension = ".nzs";

        [[ maybe_unused ]]
        AuthorMetadata Author{
            .name = "钟智强",
            .email = "johnmelodymel@qq.com",
            .wechat = "ctkqiang"
        };

        [[ maybe_unused ]]
        static constexpr std::array<std::string_view, 4> DatabaseTable = {
            "tools",
            "tools_procedures",
            "tool_parameters",
            "user_preferences"
        };
    };

    class Configuration {
    public:
        static Configuration &instance();

        Configuration(const Configuration &) = delete;

        Configuration &operator=(const Configuration &) = delete;

        Configuration(Configuration &&) = delete;

        Configuration &operator=(Configuration &&) = delete;

        void save();

        [[nodiscard]]
        IDETheme theme() const noexcept;

        void set_theme(IDETheme theme);

        [[nodiscard]]
        IDELLM llm() const noexcept;

        void set_llm(IDELLM llm);

        [[nodiscard]]
        IDELanguage language() const noexcept;

        void set_language(IDELanguage language);

        [[clang::annotate("security-sensitive")]]
        void set_user_name(const QString &name);

        [[nodiscard]]
        QString user_name() const;

        [[clang::annotate("security-sensitive")]]
        void set_llm_api_token(const QString &token);

        [[nodiscard]]
        QString llm_api_token() const;

        void clear_memory();

        [[nodiscard]]
        QString project_root() const;

        void set_project_root(const QString &path);

        static void open_documentation();

    private:
        Configuration();

        QSettings settings_{};
    };
}


#endif //NEZHAIDE_CONFIGURATION_H
