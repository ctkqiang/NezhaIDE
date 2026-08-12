#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QElapsedTimer>
#include <QHash>
#include "src/model/http_request.h"

/**
 * HTTP 客户端服务命名空间。
 */
namespace NezhaIDE::Services::HTTP {

/**
 * HTTP 请求发送与响应接收服务单例。
 *
 * 封装 QNetworkAccessManager，支持 GET/POST/PUT/PATCH/DELETE/HEAD/OPTIONS
 * 七种 HTTP 方法。所有请求异步执行，通过 Qt 信号返回结果。
 * 不支持 HTTP/2，所有请求强制使用 HTTP/1.1。
 */
class HttpClientService : public QObject {
    Q_OBJECT

public:
    static HttpClientService &instance();

    HttpClientService(const HttpClientService &) = delete;
    HttpClientService &operator=(const HttpClientService &) = delete;

    /**
     * 异步发送 HTTP 请求。
     *
     * @param req 完整的 HTTP 请求结构体，包含方法、URL、headers、body。
     *
     * @note 结果通过 responseReceived 或 requestError 信号异步返回。
     */
    static void send(const Model::HTTP::HttpRequest &req);

    /**
     * 取消指定 ID 的进行中请求。
     *
     * @param id 请求 ID，由调用方在 HttpRequest::id 中指定。
     */
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
