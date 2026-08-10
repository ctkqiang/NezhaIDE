//
// Created by 钟智强 on 2026/8/10.
//

#pragma once

#ifndef NEZHAIDE_DATABASE_HELPER_H
#define NEZHAIDE_DATABASE_HELPER_H

#include <expected>
#include <functional>
#include <sqlite3.h>
#include <string>

namespace NezhaIDE::Services {
    class DatabaseHelper {
    public:
        struct SQLQuery final {

        };
    public:
        struct DatabaseError final {
            int StatusCode{SQLITE_OK};
            std::string Message;
            std::string SQL;
        };

        using Result = std::expected<void, DatabaseError>;
        using TransactionCallback = std::function<Result(sqlite3 *)>;

    public:
        explicit DatabaseHelper(std::string explicitDatabasePath);

        ~DatabaseHelper();

        DatabaseHelper(const DatabaseHelper &) = delete;

        DatabaseHelper &operator=(const DatabaseHelper &) = delete;

        DatabaseHelper(DatabaseHelper &&) = delete;

        DatabaseHelper &operator=(DatabaseHelper &&) = delete;

    public:
        [[ nodiscard ]]
        Result initializeDatabase();

        [[ nodiscard ]]
        Result ConnectDatabase();

        [[ maybe_unused ]]
        void DisconnectDatabase() noexcept;

        [[ nodiscard ]]
        bool IsDatabaseConnected() const noexcept;
    };
}


#endif //NEZHAIDE_DATABASE_HELPER_H
