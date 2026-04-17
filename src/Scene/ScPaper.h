#pragma once

#include "ScBase.h"

namespace scene {

// Сценарии протяжки и поиска бумаги.
class ScPaper : public ScBase {
public:
    // Создает сценарий протяжки бумаги.
    ScPaper(IMachineContext& machine, ScGuard& guard);

    // Запускает непрерывное движение бумаги.
    Catalog::PaperActionResult work(Catalog::DIR direction, Catalog::SPEED speed = Catalog::SPEED::Normal, bool withThrow = true, bool engageClutch = false);

    // Запускает поиск края бумаги по оптическому датчику.
    Catalog::PaperActionResult detectPaper(bool withThrow = true, Catalog::DIR direction = Catalog::DIR::Forward, Catalog::SPEED speed = Catalog::SPEED::Custom, bool engageClutch = true, int32_t detectOffset = 0, Catalog::OpticalSensor optical = Catalog::OpticalSensor::EDGE, Catalog::ErrorCode timeoutErrorCode = Catalog::ErrorCode::PAPER_NOT_FOUND);

    // Запускает поиск метки по оптическому датчику.
    Catalog::PaperActionResult detectMark(bool withThrow = true, Catalog::DIR direction = Catalog::DIR::Forward, Catalog::SPEED speed = Catalog::SPEED::Custom, bool engageClutch = false, int32_t detectOffset = 0, Catalog::OpticalSensor optical = Catalog::OpticalSensor::MARK, Catalog::ErrorCode timeoutErrorCode = Catalog::ErrorCode::PAPER_FIND_IN_MARK);

    // Двигает бумагу на заданное число шагов.
    Catalog::PaperActionResult move(int32_t steps, Catalog::DIR direction, Catalog::SPEED speed = Catalog::SPEED::Normal, bool blocking = false, bool engageClutch = false, bool withThrow = true);

    // Останавливает сразу или с добегом до offset после оптики.
    bool stopOffset(int32_t offsetSteps);

    // Останавливает сценарий протяжки.
    bool stop(Catalog::StopMode mode = Catalog::StopMode::ForceStop);

private:
    // Отключает оптические триггеры для обоих датчиков.
    void disableOpticalTriggers();
};

}  // namespace scene

