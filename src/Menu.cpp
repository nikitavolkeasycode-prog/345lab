// Copyright (c) 2026. Subscription Tracker.
// SPDX-License-Identifier: MIT

#include "Menu.h"

#include "FileIO.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <ios>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace subscription_tracker {

namespace {

constexpr int kIdWidth         = 10;
constexpr int kNameWidth       = 22;
constexpr int kCategoryWidth   = 14;
constexpr int kCostWidth       = 10;
constexpr int kCycleWidth      = 11;
constexpr int kDateWidth       = 12;
constexpr int kStatusWidth     = 10;
constexpr int kNormCostWidth   = 12;

std::string AsciiLower(std::string_view s) {
    std::string out(s);
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return out;
}

}  // namespace

std::string Menu::Centered(std::string s, std::size_t width) {
    if (s.size() >= width) return s.substr(0, width);
    const std::size_t pad = (width - s.size()) / 2;
    return std::string(pad, ' ') + s + std::string(width - s.size() - pad, ' ');
}

std::string Menu::CycleRu(BillingCycle c) {
    switch (c) {
        case BillingCycle::kDaily:     return "ежедневно";
        case BillingCycle::kWeekly:    return "еженедельно";
        case BillingCycle::kMonthly:   return "ежемесячно";
        case BillingCycle::kQuarterly: return "ежеквартально";
        case BillingCycle::kYearly:    return "ежегодно";
    }
    return "?";
}

std::string Menu::CategoryRu(Category c) {
    switch (c) {
        case Category::kEntertainment: return "Развлечения";
        case Category::kSoftware:      return "Софт";
        case Category::kEducation:     return "Обучение";
        case Category::kUtilities:     return "Коммунальные";
        case Category::kOther:         return "Другое";
    }
    return "?";
}

std::string Menu::StatusRu(Status s) {
    switch (s) {
        case Status::kActive:    return "активна";
        case Status::kPaused:    return "пауза";
        case Status::kCancelled: return "отменена";
    }
    return "?";
}

std::string Menu::ReadLine(const std::string& prompt) {
    std::string s;
    std::cout << prompt;
    std::getline(std::cin, s);
    return s;
}

std::string Menu::ReadNonEmpty(const std::string& prompt) {
    while (true) {
        auto s = ReadLine(prompt);
        if (!s.empty()) return s;
        std::cout << "  [ошибка] значение не может быть пустым\n";
    }
}

int Menu::ReadInt(const std::string& prompt) {
    while (true) {
        auto s = ReadLine(prompt);
        try {
            std::size_t pos = 0;
            const int v = std::stoi(s, &pos);
            if (pos == s.size()) return v;
        } catch (...) {
        }
        std::cout << "  [ошибка] введите целое число\n";
    }
}

double Menu::ReadPositiveDouble(const std::string& prompt) {
    while (true) {
        auto s = ReadLine(prompt);
        try {
            std::size_t pos = 0;
            const double v = std::stod(s, &pos);
            if (pos == s.size() && std::isfinite(v) && v > 0.0) return v;
        } catch (...) {
        }
        std::cout << "  [ошибка] введите положительное число\n";
    }
}

void Menu::PrintTable(const std::vector<Subscription>& items) const {
    out_ << '\n';
    const std::string header =
        Centered("ID",        kIdWidth)       + " | " +
        Centered("Сервис",   kNameWidth)     + " | " +
        Centered("Категория", kCategoryWidth) + " | " +
        Centered("Стоим.",   kCostWidth)      + " | " +
        Centered("Цикл",     kCycleWidth)     + " | " +
        Centered("След. оплата", kDateWidth)  + " | " +
        Centered("Статус",   kStatusWidth)    + " | " +
        Centered("₽/мес",    kNormCostWidth);
    out_ << header << '\n';
    out_ << std::string(header.size(), '-') << '\n';
    for (const auto& s : items) {
        out_ << Centered(s.id(),                 kIdWidth)       << " | "
             << Centered(s.service_name(),       kNameWidth)     << " | "
             << Centered(CategoryRu(s.category()), kCategoryWidth) << " | "
             << Centered(std::to_string(static_cast<int>(s.monthly_cost())) + "₽",
                         kCostWidth)                                  << " | "
             << Centered(CycleRu(s.cycle()),     kCycleWidth)    << " | "
             << Centered(s.next_payment_date(),  kDateWidth)     << " | "
             << Centered(StatusRu(s.status()),   kStatusWidth)   << " | "
             << Centered(std::to_string(static_cast<int>(s.NormalisedMonthlyCost())) + "₽",
                         kNormCostWidth) << '\n';
    }
    out_ << "  всего записей: " << items.size() << "\n\n";
}

int Menu::MainMenuChoice() {
    out_ <<
        "\n"
        "================= Subscription Tracker =================\n"
        " 1) Показать все подписки\n"
        " 2) Добавить подписку\n"
        " 3) Редактировать подписку\n"
        " 4) Удалить подписку\n"
        " 5) Поиск по названию\n"
        " 6) Фильтр (категория / стоимость / дата / статус)\n"
        " 7) Сортировка\n"
        " 8) Аналитика\n"
        " 9) Сохранить изменения в файл\n"
        " 0) Выход\n"
        "========================================================\n";
    return ReadInt("> Выберите пункт: ");
}

bool Menu::HandleAdd() {
    out_ << "\n--- Добавление подписки ---\n";
    try {
        const auto id    = ReadNonEmpty("ID: ");
        if (mgr_.HasId(id)) {
            out_ << "  [ошибка] подписка с таким ID уже существует\n";
            return false;
        }
        const auto name  = ReadNonEmpty("Название сервиса: ");

        out_ << "Категория (1-Развлечения, 2-Софт, 3-Обучение, "
                "4-Коммунальные, 5-Другое): ";
        int cat_choice = ReadInt("");
        if (cat_choice < 1 || cat_choice > 5) {
            out_ << "  [ошибка] неверная категория\n";
            return false;
        }
        const auto cat = static_cast<Category>(cat_choice - 1);

        const auto cost = ReadPositiveDouble("Стоимость за период, ₽: ");

        out_ << "Цикл оплаты (1-daily, 2-weekly, 3-monthly, "
                "4-quarterly, 5-yearly): ";
        int cyc_choice = ReadInt("");
        if (cyc_choice < 1 || cyc_choice > 5) {
            out_ << "  [ошибка] неверный цикл\n";
            return false;
        }
        const auto cyc = static_cast<BillingCycle>(cyc_choice - 1);

        const auto date = ReadNonEmpty("Дата следующей оплаты (YYYY-MM-DD): ");

        out_ << "Статус (1-active, 2-paused, 3-cancelled) [по умолч. 1]: ";
        int st_choice = ReadInt("");
        if (st_choice == 0) st_choice = 1;
        if (st_choice < 1 || st_choice > 3) {
            out_ << "  [ошибка] неверный статус\n";
            return false;
        }
        const auto st = static_cast<Status>(st_choice - 1);

        mgr_.Add(Subscription(id, name, cat, cost, cyc, date, st));
        out_ << "  [ok] подписка добавлена\n";
        return true;
    } catch (const std::invalid_argument& e) {
        out_ << "  [ошибка валидации] " << e.what() << "\n";
        return false;
    }
}

bool Menu::HandleEdit() {
    out_ << "\n--- Редактирование ---\n";
    const auto id = ReadNonEmpty("ID редактируемой подписки: ");
    auto current = mgr_.FindById(id);
    if (!current) {
        out_ << "  [ошибка] подписка с таким ID не найдена\n";
        return false;
    }
    out_ << "  текущие данные:\n";
    PrintTable({*current});

    try {
        auto name = ReadLine("Новое название (Enter — не менять): ");
        if (name.empty()) name = current->service_name();

        out_ << "Категория (1-Развлечения, 2-Софт, 3-Обучение, "
                "4-Коммунальные, 5-Другое) [Enter — не менять]: ";
        std::string cat_s;
        std::getline(std::cin, cat_s);
        Category cat = current->category();
        if (!cat_s.empty()) {
            int c = std::stoi(cat_s);
            if (c < 1 || c > 5) {
                out_ << "  [ошибка] неверная категория\n";
                return false;
            }
            cat = static_cast<Category>(c - 1);
        }

        auto cost_s = ReadLine("Новая стоимость (Enter — не менять): ");
        double cost = current->monthly_cost();
        if (!cost_s.empty()) {
            try {
                std::size_t pos = 0;
                cost = std::stod(cost_s, &pos);
                if (pos != cost_s.size() || !(cost > 0.0)) {
                    out_ << "  [ошибка] стоимость должна быть > 0\n";
                    return false;
                }
            } catch (...) {
                out_ << "  [ошибка] некорректная стоимость\n";
                return false;
            }
        }

        out_ << "Цикл (1-daily, 2-weekly, 3-monthly, 4-quarterly, "
                "5-yearly) [Enter — не менять]: ";
        std::string cyc_s;
        std::getline(std::cin, cyc_s);
        BillingCycle cyc = current->cycle();
        if (!cyc_s.empty()) {
            int c = std::stoi(cyc_s);
            if (c < 1 || c > 5) {
                out_ << "  [ошибка] неверный цикл\n";
                return false;
            }
            cyc = static_cast<BillingCycle>(c - 1);
        }

        auto date = ReadLine("Новая дата (YYYY-MM-DD, Enter — не менять): ");
        if (date.empty()) date = current->next_payment_date();

        out_ << "Статус (1-active, 2-paused, 3-cancelled) "
                "[Enter — не менять]: ";
        std::string st_s;
        std::getline(std::cin, st_s);
        Status st = current->status();
        if (!st_s.empty()) {
            int c = std::stoi(st_s);
            if (c < 1 || c > 3) {
                out_ << "  [ошибка] неверный статус\n";
                return false;
            }
            st = static_cast<Status>(c - 1);
        }

        Subscription updated(current->id(), name, cat, cost, cyc, date, st);
        mgr_.Update(updated);
        out_ << "  [ok] подписка обновлена\n";
        return true;
    } catch (const std::invalid_argument& e) {
        out_ << "  [ошибка валидации] " << e.what() << "\n";
        return false;
    }
}

bool Menu::HandleDelete() {
    out_ << "\n--- Удаление ---\n";
    out_ << "  1) По ID\n  2) По названию сервиса\n";
    const int mode = ReadInt("> Способ: ");
    if (mode == 1) {
        const auto id = ReadNonEmpty("ID: ");
        if (mgr_.RemoveById(id)) {
            out_ << "  [ok] удалено\n";
            return true;
        }
        out_ << "  [ошибка] ID не найден\n";
        return false;
    }
    if (mode == 2) {
        const auto name = ReadNonEmpty("Название: ");
        const auto n = mgr_.RemoveByName(name);
        if (n == 0) {
            out_ << "  [ошибка] ничего не найдено\n";
            return false;
        }
        out_ << "  [ok] удалено записей: " << n << "\n";
        return true;
    }
    out_ << "  [ошибка] неизвестный способ\n";
    return false;
}

bool Menu::HandleListAll() {
    PrintTable(mgr_.All());
    return true;
}

bool Menu::HandleSearch() {
    out_ << "\n--- Поиск по названию ---\n";
    const auto needle = ReadNonEmpty("Подстрока названия: ");
    auto results = mgr_.SearchByName(needle);
    if (results.empty()) {
        out_ << "  ничего не найдено\n";
        return false;
    }
    PrintTable(results);
    return true;
}

bool Menu::HandleFilter() {
    out_ << "\n--- Фильтр (любой критерий можно пропустить) ---\n";

    const auto name = ReadLine("Подстрока названия (Enter — пропустить): ");

    out_ << "Категория (1-5, Enter — пропустить): ";
    std::string cat_s;
    std::getline(std::cin, cat_s);
    std::optional<Category> cat;
    if (!cat_s.empty()) {
        try {
            int c = std::stoi(cat_s);
            if (c < 1 || c > 5) {
                out_ << "  [ошибка] неверная категория\n";
                return false;
            }
            cat = static_cast<Category>(c - 1);
        } catch (...) {
            out_ << "  [ошибка] неверная категория\n";
            return false;
        }
    }

    DoubleRange cost;
    out_ << "Мин. стоимость (Enter — пропустить): ";
    std::string cmin_s; std::getline(std::cin, cmin_s);
    if (!cmin_s.empty()) {
        try { cost.min = std::stod(cmin_s); }
        catch (...) { out_ << "  [ошибка] некорректная стоимость\n"; return false; }
    }
    out_ << "Макс. стоимость (Enter — пропустить): ";
    std::string cmax_s; std::getline(std::cin, cmax_s);
    if (!cmax_s.empty()) {
        try { cost.max = std::stod(cmax_s); }
        catch (...) { out_ << "  [ошибка] некорректная стоимость\n"; return false; }
    }

    DateRange dates;
    out_ << "Дата от (YYYY-MM-DD, Enter — пропустить): ";
    std::string dmin; std::getline(std::cin, dmin);
    if (!dmin.empty()) dates.min = dmin;
    out_ << "Дата до (YYYY-MM-DD, Enter — пропустить): ";
    std::string dmax; std::getline(std::cin, dmax);
    if (!dmax.empty()) dates.max = dmax;

    out_ << "Статус (1-active, 2-paused, 3-cancelled, Enter — пропустить): ";
    std::string st_s; std::getline(std::cin, st_s);
    std::optional<Status> st;
    if (!st_s.empty()) {
        try {
            int c = std::stoi(st_s);
            if (c < 1 || c > 3) { out_ << "  [ошибка] неверный статус\n"; return false; }
            st = static_cast<Status>(c - 1);
        } catch (...) { out_ << "  [ошибка] неверный статус\n"; return false; }
    }

    auto results = mgr_.Filter(name, cat, cost, dates, st);
    if (results.empty()) {
        out_ << "  ничего не найдено\n";
        return false;
    }
    PrintTable(results);
    return true;
}

bool Menu::HandleSort() {
    out_ << "\n--- Сортировка ---\n";
    out_ << "  1) По стоимости (возрастание)\n"
            "  2) По стоимости (убывание)\n"
            "  3) По дате следующей оплаты (возрастание)\n"
            "  4) По дате следующей оплаты (убывание)\n"
            "  5) По алфавиту (А-Я)\n"
            "  6) По алфавиту (Я-А)\n";
    const int ch = ReadInt("> Способ: ");
    std::vector<Subscription> sorted;
    switch (ch) {
        case 1: sorted = mgr_.Sorted(SubscriptionManager::SortKey::kMonthlyCost,
                                      SubscriptionManager::SortOrder::kAscending); break;
        case 2: sorted = mgr_.Sorted(SubscriptionManager::SortKey::kMonthlyCost,
                                      SubscriptionManager::SortOrder::kDescending); break;
        case 3: sorted = mgr_.Sorted(SubscriptionManager::SortKey::kNextPaymentDate,
                                      SubscriptionManager::SortOrder::kAscending); break;
        case 4: sorted = mgr_.Sorted(SubscriptionManager::SortKey::kNextPaymentDate,
                                      SubscriptionManager::SortOrder::kDescending); break;
        case 5: sorted = mgr_.Sorted(SubscriptionManager::SortKey::kServiceName,
                                      SubscriptionManager::SortOrder::kAscending); break;
        case 6: sorted = mgr_.Sorted(SubscriptionManager::SortKey::kServiceName,
                                      SubscriptionManager::SortOrder::kDescending); break;
        default:
            out_ << "  [ошибка] неизвестный способ\n";
            return false;
    }
    PrintTable(sorted);
    return true;
}

bool Menu::HandleAnalytics() {
    out_ << "\n--- Аналитика ---\n";
    const auto today = TodayIsoDate();
    out_ << "  Дата отсчёта: " << today << "\n\n";

    const double total = mgr_.TotalMonthlySpend();
    out_ << "  Суммарные ежемесячные расходы (активные): "
         << static_cast<int>(total) << "₽\n\n";

    auto due = mgr_.DueWithinDays(7, today);
    if (due.empty()) {
        out_ << "  В ближайшие 7 дней оплат нет.\n\n";
    } else {
        out_ << "  Оплаты в ближайшие 7 дней:\n";
        PrintTable(due);
    }

    auto top = mgr_.TopCategories(3);
    if (top.empty()) {
        out_ << "  Нет активных подписок для топа категорий.\n\n";
    } else {
        out_ << "  Топ-3 категории по месячной стоимости:\n";
        for (std::size_t i = 0; i < top.size(); ++i) {
            out_ << "    " << (i + 1) << ") "
                 << CategoryRu(top[i].category)
                 << " — " << static_cast<int>(top[i].total_monthly) << "₽/мес"
                 << " (" << top[i].count << " подп.)\n";
        }
        out_ << '\n';
    }
    return true;
}

bool Menu::HandleSave() {
    try {
        mgr_.Save();
        out_ << "  [ok] сохранено в хранилище (" << mgr_.BackendName() << ")\n";
        return true;
    } catch (const std::exception& e) {
        out_ << "  [ошибка] не удалось сохранить: " << e.what() << "\n";
        return false;
    }
}

int Menu::Run() {
    out_ << "Subscription Tracker. Хранилище: " << mgr_.BackendName()
         << ". Записей: " << mgr_.Size() << "\n";
    while (true) {
        const int ch = MainMenuChoice();
        switch (ch) {
            case 0: return 0;
            case 1: HandleListAll();   break;
            case 2: HandleAdd();       break;
            case 3: HandleEdit();      break;
            case 4: HandleDelete();    break;
            case 5: HandleSearch();    break;
            case 6: HandleFilter();    break;
            case 7: HandleSort();      break;
            case 8: HandleAnalytics(); break;
            case 9: HandleSave();      break;
            default:
                out_ << "  [ошибка] неизвестный пункт меню\n";
                break;
        }
    }
}

}  // namespace subscription_tracker
