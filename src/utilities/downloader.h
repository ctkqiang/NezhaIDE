//
// Created by 钟智强 on 2026/8/12.
//
#pragma once

#ifndef NEZHAIDE_DOWNLOADER_H
#define NEZHAIDE_DOWNLOADER_H

#include <QObject>
#include <QString>
#include <string>

class QNetworkAccessManager;

namespace NezhaIDE::Utilities {

    enum class FileFormat {
        CSV,
        TXT,
        ZIP,
        SQL
    };

    struct DownloadConfig {
        int id{0};
        bool isTemp{false};
        std::string url;
        std::string fileName;
        std::string outputfilePath;
    };

    /**
     * HTTP 文件下载单例，异步执行。
     *
     * 下载完成后经 downloadFinished 通知；失败时 error 携带原因。
     * 生命周期为进程级，应用退出时未完成下载自动取消。
     */
    class Downloader final : public QObject {
        Q_OBJECT

    public:
        static Downloader &instance();

        Downloader(const Downloader &) = delete;

        Downloader &operator=(const Downloader &) = delete;

        /**
         * 异步下载 config.url 并保存到 config.outputfilePath。
         */
        void download(const DownloadConfig &config);

    signals:
        void downloadFinished(int id, bool success, const QString &error, const QString &filePath);

    private:
        Downloader();

        QNetworkAccessManager *nam_{};
    };

} // namespace NezhaIDE::Utilities

#endif //NEZHAIDE_DOWNLOADER_H
