// Copyright (c) 2026. Subscription Tracker.
// SPDX-License-Identifier: MIT
//
// In-memory business logic. The manager owns the working copy of
// the dataset and exposes CRUD, search/filter, sort, and analytics
// operations. Persistence is delegated to an IRepository.

#ifndef SUBSCRIPTION_TRACKER_SUBSCRIPTION_MANAGER_H
#define SUBSCRIPTION_TRACKER_SUBSCRIPTION_MANAGER_H

#include "IRepository.h"
#include "Subscription.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace subscription_tracker {

// Range descriptor for cost/date filters.
struct DoubleRange {
    std::optional<double> min;  // inclusive
    std::optional<double> max;  // inclusive
};

struct DateRange {
    std::optional<std::string> min;  // YYYY-MM-DD, inclusive
    std::optional<std::string> max;  // YYYY-MM-DD, inclusive
};

// A single category total, used by TopCategories.
struct CategoryTotal {
    Category category;
    double total_monthly = 0.0;
    std::size_t count = 0;
};

class SubscriptionManager {
 public:
    explicit SubscriptionManager(std::unique_ptr<IRepository> repo);

    // Loads the full dataset from the configured repository. Throws
    // std::runtime_error if the repository cannot be read.
    void Load();

    // Persists the current in-memory state. Throws on I/O failure.
    void Save() const;

    // Reports the name of the underlying storage backend.
    std::string BackendName() const;

    // --- CRUD ---------------------------------------------------------------

    // Adds `sub`. Throws std::invalid_argument if the id is already
    // taken or any field fails validation.
    void Add(Subscription sub);

    // Replaces the record with the matching id. Returns false if
    // no such id exists.
    bool Update(const Subscription& sub);

    // Removes a record by id. Returns false if not found.
    bool RemoveById(const std::string& id);

    // Removes every record whose service name matches `name`
    // (case-insensitive). Returns the number of removed records.
    std::size_t RemoveByName(const std::string& name);

    // Returns true if a record with this id exists.
    bool HasId(const std::string& id) const;

    // --- Query --------------------------------------------------------------

    const std::vector<Subscription>& All() const noexcept { return items_; }
    std::size_t Size() const noexcept { return items_.size(); }

    std::optional<Subscription> FindById(const std::string& id) const;

    // Partial, case-insensitive substring search on service name.
    std::vector<Subscription> SearchByName(const std::string& needle) const;

    // Filter helpers. An unset optional in the range struct means
    // "no bound on this side".
    std::vector<Subscription> FilterByCategory(Category c) const;
    std::vector<Subscription> FilterByCost(const DoubleRange& r) const;
    std::vector<Subscription> FilterByDate(const DateRange& r) const;
    std::vector<Subscription> FilterByStatus(Status s) const;

    // Convenience: combine multiple criteria. nullptr means "ignore".
    std::vector<Subscription> Filter(const std::string& name_substr,
                                     std::optional<Category> cat,
                                     const DoubleRange& cost,
                                     const DateRange& dates,
                                     std::optional<Status> st) const;

    // --- Sort ---------------------------------------------------------------

    enum class SortKey {
        kMonthlyCost,
        kNextPaymentDate,
        kServiceName,
    };
    enum class SortOrder { kAscending, kDescending };

    // Sorts in-place. Returns a reference to the same vector for
    // chaining.
    std::vector<Subscription>& Sort(SortKey key, SortOrder order);

    // Returns a sorted copy without mutating the manager.
    std::vector<Subscription> Sorted(SortKey key, SortOrder order) const;

    // --- Analytics ----------------------------------------------------------

    // Sum of normalised monthly cost over active subscriptions.
    double TotalMonthlySpend() const;

    // Active subscriptions whose next payment date is within
    // `days` of `today_iso`.
    std::vector<Subscription> DueWithinDays(int days,
                                            const std::string& today_iso) const;

    // Top `n` categories by total normalised monthly cost of their
    // active subscriptions. Ties broken by category ordinal.
    std::vector<CategoryTotal> TopCategories(std::size_t n) const;

 private:
    std::unique_ptr<IRepository> repo_;
    std::vector<Subscription> items_;
};

}  // namespace subscription_tracker

#endif  // SUBSCRIPTION_TRACKER_SUBSCRIPTION_MANAGER_H
