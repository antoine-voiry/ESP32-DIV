#include "webui.h"
#include "webui_html.h"
#include "shared.h"
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <WiFi.h>
#include <Preferences.h>
#include <SD.h>

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

// Pending /fs and /dl requests — set in AsyncTCP lambda (Core 0), drained in loop() (Core 1).
// SD SPI bus must only be accessed from Core 1.
static volatile bool   pendingFsList = false;
static volatile bool   pendingDlFile = false;
static char            pendingFsPath[128] = {};
static char            pendingDlPath[128] = {};
static AsyncWebServerRequest* pendingFsReq = nullptr;
static AsyncWebServerRequest* pendingDlReq = nullptr;

// Tool name emitted in going_offline — set by runTool() before teardown().
static char pendingOfflineTool[32] = {};

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
    // Copy to null-terminated stack buffer — avoids String heap ops on Core 0
    const size_t MAX_MSG = 128;
    if (len >= MAX_MSG) return;          // drop oversized frames
    char buf[MAX_MSG];
    memcpy(buf, data, len);
    buf[len] = '\0';                     // safe: buf is MAX_MSG, len < MAX_MSG

    // Extract action without String allocation
    const char* actionPtr = strstr(buf, "\"action\":\"");
    if (!actionPtr) return;
    actionPtr += 10;  // skip past "action":"

    if (strncmp(actionPtr, "launch\"", 7) == 0) {
        // Extract category and item integers
        const char* catPtr = strstr(buf, "\"category\":");
        const char* itmPtr = strstr(buf, "\"item\":");
        if (catPtr && itmPtr) {
            pendingCat  = (int8_t)atoi(catPtr + 11);
            pendingItem = (int8_t)atoi(itmPtr + 7);
        }
    } else if (strncmp(actionPtr, "stop\"", 5) == 0) {
        pendingStop = true;
    } else if (strncmp(actionPtr, "status\"", 7) == 0) {
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
        // Record tool name for going_offline event
        static const char* const catNames[] = {"wifi","ble","rf24","subghz","ir","tools","settings","files"};
        snprintf(pendingOfflineTool, sizeof(pendingOfflineTool),
                 "%s_%d", catNames[cat < 8 ? cat : 0], item);
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

    server.on("/fs", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (!req->hasParam("path")) { req->send(400, "text/plain", "missing path"); return; }
        if (pendingFsList || pendingFsReq) { req->send(503, "text/plain", "busy"); return; }
        strncpy(pendingFsPath, req->getParam("path")->value().c_str(), sizeof(pendingFsPath) - 1);
        pendingFsPath[sizeof(pendingFsPath) - 1] = '\0';
        pendingFsReq  = req;
        pendingFsList = true;
    });

    server.on("/dl", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (!req->hasParam("path")) { req->send(400, "text/plain", "missing path"); return; }
        if (pendingDlFile || pendingDlReq) { req->send(503, "text/plain", "busy"); return; }
        strncpy(pendingDlPath, req->getParam("path")->value().c_str(), sizeof(pendingDlPath) - 1);
        pendingDlPath[sizeof(pendingDlPath) - 1] = '\0';
        pendingDlReq  = req;
        pendingDlFile = true;
    });

    server.on("/config", HTTP_GET, [](AsyncWebServerRequest* req) {
        Preferences p; p.begin("webui", true);
        String ssid = p.getString("sta_ssid", "");
        uint8_t mode = p.getUChar("mode", 0);
        p.end();
        String html =
            "<!DOCTYPE html><html><head><meta charset=UTF-8>"
            "<meta name=viewport content='width=device-width,initial-scale=1'>"
            "<style>body{background:#000;color:#fff;font-family:sans-serif;padding:20px}"
            "input,select{width:100%;padding:12px;margin:8px 0 16px;background:#1c1c1c;"
            "color:#fff;border:1px solid #333;border-radius:8px;font-size:15px}"
            "button{width:100%;padding:14px;background:#ff03ff;color:#000;border:none;"
            "border-radius:8px;font-size:16px;font-weight:700;cursor:pointer}</style></head>"
            "<body><h2 style='color:#ff03ff;margin-bottom:20px'>DIV-Remote Config</h2>"
            "<form method=POST action=/config>"
            "<label>Mode</label>"
            "<select name=mode>"
            "<option value=0" + String(mode==0?" selected":"") + ">AP (DIV-Remote hotspot)</option>"
            "<option value=1" + String(mode==1?" selected":"") + ">STA (join existing WiFi)</option>"
            "</select>"
            "<label>STA SSID</label>"
            "<input name=sta_ssid value='" + ssid + "' placeholder='Your WiFi network'>"
            "<label>STA Password</label>"
            "<input name=sta_pass type=password placeholder='Leave blank to keep current'>"
            "<button type=submit>Save &amp; Restart Server</button>"
            "</form></body></html>";
        req->send(200, "text/html", html);
    });

    server.on("/config", HTTP_POST, [](AsyncWebServerRequest* req) {
        Preferences p; p.begin("webui", false);
        if (req->hasParam("mode", true))
            p.putUChar("mode", (uint8_t)req->getParam("mode", true)->value().toInt());
        if (req->hasParam("sta_ssid", true))
            p.putString("sta_ssid", req->getParam("sta_ssid", true)->value());
        if (req->hasParam("sta_pass", true)) {
            String pass = req->getParam("sta_pass", true)->value();
            if (pass.length() > 0) p.putString("sta_pass", pass);
        }
        p.end();
        req->send(200, "text/html",
            "<!DOCTYPE html><html><body style='background:#000;color:#0f8;font-family:sans-serif;padding:20px'>"
            "<h2>Saved!</h2><p>Stop and restart the server from the TFT Settings menu to apply changes.</p>"
            "</body></html>");
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

    // Drain /fs pending request (SD I/O on Core 1 — SPI bus safe)
    if (pendingFsList && pendingFsReq) {
        AsyncWebServerRequest* req = pendingFsReq;
        pendingFsReq  = nullptr;
        pendingFsList = false;
        String path = String(pendingFsPath);
        // Restrict to safe roots only
        if (!path.startsWith("/loot") && !path.startsWith("/captures") && path != "/") {
            req->send(403, "text/plain", "forbidden");
        } else {
            File dir = SD.open(path);
            if (!dir) {
                req->send(404, "text/plain", "not found");
            } else if (!dir.isDirectory()) {
                dir.close();
                req->send(404, "text/plain", "not a directory");
            } else {
                String json = "{\"event\":\"fs_listing\",\"path\":\"" + path + "\",\"files\":[";
                bool first = true;
                File entry = dir.openNextFile();
                while (entry) {
                    if (!entry.isDirectory()) {
                        String name = String(entry.name());
                        name.replace("\\", "\\\\"); name.replace("\"", "\\\"");
                        if (!first) json += ",";
                        json += "{\"name\":\"" + name + "\",\"size\":" + String(entry.size()) + "}";
                        first = false;
                    }
                    entry.close();
                    entry = dir.openNextFile();
                }
                dir.close();
                json += "]}";
                req->send(200, "application/json", json);
            }
        }
    }

    // Drain /dl pending request (SD I/O on Core 1 — SPI bus safe)
    if (pendingDlFile && pendingDlReq) {
        AsyncWebServerRequest* req = pendingDlReq;
        pendingDlReq  = nullptr;
        pendingDlFile = false;
        String path = String(pendingDlPath);
        if (!path.startsWith("/loot") && !path.startsWith("/captures")) {
            req->send(403, "text/plain", "forbidden");
        } else if (!SD.exists(path)) {
            req->send(404, "text/plain", "file not found");
        } else {
            req->send(SD, path, "application/octet-stream", true);
        }
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
    char msg[128];
    snprintf(msg, sizeof(msg),
             "{\"event\":\"going_offline\",\"reason\":\"radio_conflict\",\"tool\":\"%s\"}",
             pendingOfflineTool[0] ? pendingOfflineTool : "unknown");
    ws.textAll(msg);
    pendingOfflineTool[0] = '\0';
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
