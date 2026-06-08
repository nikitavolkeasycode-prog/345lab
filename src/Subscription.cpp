// Copyright (c) 2026. Subscription Tracker.
// SPDX-License-Identifier: MIT

#include "Subscription.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace subscription_tracker {

namespace {

// Lower-cases an ASCII string in place. Cyrillic is left as-is; the
// case-insensitive comparisons in the manager work on the English
// enum names, so this is sufficient.
std::string AsciiLower(std::string_view s) {
    std::string out(s);
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return out;
}

// Splits a delimited string into fields, honouring the chosen
// delimiter. Empty fields are preserved.
std::vector<std::string> Split(std::string_view s, char delim) {
    std::vector<std::string> parts;
    std::string current;
    for (char c : s) {
        if (c == delim) {
            parts.push_back(current);
            current.clear();
        } else {
            current.push_back(c);
        }
    }
    parts.push_back(current);
    return parts;
}

bool IsValidIsoDate(std::string_view s) {
    if (s.size() != 10) return false;
    if (s[4] != '-' || s[7] != '-') return false;
    auto is_digit = [](char c) { return c >= '0' && c <= '9'; };
    for (size_t i : {0u, 1u, 2u, 3u, 5u, 6u, 8u, 9u}) {
        if (!is_digit(s[i])) return false;
    }
    int year  = (s[0] - '0') * 1000 + (s[1] - '0') * 100 +
                (s[2] - '0') * 10  + (s[3] - '0');
    int month = (s[5] - '0') * 10  + (s[6] - '0');
    int day   = (s[8] - '0') * 10  + (s[9] - '0');
    if (month < 1 || month > 12) return false;
    if (day < 1 || day > 31) return false;
    // Cheap leap-year-aware max-day check.
    static constexpr std::array<int, 12> kDaysInMonth = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int max_day = kDaysInMonth[month - 1];
    if (month == 2) {
        bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        if (leap) max_day = 29;
    }
    return day <= max_day;
}

}  // namespace

// --- enum <-> string ------------------------------------------------------

std::optional<BillingCycle> BillingCycleFromString(std::string_view s) {
    const std::string lc = AsciiLower(s);
    if (lc == "daily")    return BillingCycle::kDaily;
    if (lc == "weekly")   return BillingCycle::kWeekly;
    if (lc == "monthly")  return BillingCycle::kMonthly;
    if (lc == "quarterly")return BillingCycle::kQuarterly;
    if (lc == "yearly" || lc == "annual") return BillingCycle::kYearly;
    return std::nullopt;
}

std::string_view BillingCycleToString(BillingCycle c) noexcept {
    switch (c) {
        case BillingCycle::kDaily:     return "daily";
        case BillingCycle::kWeekly:    return "weekly";
        case BillingCycle::kMonthly:   return "monthly";
        case BillingCycle::kQuarterly: return "quarterly";
        case BillingCycle::kYearly:    return "yearly";
    }
    return "monthly";
}

std::optional<Category> CategoryFromString(std::string_view s) {
    const std::string lc = AsciiLower(s);
    if (lc == "entertainment" || lc == "развлечения")
        return Category::kEntertainment;
    if (lc == "software" || lc == "софт" || lc == "soft")
        return Category::kSoftware;
    if (lc == "education" || lc == "обучение")
        return Category::kEducation;
    if (lc == "utilities" || lc == "коммунальные" || lc == "utility")
        return Category::kUtilities;
    if (lc == "other" || lc == "другое")
        return Category::kOther;
    return std::nullopt;
}

std::string_view CategoryToString(Category c) noexcept {
    switch (c) {
        case Category::kEntertainment: return "entertainment";
        case Category::kSoftware:      return "software";
        case Category::kEducation:     return "education";
        case Category::kUtilities:     return "utilities";
        case Category::kOther:         return "other";
    }
    return "other";
}

std::optional<Status> StatusFromString(std::string_view s) {
    const std::string lc = AsciiLower(s);
    if (lc == "active"    || lc == "активна")   return Status::kActive;
    if (lc == "paused"    || lc == "пауза")     return Status::kPaused;
    if (lc == "cancelled" || lc == "отменена" || lc == "canceled")
        return Status::kCancelled;
    return std::nullopt;
}

std::string_view StatusToString(Status s) noexcept {
    switch (s) {
        case Status::kActive:    return "active";
        case Status::kPaused:    return "paused";
        case Status::kCancelled: return "cancelled";
    }
    return "active";
}

// --- Subscription ----------------------------------------------------------

Subscription::Subscription(std::string id,
                           std::string service_name,
                           Category category,
                           double monthly_cost,
                           BillingCycle cycle,
                           std::string next_payment_date,
                           Status status)
    : id_(std::move(id)),
      service_name_(std::move(service_name)),
      category_(category),
      monthly_cost_(monthly_cost),
      cycle_(cycle),
      next_payment_date_(std::move(next_payment_date)),
      status_(status) {
    if (id_.empty()) {
        throw std::invalid_argument("Subscription id must not be empty");
    }
    set_service_name(service_name_);
    set_category(category_);
    set_monthly_cost(monthly_cost_);
    set_cycle(cycle_);
    set_next_payment_date(next_payment_date_);
    set_status(status_);
}

void Subscription::set_service_name(std::string name) {
    if (name.empty()) {
        throw std::invalid_argument("Service name must not be empty");
    }
    service_name_ = std::move(name);
}

void Subscription::set_category(Category c) noexcept { category_ = c; }
void Subscription::set_cycle(BillingCycle c) noexcept { cycle_ = c; }
void Subscription::set_status(Status s) noexcept { status_ = s; }

void Subscription::set_monthly_cost(double cost) {
    if (!(cost > 0.0) || std::isnan(cost) || std::isinf(cost)) {
        throw std::invalid_argument(
            "Monthly cost must be a finite, strictly positive number");
    }
    monthly_cost_ = cost;
}

void Subscription::set_next_payment_date(std::string date) {
    if (!IsValidIsoDate(date)) {
        throw std::invalid_argument(
            "Next payment date must be in YYYY-MM-DD format");
    }
    next_payment_date_ = std::move(date);
}

double Subscription::NormalisedMonthlyCost() const noexcept {
    switch (cycle_) {
        case BillingCycle::kDaily:     return monthly_cost_ * 30.0;
        case BillingCycle::kWeekly:    return monthly_cost_ * 4.345;
        case BillingCycle::kMonthly:   return monthly_cost_;
        case BillingCycle::kQuarterly: return monthly_cost_ / 3.0;
        case BillingCycle::kYearly:    return monthly_cost_ / 12.0;
    }
    return monthly_cost_;
}

namespace {

// Turns an ISO date (YYYY-MM-DD) into a single integer for fast
// comparison: YYYYMMDD. No timezone awareness; that's a server-side
// concern, not something a console tracker needs.
long IsoDateToOrdinal(std::string_view s) {
    return (s[0] - '0') * 10000000L + (s[1] - '0') * 1000000L +
           (s[2] - '0') * 100000L   + (s[3] - '0') * 10000L +
           (s[5] - '0') * 1000L     + (s[6] - '0') * 100L +
           (s[8] - '0') * 10L       + (s[9] - '0');
}

}  // namespace

bool Subscription::IsDueWithinDays(int days, const std::string& today_iso) const {
    if (status_ != Status::kActive) return false;
    if (!IsValidIsoDate(today_iso) || !IsValidIsoDate(next_payment_date_)) {
        return false;
    }
    const long today  = IsoDateToOrdinal(today_iso);
    const long target = IsoDateToOrdinal(next_payment_date_);
    if (target < today) return false;  // overdue is "due now", which is
                                       // arguably out of scope for the
                                       // "next 7 days" requirement.
    return (target - today) <= days;
}

std::string Subscription::ToCsv() const {
    std::ostringstream os;
    os << id_ << '|'
       << service_name_ << '|'
       << CategoryToString(category_) << '|'
       << monthly_cost_ << '|'
       << BillingCycleToString(cycle_) << '|'
       << next_payment_date_ << '|'
       << StatusToString(status_);
    return os.str();
}

std::optional<Subscription> Subscription::FromCsv(std::string_view line) {
    if (line.empty()) return std::nullopt;
    const auto parts = Split(line, '|');
    if (parts.size() != 7) return std::nullopt;

    const auto cycle = BillingCycleFromString(parts[4]);
    if (!cycle) return std::nullopt;

    const auto category = CategoryFromString(parts[2]);
    if (!category) return std::nullopt;

    const auto status = StatusFromString(parts[6]);
    if (!status) return std::nullopt;

    double cost = 0.0;
    try {
        cost = std::stod(parts[3]);
    } catch (...) {
        return std::nullopt;
    }

    try {
        return Subscription(std::move(parts[0]),
                           std::move(parts[1]),
                           *category,
                           cost,
                           *cycle,
                           std::move(parts[5]),
                           *status);
    } catch (const std::invalid_argument&) {
        return std::nullopt;
    }
}

}  // namespace subscription_tracker
