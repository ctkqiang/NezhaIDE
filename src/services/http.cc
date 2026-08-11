#if __has_include("http.h")
    #include "http.h"
    #define __HAS_HTTP 1
#endif

#include <QFile>
#include <QUrl>
#include <QUrlQuery>

namespace NezhaIDE::Services::HTTP {

HttpClientService &HttpClientService::instance()
{
    static HttpClientService svc;
    return svc;
}

HttpClientService::HttpClientService()
    : QObject(nullptr)
{
    nam_ = new QNetworkAccessManager(this);
}

void HttpClientService::send(const Model::HTTP::HttpRequest &req)
{
    auto *request = new QNetworkRequest(buildRequest(req));
    const auto body = buildBody(req.body);
    QNetworkReply *reply = nullptr;

    switch (req.method) {
    case Model::HTTP::HttpMethod::Get:
        reply = nam_->get(*request);
        break;
    case Model::HTTP::HttpMethod::Post:
        reply = nam_->post(*request, body);
        break;
    case Model::HTTP::HttpMethod::Put:
        reply = nam_->put(*request, body);
        break;
    case Model::HTTP::HttpMethod::Patch:
        reply = nam_->sendCustomRequest(*request, "PATCH", body);
        break;
    case Model::HTTP::HttpMethod::Delete:
        reply = nam_->deleteResource(*request);
        break;
    case Model::HTTP::HttpMethod::Head:
        reply = nam_->head(*request);
        break;
    case Model::HTTP::HttpMethod::Options:
        reply = nam_->sendCustomRequest(*request, "OPTIONS");
        break;
    }

    delete request;

    if (!reply) {
        emit requestError(req.id, QStringLiteral("Failed to create network request"));
        return;
    }

    const auto requestId = req.id;
    auto *timer = new QElapsedTimer();
    timer->start();

    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId, timer] {
        timer->invalidate();

        Model::HTTP::HttpResponse resp;
        resp.requestId = requestId;
        resp.elapsedMs = timer->elapsed();
        resp.statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        resp.statusText = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString().toStdString();

        for (const auto &header : reply->rawHeaderPairs()) {
            Model::HTTP::HttpHeader h;
            h.name = header.first.toStdString();
            h.value = header.second.toStdString();
            resp.headers.push_back(std::move(h));
        }

        resp.body = reply->readAll().toStdString();
        emit responseReceived(resp);

        delete timer;
        reply->deleteLater();
    });
}

void HttpClientService::cancel(Model::HTTP::RequestId id)
{
    Q_UNUSED(id);
}

QNetworkRequest HttpClientService::buildRequest(const Model::HTTP::HttpRequest &req) const
{
    QUrl url(QString::fromStdString(req.url));
    if (!req.queryParameters.empty()) {
        QUrlQuery query;
        for (const auto &p : req.queryParameters) {
            if (p.enabled) {
                query.addQueryItem(QString::fromStdString(p.name),
                                   QString::fromStdString(p.value));
            }
        }
        url.setQuery(query);
    }

    QNetworkRequest request(url);
    for (const auto &h : req.headers) {
        if (h.enabled) {
            request.setRawHeader(QByteArray::fromStdString(h.name),
                                 QByteArray::fromStdString(h.value));
        }
    }

    return request;
}

QByteArray HttpClientService::buildBody(const Model::HTTP::HttpBody &body) const
{
    switch (body.type) {
    case Model::HTTP::BodyType::Json:
        return QByteArray::fromStdString(body.content);
    case Model::HTTP::BodyType::Raw:
        return QByteArray::fromStdString(body.content);
    case Model::HTTP::BodyType::Xml:
        return QByteArray::fromStdString(body.content);
    case Model::HTTP::BodyType::FormUrlEncoded:
        return QByteArray::fromStdString(body.content);
    case Model::HTTP::BodyType::Multipart:
    case Model::HTTP::BodyType::Binary: {
        if (!body.filePath.empty()) {
            QFile file(QString::fromStdString(body.filePath));
            if (file.open(QIODevice::ReadOnly)) {
                return file.readAll();
            }
        }
        return {};
    }
    case Model::HTTP::BodyType::None:
    default:
        return {};
    }
}

} // namespace NezhaIDE::Services::HTTP
