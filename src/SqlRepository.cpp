// Copyright (c) 2026. Subscription Tracker.
// SPDX-License-Identifier: MIT

#include "SqlRepository.h"

#ifdef SUBSCRIPTION_TRACKER_WITH_SQLITE

#include <sqlite3.h>

#include <cstdio>
#include <stdexcept>

namespace subscription_tracker {

namespace {

// Throws on error. Prepared statement is destroyed before returning.
void ThrowOnError(sqlite3* db, int rc, const char* what) {
    if (rc != SQLITE_OK && rc != SQLITE_DONE && rc != SQLITE_ROW) {
        std::string msg = sqlite3_errmsg(db);
        throw std::runtime_error(std::string(what) + ": " + msg);
    }
}

}  // namespace

SqlRepository::SqlRepository(std::filesystem::path path) : path_(std::move(path)) {
    const auto parent = path_.parent_path();
    if (!parent.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(parent, ec);
    }
    if (sqlite3_open(path_.string().c_str(), &db_) != SQLITE_OK) {
        std::string msg = db_ ? sqlite3_errmsg(db_) : "open failed";
        if (db_) sqlite3_close(db_);
        db_ = nullptr;
        throw std::runtime_error("Cannot open SQLite database: " + msg);
    }
    // Enforce FK constraints; the schema relies on them.
    Exec("PRAGMA foreign_keys = ON;");
    InitSchema();
}

SqlRepository::~SqlRepository() {
    if (db_) sqlite3_close(db_);
}

void SqlRepository::Exec(const std::string& sql) {
    char* err = nullptr;
    const int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::string msg = err ? err : "unknown error";
        sqlite3_free(err);
        throw std::runtime_error("SQLite exec failed: " + msg);
    }
}

void SqlRepository::InitSchema() {
    Exec(R"SQL(
        CREATE TABLE IF NOT EXISTS subscriptions (
            id                 TEXT PRIMARY KEY,
            service_name       TEXT NOT NULL,
            category           TEXT NOT NULL,
            monthly_cost       REAL NOT NULL CHECK (monthly_cost > 0),
            billing_cycle      TEXT NOT NULL,
            next_payment_date  TEXT NOT NULL,
            status             TEXT NOT NULL
        );
    )SQL");
}

std::vector<Subscription> SqlRepository::LoadAll() {
    std::vector<Subscription> out;
    const char* kSql =
        "SELECT id, service_name, category, monthly_cost, "
        "billing_cycle, next_payment_date, status "
        "FROM subscriptions ORDER BY id;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(std::string("prepare LoadAll: ") +
                                 sqlite3_errmsg(db_));
    }

    while (true) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_DONE) break;
        if (rc != SQLITE_ROW) {
            sqlite3_finalize(stmt);
            ThrowOnError(db_, rc, "step LoadAll");
        }
        const std::string id      = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const std::string name    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const std::string cat_str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const double cost         = sqlite3_column_double(stmt, 3);
        const std::string cycle_s = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        const std::string date    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        const std::string status  = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));

        auto cycle    = BillingCycleFromString(cycle_s);
        auto category = CategoryFromString(cat_str);
        auto st       = StatusFromString(status);
        if (!cycle || !category || !st) {
            std::fprintf(stderr,
                         "[warn] sqlite: invalid enum value, skipping id=%s\n",
                         id.c_str());
            continue;
        }
        try {
            out.emplace_back(id, name, *category, cost, *cycle, date, *st);
        } catch (const std::invalid_argument& e) {
            std::fprintf(stderr,
                         "[warn] sqlite: invalid row skipped (%s): %s\n",
                         e.what(), id.c_str());
        }
    }
    sqlite3_finalize(stmt);
    return out;
}

void SqlRepository::SaveAll(const std::vector<Subscription>& items) {
    // Whole-table replace inside a transaction for atomicity.
    Exec("BEGIN TRANSACTION;");
    try {
        Exec("DELETE FROM subscriptions;");
        const char* kInsert =
            "INSERT INTO subscriptions "
            "(id, service_name, category, monthly_cost, "
            " billing_cycle, next_payment_date, status) "
            "VALUES (?, ?, ?, ?, ?, ?, ?);";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, kInsert, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(std::string("prepare insert: ") +
                                     sqlite3_errmsg(db_));
        }
        for (const auto& s : items) {
            sqlite3_reset(stmt);
            sqlite3_bind_text (stmt, 1, s.id().c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text (stmt, 2, s.service_name().c_str(), -1, SQLITE_TRANSIENT);
            const std::string cat = std::string(CategoryToString(s.category()));
            sqlite3_bind_text (stmt, 3, cat.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_double(stmt, 4, s.monthly_cost());
            const std::string cyc = std::string(BillingCycleToString(s.cycle()));
            sqlite3_bind_text (stmt, 5, cyc.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text (stmt, 6, s.next_payment_date().c_str(), -1, SQLITE_TRANSIENT);
            const std::string st  = std::string(StatusToString(s.status()));
            sqlite3_bind_text (stmt, 7, st.c_str(), -1, SQLITE_TRANSIENT);
            const int rc = sqlite3_step(stmt);
            if (rc != SQLITE_DONE) {
                sqlite3_finalize(stmt);
                Exec("ROLLBACK;");
                ThrowOnError(db_, rc, "insert");
            }
        }
        sqlite3_finalize(stmt);
        Exec("COMMIT;");
    } catch (...) {
        Exec("ROLLBACK;");
        throw;
    }
}

}  // namespace subscription_tracker

#endif  // SUBSCRIPTION_TRACKER_WITH_SQLITE
