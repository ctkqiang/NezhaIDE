//
// Created by 钟智强 on 2026/8/12.
//

#include "downloader.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTemporaryFile>
#include <QUrl>

namespace NezhaIDE::Utilities {

Downloader &Downloader::instance() {
    static Downloader instance;
    return instance;
}

Downloader::Downloader() : nam_(new QNetworkAccessManager(this)) {}

void Downloader::download(const DownloadConfig &config) {
    QNetworkRequest request(QUrl(QString::fromStdString(config.url)));
    auto *reply = nam_->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, config] {
        reply->deleteLater();
        QString error;
        if (reply->error() != QNetworkReply::NoError) {
            error = reply->errorString();
        }

        QString filePath;
        if (error.isEmpty()) {
            if (config.isTemp) {
                QTemporaryFile tmp;
                tmp.setAutoRemove(false);
                if (tmp.open()) {
                    tmp.write(reply->readAll());
                    tmp.flush();
                    filePath = tmp.fileName();
                } else {
                    error = tmp.errorString();
                }
            } else {
                filePath = QString::fromStdString(config.outputfilePath);
                QDir().mkpath(QFileInfo(filePath).absolutePath());
                QFile file(filePath);
                if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                    file.write(reply->readAll());
                    file.close();
                } else {
                    error = file.errorString();
                }
            }
        }

        emit downloadFinished(config.id, error.isEmpty(), error, filePath);
    });
}

} // namespace NezhaIDE::Utilities
