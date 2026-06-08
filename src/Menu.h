// Copyright (c) 2026. Subscription Tracker.
// SPDX-License-Identifier: MIT
//
// Text-mode menu. The menu owns nothing: it just drives the
// SubscriptionManager and prints results. Input is read from
// stdin; the wrapper is hidden so Menu can be unit-tested with
// a string stream later.

#ifndef SUBSCRIPTION_TRACKER_MENU_H
#define SUBSCRIPTION_TRACKER_MENU_H

#include "SubscriptionManager.h"

#include <iosfwd>
#include <string>

namespace subscription_tracker {

class Menu {
 public:
    Menu(SubscriptionManager& mgr, std::istream& in, std::ostream& out)
        : mgr_(mgr), in_(in), out_(out) {}

    // Blocks until the user picks "Exit". Returns the program's
    // process exit code (0 on graceful exit, non-zero on
    // unhandled fatal error).
    int Run();

 private:
    // Renders the main menu and reads a 1-based choice.
    int MainMenuChoice();

    // Each Handle* function returns true if the menu should redraw
    // (i.e. it did work), false on user-initiated back/cancel.
    bool HandleAdd();
    bool HandleEdit();
    bool HandleDelete();
    bool HandleListAll();
    bool HandleSearch();
    bool HandleFilter();
    bool HandleSort();
    bool HandleAnalytics();
    bool HandleSave();

    // Low-level helpers ------------------------------------------------------
    void PrintTable(const std::vector<Subscription>& items) const;
    static std::string Centered(std::string s, std::size_t width);
    static std::string CycleRu(BillingCycle c);
    static std::string CategoryRu(Category c);
    static std::string StatusRu(Status s);

    static std::string ReadLine(const std::string& prompt);
    static std::string ReadNonEmpty(const std::string& prompt);
    static int         ReadInt(const std::string& prompt);
    static double      ReadPositiveDouble(const std::string& prompt);

    SubscriptionManager& mgr_;
    std::istream& in_;
    std::ostream& out_;
};

}  // namespace subscription_tracker

#endif  // SUBSCRIPTION_TRACKER_MENU_H
