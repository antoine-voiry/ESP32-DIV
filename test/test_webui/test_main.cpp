// test/test_webui/test_main.cpp
#include <unity.h>
#include "webui_logic.h"

void setUp()    {}
void tearDown() {}

// parseWsAction — WebSocket message parser (BUG-002/010: commands not processed
// because WebUIService::loop() was never called inside the Phone Remote loop)

// Failure mode: launch action not recognized → tool never starts
void test_launch_action_parses_cat_and_item() {
    const char* msg = "{\"action\":\"launch\",\"category\":0,\"item\":1}";
    WsAction a = parseWsAction(msg, strlen(msg));
    TEST_ASSERT_EQUAL(WS_ACTION_LAUNCH, a.type);
    TEST_ASSERT_EQUAL(0, a.cat);
    TEST_ASSERT_EQUAL(1, a.item);
}

// Failure mode: category 3 item 0 (SubGHz replay) not dispatched
void test_launch_subghz_cat3_item0() {
    const char* msg = "{\"action\":\"launch\",\"category\":3,\"item\":0}";
    WsAction a = parseWsAction(msg, strlen(msg));
    TEST_ASSERT_EQUAL(WS_ACTION_LAUNCH, a.type);
    TEST_ASSERT_EQUAL(3, a.cat);
    TEST_ASSERT_EQUAL(0, a.item);
}

// Failure mode: stop action not recognized → running tool cannot be stopped remotely
void test_stop_action_returns_stop_type() {
    const char* msg = "{\"action\":\"stop\"}";
    WsAction a = parseWsAction(msg, strlen(msg));
    TEST_ASSERT_EQUAL(WS_ACTION_STOP, a.type);
    TEST_ASSERT_EQUAL(-1, a.cat);
    TEST_ASSERT_EQUAL(-1, a.item);
}

// Failure mode: status request never responded to → UI shows stale state
void test_status_action_returns_status_type() {
    const char* msg = "{\"action\":\"status\"}";
    WsAction a = parseWsAction(msg, strlen(msg));
    TEST_ASSERT_EQUAL(WS_ACTION_STATUS, a.type);
}

// Failure mode: unknown action sets pendingCat incorrectly → wrong tool launched
void test_unknown_action_returns_none() {
    const char* msg = "{\"action\":\"reboot\"}";
    WsAction a = parseWsAction(msg, strlen(msg));
    TEST_ASSERT_EQUAL(WS_ACTION_NONE, a.type);
}

// Failure mode: missing action key crashes or returns garbage
void test_missing_action_key_returns_none() {
    const char* msg = "{\"category\":0,\"item\":0}";
    WsAction a = parseWsAction(msg, strlen(msg));
    TEST_ASSERT_EQUAL(WS_ACTION_NONE, a.type);
}

// Failure mode: launch without category/item silently sets garbage values
void test_launch_missing_category_returns_none() {
    const char* msg = "{\"action\":\"launch\",\"item\":2}";
    WsAction a = parseWsAction(msg, strlen(msg));
    TEST_ASSERT_EQUAL(WS_ACTION_NONE, a.type);
}

// Failure mode: launch without item silently sets garbage cat with item=-1
void test_launch_missing_item_returns_none() {
    const char* msg = "{\"action\":\"launch\",\"category\":1}";
    WsAction a = parseWsAction(msg, strlen(msg));
    TEST_ASSERT_EQUAL(WS_ACTION_NONE, a.type);
}

// Failure mode: null/empty buf crashes parser
void test_null_buf_returns_none() {
    WsAction a = parseWsAction(nullptr, 0);
    TEST_ASSERT_EQUAL(WS_ACTION_NONE, a.type);
}

// Failure mode: empty string returns garbage
void test_empty_buf_returns_none() {
    const char* msg = "";
    WsAction a = parseWsAction(msg, 0);
    TEST_ASSERT_EQUAL(WS_ACTION_NONE, a.type);
}

// Failure mode: item value at boundary (5 = max valid) parsed correctly
void test_launch_item_boundary_5() {
    const char* msg = "{\"action\":\"launch\",\"category\":0,\"item\":5}";
    WsAction a = parseWsAction(msg, strlen(msg));
    TEST_ASSERT_EQUAL(WS_ACTION_LAUNCH, a.type);
    TEST_ASSERT_EQUAL(5, a.item);
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_launch_action_parses_cat_and_item);
    RUN_TEST(test_launch_subghz_cat3_item0);
    RUN_TEST(test_stop_action_returns_stop_type);
    RUN_TEST(test_status_action_returns_status_type);
    RUN_TEST(test_unknown_action_returns_none);
    RUN_TEST(test_missing_action_key_returns_none);
    RUN_TEST(test_launch_missing_category_returns_none);
    RUN_TEST(test_launch_missing_item_returns_none);
    RUN_TEST(test_null_buf_returns_none);
    RUN_TEST(test_empty_buf_returns_none);
    RUN_TEST(test_launch_item_boundary_5);
    return UNITY_END();
}
