#pragma once

// =================================================================
// =                      Пины ESP32                               =
// =================================================================

// --- Управление моторами (STEP сигналы) ---
#define MOTOR_STOL_STEP_PIN 23      // Мотор 1 (Стол)
#define MOTOR_PROTYAZHKA_STEP_PIN 19  // Мотор 2 (Протяжка)
#define MOTOR_GILOTINA_STEP_PIN 18    // Мотор 3 (Гильотина)
#define MOTOR_VIBROS_STEP_PIN 5       // Мотор 4 (Выброс)
#define MOTOR_NOZH_1_STEP_PIN 4       // Мотор 5 (Нож 1)
#define MOTOR_NOZH_2_STEP_PIN 15      // Мотор 6 (Нож 2)
#define MOTOR_NOZH_3_STEP_PIN 32      // Мотор 7 (Нож 3)
#define MOTOR_NOZH_4_STEP_PIN 33      // Мотор 8 (Нож 4)
#define MOTOR_9_STEP_PIN 25       // Мотор 9
#define MOTOR_10_STEP_PIN 26      // Мотор 10
#define MOTOR_11_STEP_PIN 27      // Мотор 11
#define MOTOR_12_STEP_PIN 14      // Мотор 12
#define MOTOR_13_STEP_PIN 12      // Мотор 13
#define MOTOR_14_STEP_PIN 13      // Мотор 14

// --- I2C Шина для MCP23017 ---
#define I2C_SDA_PIN 21              // I2C SDA
#define I2C_SCL_PIN 22              // I2C SCL

// --- UART для экрана ---
#define TFT_RX_PIN 17               // RX экрана
#define TFT_TX_PIN 16               // TX экрана

// --- Входы датчиков и прерываний ---
#define SENSOR_P2_PIN 2             // Датчик P_2
#define SENSOR_P35_PIN 35            // Датчик P_35
#define ANALOG_LINE_PIN 36          // Аналоговый датчик "Линия" (VP)
#define MCP3_INTA_PIN 39            // Прерывание с 3го MCP23017 pin INTA (VN)
#define MCP3_INTB_PIN 34            // Прерывание с 3го MCP23017 pin INTB

// =================================================================
// =                      Пины MCP1 (MCP0 в коде)                  =
// =================================================================
// Управление DIR/ENABLE для моторов 1-8

// --- Порт A ---
#define MCP1_MOTOR_STOL_DIR_PIN 0     // Мотор 1 (Стол) - DIR
#define MCP1_MOTOR_STOL_EN_PIN 1      // Мотор 1 (Стол) - ENABLE
#define MCP1_MOTOR_PROTYAZHKA_DIR_PIN 2 // Мотор 2 (Протяжка) - DIR
#define MCP1_MOTOR_PROTYAZHKA_EN_PIN 3  // Мотор 2 (Протяжка) - ENABLE
#define MCP1_MOTOR_GILOTINA_DIR_PIN 4   // Мотор 3 (Гильотина) - DIR
#define MCP1_MOTOR_GILOTINA_EN_PIN 5    // Мотор 3 (Гильотина) - ENABLE
#define MCP1_MOTOR_VIBROS_DIR_PIN 6     // Мотор 4 (Выброс) - DIR
#define MCP1_MOTOR_VIBROS_EN_PIN 7      // Мотор 4 (Выброс) - ENABLE

// --- Порт B ---
#define MCP1_MOTOR_NOZH_1_DIR_PIN 8   // Мотор 5 (Нож 1) - DIR
#define MCP1_MOTOR_NOZH_1_EN_PIN 9    // Мотор 5 (Нож 1) - ENABLE
#define MCP1_MOTOR_NOZH_2_DIR_PIN 10   // Мотор 6 (Нож 2) - DIR
#define MCP1_MOTOR_NOZH_2_EN_PIN 11    // Мотор 6 (Нож 2) - ENABLE
#define MCP1_MOTOR_NOZH_3_DIR_PIN 12   // Мотор 7 (Нож 3) - DIR
#define MCP1_MOTOR_NOZH_3_EN_PIN 13    // Мотор 7 (Нож 3) - ENABLE
#define MCP1_MOTOR_NOZH_4_DIR_PIN 14   // Мотор 8 (Нож 4) - DIR
#define MCP1_MOTOR_NOZH_4_EN_PIN 15    // Мотор 8 (Нож 4) - ENABLE

// =================================================================
// =                      Пины MCP2 (MCP1 в коде)                  =
// =================================================================
// Управление DIR/ENABLE для моторов 9-14 и муфтами

// --- Порт A ---
#define MCP2_MOTOR_9_DIR_PIN 0        // Мотор 9 - DIR
#define MCP2_MOTOR_9_EN_PIN 1         // Мотор 9 - ENABLE
#define MCP2_MOTOR_10_DIR_PIN 2       // Мотор 10 - DIR
#define MCP2_MOTOR_10_EN_PIN 3        // Мотор 10 - ENABLE
#define MCP2_MOTOR_11_DIR_PIN 4       // Мотор 11 - DIR
#define MCP2_MOTOR_11_EN_PIN 5        // Мотор 11 - ENABLE
#define MCP2_MOTOR_12_DIR_PIN 6       // Мотор 12 - DIR
#define MCP2_MOTOR_12_EN_PIN 7        // Мотор 12 - ENABLE

// --- Порт B ---
#define MCP2_MOTOR_13_DIR_PIN 8       // Мотор 13 - DIR
#define MCP2_MOTOR_13_EN_PIN 9        // Мотор 13 - ENABLE
#define MCP2_MOTOR_14_DIR_PIN 10       // Мотор 14 - DIR
#define MCP2_MOTOR_14_EN_PIN 11        // Мотор 14 - ENABLE
#define MUFTA_1_PIN 12                 // Муфта 1
#define MUFTA_2_PIN 13                 // Муфта 2
#define MUFTA_3_PIN 14                 // Муфта 3
#define MUFTA_4_PIN 15                 // Муфта 4

// =================================================================
// =                      Пины MCP3 (MCP2 в коде)                  =
// =================================================================
// Входы с датчиков и кнопок

// --- Порт A ---
#define SENSOR_STOL_PIN 0             // Датчик Стол (T_300)
#define SENSOR_BUMAGA_PIN 1           // Датчик Бумага (P_301)
#define SENSOR_GIL_NA_1_PIN 2         // Датчик Гиль-на 1 (G_302)
#define SENSOR_GIL_NA_2_PIN 3         // Датчик Гиль-на 2 (G_303)
#define SENSOR_NOZH_1_PIN 4           // Датчик Нож 1 (K_304)
#define SENSOR_NOZH_2_PIN 5           // Датчик Нож 2 (K_305)
#define SENSOR_NOZH_3_PIN 6           // Датчик Нож 3 (K_306)
#define SENSOR_NOZH_4_PIN 7           // Датчик Нож 4 (K_307)

// --- Порт B ---
#define SENSOR_1_PIN 8                // Датчик 1 (308)
#define SENSOR_2_PIN 9                // Датчик 2 (309)
#define SENSOR_3_PIN 10                // Датчик 3 (310)
#define SENSOR_4_PIN 11                // Датчик 4 (311)
#define SENSOR_5_PIN 12                // Датчик 5 (312)
#define SENSOR_6_PIN 13                // Датчик 6 (313)
#define SENSOR_7_PIN 14                // Датчик 7 (314)
#define BUTTON_PIN 15                  // Кнопка (315)

// =================================================================
// =         Массивы пинов для страницы PinTest                    =
// =================================================================

static const int espPins[] = {
    MOTOR_STOL_STEP_PIN, MOTOR_PROTYAZHKA_STEP_PIN, MOTOR_GILOTINA_STEP_PIN,
    MOTOR_VIBROS_STEP_PIN, MOTOR_NOZH_1_STEP_PIN, MOTOR_NOZH_2_STEP_PIN,
    MOTOR_NOZH_3_STEP_PIN, MOTOR_NOZH_4_STEP_PIN, MOTOR_9_STEP_PIN,
    MOTOR_10_STEP_PIN, MOTOR_11_STEP_PIN, MOTOR_12_STEP_PIN,
    MOTOR_13_STEP_PIN, MOTOR_14_STEP_PIN, SENSOR_P2_PIN
};

static const int mcp1Pins[] = {
    MCP1_MOTOR_STOL_DIR_PIN, MCP1_MOTOR_STOL_EN_PIN, MCP1_MOTOR_PROTYAZHKA_DIR_PIN,
    MCP1_MOTOR_PROTYAZHKA_EN_PIN, MCP1_MOTOR_GILOTINA_DIR_PIN, MCP1_MOTOR_GILOTINA_EN_PIN,
    MCP1_MOTOR_VIBROS_DIR_PIN, MCP1_MOTOR_VIBROS_EN_PIN, MCP1_MOTOR_NOZH_1_DIR_PIN,
    MCP1_MOTOR_NOZH_1_EN_PIN, MCP1_MOTOR_NOZH_2_DIR_PIN, MCP1_MOTOR_NOZH_2_EN_PIN,
    MCP1_MOTOR_NOZH_3_DIR_PIN, MCP1_MOTOR_NOZH_3_EN_PIN, MCP1_MOTOR_NOZH_4_DIR_PIN,
    MCP1_MOTOR_NOZH_4_EN_PIN
};

static const int mcp2Pins[] = {
    MCP2_MOTOR_9_DIR_PIN, MCP2_MOTOR_9_EN_PIN, MCP2_MOTOR_10_DIR_PIN,
    MCP2_MOTOR_10_EN_PIN, MCP2_MOTOR_11_DIR_PIN, MCP2_MOTOR_11_EN_PIN,
    MCP2_MOTOR_12_DIR_PIN, MCP2_MOTOR_12_EN_PIN, MCP2_MOTOR_13_DIR_PIN,
    MCP2_MOTOR_13_EN_PIN, MCP2_MOTOR_14_DIR_PIN, MCP2_MOTOR_14_EN_PIN,
    MUFTA_1_PIN, MUFTA_2_PIN, MUFTA_3_PIN, MUFTA_4_PIN
};

static const int mcp3Pins[] = {
    SENSOR_STOL_PIN, SENSOR_BUMAGA_PIN, SENSOR_GIL_NA_1_PIN, SENSOR_GIL_NA_2_PIN,
    SENSOR_NOZH_1_PIN, SENSOR_NOZH_2_PIN, SENSOR_NOZH_3_PIN, SENSOR_NOZH_4_PIN,
    SENSOR_1_PIN, SENSOR_2_PIN, SENSOR_3_PIN, SENSOR_4_PIN,
    SENSOR_5_PIN, SENSOR_6_PIN, SENSOR_7_PIN, BUTTON_PIN
};
