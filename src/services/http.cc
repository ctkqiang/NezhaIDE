#include "http.h"
#include "src/configuration.h"
#include <QFile>
#include <QUrl>
#include <QUrlQuery>
#include <atomic>

namespace NezhaIDE::Services::HTTP {

namespace {

constexpr int kTransferTimeoutMs = 30'000;

Model::HTTP::RequestId nextRequestId()
{
    static std::atomic<Model::HTTP::RequestId> next{1};
    return next.fetch_add(1);
}

}

HttpClientService &HttpClientService::instance()
{
    static HttpClientService svc;
    return svc;
}

HttpClientService::HttpClientService()
    : QObject(nullptr)
{
    nam_ = new QNetworkAccessManager(this);
    nam_->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    nam_->setTransferTimeout(kTransferTimeoutMs);
}

void HttpClientService::send(const Model::HTTP::HttpRequest &req)
{
    auto &self = instance();

    QNetworkRequest request = buildRequest(req);
    const QByteArray body = buildBody(req.body);

    QNetworkReply *reply = nullptr;
    switch (req.method) {
        case Model::HTTP::HttpMethod::Get:
            reply = self.nam_->get(request);
            break;
        case Model::HTTP::HttpMethod::Post:
            reply = self.nam_->post(request, body);
            break;
        case Model::HTTP::HttpMethod::Put:
            reply = self.nam_->put(request, body);
            break;
        case Model::HTTP::HttpMethod::Patch:
            reply = self.nam_->sendCustomRequest(request, "PATCH", body);
            break;
        case Model::HTTP::HttpMethod::Delete:
            reply = self.nam_->deleteResource(request);
            break;
        case Model::HTTP::HttpMethod::Head:
            reply = self.nam_->head(request);
            break;
        case Model::HTTP::HttpMethod::Options:
            reply = self.nam_->sendCustomRequest(request, "OPTIONS");
            break;
    }

    if (!reply) {
        emit self.requestError(req.id, 0, QStringLiteral("Failed to create network request"));
        return;
    }

    auto *timer = new QElapsedTimer();
    timer->start();
    self.replies_.insert(req.id, reply);

    connect(reply, &QNetworkReply::finished, &self, [&self, reply, requestId = req.id, timer] {
        self.replies_.remove(requestId);

        if (reply->error() == QNetworkReply::OperationCanceledError) {
            delete timer;
            reply->deleteLater();
            return;
        }

        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (reply->error() != QNetworkReply::NoError) {
            auto message = reply->errorString();
            if (message.isEmpty()) {
                message = QStringLiteral("Network error (%1)")
                    .arg(static_cast<int>(reply->error()));
            }
            emit self.requestError(requestId, statusCode, message);
            delete timer;
            reply->deleteLater();
            return;
        }

        Model::HTTP::HttpResponse resp;
        resp.requestId = requestId;
        resp.elapsedMs = timer->elapsed();
        resp.statusCode = statusCode;
        resp.statusText = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute)
                              .toString()
                              .toStdString();

        const auto rawHeaders = reply->rawHeaderPairs();
        resp.headers.reserve(static_cast<size_t>(rawHeaders.size()));
        for (const auto &header : rawHeaders) {
            Model::HTTP::HttpHeader h;
            h.name = header.first.toStdString();
            h.value = header.second.toStdString();
            if (header.first.compare("Content-Type", Qt::CaseInsensitive) == 0) {
                resp.contentType = header.second.toStdString();
            }
            resp.headers.push_back(std::move(h));
        }

        const auto bytes = reply->readAll();
        resp.body.assign(bytes.constData(), static_cast<size_t>(bytes.size()));

        emit self.responseReceived(resp);

        delete timer;
        reply->deleteLater();
    });
}

void HttpClientService::cancel(Model::HTTP::RequestId id)
{
    auto &self = instance();
    if (auto it = self.replies_.find(id); it != self.replies_.end()) {
        auto *reply = it.value();
        self.replies_.erase(it);
        reply->abort();
    }
}

QNetworkRequest HttpClientService::buildRequest(const Model::HTTP::HttpRequest &req)
{
    QUrl url(QString::fromStdString(req.url));
    if (!req.queryParameters.empty()) {
        QUrlQuery query;
        for (const auto &p : req.queryParameters) {
            if (p.enabled && !p.name.empty()) {
                query.addQueryItem(
                    QString::fromStdString(p.name),
                    QString::fromStdString(p.value)
                );
            }
        }
        url.setQuery(query);
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("NezhaIDE/%1")
        .arg(QString::fromUtf8(
            NezhaIDE::Constants::ApplicationVersion.data(),
            static_cast<int>(NezhaIDE::Constants::ApplicationVersion.size()))));

    for (const auto &h : req.headers) {
        if (h.enabled && !h.name.empty()) {
            request.setRawHeader(
                QByteArray::fromStdString(h.name),
                QByteArray::fromStdString(h.value)
            );
        }
    }

    return request;
}

QByteArray HttpClientService::buildBody(const Model::HTTP::HttpBody &body)
{
    switch (body.type) {
        case Model::HTTP::BodyType::Json:
        case Model::HTTP::BodyType::Raw:
        case Model::HTTP::BodyType::Xml:
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

}
