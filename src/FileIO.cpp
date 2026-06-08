// Copyright (c) 2026. Subscription Tracker.
// SPDX-License-Identifier: MIT

#include "FileIO.h"

#include <algorithm>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

namespace subscription_tracker {

namespace {

// Trim ASCII whitespace from both ends of a string view.
std::string_view Trim(std::string_view s) {
    auto not_space = [](char c) { return !std::isspace(static_cast<unsigned char>(c)); };
    auto begin = std::find_if(s.begin(), s.end(), not_space);
    auto end   = std::find_if(s.rbegin(), s.rend(), not_space).base();
    if (begin >= end) return {};
    return std::string_view(&*begin, static_cast<std::size_t>(end - begin));
}

}  // namespace

FileRepository::FileRepository(std::filesystem::path path)
    : path_(std::move(path)) {}

std::vector<Subscription> FileRepository::LoadAll() {
    std::vector<Subscription> out;
    if (!std::filesystem::exists(path_)) {
        // No data file yet -> empty dataset. This is the "first run"
        // path described in the technical specification.
        return out;
    }

    std::ifstream in(path_);
    if (!in.is_open()) {
        throw std::runtime_error("Cannot open data file: " + path_.string());
    }

    std::string line;
    std::size_t line_no = 0;
    while (std::getline(in, line)) {
        ++line_no;
        auto view = Trim(line);
        if (view.empty() || view.front() == '#') continue;
        auto parsed = Subscription::FromCsv(view);
        if (!parsed) {
            // Per the TZ: "обработка ошибок без аварийного завершения".
            // We log the offending line to stderr and keep going.
            std::fprintf(stderr,
                         "[warn] %s:%zu: malformed line skipped: %s\n",
                         path_.string().c_str(), line_no, line.c_str());
            continue;
        }
        out.push_back(std::move(*parsed));
    }
    return out;
}

void FileRepository::SaveAll(const std::vector<Subscription>& items) {
    const auto parent = path_.parent_path();
    if (!parent.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(parent, ec);
        // Ignore the error: if create_directories failed the open()
        // below will throw a more informative message.
    }

    // Write to a sibling temp file first and atomically replace the
    // target. This keeps storage consistent on crash / power loss.
    const auto tmp = path_.string() + ".tmp";
    {
        std::ofstream out(tmp, std::ios::trunc);
        if (!out.is_open()) {
            throw std::runtime_error("Cannot open temp file: " + tmp);
        }
        out << "# id|service_name|category|monthly_cost|billing_cycle|"
               "next_payment_date|status\n";
        for (const auto& s : items) {
            out << s.ToCsv() << '\n';
        }
        if (!out) {
            throw std::runtime_error("Write failed for: " + tmp);
        }
    }
    std::error_code ec;
    std::filesystem::rename(tmp, path_, ec);
    if (ec) {
        // Fall back to a non-atomic replace on filesystems that
        // don't support rename-over-existing (e.g. some FAT mounts).
        std::filesystem::remove(path_, ec);
        std::filesystem::rename(tmp, path_, ec);
        if (ec) {
            throw std::runtime_error("Cannot commit save: " + ec.message());
        }
    }
}

std::string TodayIsoDate() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    tm = *std::localtime(&t);
#endif
    std::ostringstream os;
    os << std::put_time(&tm, "%Y-%m-%d");
    return os.str();
}

}  // namespace subscription_tracker
