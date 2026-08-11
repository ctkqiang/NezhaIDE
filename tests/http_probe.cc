#include "src/services/http.h"
#include "src/model/http_request.h"
#include <QCoreApplication>
#include <QTimer>
#include <cstdio>

using namespace NezhaIDE::Model::HTTP;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    HttpRequest req;
    req.id = 42;
    req.method = HttpMethod::Get;
    req.url = argc > 1 ? argv[1] : "https://www.baidu.com";

    QObject::connect(&NezhaIDE::Services::HTTP::HttpClientService::instance(),
        &NezhaIDE::Services::HTTP::HttpClientService::responseReceived,
        [&app](const HttpResponse &resp) {
            std::printf("=== RESPONSE ===\n");
            std::printf("statusCode: %d\n", resp.statusCode);
            std::printf("statusText: %s\n", resp.statusText.c_str());
            std::printf("elapsedMs:  %lld\n", static_cast<long long>(resp.elapsedMs));
            std::printf("headers:    %zu\n", resp.headers.size());
            std::printf("bodySize:   %zu\n", resp.body.size());
            std::printf("bodyPreview: %.*s\n", static_cast<int>(resp.body.size() < 500 ? resp.body.size() : 500), resp.body.c_str());
            app.exit(0);
        });

    QObject::connect(&NezhaIDE::Services::HTTP::HttpClientService::instance(),
        &NezhaIDE::Services::HTTP::HttpClientService::requestError,
        [&app](RequestId id, int statusCode, const QString &error) {
            std::printf("=== ERROR ===\nrequestId: %llu\nstatusCode: %d\nerror: %s\n",
                static_cast<unsigned long long>(id), statusCode, error.toUtf8().constData());
            app.exit(1);
        });

    QTimer::singleShot(20000, [&app] {
        std::printf("=== TIMEOUT: no response in 20s ===\n");
        app.exit(2);
    });

    NezhaIDE::Services::HTTP::HttpClientService::send(req);
    return app.exec();
}
