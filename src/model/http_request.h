#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace NezhaIDE::Model::HTTP {

using RequestId = std::uint64_t;
using HeaderId = std::uint64_t;
using ParameterId = std::uint64_t;

enum class HttpMethod {
    Get,
    Post,
    Put,
    Patch,
    Delete,
    Head,
    Options
};

enum class BodyType {
    None,
    Raw,
    Json,
    Xml,
    FormUrlEncoded,
    Multipart,
    Binary
};

struct HttpHeader {
    HeaderId id{};
    std::string name;
    std::string value;
    bool enabled{true};
};

struct HttpParameter {
    ParameterId id{};
    std::string name;
    std::string value;
    bool enabled{true};
};

struct HttpBody {
    BodyType type{BodyType::None};
    std::string content;
    std::string filePath;
};

struct HttpRequest {
    RequestId id{};
    std::string name;
    HttpMethod method{HttpMethod::Get};
    std::string url;
    std::vector<HttpParameter> queryParameters;
    std::vector<HttpHeader> headers;
    HttpBody body;
};

struct HttpResponse {
    RequestId requestId{};
    int statusCode{};
    std::string statusText;
    std::vector<HttpHeader> headers;
    std::string body;
    std::string contentType;
    int64_t elapsedMs{};
};

} // namespace NezhaIDE::Model::HTTP
