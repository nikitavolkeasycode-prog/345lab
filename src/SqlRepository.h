// Copyright (c) 2026. Subscription Tracker.
// SPDX-License-Identifier: MIT
//
// SQLite-backed repository. Optional +3-point bonus. The class is
// only compiled when SUBSCRIPTION_TRACKER_WITH_SQLITE is defined,
// so projects without SQLite still build cleanly.

#ifndef SUBSCRIPTION_TRACKER_SQL_REPOSITORY_H
#define SUBSCRIPTION_TRACKER_SQL_REPOSITORY_H

#include "IRepository.h"

#ifdef SUBSCRIPTION_TRACKER_WITH_SQLITE

#include <filesystem>
#include <string>

struct sqlite3;

namespace subscription_tracker {

class SqlRepository : public IRepository {
 public:
    // Opens (or creates) the database at `path`. The schema is set
    // up on first use; an empty file is acceptable.
    explicit SqlRepository(std::filesystem::path path);
    ~SqlRepository() override;

    SqlRepository(const SqlRepository&) = delete;
    SqlRepository& operator=(const SqlRepository&) = delete;

    std::vector<Subscription> LoadAll() override;
    void SaveAll(const std::vector<Subscription>& items) override;
    std::string BackendName() const override { return "sqlite"; }

 private:
    void InitSchema();
    void Exec(const std::string& sql);

    std::filesystem::path path_;
    sqlite3* db_ = nullptr;
};

}  // namespace subscription_tracker

#endif  // SUBSCRIPTION_TRACKER_WITH_SQLITE
#endif  // SUBSCRIPTION_TRACKER_SQL_REPOSITORY_H
