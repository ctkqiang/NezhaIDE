//
// Created by 钟智强 on 2026/8/11.
//

#pragma once

#ifndef NEZHAIDE_HTTP_H
#define NEZHAIDE_HTTP_H

#if __has_include(<vector>)
    #include <string>
    #include <vector>

    #define __HAS_VECTOR 1
#else
    $define __HAS_VECTOR 0
#endif

namespace NezhaIDE::Services::HTTP {
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
    }

#endif //NEZHAIDE_HTTP_H
