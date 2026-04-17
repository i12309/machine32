#pragma once

#include <memory>
#include <stdint.h>

#include "Screen/page.h"

namespace machine32::screen {

// Создаёт новый объект страницы по page_id.
//
// Снаружи runtime не должен знать, какой именно класс отвечает за страницу.
// Весь выбор скрывается фабрикой, а сама таблица соответствий page_id -> class
// генерируется автоматически pre-build скриптом.
std::unique_ptr<page> create_page(uint32_t page_id);

}  // namespace machine32::screen
