// Copyright (c) 2026. Subscription Tracker.
// SPDX-License-Identifier: MIT
//
// Entry point. Sets up console encoding for Cyrillic, picks the
// storage backend (file by default, SQLite when compiled with
// SUBSCRIPTION_TRACKER_WITH_SQLITE and the --sql flag is given),
// then hands control to the menu.

#include "FileIO.h"
#include "Menu.h"
#include "SubscriptionManager.h"

#ifdef SUBSCRIPTION_TRACKER_WITH_SQLITE
#include "SqlRepository.h"
#endif

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

namespace {

// Best-effort console setup for Windows so that Cyrillic renders
// correctly. On POSIX systems (UTF-8 locale assumed) the calls are
// harmless no-ops.
void ConfigureConsole() {
#ifdef _WIN32
    // 65001 = UTF-8. CP_UTF8 is also defined in modern SDKs; we use
    // the literal to stay compatible with older toolchains.
    ::SetConsoleOutputCP(65001);
    ::SetConsoleCP(65001);
#endif
}

void PrintUsage(const char* exe) {
    std::cout <<
        "Использование:\n"
        "  " << exe << " [--sql] [--file PATH] [--data-dir DIR]\n\n"
        "По умолчанию данные хранятся в <exe-dir>/data/subscriptions.txt.\n"
        "С --sql и сборкой с SQLite используется data/subscriptions.db.\n";
}

}  // namespace

int main(int argc, char** argv) {
    ConfigureConsole();

    bool use_sql = false;
    fs::path file_path;     // explicit --file override
    fs::path data_dir;      // explicit --data-dir override

    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--sql") {
            use_sql = true;
        } else if (a == "--file" && i + 1 < argc) {
            file_path = argv[++i];
        } else if (a == "--data-dir" && i + 1 < argc) {
            data_dir = argv[++i];
        } else if (a == "--help" || a == "-h") {
            PrintUsage(argv[0]);
            return 0;
        } else {
            std::cerr << "Неизвестный аргумент: " << a << "\n";
            PrintUsage(argv[0]);
            return 1;
        }
    }

    try {
        // Resolve the data directory.
        fs::path base;
        if (!data_dir.empty()) {
            base = data_dir;
        } else {
            // Live next to the executable so the program works from
            // any cwd (e.g. when launched from an IDE).
            base = fs::path(argv[0]).parent_path();
            if (base.empty()) base = fs::current_path();
            base /= "data";
        }
        std::error_code ec;
        fs::create_directories(base, ec);

        std::unique_ptr<subscription_tracker::IRepository> repo;

        if (use_sql) {
#ifdef SUBSCRIPTION_TRACKER_WITH_SQLITE
            auto db = file_path.empty() ? (base / "subscriptions.db")
                                        : file_path;
            repo = std::make_unique<subscription_tracker::SqlRepository>(db);
#else
            std::cerr << "[warn] бинарник собран без поддержки SQLite, "
                         "используется файловое хранилище.\n";
            auto p = file_path.empty() ? (base / "subscriptions.txt")
                                       : file_path;
            repo = std::make_unique<subscription_tracker::FileRepository>(p);
#endif
        } else {
            auto p = file_path.empty() ? (base / "subscriptions.txt")
                                       : file_path;
            repo = std::make_unique<subscription_tracker::FileRepository>(p);
        }

        subscription_tracker::SubscriptionManager mgr(std::move(repo));
        mgr.Load();

        subscription_tracker::Menu menu(mgr, std::cin, std::cout);
        return menu.Run();
    } catch (const std::exception& e) {
        std::cerr << "Фатальная ошибка: " << e.what() << "\n";
        return 2;
    }
}
