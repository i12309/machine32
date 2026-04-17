#pragma once

#include <stdint.h>

#include "config/ScreenConfig.h"
#include "runtime/SinglePageRuntime.h"

namespace machine32::screen {

// BOOT — последовательность поднятия экрана.
//
// Шаги:
//   1) Поднимаем оба выхода (physical + web) и тикаем фиксированное окно.
//   2) Если в течение окна пришёл коннект от физического монитора — работаем с ним.
//   3) Иначе ждём web второе окно — если кто-то подключился, работаем с ним.
//   4) Иначе уходим в "тихий" режим: страницу не активируем, runtime тикает,
//      и при появлении любого выхода — ставим стартовую страницу автоматически.
//
// Тип стартовой страницы передаётся как параметр шаблона: типобезопасно,
// никаких pageId-строк, никакого реестра.
struct BootDeps {
    // Источник времени в миллисекундах. На ESP — &millis. В тестах — fake clock.
    uint32_t (*now_ms)() = nullptr;
    // Уступить процессорное время на 1 итерации ожидания. На ESP — &delay-обёртка.
    // Может быть nullptr, тогда BOOT просто крутит цикл.
    void (*yield_ms)(uint32_t) = nullptr;
};

enum class BootMode : uint8_t {
    Error = 0,        // init не удался
    Physical,         // подключился физический монитор
    Web,              // подключился web-клиент
    Silent,           // никто не ответил, ждём в фоне
};

class Boot {
public:
    static constexpr uint32_t kDefaultPhysicalProbeMs = 800;
    static constexpr uint32_t kDefaultWebProbeMs      = 1500;
    static constexpr uint32_t kSilentRetryStepMs      = 100;

    // Полная последовательность boot. После возврата:
    //  - при Physical/Web стартовая страница уже активна;
    //  - при Silent runtime жив, страница не создана; продолжай дёргать tick().
    template <typename StartPage>
    BootMode run(screenlib::SinglePageRuntime& rt,
                 const screenlib::ScreenConfig& cfg,
                 const BootDeps& deps,
                 uint32_t physical_probe_ms = kDefaultPhysicalProbeMs,
                 uint32_t web_probe_ms      = kDefaultWebProbeMs) {
        _deps = deps;

        if (!rt.init(cfg)) {
            _mode = BootMode::Error;
            return _mode;
        }

        if (waitConnected(rt, physical_probe_ms, /*want_physical=*/true)) {
            rt.template start<StartPage>();
            _mode = BootMode::Physical;
            return _mode;
        }

        if (waitConnected(rt, web_probe_ms, /*want_physical=*/false)) {
            rt.template start<StartPage>();
            _mode = BootMode::Web;
            return _mode;
        }

        _mode = BootMode::Silent;
        _started = false;
        return _mode;
    }

    // Дёргается из главного цикла. В тихом режиме сам стартует страницу,
    // как только что-то подключилось. В остальных режимах — обычный rt.tick().
    template <typename StartPage>
    void tick(screenlib::SinglePageRuntime& rt) {
        rt.tick();
        if (_mode != BootMode::Silent || _started) {
            return;
        }
        if (rt.connectedPhysical() || rt.connectedWeb()) {
            rt.template start<StartPage>();
            _started = true;
            _mode = rt.connectedPhysical() ? BootMode::Physical : BootMode::Web;
        }
    }

    BootMode mode() const { return _mode; }

private:
    bool waitConnected(screenlib::SinglePageRuntime& rt, uint32_t window_ms, bool want_physical) {
        if (_deps.now_ms == nullptr) {
            // Без источника времени просто один тик и проверка.
            rt.tick();
            return want_physical ? rt.connectedPhysical() : rt.connectedWeb();
        }

        const uint32_t deadline = _deps.now_ms() + window_ms;
        while (_deps.now_ms() < deadline) {
            rt.tick();
            const bool ok = want_physical ? rt.connectedPhysical() : rt.connectedWeb();
            if (ok) {
                return true;
            }
            if (_deps.yield_ms != nullptr) {
                _deps.yield_ms(kSilentRetryStepMs);
            }
        }
        return false;
    }

    BootDeps _deps{};
    BootMode _mode = BootMode::Error;
    bool _started = false;
};

}  // namespace machine32::screen
