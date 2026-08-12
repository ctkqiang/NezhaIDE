//
// Created by 钟智强 on 2026/8/12.
//
#pragma once

#ifndef NEZHAIDE_CREDENTIAL_DATASET_H
#define NEZHAIDE_CREDENTIAL_DATASET_H

#include "src/model/column_def.h"
#include <array>
#include <string>
#include <string_view>

namespace NezhaIDE::Model {

/**
 * 凭据数据集类型：用户名列表或密码列表。
 */
enum class CredentialDatasetType {
    Username,
    Password
};

/**
 * 凭据数据集的来源，用于溯源审计。
 *
 * File 来自本地文件，GitHub 来自用户显式指定的远程列表（视为不可信输入），
 * Generator 由内置随机生成器产生，Custom 为自定义密码文件。
 */
enum class CredentialDatasetSource {
    File,
    GitHub,
    Generator,
    Custom
};

/**
 * 凭据数据集元信息，记录名称、类型、来源溯源与条目统计。
 *
 * 与凭证内容分离存储：内容位于 credential_entries 表，
 * 本表只保存审计所需元数据。
 *
 * @see CredentialEntry
 */
struct CredentialDataset final {
    std::int64_t id{0};
    std::string name;
    std::string dataset_type;
    std::string source;
    std::string file_path;
    std::string imported_at;
    std::int64_t entry_count{0};

    static consteval std::string_view table_name() {
        return "credential_datasets";
    }

    static consteval auto columns() {
        return std::array{
            ColumnDef{"id", "INTEGER", true, true},
            ColumnDef{"name", "TEXT", false, false},
            ColumnDef{"dataset_type", "TEXT", false, false},
            ColumnDef{"source", "TEXT", false, false},
            ColumnDef{"file_path", "TEXT", false, false},
            ColumnDef{"imported_at", "TEXT", false, false},
            ColumnDef{"entry_count", "INTEGER", false, false},
        };
    }

    template<typename Query>
    void bind_values(Query &query, const bool for_insert) const {
        if (for_insert) {
            query.Bind(1, name);
            query.Bind(2, dataset_type);
            query.Bind(3, source);
            query.Bind(4, file_path);
            query.Bind(5, imported_at);
            query.Bind(6, static_cast<std::int64_t>(entry_count));
        } else {
            query.Bind(1, name);
            query.Bind(2, dataset_type);
            query.Bind(3, source);
            query.Bind(4, file_path);
            query.Bind(5, imported_at);
            query.Bind(6, static_cast<std::int64_t>(entry_count));
            query.Bind(7, static_cast<std::int64_t>(id));
        }
    }

    template<typename Query>
    static CredentialDataset from_query(Query &query) {
        CredentialDataset d;
        d.id = static_cast<std::int64_t>(query.ColumnInt64(0));
        d.name = query.ColumnText(1);
        d.dataset_type = query.ColumnText(2);
        d.source = query.ColumnText(3);
        d.file_path = query.ColumnText(4);
        d.imported_at = query.ColumnText(5);
        d.entry_count = static_cast<std::int64_t>(query.ColumnInt64(6));
        return d;
    }
};

/**
 * 凭据数据集单条内容，通过 dataset_id 关联所属数据集。
 *
 * value 为明文条目（用户名或密码），line_number 记录在源文件中的行号。
 */
struct CredentialEntry final {
    std::int64_t id{0};
    std::int64_t dataset_id{0};
    std::string value;
    std::int64_t line_number{0};

    static consteval std::string_view table_name() {
        return "credential_entries";
    }

    static consteval auto columns() {
        return std::array{
            ColumnDef{"id", "INTEGER", true, true},
            ColumnDef{"dataset_id", "INTEGER", false, false},
            ColumnDef{"value", "TEXT", false, false},
            ColumnDef{"line_number", "INTEGER", false, false},
        };
    }

    template<typename Query>
    void bind_values(Query &query, const bool for_insert) const {
        if (for_insert) {
            query.Bind(1, static_cast<std::int64_t>(dataset_id));
            query.Bind(2, value);
            query.Bind(3, static_cast<std::int64_t>(line_number));
        } else {
            query.Bind(1, static_cast<std::int64_t>(dataset_id));
            query.Bind(2, value);
            query.Bind(3, static_cast<std::int64_t>(line_number));
            query.Bind(4, static_cast<std::int64_t>(id));
        }
    }

    template<typename Query>
    static CredentialEntry from_query(Query &query) {
        CredentialEntry e;
        e.id = static_cast<std::int64_t>(query.ColumnInt64(0));
        e.dataset_id = static_cast<std::int64_t>(query.ColumnInt64(1));
        e.value = query.ColumnText(2);
        e.line_number = static_cast<std::int64_t>(query.ColumnInt64(3));
        return e;
    }
};

} // namespace NezhaIDE::Model

#endif // NEZHAIDE_CREDENTIAL_DATASET_H
