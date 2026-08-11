#pragma once

#include <cstdint>
#include <string>
#include <vector>

/**
 * HTTP 请求/响应数据模型命名空间。
 */
namespace NezhaIDE::Model::HTTP {

using RequestId = std::uint64_t;
using HeaderId = std::uint64_t;
using ParameterId = std::uint64_t;

/**
 * HTTP 请求方法枚举。
 */
enum class HttpMethod {
    Get,
    Post,
    Put,
    Patch,
    Delete,
    Head,
    Options
};

/**
 * HTTP 请求 Body 类型枚举。
 */
enum class BodyType {
    None,
    Raw,
    Json,
    Xml,
    FormUrlEncoded,
    Multipart,
    Binary
};

/**
 * HTTP Header 键值对，可独立启用/禁用。
 */
struct HttpHeader {
    HeaderId id{};
    std::string name;
    std::string value;
    bool enabled{true};
};

/**
 * HTTP 查询/表单参数，可独立启用/禁用。
 */
struct HttpParameter {
    ParameterId id{};
    std::string name;
    std::string value;
    bool enabled{true};
};

/**
 * HTTP 请求 Body 定义。
 */
struct HttpBody {
    BodyType type{BodyType::None};
    std::string content;
    std::string filePath;
};

/**
 * 完整的 HTTP 请求描述，包含方法、URL、headers、query 参数和 body。
 */
struct HttpRequest {
    RequestId id{};
    std::string name;
    HttpMethod method{HttpMethod::Get};
    std::string url;
    std::vector<HttpParameter> queryParameters;
    std::vector<HttpHeader> headers;
    HttpBody body;
};

/**
 * HTTP 响应描述，由 HttpClientService 填充并发出。
 */
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
