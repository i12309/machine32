// Пример: как собрать main() для устройства с BOOT-последовательностью.
//
// Это иллюстрация, не часть продакшен-сборки. Реальный main() на ESP32 будет
// дёргать те же шаги, только source времени — Arduino millis().

#include <cstdio>

#include "config/ScreenConfig.h"
#include "runtime/SinglePageRuntime.h"

#include "Screen/BOOT.h"
#include "Screen/load_page.h"
#include "Screen/main_menu.h"

#ifdef ARDUINO
#include <Arduino.h>
#endif

namespace {

uint32_t now_ms() {
#ifdef ARDUINO
    return millis();
#else
    static uint32_t fake = 0;
    fake += 50;
    return fake;
#endif
}

void yield_ms(uint32_t ms) {
#ifdef ARDUINO
    delay(ms);
#else
    (void)ms;
#endif
}

}  // namespace

int main() {
    using namespace machine32::screen;

    // Конфиг: оба выхода включены, BOOT решит, кто реально доступен.
    screenlib::ScreenConfig cfg{};
    cfg.physical.enabled = true;
    cfg.physical.type    = screenlib::OutputType::Uart;
    cfg.web.enabled      = true;
    cfg.web.type         = screenlib::OutputType::WsServer;
    cfg.web.wsServer.port = 81;
    cfg.mirrorMode       = screenlib::MirrorMode::Mirror;

    BootDeps deps{};
    deps.now_ms   = &now_ms;
    deps.yield_ms = &yield_ms;

    screenlib::SinglePageRuntime rt;
    Boot boot;

    const BootMode mode = boot.run<LoadPage>(rt, cfg, deps);

    switch (mode) {
        case BootMode::Physical: std::printf("[boot] physical monitor connected\n"); break;
        case BootMode::Web:      std::printf("[boot] web client connected\n");       break;
        case BootMode::Silent:   std::printf("[boot] no UI, running silent\n");      break;
        case BootMode::Error:    std::printf("[boot] init error: %s\n", rt.lastError()); return 1;
    }

    // Главный цикл. Тикаем через Boot, чтобы он сам поднял UI,
    // когда web-клиент появится позже.
    while (true) {
        boot.tick<LoadPage>(rt);

#ifdef ARDUINO
        delay(1);
#else
        break;  // на хосте — один тик и выход
#endif
    }

    return 0;
}
