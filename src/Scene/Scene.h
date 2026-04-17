#pragma once

#include "Catalog.h"
#include "ScGuard.h"
#include "ScGuillotine.h"
#include "ScPaper.h"
#include "ScTable.h"
#include "ScThrow.h"

namespace scene {

// Фасад нового модуля сцен.
// Объединяет отдельные сценарии в единый API.
class Scene {
public:
    // Возвращает singleton-экземпляр фасада.
    static Scene& getInstance();

    // Группа методов сценария гильотины.
    bool guillotineWork(Catalog::DIR direction, uint32_t delayMs = 0, Catalog::SPEED speed = Catalog::SPEED::Normal, DeviceError::Kind kind = DeviceError::Kind::Fatal, bool withThrow = true);
    bool guillotineStop(Catalog::StopMode mode);

    // Группа методов сценария стола.
    Catalog::TableActionResult tableUp(Catalog::SPEED speed = Catalog::SPEED::Normal, bool blocking = false);
    Catalog::TableActionResult tableDown(Catalog::SPEED speed = Catalog::SPEED::Normal);
    bool tableStop(Catalog::StopMode mode = Catalog::StopMode::ForceStop);

    // Группа методов сценария бумаги.
    Catalog::PaperActionResult paperWork(Catalog::DIR direction, Catalog::SPEED speed = Catalog::SPEED::Normal, bool withThrow = true, bool engageClutch = false);
    Catalog::PaperActionResult paperDetectPaper(bool withThrow = true, Catalog::DIR direction = Catalog::DIR::Forward, Catalog::SPEED speed = Catalog::SPEED::Custom, bool engageClutch = true, int32_t detectOffset = 0, Catalog::OpticalSensor optical = Catalog::OpticalSensor::EDGE, Catalog::ErrorCode timeoutErrorCode = Catalog::ErrorCode::PAPER_NOT_FOUND);
    Catalog::PaperActionResult paperDetectMark(bool withThrow = true, Catalog::DIR direction = Catalog::DIR::Forward, Catalog::SPEED speed = Catalog::SPEED::Custom, bool engageClutch = false, int32_t detectOffset = 0, Catalog::OpticalSensor optical = Catalog::OpticalSensor::MARK, Catalog::ErrorCode timeoutErrorCode = Catalog::ErrorCode::PAPER_FIND_IN_MARK);
    Catalog::PaperActionResult paperMove(int32_t steps, Catalog::DIR direction, Catalog::SPEED speed = Catalog::SPEED::Normal, bool blocking = false, bool engageClutch = false, bool withThrow = true);
    bool paperStop(Catalog::StopMode mode = Catalog::StopMode::ForceStop);
    bool paperStopOffset(int32_t offsetSteps);

    // Группа методов сценария THROW.
    bool throwWork(Catalog::DIR direction, Catalog::SPEED speed = Catalog::SPEED::Slow);
    bool throwStop(Catalog::StopMode mode = Catalog::StopMode::SoftStop);

    // Простые read-only фасады для состояния узлов.
    bool isPaperRunning() const;
    bool isGuillotineRunning() const;
    bool isTableRunning() const;
    int32_t paperPosition() const;

private:
    // Закрытый конструктор singleton-фасада.
    Scene();

    IMachineContext& machine_;
    ScGuard guard_;
    ScPaper paper_;
    ScThrow throw_;
    ScTable table_;
    ScGuillotine guillotine_;
};

}  // namespace scene

