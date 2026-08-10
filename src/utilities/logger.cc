//
// Created by 钟智强 on 2026/8/10.
//

#include "logger.h"
#include <chrono>
#include <iostream>

#include "../configuration.h"

namespace NezhaIDE::Utilities {

    Logger &Logger::instance() {
        static Logger inst;
        return inst;
    }

    Logger::~Logger() {
        if (file_.is_open()) {
            file_.close();
        }
    }

    void Logger::set_level(const LogLevel level) {
        std::lock_guard lock(mutex_);
        minimum_level_ = level;
    }

    void Logger::set_log_file(const std::string_view path) {
        std::lock_guard lock(mutex_);
        if (file_.is_open()) {
            file_.close();
        }
        file_.open(path.data(), std::ios::out | std::ios::app);
        file_enabled_ = file_.is_open();
    }

    void Logger::enable_console(const bool enabled) {
        std::lock_guard lock(mutex_);
        console_enabled_ = enabled;
    }

    void Logger::enable_file(const bool enabled) {
        std::lock_guard lock(mutex_);
        file_enabled_ = enabled;
    }

    LogLevel Logger::level() const noexcept {
        std::lock_guard lock(mutex_);
        return minimum_level_;
    }

    bool Logger::should_log(const LogLevel level) const noexcept {
        return level >= minimum_level_;
    }

    void Logger::write(
        const LogLevel level,
        const std::string_view file,
        const int line,
        const std::string_view function,
        const std::string_view message
    ) {
        std::lock_guard lock(mutex_);

        const auto entry = std::format(
            "[{}] [{}] {}:{} ({}) - {}",
            timestamp(),
            level_name(level),
            file,
            line,
            function,
            message
        );

        if (console_enabled_) {
            auto &stream = (level >= LogLevel::Error)
                ? std::cerr
                : std::cout;
            stream << entry << std::endl;
        }

        if (file_enabled_ && file_.is_open()) {
            file_ << entry << std::endl;
            file_.flush();
        }
    }

    std::string Logger::level_name(const LogLevel level) {
        switch (level) {
            case LogLevel::Debug:   return "DEBUG";
            case LogLevel::Info:    return "INFO";
            case LogLevel::Warn:    return "WARN";
            case LogLevel::Error:   return "ERROR";
            case LogLevel::WTF:     return "WTF";
            case LogLevel::Verbose: return "VERBOSE";
        }
        return "UNKNOWN";
    }

    std::string Logger::timestamp() {
        const auto now = std::chrono::system_clock::now();
        const auto time = std::chrono::floor<std::chrono::seconds>(now);
        return std::format("[{}] {:%Y-%m-%d %H:%M:%S}", NezhaIDE::Constants::ApplicationName, time);
    }
}
