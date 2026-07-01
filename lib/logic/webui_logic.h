// lib/logic/webui_logic.h
// Pure C — no Arduino or hardware headers.
// WebSocket message parsing extracted from WebUIService::onWsEvent.
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

typedef enum {
    WS_ACTION_NONE   = 0,
    WS_ACTION_LAUNCH = 1,
    WS_ACTION_STOP   = 2,
    WS_ACTION_STATUS = 3
} WsActionType;

typedef struct {
    WsActionType type;
    int8_t cat;
    int8_t item;
} WsAction;

// Parse a null-terminated WebSocket text frame and return the decoded action.
// buf must be null-terminated and len must equal strlen(buf).
// Returns WS_ACTION_NONE on any parse failure.
// WS_ACTION_LAUNCH sets cat/item; STOP and STATUS leave them at -1.
inline WsAction parseWsAction(const char* buf, size_t len) {
    WsAction result = {WS_ACTION_NONE, -1, -1};
    if (buf == nullptr || len == 0) return result;

    const char* actionPtr = strstr(buf, "\"action\":\"");
    if (!actionPtr) return result;
    actionPtr += 10;

    if (strncmp(actionPtr, "launch\"", 7) == 0) {
        const char* catPtr = strstr(buf, "\"category\":");
        const char* itmPtr = strstr(buf, "\"item\":");
        if (!catPtr || !itmPtr) return result;
        result.type = WS_ACTION_LAUNCH;
        result.cat  = (int8_t)atoi(catPtr + 11);
        result.item = (int8_t)atoi(itmPtr + 7);
    } else if (strncmp(actionPtr, "stop\"", 5) == 0) {
        result.type = WS_ACTION_STOP;
    } else if (strncmp(actionPtr, "status\"", 7) == 0) {
        result.type = WS_ACTION_STATUS;
    }
    return result;
}
