// Copyright (c) 2026. Subscription Tracker.
// SPDX-License-Identifier: MIT

#include "SubscriptionManager.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <unordered_map>

namespace subscription_tracker {

namespace {

std::string AsciiLower(std::string_view s) {
    std::string out(s);
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return out;
}

}  // namespace

SubscriptionManager::SubscriptionManager(std::unique_ptr<IRepository> repo)
    : repo_(std::move(repo)) {}

void SubscriptionManager::Load() {
    if (!repo_) throw std::runtime_error("No repository configured");
    items_ = repo_->LoadAll();
}

void SubscriptionManager::Save() const {
    if (!repo_) throw std::runtime_error("No repository configured");
    repo_->SaveAll(items_);
}

std::string SubscriptionManager::BackendName() const {
    return repo_ ? repo_->BackendName() : std::string{"none"};
}

bool SubscriptionManager::HasId(const std::string& id) const {
    const auto target = AsciiLower(id);
    return std::any_of(items_.begin(), items_.end(),
                       [&](const Subscription& s) {
                           return AsciiLower(s.id()) == target;
                       });
}

void SubscriptionManager::Add(Subscription sub) {
    if (HasId(sub.id())) {
        throw std::invalid_argument(
            "Subscription with id '" + sub.id() + "' already exists");
    }
    items_.push_back(std::move(sub));
}

bool SubscriptionManager::Update(const Subscription& sub) {
    for (auto& current : items_) {
        if (AsciiLower(current.id()) == AsciiLower(sub.id())) {
            current = sub;
            return true;
        }
    }
    return false;
}

bool SubscriptionManager::RemoveById(const std::string& id) {
    const auto target = AsciiLower(id);
    const auto it = std::find_if(items_.begin(), items_.end(),
                                 [&](const Subscription& s) {
                                     return AsciiLower(s.id()) == target;
                                 });
    if (it == items_.end()) return false;
    items_.erase(it);
    return true;
}

std::size_t SubscriptionManager::RemoveByName(const std::string& name) {
    const auto needle = AsciiLower(name);
    const auto before = items_.size();
    items_.erase(std::remove_if(items_.begin(), items_.end(),
                                [&](const Subscription& s) {
                                    return AsciiLower(s.service_name()) == needle;
                                }),
                 items_.end());
    return before - items_.size();
}

std::optional<Subscription> SubscriptionManager::FindById(
        const std::string& id) const {
    const auto target = AsciiLower(id);
    for (const auto& s : items_) {
        if (AsciiLower(s.id()) == target) return s;
    }
    return std::nullopt;
}

std::vector<Subscription> SubscriptionManager::SearchByName(
        const std::string& needle) const {
    const auto n = AsciiLower(needle);
    std::vector<Subscription> out;
    for (const auto& s : items_) {
        if (AsciiLower(s.service_name()).find(n) != std::string::npos) {
            out.push_back(s);
        }
    }
    return out;
}

std::vector<Subscription> SubscriptionManager::FilterByCategory(Category c) const {
    std::vector<Subscription> out;
    for (const auto& s : items_) {
        if (s.category() == c) out.push_back(s);
    }
    return out;
}

std::vector<Subscription> SubscriptionManager::FilterByCost(
        const DoubleRange& r) const {
    std::vector<Subscription> out;
    for (const auto& s : items_) {
        const double cost = s.monthly_cost();
        if (r.min && cost < *r.min) continue;
        if (r.max && cost > *r.max) continue;
        out.push_back(s);
    }
    return out;
}

std::vector<Subscription> SubscriptionManager::FilterByDate(
        const DateRange& r) const {
    std::vector<Subscription> out;
    for (const auto& s : items_) {
        const auto& d = s.next_payment_date();
        if (r.min && d < *r.min) continue;
        if (r.max && d > *r.max) continue;
        out.push_back(s);
    }
    return out;
}

std::vector<Subscription> SubscriptionManager::FilterByStatus(Status s) const {
    std::vector<Subscription> out;
    for (const auto& sub : items_) {
        if (sub.status() == s) out.push_back(sub);
    }
    return out;
}

std::vector<Subscription> SubscriptionManager::Filter(
        const std::string& name_substr,
        std::optional<Category> cat,
        const DoubleRange& cost,
        const DateRange& dates,
        std::optional<Status> st) const {
    std::vector<Subscription> out;
    const auto needle = AsciiLower(name_substr);
    for (const auto& s : items_) {
        if (!needle.empty() &&
            AsciiLower(s.service_name()).find(needle) == std::string::npos) {
            continue;
        }
        if (cat && s.category() != *cat) continue;
        if (st  && s.status()   != *st)  continue;
        if (cost.min && s.monthly_cost() < *cost.min) continue;
        if (cost.max && s.monthly_cost() > *cost.max) continue;
        if (dates.min && s.next_payment_date() < *dates.min) continue;
        if (dates.max && s.next_payment_date() > *dates.max) continue;
        out.push_back(s);
    }
    return out;
}

SubscriptionManager::SortOrder Flip(SubscriptionManager::SortOrder o) {
    return o == SubscriptionManager::SortOrder::kAscending
               ? SubscriptionManager::SortOrder::kDescending
               : SubscriptionManager::SortOrder::kAscending;
}

std::vector<Subscription>& SubscriptionManager::Sort(SortKey key, SortOrder order) {
    auto cmp = [key, order](const Subscription& a, const Subscription& b) {
        bool less = false;
        switch (key) {
            case SortKey::kMonthlyCost:
                less = a.monthly_cost() < b.monthly_cost();
                break;
            case SortKey::kNextPaymentDate:
                less = a.next_payment_date() < b.next_payment_date();
                break;
            case SortKey::kServiceName:
                less = AsciiLower(a.service_name()) < AsciiLower(b.service_name());
                break;
        }
        return order == SortOrder::kAscending ? less : !less;
    };
    std::sort(items_.begin(), items_.end(), cmp);
    return items_;
}

std::vector<Subscription> SubscriptionManager::Sorted(SortKey key,
                                                     SortOrder order) const {
    auto copy = items_;
    auto cmp = [key, order](const Subscription& a, const Subscription& b) {
        bool less = false;
        switch (key) {
            case SortKey::kMonthlyCost:
                less = a.monthly_cost() < b.monthly_cost();
                break;
            case SortKey::kNextPaymentDate:
                less = a.next_payment_date() < b.next_payment_date();
                break;
            case SortKey::kServiceName:
                less = AsciiLower(a.service_name()) < AsciiLower(b.service_name());
                break;
        }
        return order == SortOrder::kAscending ? less : !less;
    };
    std::sort(copy.begin(), copy.end(), cmp);
    return copy;
}

double SubscriptionManager::TotalMonthlySpend() const {
    double total = 0.0;
    for (const auto& s : items_) {
        if (s.status() == Status::kActive) {
            total += s.NormalisedMonthlyCost();
        }
    }
    return total;
}

std::vector<Subscription> SubscriptionManager::DueWithinDays(
        int days, const std::string& today_iso) const {
    std::vector<Subscription> out;
    for (const auto& s : items_) {
        if (s.IsDueWithinDays(days, today_iso)) out.push_back(s);
    }
    return out;
}

std::vector<CategoryTotal> SubscriptionManager::TopCategories(std::size_t n) const {
    // Aggregate (total_monthly, count) keyed by category. The pair is
    // trivially default-constructible, so we sidestep any MSVC
    // issues around initialising a struct that mixes a scoped-enum
    // with default-initialised scalar members in an unordered_map.
    std::unordered_map<Category, std::pair<double, std::size_t>> agg;
    for (const auto& s : items_) {
        if (s.status() != Status::kActive) continue;
        auto& bucket = agg[s.category()];
        bucket.first  += s.NormalisedMonthlyCost();
        bucket.second += 1;
    }
    std::vector<CategoryTotal> vec;
    vec.reserve(agg.size());
    for (const auto& kv : agg) {
        vec.push_back({kv.first, kv.second.first, kv.second.second});
    }
    std::sort(vec.begin(), vec.end(),
              [](const CategoryTotal& a, const CategoryTotal& b) {
                  if (a.total_monthly != b.total_monthly) {
                      return a.total_monthly > b.total_monthly;
                  }
                  return static_cast<int>(a.category) < static_cast<int>(b.category);
              });
    if (vec.size() > n) vec.resize(n);
    return vec;
}

}  // namespace subscription_tracker
