// Copyright (c) 2026. Subscription Tracker.
// SPDX-License-Identifier: MIT
//
// Domain model for a single subscription / recurring payment.

#ifndef SUBSCRIPTION_TRACKER_SUBSCRIPTION_H
#define SUBSCRIPTION_TRACKER_SUBSCRIPTION_H

#include <array>
#include <optional>
#include <string>
#include <string_view>

namespace subscription_tracker {

// Payment cycle. Stored as a string in the data file, validated on parse.
enum class BillingCycle {
    kDaily,
    kWeekly,
    kMonthly,
    kQuarterly,
    kYearly,
};

// Categories allowed by the technical specification.
enum class Category {
    kEntertainment,
    kSoftware,
    kEducation,
    kUtilities,
    kOther,
};

// Active = billed and counted in totals. Paused = kept in the list
// but excluded from analytics. Cancelled = terminal state.
enum class Status {
    kActive,
    kPaused,
    kCancelled,
};

// Free functions to convert between the enums and their canonical
// string representation. Unknown values return std::nullopt so the
// caller can decide how to handle malformed input.
std::optional<BillingCycle> BillingCycleFromString(std::string_view s);
std::string_view BillingCycleToString(BillingCycle c) noexcept;

std::optional<Category> CategoryFromString(std::string_view s);
std::string_view CategoryToString(Category c) noexcept;

std::optional<Status> StatusFromString(std::string_view s);
std::string_view StatusToString(Status s) noexcept;

// A single recurring-payment record.
class Subscription {
 public:
    // Constructs a subscription with the given ID. The remaining fields
    // are validated through the setters and an invalid value will
    // throw std::invalid_argument, which the menu layer must catch.
    Subscription(std::string id,
                 std::string service_name,
                 Category category,
                 double monthly_cost,
                 BillingCycle cycle,
                 std::string next_payment_date,  // ISO-8601: YYYY-MM-DD
                 Status status = Status::kActive);

    // --- Accessors -----------------------------------------------------------
    const std::string& id() const noexcept { return id_; }
    const std::string& service_name() const noexcept { return service_name_; }
    Category category() const noexcept { return category_; }
    double monthly_cost() const noexcept { return monthly_cost_; }
    BillingCycle cycle() const noexcept { return cycle_; }
    const std::string& next_payment_date() const noexcept { return next_payment_date_; }
    Status status() const noexcept { return status_; }

    // --- Mutators (each validates input) ------------------------------------
    void set_service_name(std::string name);
    void set_category(Category c) noexcept;
    void set_monthly_cost(double cost);
    void set_cycle(BillingCycle c) noexcept;
    void set_next_payment_date(std::string date);
    void set_status(Status s) noexcept;

    // Returns the cost normalised to a per-month figure so analytics
    // can sum regardless of billing cycle.
    double NormalisedMonthlyCost() const noexcept;

    // Returns true if the next-payment date is within `days` of today
    // (inclusive) and the subscription is active.
    bool IsDueWithinDays(int days, const std::string& today_iso) const;

    // Serialises a record to a single CSV line. Uses '|' as the field
    // delimiter (commas may appear in service names).
    std::string ToCsv() const;

    // Parses a CSV line previously produced by ToCsv().
    // Returns std::nullopt on malformed input.
    static std::optional<Subscription> FromCsv(std::string_view line);

 private:
    std::string id_;
    std::string service_name_;
    Category category_;
    double monthly_cost_;
    BillingCycle cycle_;
    std::string next_payment_date_;  // YYYY-MM-DD
    Status status_;
};

}  // namespace subscription_tracker

#endif  // SUBSCRIPTION_TRACKER_SUBSCRIPTION_H
