//
// Created by 钟智强 on 2026/8/10.
//
#pragma once

#ifndef NEZHAIDE_USER_PREFERENCE_H
#define NEZHAIDE_USER_PREFERENCE_H

#include "src/model/column_def.h"
#include "src/configuration.h"
#include <array>
#include <string>
#include <string_view>

namespace NezhaIDE::Model {

    struct UserPreference {
        int id{0};

        [[ maybe_unused ]]
        std::string user_name;

        IDETheme theme{IDETheme::Auto};
        IDELanguage language{IDELanguage::Chinese};
        IDELLM llm{IDELLM::DeepSeek};
        std::string llm_api_token;

        static consteval std::string_view table_name() {
            return "user_preferences";
        }

        static consteval auto columns() {
            return std::array{
                ColumnDef{"id", "INTEGER", true, true},
                ColumnDef{"user_name", "TEXT", false, false},
                ColumnDef{"theme", "INTEGER", false, false},
                ColumnDef{"language", "INTEGER", false, false},
                ColumnDef{"llm", "INTEGER", false, false},
                ColumnDef{"llm_api_token", "TEXT", false, false},
            };
        }

        template<typename Query>
        void bind_values(Query& query, bool for_insert) const {
            if (for_insert) {
                query.Bind(1, user_name);
                query.Bind(2, static_cast<std::int64_t>(theme));
                query.Bind(3, static_cast<std::int64_t>(language));
                query.Bind(4, static_cast<std::int64_t>(llm));
                query.Bind(5, llm_api_token);
            } else {
                query.Bind(1, user_name);
                query.Bind(2, static_cast<std::int64_t>(theme));
                query.Bind(3, static_cast<std::int64_t>(language));
                query.Bind(4, static_cast<std::int64_t>(llm));
                query.Bind(5, llm_api_token);
                query.Bind(6, static_cast<std::int64_t>(id));
            }
        }

        template<typename Query>
        static UserPreference from_query(Query& query) {
            UserPreference pref;
            pref.id = static_cast<int>(query.ColumnInt64(0));
            pref.user_name = query.ColumnText(1);
            pref.theme = static_cast<IDETheme>(query.ColumnInt64(2));
            pref.language = static_cast<IDELanguage>(query.ColumnInt64(3));
            pref.llm = static_cast<IDELLM>(query.ColumnInt64(4));
            pref.llm_api_token = query.ColumnText(5);
            return pref;
        }
    };
}

#endif //NEZHAIDE_USER_PREFERENCE_H