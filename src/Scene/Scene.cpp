#include "Scene.h"

#include "App/App.h"

namespace scene {

Scene& Scene::getInstance() {
    // Единая точка доступа к фасаду сцен.
    static Scene instance;
    return instance;
}

Scene::Scene()
    : machine_(App::ctx())
    , guard_()
    , paper_(machine_, guard_)
    , throw_(machine_, guard_)
    , table_(machine_, guard_)
    , guillotine_(machine_, guard_) {}

bool Scene::guillotineWork(Catalog::DIR direction, uint32_t delayMs, Catalog::SPEED speed, DeviceError::Kind kind, bool withThrow) {
    // Делегируем запуск в сценарий гильотины.
    return guillotine_.work(direction, delayMs, speed, kind, withThrow);
}

bool Scene::guillotineStop(Catalog::StopMode mode) {
    // Делегируем остановку в сценарий гильотины.
    return guillotine_.stop(mode);
}

Catalog::TableActionResult Scene::tableUp(Catalog::SPEED speed, bool blocking) {
    // Делегируем подъем стола.
    return table_.up(speed, blocking);
}

Catalog::TableActionResult Scene::tableDown(Catalog::SPEED speed) {
    // Делегируем опускание стола.
    return table_.down(speed);
}

bool Scene::tableStop(Catalog::StopMode mode) {
    // Делегируем остановку стола.
    return table_.stop(mode);
}

Catalog::PaperActionResult Scene::paperWork(Catalog::DIR direction, Catalog::SPEED speed, bool withThrow, bool engageClutch) {
    // Делегируем базовую протяжку бумаги.
    return paper_.work(direction, speed, withThrow, engageClutch);
}

Catalog::PaperActionResult Scene::paperDetectPaper(bool withThrow, Catalog::DIR direction, Catalog::SPEED speed, bool engageClutch, int32_t detectOffset, Catalog::OpticalSensor optical, Catalog::ErrorCode timeoutErrorCode) {
    // Делегируем поиск края бумаги.
    return paper_.detectPaper(withThrow, direction, speed, engageClutch, detectOffset, optical, timeoutErrorCode);
}

Catalog::PaperActionResult Scene::paperDetectMark(bool withThrow, Catalog::DIR direction, Catalog::SPEED speed, bool engageClutch, int32_t detectOffset, Catalog::OpticalSensor optical, Catalog::ErrorCode timeoutErrorCode) {
    // Делегируем поиск метки.
    return paper_.detectMark(withThrow, direction, speed, engageClutch, detectOffset, optical, timeoutErrorCode);
}

Catalog::PaperActionResult Scene::paperMove(int32_t steps, Catalog::DIR direction, Catalog::SPEED speed, bool blocking, bool engageClutch, bool withThrow) {
    // Делегируем движение бумаги на шаги.
    return paper_.move(steps, direction, speed, blocking, engageClutch, withThrow);
}

bool Scene::paperStop(Catalog::StopMode mode) {
    // Делегируем остановку бумаги.
    return paper_.stop(mode);
}

bool Scene::paperStopOffset(int32_t offsetSteps) {
    // Делегируем остановку с offset после оптики.
    return paper_.stopOffset(offsetSteps);
}

bool Scene::throwWork(Catalog::DIR direction, Catalog::SPEED speed) {
    // Делегируем запуск режима THROW.
    return throw_.work(direction, speed);
}

bool Scene::throwStop(Catalog::StopMode mode) {
    // Делегируем остановку режима THROW.
    return throw_.stop(mode);
}

bool Scene::isPaperRunning() const {
    // Проверка признака движения мотора бумаги.
    return machine_.mPaper->isRunning();
}

bool Scene::isGuillotineRunning() const {
    // Проверка признака движения мотора гильотины.
    return machine_.mGuillotine->isRunning();
}

bool Scene::isTableRunning() const {
    // Проверка признака движения мотора стола.
    return machine_.mTable->isRunning();
}

int32_t Scene::paperPosition() const {
    // Возврат текущей позиции бумаги.
    return machine_.mPaper->getCurrentPosition();
}

}  // namespace scene
