//
// Created by 钟智强 on 2026/8/10.
//
//
#pragma once

#if __has_include("src/model/http_request.h") && \
     __has_include(<QObject>) && \
       __has_include(<QNetworkAccessManager>) && \
       __has_include(<QNetworkReply>) && \
       __has_include(<QElapsedTimer>)

    #include "src/model/http_request.h"
    #include <QObject>
    #include <QNetworkAccessManager>
    #include <QNetworkReply>
    #include <QElapsedTimer>

    #define __HAS_QOBJECT 1
    #define __HAS_HTTP_REQUEST_OOBJ 1
#else
    #define __HAS_QOBJECT 0
    #define __HAS_HTTP_REQUEST_OOBJ 0

    #error "NezhaIDE::Services::HTTP 缺少必要的依赖项"
#endif

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
    void requestError(Model::HTTP::RequestId id, const QString &error);

private:
    HttpClientService();
    ~HttpClientService() override = default;

    static QNetworkRequest buildRequest(const Model::HTTP::HttpRequest &req);

    static QByteArray buildBody(const Model::HTTP::HttpBody &body);

    QNetworkAccessManager *nam_{};
};

}
