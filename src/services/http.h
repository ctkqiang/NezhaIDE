#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QElapsedTimer>
#include <QHash>
#include "src/model/http_request.h"

namespace NezhaIDE::Services::HTTP {

class HttpClientService : public QObject {
    Q_OBJECT

public:
    static HttpClientService &instance();

    HttpClientService(const HttpClientService &) = delete;
    HttpClientService &operator=(const HttpClientService &) = delete;

    static void send(const Model::HTTP::HttpRequest &req);

    static void cancel(Model::HTTP::RequestId id);

signals:
    void responseReceived(const Model::HTTP::HttpResponse &resp);
    void requestError(Model::HTTP::RequestId id, int statusCode, const QString &error);

private:
    HttpClientService();
    ~HttpClientService() override = default;

    static QNetworkRequest buildRequest(const Model::HTTP::HttpRequest &req);

    static QByteArray buildBody(const Model::HTTP::HttpBody &body);

    QNetworkAccessManager *nam_{};
    QHash<Model::HTTP::RequestId, QNetworkReply *> replies_{};
};

}
