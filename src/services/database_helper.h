//
// Created by 钟智强 on 2026/8/10.
//

#pragma once

#ifndef NEZHAIDE_DATABASE_HELPER_H
#define NEZHAIDE_DATABASE_HELPER_H

#include <expected>
#include <functional>
#include <string>
#include <string_view>

#if __has_include(<sqlite3.h>)
    #include <sqlite3.h>

    #define HAS_SQLITE3 1

#else
    #define HAS_SQLITE3 0
    #error "缺少 SQLite3 库，请先安装 libsqlite3-dev。"
#endif

namespace NezhaIDE::Services {

class DatabaseHelper final {
public:
    struct SQLQuery final {
    public:
        SQLQuery(
            sqlite3 *database,
            sqlite3_stmt *statement,
            std::string sql
        );

        ~SQLQuery();

        SQLQuery(const SQLQuery &) = delete;

        SQLQuery &operator=(const SQLQuery &) = delete;

        SQLQuery(SQLQuery &&other) noexcept;

        SQLQuery &operator=(SQLQuery &&other) noexcept;

    public:
        [[ nodiscard ]]
        bool IsValid() const noexcept;

        [[ nodiscard ]]
        std::expected<void, int> Bind(
            int index,
            std::string_view value
        ) const;

        [[ nodiscard ]]
        std::expected<void, int> Bind(
            int index,
            const char *value
        ) const;

        [[ nodiscard ]]
        std::expected<void, int> Bind(
            int index,
            std::int64_t value
        ) const;

        [[ nodiscard ]]
        std::expected<void, int> Bind(
            int index,
            double value
        ) const;

        [[ nodiscard ]]
        std::expected<void, int> Bind(
            int index,
            bool value
        ) const;

        [[ nodiscard ]]
        std::expected<void, int> BindNull(
            int index
        ) const;

        [[ nodiscard ]]
        std::expected<bool, int> Step() const;

        [[ nodiscard ]]
        std::expected<void, int> Execute() const;

        [[ nodiscard ]]
        int ColumnCount() const noexcept;

        [[ nodiscard ]]
        std::string ColumnText(
            int column
        ) const;

        [[ nodiscard ]]
        std::int64_t ColumnInt64(
            int column
        ) const;

        [[ nodiscard ]]
        double ColumnDouble(
            int column
        ) const;

        [[ nodiscard ]]
        bool ColumnBool(
            int column
        ) const;

        [[ nodiscard ]]
        bool ColumnIsNull(
            int column
        ) const noexcept;

        [[ nodiscard ]]
        sqlite3_stmt *Handle() const noexcept;

    private:
        sqlite3 *Database{nullptr};

        sqlite3_stmt *Statement{nullptr};

        std::string SQL;
    };

public:
    struct DatabaseError final {
        int StatusCode{SQLITE_OK};

        std::string Message;

        std::string SQL;
    };

    using Result = std::expected<void, DatabaseError>;

    using TransactionCallback =
        std::function<Result(sqlite3 *)>;

public:
    explicit DatabaseHelper(
        std::string explicitDatabasePath
    );

    ~DatabaseHelper();

    DatabaseHelper(const DatabaseHelper &) = delete;

    DatabaseHelper &operator=(
        const DatabaseHelper &
    ) = delete;

    DatabaseHelper(DatabaseHelper &&) = delete;

    DatabaseHelper &operator=(
        DatabaseHelper &&
    ) = delete;

public:
    [[ nodiscard ]]
    Result initializeDatabase();

    [[ nodiscard ]]
    Result ConnectDatabase();

    [[ nodiscard ]]
    std::expected<SQLQuery, DatabaseError> Prepare(
        std::string_view SQL
    ) const;

    [[ nodiscard ]]
    Result Execute(
        std::string_view SQL
    );

    [[ nodiscard ]]
    Result Transaction(
        const TransactionCallback &Callback
    );

    [[ maybe_unused ]]
    void DisconnectDatabase() noexcept;

    [[ nodiscard ]]
    bool IsDatabaseConnected() const noexcept;

private:
    [[ nodiscard ]]
    Result ConfigureDatabase();

    [[ nodiscard ]]
    Result EnableForeignKeys();

    [[ nodiscard ]]
    Result EnableWAL();

    [[ nodiscard ]]
    DatabaseError CreateDatabaseError(
        int StatusCode,
        std::string_view SQL = {}
    ) const;

private:
    std::string ExplicitDatabasePath;

    sqlite3 *Database{nullptr};
};

} // namespace NezhaIDE::Services

#endif //NEZHAIDE_DATABASE_HELPER_H

