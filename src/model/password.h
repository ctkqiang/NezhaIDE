//
// Created by 钟智强 on 2026/8/12.
//
#pragma once

#ifndef NEZHAIDE_PASSWORD_H
#define NEZHAIDE_PASSWORD_H

#include "src/model/column_def.h"
#include <array>
#include <string>
#include <string_view>

namespace NezhaIDE::Model {

/**
 * RockYou 密码列表条目，单列文本导入 SQLite。
 *
 * 与其它 ORM 模型一致：元信息走列定义，内容按行存储。
 * 密码明文仅本地保存，不进入日志。
 */
struct RockYou final {
    std::int64_t id{0};
    std::string password;

    static consteval std::string_view table_name() {
        return "rockyou";
    }

    static consteval auto columns() {
        return std::array{
            ColumnDef{"id", "INTEGER", true, true},
            ColumnDef{"password", "TEXT", false, false},
        };
    }

    template<typename Query>
    void bind_values(Query &query, const bool for_insert) const {
        if (for_insert) {
            query.Bind(1, password);
        } else {
            query.Bind(1, password);
            query.Bind(2, static_cast<std::int64_t>(id));
        }
    }

    template<typename Query>
    static RockYou from_query(Query &query) {
        RockYou r;
        r.id = static_cast<std::int64_t>(query.ColumnInt64(0));
        r.password = query.ColumnText(1);
        return r;
    }
};

}

#endif //NEZHAIDE_PASSWORD_H
