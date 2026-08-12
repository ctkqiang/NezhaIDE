//
// Created by 钟智强 on 2026/8/10.
//
#pragma once

#ifndef NEZHAIDE_LOGGER_H
#define NEZHAIDE_LOGGER_H
#include <format>
#include <fstream>
#include <mutex>
#include <string_view>

/**
 * 日志与调试工具命名空间。
 */
namespace NezhaIDE::Utilities {

    /**
     * 日志严重级别。
     */
    enum class LogLevel {
        Info = 0,
        Debug,
        Warn,
        Error,
        WTF,
        Verbose
    };

    /**
     * 线程安全的日志记录器单例。
     *
     * 支持控制台和文件双输出通道，模板化格式化消息，
     * 按级别过滤日志输出。
     */
    class Logger final {
    public:
        /**
         * 获取全局日志记录器实例。
         *
         * @return 单例引用。
         */
        static Logger &instance();

        Logger(const Logger &) = delete;

        Logger &operator=(const Logger &) = delete;

        Logger(Logger &&) = delete;

        Logger &operator=(Logger &&) = delete;

        void set_level(LogLevel level);

        void set_log_file(std::string_view path);

        void enable_console(bool enabled);

        void enable_file(bool enabled);

        [[nodiscard]]
        LogLevel level() const noexcept;

        /**
         * 记录一条格式化日志消息。
         *
         * @param level 日志级别，低于 minimum_level_ 的消息将被丢弃
         * @param file 源文件名（通常传 __FILE__）
         * @param line 行号（通常传 __LINE__）
         * @param function 函数名（通常传 __func__）
         * @param format C++26 格式化字符串
         * @param args 格式化参数
         */
        template<typename... Args>
        void log(
            const LogLevel level,
            const std::string_view file,
            const int line,
            const std::string_view function,
            std::format_string<Args...> format,
            Args &&... args
        ) {
            if (!should_log(level)) {
                return;
            }

            const std::string message = std::format(format, std::forward<Args>(args)...);

            write(level, file, line, function, message);
        }

    private:
        Logger() = default;

        ~Logger();

        [[nodiscard]]
        bool should_log(LogLevel level) const noexcept;

        void write(
            LogLevel level,
            std::string_view file,
            int line,
            std::string_view function,
            std::string_view message
        );

        [[nodiscard]]
        static std::string level_name(LogLevel level);

        [[nodiscard]]
        static std::string timestamp();

    private:
        LogLevel minimum_level_ = LogLevel::Info;

        bool console_enabled_ = true;
        bool file_enabled_ = false;

        [[ maybe_unused ]]
        std::ofstream file_;

        mutable std::mutex mutex_;
    };
}


#endif //NEZHAIDE_LOGGER_H
