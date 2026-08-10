//
// Created by 钟智强 on 2026/8/10.
//
#pragma once

#ifndef NEZHAIDE_USER_PREFERENCE_REPOSITORY_H
#define NEZHAIDE_USER_PREFERENCE_REPOSITORY_H

#include "src/model/user_preference.h"
#include "src/services/database_helper.h"
#include <expected>
#include <string_view>

namespace NezhaIDE::Repository {

    class UserPreferenceRepository {
    public:
        using Result = NezhaIDE::Services::DatabaseHelper::Result;
        using DatabaseError = NezhaIDE::Services::DatabaseHelper::DatabaseError;

        explicit UserPreferenceRepository(Services::DatabaseHelper& db);

        [[nodiscard]]
        Result initialize_schema();

        [[nodiscard]]
        static consteval std::string_view schema_sql() {
            return R"SQL(
                CREATE TABLE IF NOT EXISTS user_preferences (
                    id          INTEGER PRIMARY KEY AUTOINCREMENT,
                    user_name   TEXT    NOT NULL DEFAULT '',
                    theme       INTEGER NOT NULL DEFAULT 0,
                    language    INTEGER NOT NULL DEFAULT 0,
                    llm         INTEGER NOT NULL DEFAULT 0,
                    llm_api_token TEXT  NOT NULL DEFAULT ''
                );
            )SQL";
        }

        [[nodiscard]]
        std::expected<Model::UserPreference, DatabaseError> load(int id);

        [[nodiscard]]
        Result save(const Model::UserPreference& pref);

        [[nodiscard]]
        Result remove(int id);

    private:
        Services::DatabaseHelper& db_;
    };
}

#endif //NEZHAIDE_USER_PREFERENCE_REPOSITORY_H
