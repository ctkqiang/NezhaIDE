//
// Created by 钟智强 on 2026/8/10.
//
#pragma once

#ifndef NEZHAIDE_ORM_H
#define NEZHAIDE_ORM_H

#include "src/model/column_def.h"
#include "src/services/database_helper.h"
#include <algorithm>
#include <expected>
#include <ranges>
#include <string>
#include <string_view>

namespace NezhaIDE::Repository {

    template<typename Model>
    class Repository {
    public:
        using Result = Services::DatabaseHelper::Result;
        using DatabaseError = Services::DatabaseHelper::DatabaseError;

        explicit Repository(Services::DatabaseHelper& db) : db_(db) {}

        Result initialize_schema() {
            return db_.Execute(build_create_table_sql());
        }

        Result save(Model& instance) {
            const auto pk_col = primary_key_column();
            if (pk_col == nullptr) {
                return std::unexpected(DatabaseError{SQLITE_ERROR, "模型无主键", {}});
            }
            if (instance.id == 0) {
                return insert(instance);
            }
            return update(instance);
        }

        std::expected<Model, DatabaseError> load(const int id) {
            const auto sql = build_select_sql();
            auto qr = db_.Prepare(sql);
            if (!qr.has_value()) {
                return std::unexpected(qr.error());
            }
            auto& query = qr.value();
            query.Bind(1, static_cast<std::int64_t>(id));
            auto sr = query.Step();
            if (!sr.has_value()) {
                return std::unexpected(DatabaseError{sr.error(), {}, sql});
            }
            if (!sr.value()) {
                return std::unexpected(DatabaseError{SQLITE_DONE, "记录不存在", sql});
            }
            return Model::from_query(query);
        }

        Result remove(const int id) {
            const auto sql = build_delete_sql();
            auto qr = db_.Prepare(sql);
            if (!qr.has_value()) {
                return std::unexpected(qr.error());
            }
            auto& query = qr.value();
            query.Bind(1, static_cast<std::int64_t>(id));
            return query.Execute();
        }

    private:
        Result insert(Model& instance) {
            const auto sql = build_insert_sql();
            auto qr = db_.Prepare(sql);
            if (!qr.has_value()) {
                return std::unexpected(qr.error());
            }
            auto& query = qr.value();
            instance.bind_values(query, true);
            auto er = query.Execute();
            if (!er.has_value()) {
                return std::unexpected(DatabaseError{er.error(), {}, sql});
            }
            instance.id = static_cast<int>(db_.last_insert_rowid());
            return {};
        }

        Result update(const Model& instance) {
            const auto sql = build_update_sql();
            auto qr = db_.Prepare(sql);
            if (!qr.has_value()) {
                return std::unexpected(qr.error());
            }
            auto& query = qr.value();
            instance.bind_values(query, false);
            auto er = query.Execute();
            if (!er.has_value()) {
                return std::unexpected(DatabaseError{er.error(), {}, sql});
            }
            return {};
        }

        static constexpr const ::NezhaIDE::Model::ColumnDef* primary_key_column() {
            const auto cols = Model::columns();
            for (const auto& col : cols) {
                if (col.is_primary) {
                    return &col;
                }
            }
            return nullptr;
        }

        static std::string build_create_table_sql() {
            const auto cols = Model::columns();
            std::string sql = "CREATE TABLE IF NOT EXISTS ";
            sql += Model::table_name();
            sql += " (";
            bool first = true;
            for (const auto& col : cols) {
                if (!first) {
                    sql += ", ";
                }
                first = false;
                sql += col.name;
                sql += " ";
                sql += col.sql_type;
                if (col.is_primary) {
                    sql += " PRIMARY KEY";
                }
                if (col.is_autoincrement) {
                    sql += " AUTOINCREMENT";
                }
            }
            sql += ");";
            return sql;
        }

        static std::string build_insert_sql() {
            const auto cols = Model::columns();
            std::string sql = "INSERT INTO ";
            sql += Model::table_name();
            sql += " (";
            std::string values;
            bool first = true;
            int param_index = 1;
            for (const auto& col : cols) {
                if (col.is_autoincrement) {
                    continue;
                }
                if (!first) {
                    sql += ", ";
                    values += ", ";
                }
                first = false;
                sql += col.name;
                values += "?";
                ++param_index;
            }
            sql += ") VALUES (";
            sql += values;
            sql += ");";
            return sql;
        }

        static std::string build_update_sql() {
            const auto cols = Model::columns();
            const auto* pk = primary_key_column();
            std::string sql = "UPDATE ";
            sql += Model::table_name();
            sql += " SET ";
            bool first = true;
            int param_index = 1;
            for (const auto& col : cols) {
                if (col.is_primary) {
                    continue;
                }
                if (!first) {
                    sql += ", ";
                }
                first = false;
                sql += col.name;
                sql += "=?";
                ++param_index;
            }
            sql += " WHERE ";
            sql += pk != nullptr ? pk->name : "id";
            sql += "=?;";
            return sql;
        }

        static std::string build_select_sql() {
            const auto cols = Model::columns();
            const auto* pk = primary_key_column();
            std::string sql = "SELECT ";
            bool first = true;
            for (const auto& col : cols) {
                if (!first) {
                    sql += ", ";
                }
                first = false;
                sql += col.name;
            }
            sql += " FROM ";
            sql += Model::table_name();
            sql += " WHERE ";
            sql += pk != nullptr ? pk->name : "id";
            sql += " = ?;";
            return sql;
        }

        static std::string build_delete_sql() {
            const auto* pk = primary_key_column();
            std::string sql = "DELETE FROM ";
            sql += Model::table_name();
            sql += " WHERE ";
            sql += pk != nullptr ? pk->name : "id";
            sql += " = ?;";
            return sql;
        }

        Services::DatabaseHelper& db_;
    };
}
#endif //NEZHAIDE_ORM_H