#pragma once

#include "src/model/http_request.h"
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QElapsedTimer>

namespace NezhaIDE::Services::HTTP {

class HttpClientService : public QObject {
    Q_OBJECT

public:
    static HttpClientService &instance();

    HttpClientService(const HttpClientService &) = delete;
    HttpClientService &operator=(const HttpClientService &) = delete;

    void send(const Model::HTTP::HttpRequest &req);
    void cancel(Model::HTTP::RequestId id);

signals:
    void responseReceived(const Model::HTTP::HttpResponse &resp);
    void requestError(Model::HTTP::RequestId id, const QString &error);

private:
    HttpClientService();
    ~HttpClientService() override = default;

    QNetworkRequest buildRequest(const Model::HTTP::HttpRequest &req) const;
    QByteArray buildBody(const Model::HTTP::HttpBody &body) const;

    QNetworkAccessManager *nam_{};
};

} // namespace NezhaIDE::Services::HTTP
