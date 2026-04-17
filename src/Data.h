#pragma once
#include <ArduinoJson.h>
#include <vector>
#include <cmath>
#include <algorithm>

#include "Core.h"



class Data {

    struct ParamType{
        unsigned long startTime; // время начала выполнения

        int cutsCount; // счетчик всех резов (включая технические и разделительные)
        int productCutsCount; // счетчик продуктовых резов (визиток)
        int cyclesCount; // текущий цикл - т.е. какой лист по счету

        int paperPosition = -1; // позиция мотора от края метки или бумаги
        int markPosition = -1; // Позиция метки которую нашли

        float delta_over; // дельта для вытелетов между продукцией

        float paperStepError = 0.0f;
        bool frame = false;
        bool checkFull = false;
        // --------------

        void reset(){
            startTime = 0;
            cutsCount = 0;
            productCutsCount = 0;
            cyclesCount = 0;

            paperPosition = -1;
            markPosition = -1;
            frame = false;
            delta_over = 0;
            paperStepError = 0.0f;
            checkFull = false;
        }
    };

    // Счетчики циклов и резов
    public: static ParamType param;

    private: struct Tuning {
        // Для обнаружения края бумаги
        float EDGE_DISTANCE_mm = 17.0f; // расстояние для прогонки бумаги к датчику для точного нахождения края
        // Для калибровки
        float SENSOR_DISTANCE_mm = 17.0f; // расстояние между датчиком и ножом
        float DELTA_mm = 0.0f; // погрешность расстояния между датчиком и ножом (указывает пользователь)
        float MARK_LENGHT_mm = 3.0f; // ширина метки
        float MARK_CENTER_DISTANCE_mm = 9.5f; // расстояние от переднего края листа до центра метки
        float OFFSET_FIRSTCUT_MM = 8.0f; // расстояние от переднего края до линии реза для автоподбора DELTA
        float OFFSET_TOL_MM = 0.15f; // допуск ошибки попадания ножа при автокалибровке
        int OFFSET_MAX_ITER = 2; // максимум итераций автоподбора DELTA
        float OVER_mm = 2.0f; // Вылет
        float DISTANCE_BETWEEN_MARKS_mm = 40.0f; //  дистанция между метками в мм
        int CUT_count = 12; // Кол-во калибровочных резов

        // Для профилирования
        int PROFILE_WIDTH_step = 2000; // Сколько микрошагов надо отмотать что бы потом померять линейкой и высчитать Коэф (сколько шагов в 1 мм)
        int PROFILE_COUNT_CUT = 5; // кол-во полосок которое для калибровки надо сделать

        // Загрузка секции calibration
        bool load() {
            // Получаем объект калибровки
            JsonObjectConst obj = Core::config.doc["tuning"];
            if (obj.isNull()) {
                Log::D("Секция tuning не найдена, используются значения по умолчанию");
                return true;
            }

            // Заполняем структуру с проверками
            tuning.EDGE_DISTANCE_mm = obj["EDGE_DISTANCE_mm"] | 17.0f;
            tuning.SENSOR_DISTANCE_mm = obj["SENSOR_DISTANCE_mm"] | 17.0f;
            tuning.MARK_LENGHT_mm = obj["MARK_LENGHT_mm"] | 3.0f;
            tuning.MARK_CENTER_DISTANCE_mm = obj["MARK_CENTER_DISTANCE_mm"] | 9.5f;
            tuning.OFFSET_FIRSTCUT_MM = obj["OFFSET_FIRSTCUT_MM"] | 8.0f;
            tuning.OFFSET_TOL_MM = obj["OFFSET_TOL_MM"] | 0.15f;
            tuning.OFFSET_MAX_ITER = obj["OFFSET_MAX_ITER"] | 2;
            //tuning.FIRST_CUT_mm = obj["FIRST_CUT_mm"] | 8.0f;
            tuning.DISTANCE_BETWEEN_MARKS_mm = obj["DISTANCE_BETWEEN_MARKS_mm"] | 40.0f;
            tuning.OVER_mm = obj["OVER_mm"] | 2.0f;
            //tuning.PAPER_LENGHT_mm = obj["PAPER_LENGHT_mm"] | 450.0f;
            tuning.CUT_count = obj["CUT_count"] | 12;
            tuning.PROFILE_WIDTH_step = obj["PROFILE_WIDTH_step"] | 2000;
            tuning.PROFILE_COUNT_CUT = obj["PROFILE_COUNT_CUT"] | 5;
            tuning.DELTA_mm = obj["DELTA_mm"] | 0.0f;

            // Проверка критических значений
            if (tuning.CUT_count < 1 ||
                //tuning.PAPER_LENGHT_mm <= 0 ||
                tuning.MARK_CENTER_DISTANCE_mm <= 0 ||
                tuning.OFFSET_FIRSTCUT_MM <= 0 ||
                tuning.OFFSET_TOL_MM <= 0 ||
                tuning.OFFSET_MAX_ITER <= 0 ||
                tuning.DISTANCE_BETWEEN_MARKS_mm <= 0 ||
                tuning.PROFILE_WIDTH_step <= 0 ||
                tuning.PROFILE_COUNT_CUT <= 0)
            {
                Log::D("Невалидные параметры tuning");
                return false;
            }

            return true;
        }
        void save() {
            JsonObject tuningObj = Core::config.doc["tuning"].to<JsonObject>();
            tuning.serialize(tuningObj);
            Core::config.save();
        }
        void serialize(JsonObject& obj) {
            obj["SENSOR_DISTANCE_mm"] = tuning.SENSOR_DISTANCE_mm;
            obj["MARK_LENGHT_mm"] = tuning.MARK_LENGHT_mm;
            obj["MARK_CENTER_DISTANCE_mm"] = tuning.MARK_CENTER_DISTANCE_mm;
            obj["OFFSET_FIRSTCUT_MM"] = tuning.OFFSET_FIRSTCUT_MM;
            obj["OFFSET_TOL_MM"] = tuning.OFFSET_TOL_MM;
            obj["OFFSET_MAX_ITER"] = tuning.OFFSET_MAX_ITER;
            //obj["FIRST_CUT_mm"] = tuning.FIRST_CUT_mm;
            obj["DISTANCE_BETWEEN_MARKS_mm"] = tuning.DISTANCE_BETWEEN_MARKS_mm;
            obj["OVER_mm"] = tuning.OVER_mm;
            //obj["PAPER_LENGHT_mm"] = tuning.PAPER_LENGHT_mm;
            obj["CUT_count"] = tuning.CUT_count;
            obj["PROFILE_WIDTH_step"] = tuning.PROFILE_WIDTH_step;
            obj["PROFILE_COUNT_CUT"] = tuning.PROFILE_COUNT_CUT;
            obj["DELTA_mm"] = tuning.DELTA_mm;
            obj["EDGE_DISTANCE_mm"] = tuning.EDGE_DISTANCE_mm;
        }
        void print(){ Log::D("Tuning:"); Log::D("SENSOR_DISTANCE_mm: %f, MARK_LENGHT_mm: %f, MARK_CENTER_DISTANCE_mm: %f, OFFSET_FIRSTCUT_MM: %f, OFFSET_TOL_MM: %f, OFFSET_MAX_ITER: %d, OVER_mm: %f, DISTANCE_BETWEEN_MARKS_mm: %f, CUT_count: %d, PROFILE_WIDTH_step: %d, PROFILE_COUNT_CUT: %d, DELTA_mm: %f, EDGE_DISTANCE_mm: %f",SENSOR_DISTANCE_mm,MARK_LENGHT_mm,MARK_CENTER_DISTANCE_mm,OFFSET_FIRSTCUT_MM,OFFSET_TOL_MM,OFFSET_MAX_ITER,OVER_mm,DISTANCE_BETWEEN_MARKS_mm,CUT_count,PROFILE_WIDTH_step,PROFILE_COUNT_CUT,DELTA_mm,EDGE_DISTANCE_mm);}
    };
    public: static Tuning tuning;


    // #################################################################################
    // Структуры данных
    private: struct Profile {
        long ID;
        String NAME;
        float RATIO_mm;
        String DESC;
        float LENGHT_mm;
        //int MARK;

        void setID(long maxID){ID = maxID+1;}
        void clear(){ID = -1;NAME="";DESC="";RATIO_mm=0.0;LENGHT_mm=0.0;}
        bool valid(){if (ID == -1) return false; else return true;}
        // Сериализация одного профиля
        void serialize(JsonObject& obj) {
            obj["ID"] = ID;
            obj["NAME"] = NAME;
            obj["RATIO_mm"] = RATIO_mm;
            obj["DESC"] = DESC;
            obj["LENGHT_mm"] = LENGHT_mm;
            //obj["MARK"] = MARK;
        }
        void print() {
            Log::D("{ ID: '%ld', NAME: '%s', RATIO_mm:'%f', DESC:'%s', LENGHT_mm:'%f' }",ID,NAME.c_str(),RATIO_mm,DESC.c_str(),LENGHT_mm);
        }
    };
    private: struct Profiles {
        std::vector<Profile> items;
        int countItems, curentPage, pageSize, totalPages;
        Profile* ListItems; // для поиска

        void load() {
            countItems = 0;
            if (!Core::data.doc["profiles"].is<JsonArray>()) return;

            JsonArray Array = Core::data.doc["profiles"];
            int maxProfiles = Core::config.doc["settings"]["MAX_PROFILES"] | 50;
            items.reserve(maxProfiles); // Резервируем место для оптимизации
            for (JsonObject profile : Array) {
                if (countItems >= maxProfiles) break;
                items.emplace_back(); // Добавляем новый элемент и получаем ссылку на него
                items[countItems].ID = profile["ID"].as<long>();
                items[countItems].NAME = profile["NAME"].as<String>();
                items[countItems].DESC = profile["DESC"].as<String>();
                items[countItems].RATIO_mm = profile["RATIO_mm"].as<float>();
                items[countItems].LENGHT_mm = profile["LENGHT_mm"].as<float>();
                countItems++;
            }

            curentPage = 0;
            pageSize = 5;
            countPages();
        }
        int countPages(){totalPages = (countItems + pageSize - 1) / pageSize; return totalPages;}
        void add(const Profile& profile) {
            Log::D(__func__);
            int maxProfiles = Core::config.doc["settings"]["MAX_PROFILES"] | 50;
            if (countItems >= maxProfiles) return;
            items.push_back(profile);
            countItems++;
            save();
        }
        void edit(const Profile& profile)
        {
            for (int i = 0; i < countItems; i++)
            {
                if (items[i].ID == profile.ID)
                {
                    items[i] = profile;
                    break;
                }
            }
            save();
        }
        void remove(const long &ID)
        {
            for (int i = 0; i < countItems; i++)
            {
                if (items[i].ID == ID)
                {
                    for (int j = i; j < countItems - 1; j++)
                    {
                        items[j] = items[j + 1];
                    }
                    countItems--;
                    break;
                }
            }
            save();
        }
        void save() {
            JsonArray profilesArray = Core::data.doc["profiles"].to<JsonArray>();
            profiles.serialize(profilesArray);
            Core::data.save();
        }
        void serialize(JsonArray& array) {
            array.clear();  // Очистка перед заполнением
            // Проходим по всем профилям
            for (int i = 0; i < countItems; i++) {
                JsonObject itemObj = array.add<JsonObject>();
                items[i].serialize(itemObj);
            }
        }
        void print() {
            for (int i = 0; i < countItems; i++) {items[i].print();}
        }
        Profile* getPage(int &outItemCount)
        {
            int startIndex = curentPage * pageSize;
            if (startIndex >= countItems)
            {
                outItemCount = 0;
                return nullptr;
            }
            int endIndex = min(startIndex + pageSize, countItems);
            outItemCount = endIndex - startIndex;
            return &items[startIndex];
        }
        void getByPage(int numberPage, Profile& search) {
            if (pageSize <= 0 || numberPage < 0 || numberPage >= pageSize) {
                search.clear();
                return;
            }

            int startIndex = curentPage * pageSize;
            if (startIndex >= countItems) {
                search.clear();
                return;
            }

            int index = startIndex + numberPage;
            if (index >= countItems) {
                search.clear();
                return;
            }

            search=items[index];
        }
        void getByID(const long &ID, Profile& search)
        {
            search.clear();
            for (int i = 0; i < countItems; i++)
            {
                if (items[i].ID == ID) {search=items[i]; break;}
            }
        }
        bool getNameByID(const long &ID, String& outName)
        {
            for (int i = 0; i < countItems; i++)
            {
                if (items[i].ID == ID) {outName = items[i].NAME; return true;}
            }
            outName = "";
            return false;
        }
        long maxID() {
            if (countItems == 0) {return -1;}
            long maxID = items[0].ID; // Берём ID первого элемента как начальное максимальное значение
            for (int i = 1; i < countItems; ++i) {if (items[i].ID > maxID) {maxID = items[i].ID;}}
            return maxID;
        }
    };
    public: static Profiles profiles;

    // #################################################################################
    private: struct Task {
        long ID;
        String NAME;
        long PROFILE_ID;
        float OVER_mm;
        float PRODUCT_mm;
        float FIRST_CUT_mm; // расстояние от края листа до первого реза, если без метки
        float MARK_mm; // Длина метки, если резем с меткой
        float LASTCUT_mm; // расстояние до последнего реза
        int MARK;

        void setID(long maxID){ID = maxID+1;}
        void clear(){ID = -1;NAME="";PROFILE_ID=-1;OVER_mm=0;PRODUCT_mm=0;MARK=0;FIRST_CUT_mm=0;MARK_mm=0;LASTCUT_mm=0;}
        bool valid(){if (ID == -1) return false; else return true;}
        // Сериализация
        void serialize(JsonObject& obj) {
            obj["ID"] = ID;
            obj["NAME"] = NAME;
            obj["PROFILE_ID"] = PROFILE_ID;
            obj["OVER_mm"] = OVER_mm;
            obj["PRODUCT_mm"] = PRODUCT_mm;
            obj["FIRST_CUT_mm"] = FIRST_CUT_mm;
            obj["MARK_mm"] = MARK_mm;
            obj["LASTCUT_mm"] = LASTCUT_mm;
            obj["MARK"] = MARK;
        }
        void print() {
            Log::D("{ ID: '%ld', NAME: '%s', PROFILE_ID: '%ld', OVER_mm: '%f', PRODUCT_mm: '%f', MARK:'%d', MARK_mm:'%f', FIRST_CUT_mm:'%f', LASTCUT_mm:'%f' }",ID,NAME.c_str(),PROFILE_ID,OVER_mm,PRODUCT_mm,MARK,MARK_mm,FIRST_CUT_mm,LASTCUT_mm);
        }
    };
    private: struct Tasks {
        std::vector<Task> items;
        int countItems, curentPage, pageSize, totalPages;
        Task* ListItems; // для поиска

        void load() {
            countItems = 0;
            if (!Core::data.doc["tasks"].is<JsonArray>()) return;

            JsonArray Array = Core::data.doc["tasks"];
            int maxTasks = Core::config.doc["settings"]["MAX_TASKS"] | 50;
            items.reserve(maxTasks); // Резервируем место для оптимизации
            for (JsonObject task : Array) {
                if (countItems >= maxTasks) break;
                items.emplace_back(); // Добавляем новый элемент и получаем ссылку на него
                items[countItems].ID = task["ID"].as<long>();
                items[countItems].NAME = task["NAME"].as<String>();
                items[countItems].PROFILE_ID = task["PROFILE_ID"].as<long>();
                items[countItems].PRODUCT_mm = task["PRODUCT_mm"].as<float>();
                items[countItems].FIRST_CUT_mm = task["FIRST_CUT_mm"].as<float>();
                items[countItems].MARK_mm = task["MARK_mm"].as<float>();
                items[countItems].LASTCUT_mm = task["LASTCUT_mm"].as<float>();
                items[countItems].MARK = task["MARK"].as<int>();
                items[countItems].OVER_mm = task["OVER_mm"].as<float>();
                countItems++;
            }

            curentPage = 0;
            pageSize = 5;
            countPages();
        }
        int countPages(){totalPages = (countItems + pageSize - 1) / pageSize; return totalPages;}
        void add(const Task& task) {
            Log::D(__func__);
            int maxTasks = Core::config.doc["settings"]["MAX_TASKS"] | 50;
            if (countItems >= maxTasks) return;
            items.push_back(task);
            countItems++;
            save();
        }
        void edit(const Task& task)
        {
            for (int i = 0; i < countItems; i++)
            {
                if (items[i].ID == task.ID)
                {
                    items[i] = task;
                    break;
                }
            }
            save();
        }
        void remove(const long &ID)
        {
            for (int i = 0; i < countItems; i++)
            {
                if (items[i].ID == ID)
                {
                    for (int j = i; j < countItems - 1; j++)
                    {
                        items[j] = items[j + 1];
                    }
                    countItems--;
                    break;
                }
            }
            save();
        }
        void save() {
            JsonArray profilesArray = Core::data.doc["tasks"].to<JsonArray>();
            tasks.serialize(profilesArray);
            Core::data.save();
        }
        void serialize(JsonArray& array) {
            array.clear();  // Очистка перед заполнением
            // Проходим по всем профилям
            for (int i = 0; i < countItems; i++) {
                JsonObject itemObj = array.add<JsonObject>();
                items[i].serialize(itemObj);
            }
        }
        void print() {
            for (int i = 0; i < countItems; i++) {items[i].print();}
        }
        Task* getPage(int &outItemCount)
        {
            int startIndex = curentPage * pageSize;
            if (startIndex >= countItems)
            {
                outItemCount = 0;
                return nullptr;
            }
            int endIndex = min(startIndex + pageSize, countItems);
            outItemCount = endIndex - startIndex;
            return &items[startIndex];
        }
        void getByPage(int numberPage, Task& search) {
            if (pageSize <= 0 || numberPage < 0 || numberPage >= pageSize) {
                search.clear();
                return;
            }

            int startIndex = curentPage * pageSize;
            if (startIndex >= countItems) {
                search.clear();
                return;
            }

            int index = startIndex + numberPage;
            if (index >= countItems) {
                search.clear();
                return;
            }

            search=items[index];
        }
        void getByID(const long &ID, Task& search)
        {
            search.clear();
            for (int i = 0; i < countItems; i++)
            {
                if (items[i].ID == ID) {search=items[i]; break;}
            }
        }
        bool getNameByID(const long &ID, String& outName)
        {
            for (int i = 0; i < countItems; i++)
            {
                if (items[i].ID == ID) {outName = items[i].NAME; return true;}
            }
            outName = "";
            return false;
        }
        long maxID() {
            if (countItems == 0) {return -1;}
            long maxID = items[0].ID; // Берём ID первого элемента как начальное максимальное значение
            for (int i = 1; i < countItems; ++i) {if (items[i].ID > maxID) {maxID = items[i].ID;}}
            return maxID;
        }
    };
    public: static Tasks tasks;

    // #################################################################################
    // Рабочий процесс
    private: struct Work {
        Profile profile;
        Task task;
        int TOTAL_CYCLES; // кол-во листов
        int TOTAL_CUTS;  // кол-во резов

        void clear(){profile.clear(); task.clear(); TOTAL_CYCLES=0; TOTAL_CUTS=0;}
        bool valid(){return profile.valid() && task.valid();}
        void print(){ Log::D("profile:"); profile.print(); Log::D("task:");task.print(); Log::D("TOTAL_CYCLES: %d, TOTAL_CUTS: %d",TOTAL_CYCLES,TOTAL_CUTS);}
    };
    public: static Work work;

    // Произвести расчет кол-ва резов на лист
    static int getCUTs_count(){

        float paper = Data::work.profile.LENGHT_mm;
        float product = Data::work.task.PRODUCT_mm + (Data::work.task.OVER_mm * 2);

        if (Data::work.task.MARK){
            paper = paper - Data::work.task.FIRST_CUT_mm; // TODO - мы не знаем длину до метки!  предположим что она такая
            paper = paper - Data::work.task.MARK_mm;
        }
        else {
            paper = paper - Data::work.task.FIRST_CUT_mm;
        }
        return (paper/product);
    }
    static int getFinishCUTs_count(){

        float paper = Data::work.profile.LENGHT_mm;
        if (Data::work.task.MARK){
            paper = paper - Data::work.task.FIRST_CUT_mm; // TODO - мы не знаем длину до метки!  предположим что она такая
            paper = paper - Data::work.task.MARK_mm;
        }
        else {
            paper = paper - Data::work.task.FIRST_CUT_mm;
        }
        float product = Data::work.task.PRODUCT_mm + (Data::work.task.OVER_mm * 2);
        int CUTs_count = getCUTs_count();
        paper = paper - (product*CUTs_count);
        if (paper<0) paper = 0;
        return paper / 15; // TODO - сделать параметром на какие кусочки длинной резать хвост бумаги
    }

};
