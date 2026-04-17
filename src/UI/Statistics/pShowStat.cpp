#include "pShowStat.h"
#include "Catalog.h"
#include "pStatistics.h"

#include "Data.h"
#include "Service/Stats.h"

namespace {
    String formatCount(uint64_t value, const char* unit) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%llu", static_cast<unsigned long long>(value));
        String result(buffer);
        if (unit && unit[0] != '\0') {
            result += " ";
            result += unit;
        }
        return result;
    }

    String formatMeters(uint64_t um) {
        float meters = static_cast<float>(um) / 1000000.0f;
        String result(meters, 2);
        result += " м";
        return result;
    }

    String getTaskName(long id) {
        String name;
        if (Data::tasks.getNameByID(id, name)) return name;
        return String("ID ") + String(id);
    }

    String getProfileName(long id) {
        String name;
        if (Data::profiles.getNameByID(id, name)) return name;
        return String("ID ") + String(id);
    }
}

void pShowStat::showDeviceMotors() {
    nav.clear();
    setView(ViewMode::DeviceMotors, -1, -1, false);
}

void pShowStat::showTasks() {
    nav.clear();
    setView(ViewMode::Tasks, -1, -1, false);
}

void pShowStat::showProfiles() {
    nav.clear();
    setView(ViewMode::Profiles, -1, -1, false);
}

void pShowStat::setView(ViewMode newView, long taskId, long profileId, bool push) {
    if (push) {
        ViewState state;
        state.view = view;
        state.taskId = viewTaskId;
        state.profileId = viewProfileId;
        state.page = page;
        nav.push_back(state);
    }

    view = newView;
    viewTaskId = taskId;
    viewProfileId = profileId;
    page = 0;

    buildRows();
    renderPage();
}

void pShowStat::buildRows() {
    rows.clear();

    Stats& stats = Stats::getInstance();

    switch (view) {
        case ViewMode::DeviceMotors: {
            title = "Моторы";
            field1 = "Мотор";
            field2 = "Пробег";
            auto motors = stats.getDeviceMotors();
            for (const auto& motor : motors) {
                Row row;
                row.label = motor.name;
                row.value = formatCount(motor.steps, "шаг");
                rows.push_back(row);
            }
            break;
        }
        case ViewMode::Tasks: {
            title = "Задания";
            field1 = "Задание";
            field2 = "Запусков";
            auto tasks = stats.getTaskRuns();
            for (const auto& task : tasks) {
                Row row;
                row.label = getTaskName(task.id);
                row.value = formatCount(task.runs, "кол-во");
                row.action.type = ActionType::OpenTaskDetail;
                row.action.taskId = task.id;
                rows.push_back(row);
            }
            break;
        }
        case ViewMode::Profiles: {
            title = "Профили";
            field1 = "Профиль";
            field2 = "Запусков";
            auto profiles = stats.getProfileRuns();
            for (const auto& profile : profiles) {
                Row row;
                row.label = getProfileName(profile.id);
                row.value = formatCount(profile.runs, "кол-во");
                row.action.type = ActionType::OpenProfileDetailGlobal;
                row.action.profileId = profile.id;
                rows.push_back(row);
            }
            break;
        }
        case ViewMode::TaskDetail: {
            title = getTaskName(viewTaskId);
            field1 = "Метрика";
            field2 = "Значение";

            Stats::AggregateStat agg;
            stats.getTaskAggregate(viewTaskId, agg);

            Row row;
            row.label = "Запусков";
            row.value = formatCount(agg.runs, "");
            rows.push_back(row);

            row.label = "Продуктов";
            row.value = formatCount(agg.products, "шт");
            rows.push_back(row);

            row.label = "Листов";
            row.value = formatCount(agg.sheets, "шт");
            rows.push_back(row);

            row.label = "Длина бумаги";
            row.value = formatMeters(agg.paper_um);
            rows.push_back(row);

            row.label = "Отказов";
            row.value = formatCount(agg.rejects, "");
            rows.push_back(row);

            row.label = "Профилей";
            row.value = formatCount(agg.profiles, "кол-во");
            row.action.type = ActionType::OpenTaskProfiles;
            row.action.taskId = viewTaskId;
            rows.push_back(row);

            row = Row();
            row.label = "Моторы";
            row.value = "Показать";
            row.action.type = ActionType::OpenTaskMotors;
            row.action.taskId = viewTaskId;
            rows.push_back(row);
            break;
        }
        case ViewMode::TaskProfiles: {
            title = getTaskName(viewTaskId);
            field1 = "Профиль";
            field2 = "Запусков";
            auto profiles = stats.getTaskProfileRuns(viewTaskId);
            for (const auto& profile : profiles) {
                Row row;
                row.label = getProfileName(profile.id);
                row.value = formatCount(profile.runs, "кол-во");
                row.action.type = ActionType::OpenProfileDetailTask;
                row.action.taskId = viewTaskId;
                row.action.profileId = profile.id;
                rows.push_back(row);
            }
            break;
        }
        case ViewMode::TaskMotors: {
            title = "Моторы";
            field1 = "Мотор";
            field2 = "Пробег";

            Stats::AggregateStat agg;
            stats.getTaskAggregate(viewTaskId, agg);
            for (const auto& motor : agg.motors) {
                Row row;
                row.label = motor.name;
                row.value = formatCount(motor.steps, "шаг");
                rows.push_back(row);
            }
            break;
        }
        case ViewMode::ProfileDetailGlobal: {
            title = getProfileName(viewProfileId);
            field1 = "Метрика";
            field2 = "Значение";

            Stats::AggregateStat agg;
            stats.getProfileAggregate(viewProfileId, agg);

            Row row;
            row.label = "Запусков";
            row.value = formatCount(agg.runs, "");
            rows.push_back(row);

            row.label = "Продуктов";
            row.value = formatCount(agg.products, "шт");
            rows.push_back(row);

            row.label = "Листов";
            row.value = formatCount(agg.sheets, "шт");
            rows.push_back(row);

            row.label = "Длина бумаги";
            row.value = formatMeters(agg.paper_um);
            rows.push_back(row);

            row.label = "Отказов";
            row.value = formatCount(agg.rejects, "");
            rows.push_back(row);

            row = Row();
            row.label = "Моторы";
            row.value = "Показать";
            row.action.type = ActionType::OpenProfileMotorsGlobal;
            row.action.profileId = viewProfileId;
            rows.push_back(row);
            break;
        }
        case ViewMode::ProfileDetailTask: {
            title = getProfileName(viewProfileId);
            field1 = "Метрика";
            field2 = "Значение";

            Stats::AggregateStat agg;
            stats.getTaskProfileAggregate(viewTaskId, viewProfileId, agg);

            Row row;
            row.label = "Запусков";
            row.value = formatCount(agg.runs, "");
            rows.push_back(row);

            row.label = "Продуктов";
            row.value = formatCount(agg.products, "шт");
            rows.push_back(row);

            row.label = "Листов";
            row.value = formatCount(agg.sheets, "шт");
            rows.push_back(row);

            row.label = "Длина бумаги";
            row.value = formatMeters(agg.paper_um);
            rows.push_back(row);

            row.label = "Отказов";
            row.value = formatCount(agg.rejects, "");
            rows.push_back(row);

            row = Row();
            row.label = "Моторы";
            row.value = "Показать";
            row.action.type = ActionType::OpenProfileMotorsTask;
            row.action.taskId = viewTaskId;
            row.action.profileId = viewProfileId;
            rows.push_back(row);
            break;
        }
        case ViewMode::ProfileMotorsGlobal: {
            title = "Моторы";
            field1 = "Мотор";
            field2 = "Пробег";

            Stats::AggregateStat agg;
            stats.getProfileAggregate(viewProfileId, agg);
            for (const auto& motor : agg.motors) {
                Row row;
                row.label = motor.name;
                row.value = formatCount(motor.steps, "шаг");
                rows.push_back(row);
            }
            break;
        }
        case ViewMode::ProfileMotorsTask: {
            title = "Моторы";
            field1 = "Мотор";
            field2 = "Пробег";

            Stats::AggregateStat agg;
            stats.getTaskProfileAggregate(viewTaskId, viewProfileId, agg);
            for (const auto& motor : agg.motors) {
                Row row;
                row.label = motor.name;
                row.value = formatCount(motor.steps, "шаг");
                rows.push_back(row);
            }
            break;
        }
    }
}

void pShowStat::renderPage() {
    if (!isShow) {
        Page::show();
    }

    tTitle.setText(title.c_str());
    tField1.setText(field1.c_str());
    tField2.setText(field2.c_str());

    clearRows();

    if (rows.empty()) {
        totalPages = 1;
    } else {
        totalPages = static_cast<uint8_t>((rows.size() + kPageSize - 1) / kPageSize);
    }
    if (page >= totalPages) page = totalPages - 1;

    const size_t start = static_cast<size_t>(page) * kPageSize;
    for (int i = 0; i < kPageSize; i++) {
        const size_t index = start + static_cast<size_t>(i);
        if (index >= rows.size()) break;
        paramFields[i]->setText(rows[index].label.c_str());
        valueFields[i]->setText(rows[index].value.c_str());
        if (rows[index].action.type != ActionType::None) valueFields[i]->Set_background_color_bco(55131); // если ячейка активная то выделяем ее цветом 
    }

    updateNextButton();
}

void pShowStat::clearRows() {
    for (int i = 0; i < kPageSize; i++) {
        paramFields[i]->setText("");
        valueFields[i]->setText("");
        valueFields[i]->Set_background_color_bco(Catalog::Color::white); // стираем цвета
    }
}

void pShowStat::updateNextButton() {
    bool showNext = (rows.size() > 0) && (page + 1 < totalPages);
    Page::setVisible(bNextPage, showNext);
}

void pShowStat::handleAction(const RowAction& action) {
    switch (action.type) {
        case ActionType::None:
            return;
        case ActionType::OpenTaskDetail:
            setView(ViewMode::TaskDetail, action.taskId, -1, true);
            return;
        case ActionType::OpenTaskProfiles:
            setView(ViewMode::TaskProfiles, action.taskId, -1, true);
            return;
        case ActionType::OpenTaskMotors:
            setView(ViewMode::TaskMotors, action.taskId, -1, true);
            return;
        case ActionType::OpenProfileDetailGlobal:
            setView(ViewMode::ProfileDetailGlobal, -1, action.profileId, true);
            return;
        case ActionType::OpenProfileDetailTask:
            setView(ViewMode::ProfileDetailTask, action.taskId, action.profileId, true);
            return;
        case ActionType::OpenProfileMotorsGlobal:
            setView(ViewMode::ProfileMotorsGlobal, -1, action.profileId, true);
            return;
        case ActionType::OpenProfileMotorsTask:
            setView(ViewMode::ProfileMotorsTask, action.taskId, action.profileId, true);
            return;
    }
}

int pShowStat::rowIndexFromPtr(void* ptr) const {
    for (int i = 0; i < kPageSize; i++) {
        if (ptr == paramFields[i] || ptr == valueFields[i]) return i;
    }
    return -1;
}

void pShowStat::nextPage() {
    if (page + 1 < totalPages) {
        page++;
        renderPage();
    }
}

void pShowStat::goBack() {
    if (page > 0) {
        page--;
        renderPage();
        return;
    }
    if (!nav.empty()) {
        ViewState state = nav.back();
        nav.pop_back();
        view = state.view;
        viewTaskId = state.taskId;
        viewProfileId = state.profileId;
        page = state.page;
        buildRows();
        renderPage();
        return;
    }
    pStatistics::getInstance().show();
}

void pShowStat::pop_bBackMainMenu(void* ptr) {
    Log::D(__func__);
    pShowStat::getInstance().goBack();
}

void pShowStat::pop_bNextPage(void* ptr) {
    Log::D(__func__);
    pShowStat::getInstance().nextPage();
}

void pShowStat::pop_row(void* ptr) {
    pShowStat& UI = pShowStat::getInstance();
    int localIndex = UI.rowIndexFromPtr(ptr);
    if (localIndex < 0) return;
    size_t index = static_cast<size_t>(UI.page) * kPageSize + static_cast<size_t>(localIndex);
    if (index >= UI.rows.size()) return;
    UI.handleAction(UI.rows[index].action);
}
