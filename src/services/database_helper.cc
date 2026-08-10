//
// Created by 钟智强 on 2026/8/10.
//

#include "database_helper.h"
#include <utility>

namespace NezhaIDE::Services {
    DatabaseHelper::SQLQuery::SQLQuery(sqlite3 *const database, sqlite3_stmt *const statement, std::string sql)
        : Database(database)
          , Statement(statement)
          , SQL(std::move(sql)) {
    }

    DatabaseHelper::SQLQuery::~SQLQuery() {
        if (Statement != nullptr) {
            sqlite3_finalize(Statement);
        }
    }

    DatabaseHelper::SQLQuery::SQLQuery(DatabaseHelper::SQLQuery &&other) noexcept
        : Database(other.Database)
          , Statement(other.Statement)
          , SQL(std::move(other.SQL)) {
        other.Statement = nullptr;
        other.Database = nullptr;
    }

    DatabaseHelper::SQLQuery &DatabaseHelper::SQLQuery::operator=(DatabaseHelper::SQLQuery &&other) noexcept {
        if (this != &other) {
            if (Statement != nullptr) {
                sqlite3_finalize(Statement);
            }

            Database = other.Database;
            Statement = other.Statement;
            SQL = std::move(other.SQL);

            other.Statement = nullptr;
            other.Database = nullptr;
        }

        return *this;
    }

    bool DatabaseHelper::SQLQuery::IsValid() const noexcept {
        return Statement != nullptr;
    }

    std::expected<void, int> DatabaseHelper::SQLQuery::Bind(const int index, const std::string_view value) const {
        const auto rc = sqlite3_bind_text(
            Statement, index, value.data(),
            static_cast<int>(value.size()), SQLITE_TRANSIENT
        );

        if (rc != SQLITE_OK) {
            return std::unexpected(rc);
        }

        return {};
    }

    std::expected<void, int> DatabaseHelper::SQLQuery::Bind(const int index, const char *const value) const {
        const auto rc = sqlite3_bind_text(
            Statement,
            index,
            value,
            -1,
            SQLITE_TRANSIENT
        );

        if (rc != SQLITE_OK) {
            return std::unexpected(rc);
        }

        return {};
    }

    std::expected<void, int> DatabaseHelper::SQLQuery::Bind(const int index, const std::int64_t value) const {
        if (const auto rc = sqlite3_bind_int64(Statement, index, value); rc != SQLITE_OK) {
            return std::unexpected(rc);
        }

        return {};
    }

    std::expected<void, int> DatabaseHelper::SQLQuery::Bind(const int index, const double value) const {
        if (const auto rc = sqlite3_bind_double(Statement, index, value); rc != SQLITE_OK) {
            return std::unexpected(rc);
        }

        return {};
    }

    std::expected<void, int> DatabaseHelper::SQLQuery::Bind(const int index, const bool value) const {
        if (const auto rc = sqlite3_bind_int(Statement, index, value ? 1 : 0); rc != SQLITE_OK) {
            return std::unexpected(rc);
        }

        return {};
    }

    std::expected<void, int> DatabaseHelper::SQLQuery::BindNull(const int index) const {
        if (const auto rc = sqlite3_bind_null(Statement, index); rc != SQLITE_OK) {
            return std::unexpected(rc);
        }

        return {};
    }

    std::expected<bool, int> DatabaseHelper::SQLQuery::Step() const {
        const auto status = sqlite3_step(Statement);
        if (status == SQLITE_ROW) {
            return true;
        }

        if (status == SQLITE_DONE) {
            return false;
        }

        return std::unexpected(status);
    }

    std::expected<void, int> DatabaseHelper::SQLQuery::Execute() const {
        if (const auto result = Step(); !result.has_value()) {
            return std::unexpected(result.error());
        }

        return {};
    }

    int DatabaseHelper::SQLQuery::ColumnCount() const noexcept {
        return sqlite3_column_count(Statement);
    }

    std::string DatabaseHelper::SQLQuery::ColumnText(const int column) const {
        const auto *const text = reinterpret_cast<const char *>(
            sqlite3_column_text(Statement, column));
        if (text == nullptr) {
            return {};
        }
        return {text};
    }

    std::int64_t DatabaseHelper::SQLQuery::ColumnInt64(const int column) const {
        return sqlite3_column_int64(Statement, column);
    }

    double DatabaseHelper::SQLQuery::ColumnDouble(const int column) const {
        return sqlite3_column_double(Statement, column);
    }

    bool DatabaseHelper::SQLQuery::ColumnBool(const int column) const {
        return sqlite3_column_int(Statement, column) != 0;
    }

    bool DatabaseHelper::SQLQuery::ColumnIsNull(const int column) const noexcept {
        return sqlite3_column_type(Statement, column) == SQLITE_NULL;
    }

    sqlite3_stmt *DatabaseHelper::SQLQuery::Handle() const noexcept {
        return Statement;
    }

    DatabaseHelper::DatabaseHelper(std::string explicitDatabasePath)
        : ExplicitDatabasePath(std::move(explicitDatabasePath)) {
    }

    DatabaseHelper::~DatabaseHelper() {
        DisconnectDatabase();
    }

    DatabaseHelper::Result DatabaseHelper::initializeDatabase() {
        auto connectResult = ConnectDatabase();
        if (!connectResult.has_value()) {
            return connectResult;
        }
        return ConfigureDatabase();
    }

    DatabaseHelper::Result DatabaseHelper::ConnectDatabase() {
        if (Database != nullptr) {
            return {};
        }
        constexpr auto flags = SQLITE_OPEN_READWRITE
                               | SQLITE_OPEN_CREATE
                               | SQLITE_OPEN_FULLMUTEX;

        const auto rc = sqlite3_open_v2(
            ExplicitDatabasePath.c_str(),
            &Database,
            flags,
            nullptr
        );

        if (rc != SQLITE_OK) {
            return std::unexpected(CreateDatabaseError(rc));
        }

        return {};
    }

    DatabaseHelper::Result DatabaseHelper::ConfigureDatabase() {
        if (auto fkResult = EnableForeignKeys(); !fkResult.has_value()) {
            return fkResult;
        }

        if (auto walResult = EnableWAL(); !walResult.has_value()) {
            return walResult;
        }

        return {};
    }

    DatabaseHelper::Result DatabaseHelper::EnableForeignKeys() const {
        return Execute("PRAGMA foreign_keys = ON;");
    }

    DatabaseHelper::Result DatabaseHelper::EnableWAL() const {
        return Execute("PRAGMA journal_mode=WAL;");
    }


    std::expected<DatabaseHelper::SQLQuery, DatabaseHelper::DatabaseError>
    DatabaseHelper::Prepare(const std::string_view SQL) const {
        sqlite3_stmt *stmt = nullptr;
        const auto rc = sqlite3_prepare_v2(
            Database, SQL.data(), static_cast<int>(SQL.size()), &stmt, nullptr);
        if (rc != SQLITE_OK) {
            return std::unexpected(CreateDatabaseError(rc, SQL));
        }
        return DatabaseHelper::SQLQuery(Database, stmt, std::string(SQL));
    }

    DatabaseHelper::Result DatabaseHelper::Execute(const std::string_view SQL) const {
        auto prepareResult = Prepare(SQL);
        if (!prepareResult.has_value()) {
            return std::unexpected(prepareResult.error());
        }
        auto &query = prepareResult.value();
        const auto stepResult = query.Execute();
        if (!stepResult.has_value()) {
            return std::unexpected(
                CreateDatabaseError(stepResult.error(), SQL));
        }
        return {};
    }

    DatabaseHelper::Result DatabaseHelper::Transaction(
        const TransactionCallback &Callback) {
        auto beginResult = Execute("BEGIN TRANSACTION;");
        if (!beginResult.has_value()) {
            return beginResult;
        }
        const auto callbackResult = Callback(Database);
        if (!callbackResult.has_value()) {
            [[maybe_unused]] const auto rollback = Execute("ROLLBACK;");
            return callbackResult;
        }
        return Execute("COMMIT;");
    }

    void DatabaseHelper::DisconnectDatabase() noexcept {
        if (Database != nullptr) {
            sqlite3_close(Database);
            Database = nullptr;
        }
    }

    bool DatabaseHelper::IsDatabaseConnected() const noexcept {
        return Database != nullptr;
    }

    std::int64_t DatabaseHelper::last_insert_rowid() const noexcept {
        return sqlite3_last_insert_rowid(Database);
    }

    DatabaseHelper::DatabaseError DatabaseHelper::CreateDatabaseError(
        const int StatusCode, const std::string_view SQL) const {
        DatabaseError error;
        error.StatusCode = StatusCode;
        if (Database != nullptr) {
            error.Message = sqlite3_errmsg(Database);
        }
        if (!SQL.empty()) {
            error.SQL = SQL;
        }
        return error;
    }
}
