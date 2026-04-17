#include "MCP.h"

#include "McpTrigger.h"

void IRAM_ATTR MCP::interruptRouterA(void* arg) {
    auto* self = static_cast<MCP*>(arg);
    if (self) self->dispatchBankISR(0);
}

void IRAM_ATTR MCP::interruptRouterB(void* arg) {
    auto* self = static_cast<MCP*>(arg);
    if (self) self->dispatchBankISR(1);
}

void IRAM_ATTR MCP::dispatchBankISR(uint8_t bank) {
    auto& handlers = (bank == 0) ? handlersA : handlersB;
    for (auto& slot : handlers) {
        if (slot.active) McpTrigger::mcp_isr(slot.arg);
    }
}
