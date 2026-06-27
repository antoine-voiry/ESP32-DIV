#include "webui.h"
#include "webui_html.h"
#include "shared.h"
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <WiFi.h>
#include <Preferences.h>

// Forward declarations — avoid circular includes with wificonfig.h/bleconfig.h/subconfig.h
extern bool feature_exit_requested;
extern bool feature_active;
extern bool in_sub_menu;
extern bool submenu_initialized;

namespace PacketMonitor   { void ptmSetup();           void ptmLoop();           }
namespace BeaconSpammer   { void beaconSpamSetup();    void beaconSpamLoop();    }
namespace Deauther        { void deautherSetup();      void deautherLoop();      }
namespace DeauthDetect    { void deauthdetectSetup();  void deauthdetectLoop();  }
namespace WifiScan        { void wifiscanSetup();      void wifiscanLoop();      }
namespace CaptivePortal   { void cportalSetup();       void cportalLoop();       }
namespace replayat        { void ReplayAttackSetup();  void ReplayAttackLoop();  }
namespace subbrute        { void subBruteSetup();      void subBruteLoop();      }
namespace subjammer       { void subjammerSetup();     void subjammerLoop();     }

namespace WebUIService {

static AsyncWebServer server(80);
static AsyncWebSocket ws("/ws");
static bool active = false;
static unsigned long lastCleanup = 0;

// Pending command — written by WS callback (Core 0), read by loop() (Core 1).
// volatile ensures the Core 1 read sees the Core 0 write without caching.
static volatile int8_t pendingCat  = -1;
static volatile int8_t pendingItem = -1;
static volatile bool   pendingStop = false;

// ─── JSON helpers (no ArduinoJson dependency) ──────────────────────────────

static String jsonStr(const String& json, const char* key) {
    String k = "\""; k += key; k += "\":\"";
    int i = json.indexOf(k);
    if (i < 0) return "";
    i += k.length();
    int e = json.indexOf('"', i);
    return (e < 0) ? "" : json.substring(i, e);
}

static int jsonInt(const String& json, const char* key) {
    String k = "\""; k += key; k += "\":";
    int i = json.indexOf(k);
    if (i < 0) return -1;
    return json.substring(i + k.length()).toInt();
}

// ─── WS event handler (runs on Core 0 — only set volatile flags here) ──────

static void onWsEvent(AsyncWebSocket*, AsyncWebSocketClient*,
                      AwsEventType type, void* arg, uint8_t* data, size_t len) {
    if (type != WS_EVT_DATA) return;
    AwsFrameInfo* info = (AwsFrameInfo*)arg;
    if (!info->final || info->index != 0 || info->len != len || info->opcode != WS_TEXT) return;
    data[len] = 0;
    String msg = (char*)data;
    String action = jsonStr(msg, "action");
    if (action == "launch") {
        pendingCat  = (int8_t)jsonInt(msg, "category");
        pendingItem = (int8_t)jsonInt(msg, "item");
    } else if (action == "stop") {
        pendingStop = true;
    } else if (action == "status") {
        // status response built in loop() — set a flag
        pendingCat = -2;  // sentinel: status request
    }
}

// ─── Dispatch table — called from loop() on Core 1 ─────────────────────────

static void runTool(int8_t cat, int8_t item) {
    feature_active = true;
    feature_exit_requested = false;
    in_sub_menu = true;
    submenu_initialized = false;

    // Emit tool_started
    String catNames[] = {"WiFi","BLE","2.4GHz","SubGHz","IR","Tools","Settings","Files"};
    String itemNames[][6] = {
        {"Packet Monitor","Beacon Spammer","WiFi Deauther","Deauth Detector","WiFi Scanner","Captive Portal"},
        {"BLE Jammer","BLE Spoofer","Sour Apple","Sniffer","BLE Scanner",""},
        {"Scanner","Spectrum Analyzer","WLAN Jammer","Proto Kill","",""},
        {"Replay Attack","Brute Force","SubGHz Jammer","Saved Profile","",""},
        {"","","","","",""},{"","","","","",""},{"","","","","",""},{"","","","","",""}
    };
    String toolName = (cat >= 0 && cat < 8 && item >= 0 && item < 6) ? itemNames[cat][item] : "Unknown";
    String started = "{\"event\":\"tool_started\",\"category\":" + String(cat) +
                     ",\"item\":" + String(item) + ",\"name\":\"" + toolName + "\"}";
    ws.textAll(started);

    // WiFi tools seize the radio — teardown WebUI before *Setup() so the guard
    // inside each *Setup() sees isActive()==false and doesn't double-fire.
    if (cat == 0 && active) {
        teardown();
        delay(100);
    }

    // TOOL_LOOP: also drains pendingStop inside the spin because loop() is
    // blocked here and can't consume it for the duration of the feature run.
    #define TOOL_LOOP(Setup, Loop) \
        Setup(); \
        while (!feature_exit_requested) { \
            if (pendingStop) { pendingStop = false; feature_exit_requested = true; break; } \
            Loop(); yield(); \
        } \
        break;

    switch ((cat << 4) | item) {
        case (0 << 4) | 0: TOOL_LOOP(PacketMonitor::ptmSetup,          PacketMonitor::ptmLoop)
        case (0 << 4) | 1: TOOL_LOOP(BeaconSpammer::beaconSpamSetup,   BeaconSpammer::beaconSpamLoop)
        case (0 << 4) | 2: TOOL_LOOP(Deauther::deautherSetup,          Deauther::deautherLoop)
        case (0 << 4) | 3: TOOL_LOOP(DeauthDetect::deauthdetectSetup,  DeauthDetect::deauthdetectLoop)
        case (0 << 4) | 4: TOOL_LOOP(WifiScan::wifiscanSetup,          WifiScan::wifiscanLoop)
        case (0 << 4) | 5: TOOL_LOOP(CaptivePortal::cportalSetup,      CaptivePortal::cportalLoop)
        case (3 << 4) | 0: TOOL_LOOP(replayat::ReplayAttackSetup,      replayat::ReplayAttackLoop)
        case (3 << 4) | 1: TOOL_LOOP(subbrute::subBruteSetup,          subbrute::subBruteLoop)
        case (3 << 4) | 2: TOOL_LOOP(subjammer::subjammerSetup,        subjammer::subjammerLoop)
        default: break;
    }
    #undef TOOL_LOOP

    feature_active = false;
    feature_exit_requested = false;
    submenu_initialized = false;
    ws.textAll("{\"event\":\"tool_stopped\"}");
}

// ─── Public API ─────────────────────────────────────────────────────────────

bool isActive() { return active; }

void broadcastEvent(const String& json) {
    if (active) ws.textAll(json);
}

void setup() {
    Preferences prefs;
    prefs.begin("webui", true);
    uint8_t mode   = prefs.getUChar("mode", 0);
    String apSSID  = prefs.getString("ap_ssid", "DIV-Remote");
    String staSsid = prefs.getString("sta_ssid", "");
    String staPass = prefs.getString("sta_pass", "");
    prefs.end();

    if (mode == 0) {
        WiFi.mode(WIFI_AP);
        WiFi.softAP(apSSID.c_str());
        Serial.printf("[WebUI] AP: %s  IP: %s\n", apSSID.c_str(),
                      WiFi.softAPIP().toString().c_str());
    } else {
        WiFi.mode(WIFI_STA);
        WiFi.begin(staSsid.c_str(), staPass.c_str());
        unsigned long t = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - t < 12000) {
            delay(200); yield();
        }
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[WebUI] STA connect failed — aborting");
            return;
        }
        Serial.printf("[WebUI] STA IP: %s\n", WiFi.localIP().toString().c_str());
    }

    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        AsyncWebServerResponse* resp = req->beginResponse_P(
            200, "text/html", webui_html_gz, webui_html_gz_len);
        resp->addHeader("Content-Encoding", "gzip");
        resp->addHeader("Cache-Control", "max-age=3600");
        req->send(resp);
    });

    server.onNotFound([](AsyncWebServerRequest* req) {
        req->send(404, "text/plain", "Not found");
    });

    server.begin();
    active = true;
}

void loop() {
    if (!active) return;

    // Process pending stop
    if (pendingStop) {
        pendingStop = false;
        feature_exit_requested = true;
    }

    // Process pending status request
    if (pendingCat == -2) {
        pendingCat = -1;
        String ip = (WiFi.getMode() == WIFI_AP) ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
        String mode = (WiFi.getMode() == WIFI_AP) ? "ap" : "sta";
        String resp = "{\"event\":\"status\",\"mode\":\"" + mode + "\",\"ip\":\"" + ip + "\",\"active_tool\":\"idle\"}";
        ws.textAll(resp);
    }

    // Process pending tool launch
    if (pendingCat >= 0) {
        int8_t cat = pendingCat, item = pendingItem;
        pendingCat = -1; pendingItem = -1;
        runTool(cat, item);
    }

    // Periodic WS client cleanup
    unsigned long now = millis();
    if (now - lastCleanup > 5000) {
        ws.cleanupClients();
        lastCleanup = now;
    }
}

void teardown() {
    if (!active) return;
    ws.textAll("{\"event\":\"going_offline\",\"reason\":\"radio_conflict\"}");
    delay(80);
    ws.closeAll();
    server.end();
    WiFi.softAPdisconnect(true);
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    active = false;
    Serial.println("[WebUI] Torn down — radio released");
}

} // namespace WebUIService
