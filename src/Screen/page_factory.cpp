#include "Screen/page_factory.h"

#include "Screen/generated/page_factory.generated.h"

namespace machine32::screen {

std::unique_ptr<page> create_page(uint32_t page_id) {
    return generated::create_page(page_id);
}

}  // namespace machine32::screen
