// Copyright (c) 2026. Subscription Tracker.
// SPDX-License-Identifier: MIT
//
// Storage abstraction. The default FileRepository persists to a
// text file. SqlRepository provides the same API against a SQLite
// database, fulfilling the optional +3 point SQL bonus.

#ifndef SUBSCRIPTION_TRACKER_IREPOSITORY_H
#define SUBSCRIPTION_TRACKER_IREPOSITORY_H

#include "Subscription.h"

#include <string>
#include <vector>

namespace subscription_tracker {

class IRepository {
 public:
    virtual ~IRepository() = default;

    // Returns the full list of subscriptions in storage order.
    virtual std::vector<Subscription> LoadAll() = 0;

    // Replaces the entire dataset. Implementations should be
    // atomic: either the new set is fully persisted or storage
    // is left untouched on failure.
    virtual void SaveAll(const std::vector<Subscription>& items) = 0;

    // Short name for log lines ("file" / "sql").
    virtual std::string BackendName() const = 0;
};

}  // namespace subscription_tracker

#endif  // SUBSCRIPTION_TRACKER_IREPOSITORY_H
