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
#error "QSettings 不存在"
#endif

#include <array>

/**
 * 应用配置与常量命名空间。
 *
 * 包含 IDE 主题、语言、LLM 提供商等持久化配置项，
 * 以及应用元数据常量（名称、版本、数据库表名等）。
 */
namespace NezhaIDE {
    /**
     * 作者元数据，用于界面展示和文档生成。
     */
    struct AuthorMetadata final {
        std::string_view name;
        std::string_view email;
        std::string_view wechat;
    };

    /**
     * 支持的 LLM 大模型提供商。
     */
    enum class IDELLM {
        DeepSeek,
        Kimi,
        OpenRouter,
    };

    /**
     * IDE 主题枚举，支持自动跟随系统、浅色、深色及第三方风格。
     */
    enum class IDETheme {
        Auto,
        Light,
        Dark,
        GitHub,
        Xcode
    };

    /**
     * IDE 界面语言。
     */
    enum class IDELanguage {
        Chinese,
        English,
        German
    };

    /**
     * 应用级编译期常量，包括名称、版本号、数据库名、文件扩展名等。
     */
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
        static constexpr std::string_view PasswordList =
                "https://github.com/ctkqiang/NezhaIDE/releases/download/password-list/rockyou.txt";

        [[ maybe_unused ]]
        static constexpr std::array<std::string_view, 6> DatabaseTable = {
            "tools",
            "tools_procedures",
            "tool_parameters",
            "user_preferences",
            "credential_datasets",
            "credential_entries"
        };
    };

    /**
     * 运行时配置单例，基于 QSettings 持久化用户偏好。
     *
     * 管理主题、语言、LLM 提供商、API token、项目根目录等设置。
     * 禁止拷贝和移动，仅通过 instance() 访问。
     */
    class Configuration {
    public:
        /**
         * 获取全局唯一配置实例。
         *
         * @return 单例引用。
         */
        static Configuration &instance();

        Configuration(const Configuration &) = delete;

        Configuration &operator=(const Configuration &) = delete;

        Configuration(Configuration &&) = delete;

        Configuration &operator=(Configuration &&) = delete;

        /**
         * 将当前配置写入持久化存储。
         */
        void save();

        [[nodiscard]]
        IDETheme theme() const noexcept;

        void set_theme(IDETheme theme);

        [[nodiscard]]
        IDELLM llm() const noexcept;

        void set_llm(IDELLM llm);

        [[nodiscard]]
        IDELanguage language() const noexcept;

        /**
         * 设置界面语言并持久化。
         *
         * @param language 目标语言枚举值。
         */
        void set_language(IDELanguage language);

        [[clang::annotate("security-sensitive")]]
        void set_user_name(const QString &name);

        [[nodiscard]]
        QString user_name() const;

        [[clang::annotate("security-sensitive")]]
        void set_llm_api_token(const QString &token);

        [[nodiscard]]
        QString llm_api_token() const;

        /**
         * 清除敏感数据（LLM token、用户名），不删除其他配置。
         */
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
