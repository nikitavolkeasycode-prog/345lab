// Copyright (c) 2026. Subscription Tracker.
// SPDX-License-Identifier: MIT
//
// TXT-backed repository. Stores one subscription per line, fields
// separated by '|'. Lines starting with '#' and blank lines are
// ignored. The data directory is created on demand.

#ifndef SUBSCRIPTION_TRACKER_FILE_IO_H
#define SUBSCRIPTION_TRACKER_FILE_IO_H

#include "IRepository.h"
#include "Subscription.h"

#include <filesystem>
#include <iosfwd>
#include <string>
#include <vector>

namespace subscription_tracker {

class FileRepository : public IRepository {
 public:
    // `path` is the on-disk file that backs the dataset. The parent
    // directory is created lazily by SaveAll().
    explicit FileRepository(std::filesystem::path path);

    std::vector<Subscription> LoadAll() override;
    void SaveAll(const std::vector<Subscription>& items) override;
    std::string BackendName() const override { return "file"; }

    const std::filesystem::path& path() const noexcept { return path_; }

 private:
    std::filesystem::path path_;
};

// Returns the current local date as YYYY-MM-DD, used by the menu
// when the user picks "subscriptions due soon" without specifying
// a date.
std::string TodayIsoDate();

}  // namespace subscription_tracker

#endif  // SUBSCRIPTION_TRACKER_FILE_IO_H
