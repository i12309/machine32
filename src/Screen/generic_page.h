#pragma once

#include <stdint.h>

#include "Screen/page.h"
#include "generated/shared/page_descriptors.generated.h"

namespace machine32::screen {

// Универсальная страница-заглушка.
//
// Она нужна для двух сценариев:
// 1. Страница уже существует в ScreenUI, но её бизнес-логика ещё не написана.
// 2. Проект находится в процессе миграции, и backend пока не покрыт
//    специализированными page-классами целиком.
//
// generic_page позволяет открыть страницу по ID и безопасно продолжать работу,
// не требуя заранее описывать весь backend-код для всех экранов.
class generic_page final : public page {
public:
    explicit generic_page(const Screen32PageDescriptor* descriptor) : _descriptor(descriptor) {}

    // Возвращает page_id страницы из generated meta.
    uint32_t page_id() const override {
        return (_descriptor != nullptr) ? _descriptor->page_id : 0;
    }

    // Даёт доступ к generated descriptor, если бизнес-логике нужен page_name
    // или object_name страницы.
    const Screen32PageDescriptor* descriptor() const { return _descriptor; }

private:
    const Screen32PageDescriptor* _descriptor = nullptr;
};

}  // namespace machine32::screen
