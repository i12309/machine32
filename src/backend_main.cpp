#include <cstdio>

#include "generated/shared/page_ids.generated.h"

int main() {
    std::printf("machine32 backend skeleton. first page id = %u\n", static_cast<unsigned>(SCREEN32_PAGE_ID_LOAD));
    return 0;
}
