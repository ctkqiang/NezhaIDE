//
// Created by 钟智强 on 2026/8/10.
//

#include "user_preference_repository.h"

namespace NezhaIDE::Repository {

    UserPreferenceRepository::UserPreferenceRepository(Services::DatabaseHelper& db)
        : db_(db) {
    }

    UserPreferenceRepository::Result UserPreferenceRepository::initialize_schema() {
        return db_.Execute(schema_sql());
    }

    std::expected<UserPreference, UserPreferenceRepository::DatabaseError>
    UserPreferenceRepository::load(const int id) {
        auto queryResult = db_.Prepare("SELECT id, user_name, theme, language, llm, llm_api_token "
                                       "FROM user_preferences WHERE id = ?;");
        if (!queryResult.has_value()) {
            return std::unexpected(queryResult.error());
        }
        auto& query = queryResult.value();
        auto bindResult = query.Bind(1, static_cast<std::int64_t>(id));
        if (!bindResult.has_value()) {
            return std::unexpected(DatabaseError{bindResult.error(), {}, {}});
        }
        auto stepResult = query.Step();
        if (!stepResult.has_value()) {
            return std::unexpected(DatabaseError{stepResult.error(), {}, {}});
        }
        if (!stepResult.value()) {
            return std::unexpected(DatabaseError{SQLITE_DONE, "未找到记录", {}});
        }
        UserPreference pref;
        pref.id = static_cast<int>(query.ColumnInt64(0));
        pref.user_name = query.ColumnText(1);
        pref.theme = static_cast<IDETheme>(query.ColumnInt64(2));
        pref.language = static_cast<IDELanguage>(query.ColumnInt64(3));
        pref.llm = static_cast<IDELLM>(query.ColumnInt64(4));
        pref.llm_api_token = query.ColumnText(5);
        return pref;
    }

    UserPreferenceRepository::Result UserPreferenceRepository::save(const Model::UserPreference& pref) {
        if (pref.id == 0) {
            auto queryResult = db_.Prepare(
                "INSERT INTO user_preferences (user_name, theme, language, llm, llm_api_token) "
                "VALUES (?, ?, ?, ?, ?);");
            if (!queryResult.has_value()) {
                return std::unexpected(queryResult.error());
            }
            auto& query = queryResult.value();
            query.Bind(1, pref.user_name);
            query.Bind(2, static_cast<std::int64_t>(pref.theme));
            query.Bind(3, static_cast<std::int64_t>(pref.language));
            query.Bind(4, static_cast<std::int64_t>(pref.llm));
            query.Bind(5, pref.llm_api_token);
            return query.Execute();
        }
        auto queryResult = db_.Prepare(
            "UPDATE user_preferences SET user_name=?, theme=?, language=?, llm=?, llm_api_token=? "
            "WHERE id=?;");
        if (!queryResult.has_value()) {
            return std::unexpected(queryResult.error());
        }
        auto& query = queryResult.value();
        query.Bind(1, pref.user_name);
        query.Bind(2, static_cast<std::int64_t>(pref.theme));
        query.Bind(3, static_cast<std::int64_t>(pref.language));
        query.Bind(4, static_cast<std::int64_t>(pref.llm));
        query.Bind(5, pref.llm_api_token);
        query.Bind(6, static_cast<std::int64_t>(pref.id));
        return query.Execute();
    }

    UserPreferenceRepository::Result UserPreferenceRepository::remove(const int id) {
        auto queryResult = db_.Prepare("DELETE FROM user_preferences WHERE id = ?;");
        if (!queryResult.has_value()) {
            return std::unexpected(queryResult.error());
        }
        auto& query = queryResult.value();
        query.Bind(1, static_cast<std::int64_t>(id));
        return query.Execute();
    }
}
