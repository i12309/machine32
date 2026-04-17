#include "IOptical.h"
#include "App/App.h"
#include "State/Scene.h"

void IRAM_ATTR IOptical::sensor_isr(void* arg) {
    auto* self = static_cast<IOptical*>(arg);
    // Захватываем позицию мотора прямо в ISR — задержка единицы микросекунд.
    if (self->ePaper) self->ePaper->setCapture(self->ePaper->getCount());
    else if (self->mPaper) self->mPaper->capturePosition();
    /*
Важный момент, который стоит проверить на железе
FastAccelStepper::getCurrentPosition() вызывается из аппаратного прерывания GPIO. В большинстве случаев для MCPWM_PCNT/RMT это атомарное чтение счётчика и работает, но сама функция не объявлена IRAM_ATTR в библиотеке. На ESP32 при активной flash-операции (запись в NVS, OTA) такой вызов из ISR может упасть с Cache disabled but cached memory region accessed.

На практике: если в момент срабатывания датчика ты не пишешь в NVS/flash — всё будет работать. Если начнутся случайные крэши при параллельных операциях с флешем — придётся либо оборачивать чтение в pcnt_get_counter_value() напрямую (минуя FastAccelStepper), либо переносить захват из ISR в отдельный high-priority task, разбуженный через portYIELD_FROM_ISR.

Для отладки jitter это не должно помешать — просто держи в уме.
    */
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(self->taskHandle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void IOptical::initTrigger() {
    if (taskHandle != nullptr) return;

    BaseType_t status = xTaskCreatePinnedToCore(
        IOptical::taskTrigger,
        "IOpticalTrigger",
        6144,
        this,
        configMAX_PRIORITIES - 2,
        &taskHandle,
        1
    );
    if (status != pdPASS) {
        Log::E("[IOptical] failed to create taskTrigger for pin=%d", pin);
        taskHandle = nullptr;
    }
}

void IOptical::taskTrigger(void* pvParameters) {
    auto* self = static_cast<IOptical*>(pvParameters);
    if (self == nullptr) {
        vTaskDelete(nullptr);
        return;
    }

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (self->triggerState == -1) continue;
        const bool triggerMatched =
            ((self->triggerState == FALLING && self->checkWhite()) ||
             (self->triggerState == RISING && self->checkBlack()));

        if (!triggerMatched) continue;

        // Режим совместимости:
        // offsetSteps == 0 -> старое поведение (мгновенная force-остановка).
        if (self->offsetSteps == 0) {
            Scene::getInstance().paperStop(Catalog::StopMode::ForceStop);
        } else {
            // Новый режим:
            // по событию датчика не "рубим" мотор сразу, а даем Scene перенацелить движение
            // на точную позицию edgePos + offsetSteps.
            Scene::getInstance().paperStopOffset(self->offsetSteps);
        }

    }
}

void IOptical::enableTrigger(int state, int32_t _offsetSteps) {
    triggerState = state;
    offsetSteps = _offsetSteps;

    const int irq = digitalPinToInterrupt(pin);
    if (irq == NOT_AN_INTERRUPT) {
        Log::E("[IOptical] pin=%d is not interrupt-capable", pin);
        return;
    }
    if (taskHandle == nullptr) {
        Log::E("[IOptical] trigger task is not initialized for pin=%d", pin);
        return;
    }

    const int irqMode =
        (triggerState == RISING)
            ? (sig ? RISING : FALLING)
            : (sig ? FALLING : RISING);
    if (interruptAttached) detachInterrupt(irq);

    // Привязываем выбранный энкодер заранее, чтобы ISR не трогал App::ctx().
    //if (ePaper == nullptr) ePaper = App::ctx().ePaper;
    /*if (ePaper != nullptr) {
        //ePaper->resetCapture();
        ePaper->resumeCount();
    }*/

    attachInterruptArg(irq, sensor_isr, this, irqMode);
    interruptAttached = true;
}

void IOptical::disableTrigger() {
    //if (ePaper != nullptr) ePaper->pauseCount();
    if (!interruptAttached) return;

    const int irq = digitalPinToInterrupt(pin);
    if (irq == NOT_AN_INTERRUPT) {
        interruptAttached = false;
        return;
    }
    detachInterrupt(irq);
    interruptAttached = false;
    triggerState = -1;
}
