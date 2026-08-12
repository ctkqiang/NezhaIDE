//
// Created by 钟智强 on 2026/8/12.
//
#pragma once

#ifndef NEZHAIDE_DOWNLOADER_H
#define NEZHAIDE_DOWNLOADER_H

#include <string>

namespace NezhaIDE::Utilities {
    enum class FileFormat {
        CSV,
        TXT,
        ZIP,
        SQL
    };

    struct DownloadConfig {
        int id{0};

        bool isTemp;

        std::string url;
        std::string fileName;
        std::string outputfilePath;
    };

    class Downloader {
        static void download(const DownloadConfig& config);
        static void crawl(const DownloadConfig& config);
    };
}

#endif //NEZHAIDE_DOWNLOADER_H
