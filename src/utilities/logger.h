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

namespace NezhaIDE::Utilities {
    enum class LogLevel {
        Info = 0,
        Debug,
        Warn,
        Error,
        WTF,
        Verbose
    };

    class Logger final {
    public:
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
