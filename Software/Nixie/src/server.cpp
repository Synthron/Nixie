#include "server.h"
#include "config.h"
#include <WebServer.h>

extern Wifi wifi;

// WiFi-manager web server (only active in AP mode)
static WebServer webServer(80);
static bool wifiManagerActive = false;

// WiFi-manager page: a dropdown of scanned networks + a password field.
// Split into head/tail so the scanned SSID options can be injected between them.
static const char WIFI_MANAGER_HTML_HEAD[] = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Nixie WiFi Setup</title>
  <style>
    body { font-family: sans-serif; max-width: 400px; margin: 40px auto; padding: 0 16px; }
    input, select { width: 100%; padding: 8px; margin: 6px 0 14px; box-sizing: border-box; }
    button { width: 100%; padding: 10px; font-size: 16px; }
  </style>
</head>
<body>
  <h1>Nixie WiFi Setup</h1>
  <p>Select your WiFi network and enter its password.</p>
  <form method="POST" action="/">
    <label for="ssid">Network</label>
    <select id="ssid" name="ssid" required>
      <option value="">-- Select a network --</option>
)rawliteral";

static const char WIFI_MANAGER_HTML_TAIL[] = R"rawliteral(
    </select>
    <label for="passwd">Password</label>
    <input type="password" id="passwd" name="passwd">
    <button type="submit">Save & Reboot</button>
  </form>
</body>
</html>
)rawliteral";

// Scan for nearby networks and build the <option> list for the dropdown.
static String build_ssid_options()
{
  String options = "";
  int n = WiFi.scanNetworks();
  if (n < 0) return options; // scan failed

  String seen = ""; // deduplicate SSIDs (multiple APs may share one SSID)
  for (int i = 0; i < n; i++)
  {
    String ssid = WiFi.SSID(i);
    if (ssid.length() == 0) continue; // skip hidden networks
    if (seen.indexOf(ssid + "\n") >= 0) continue;
    seen += ssid + "\n";

    // HTML-escape the SSID for the value and label
    String esc = ssid;
    esc.replace("&", "\046amp;");
    esc.replace("\"", "\046quot;");
    esc.replace("<", "\046lt;");
    esc.replace(">", "\046gt;");
    options += "<option value=\"" + esc + "\">" + esc + "</option>";
  }
  WiFi.scanDelete(); // free the scan results
  return options;
}

static void handle_wifi_root()
{
  String html = String(WIFI_MANAGER_HTML_HEAD) + build_ssid_options() + String(WIFI_MANAGER_HTML_TAIL);
  webServer.send(200, "text/html", html);
}

static void handle_wifi_save()
{
  String ssid = webServer.arg("ssid");
  String passwd = webServer.arg("passwd");
  ssid.trim();
  passwd.trim();

  if (ssid.length() == 0)
  {
    webServer.send(400, "text/plain", "SSID is required");
    return;
  }

  strlcpy(wifi.ssid, ssid.c_str(), sizeof(wifi.ssid));
  strlcpy(wifi.passwd, passwd.c_str(), sizeof(wifi.passwd));
  wifi.mode = 1; // STA mode
  config_save();

  webServer.send(200, "text/html",
                 "<!DOCTYPE html><html><body><h1>Saved. Rebooting...</h1></body></html>");
  delay(500);
  ESP.restart();
}

static void start_wifi_manager()
{
  wifiManagerActive = true;
  webServer.on("/", HTTP_GET, handle_wifi_root);
  webServer.on("/", HTTP_POST, handle_wifi_save);
  webServer.begin();
}

// Service the WiFi-manager web server (called from Web_Tasks on core 0).
void web_server_handle()
{
  if (wifiManagerActive)
  {
    webServer.handleClient();
  }
}

// start wifi
void wifi_start(Wifi& wifi)
{
  // wifi settings are loaded once during boot via config_load()

  bool connected = false;

  // Attempt to connect as a station using the saved credentials
  if (wifi.mode == 1 && wifi.ssid[0] != '\0')
  {
    WiFi.mode(WIFI_STA);
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.setHostname(wifi.hostname);
    WiFi.begin(wifi.ssid, wifi.passwd);
    delay(2000); // wait 2sec to connect to network
    connected = WiFi.isConnected();
  }

  // If no connection, start an access point with a WiFi manager
  if (!connected)
  {
    String mac = WiFi.macAddress();
    mac.replace(":", "-");
    String ap_ssid = "Nixie-" + mac.substring(9, 17);

    WiFi.mode(WIFI_AP);
    delay(250);
    IPAddress local_IP(4, 3, 2, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(local_IP, INADDR_NONE, subnet);
    delay(250);
    WiFi.softAP(ap_ssid.c_str(), "Nixie1234");
    delay(250);

    wifi.mode = 2; // AP mode
    start_wifi_manager();
  }
}

// configure OTA
void ota_start()
{
  ArduinoOTA
      .onStart([]()
               {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type); })
      .onEnd([]()
             { Serial.println("\nEnd"); })
      .onProgress([](unsigned int progress, unsigned int total)
                  { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); })
      .onError([](ota_error_t error)
               {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed"); });

  ArduinoOTA.begin();
}