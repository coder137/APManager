#include "APManager.h"

// DONE
void WifiHelper::startWiFi(const char *ssid, const char *password)
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    // ~ OLD Code
    // WiFi.mode(WIFI_STA);

    // ! THIS IS IMPORTANT FOR SOME REASON
    // * If connection hangups happen use this
    // ETS_UART_INTR_DISABLE();
    // wifi_station_disconnect();
    // ETS_UART_INTR_ENABLE();

    // WiFi.begin(storeSSID, storePassword);
}

// DONE
/**
 * bool noTimeout -> false (timeout to check if WIFI is connected)
 * bool noTimeout -> true (loops forever) TEST
 */
bool WifiHelper::checkWiFiConnect(int counterMatch, bool noTimeout)
{
    int counter = 0;
    while (1)
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.printf(".");
            delay(1000);
            counter++;
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            return true;
        }

        if (!noTimeout)
        {
            if (counter >= counterMatch)
            {
                return false;
            }
        }
    }
}

// DONE
bool WifiHelper::startAP()
{
    String ap_ssid = "ESP-" + WiFi.macAddress();
    WiFi.mode(WIFI_AP);
    boolean result = WiFi.softAP(ap_ssid.c_str(), ap_ssid.c_str());
    Serial.printf("Started with IP: %s\n", WiFi.softAPIP().toString().c_str());
    return result;
}

// TODO, Add more debug apis here
// DONE, Remove lambdas from here
void WifiHelper::startWebserver(int port)
{

    server.onNotFound(std::bind(&APManager::handleNotFound, this));

    // Start
    server.begin(port);
    isServerRunning = true;
    Serial.println("Started webserver");
}

void WifiHelper::saveToFile(const char *filename, const char *param1, const char *param2, bool overwrite)
{
    if (!overwrite)
    {
        bool found = SPIFFS.exists(filename);
        if (found)
        {
            Serial.println("Overwrite for file is false!!");
            return;
        }
    }

    File f = SPIFFS.open(filename, "w");
    if (!f)
    {
        Serial.println("File open FAILED");
        return;
    }

    f.println(param1);
    f.println(param2);
    f.flush();
    f.close();
    Serial.printf("Written %s::%s to file--%s\n", param1, param2, filename);
}

ESP8266WebServer &WifiHelper::getServerReference()
{
    return server;
}

// TODO, Work on this
String WifiHelper::validatePostForm()
{
    String data = "";

    // NOTE, This is how you get body data ("plain") -> body
    if (!server.hasArg("plain"))
    {
        // server.send(400, "text/plain", "Body not received");
        // return ;
        data = "Body not received";
        return data;
    }

    // DONE, Add other things here
    return data;
}

String WifiHelper::getJSONObject(cJSON *jObj, const char *objectname)
{
    String data = "";
    cJSON *object = cJSON_GetObjectItemCaseSensitive(jObj, objectname);
    if (object == NULL)
    {
        // server.send(400, "text/plain", "PASSWORD Parameter is required");
        // return ;
        return data;
    }
    char *password = object->valuestring;
    data = String(password);
    return data;
}

// * PRIVATE -----------------

void WifiHelper::handleNotFound()
{
    server.send(404, "text/plain", "Request Invalid");
}

// * APManager ---------------------

// DONE
// TODO, Migrate to APManager
int APManager::autoconnect()
{
    // DONE, Check if SPiffs file is present or no
    bool check = SPIFFS.exists(manager::file::WIFITXT);
    if (!check)
    {
        // Does not exist
        Serial.printf("%s does not exist\n", manager::file::WIFITXT);

        // DONE, Start AP here
        bool started = startAP();
        if (!started)
        {
            Serial.println("Could NOT start SOFTAP");
            return AP_ERROR;
        }
        Serial.println("Started SOFTAP");

        startWebserver(80);
        while (serverRunning())
        {
            server.handleClient();
        }
        server.close();
        Serial.println("Closing Webserver here...");
        Serial.println("WiFI Connected successfully");
        return STARTED;
    }

    // DONE, Open the file
    Serial.printf("Trying values from file: %s\n", manager::file::WIFITXT);
    File f = SPIFFS.open(manager::file::WIFITXT, "r");
    String ssid = f.readStringUntil('\n');
    String password = f.readStringUntil('\n');
    ssid.trim();
    password.trim();

    Serial.println(ssid);
    Serial.println(password);
    // * Close the file
    f.close();

    // Start connection to wifi
    startWiFi(ssid.c_str(), password.c_str());

    // NOTE, Since noTimeout is True, connected will only return TRUE
    // * Pass false to the parameter to enable checking (add your own custom code below it)
    bool connected = checkWiFiConnect(15, true);
    if (!connected)
    {
        SPIFFS.remove(manager::file::WIFITXT);
        autoconnect();
    }
    return STARTED;
}

void APManager::configureServer()
{
    // HTTP POST
    server.on(manager::endpoint::CONNECTWIFI, HTTP_POST, std::bind(&APManager::handleConnectWifi, this));

    // HTTP GET
    server.on(manager::endpoint::DEVICEINFO, HTTP_GET, std::bind(&APManager::handleDeviceInfo, this));
    server.on(manager::endpoint::SCANINFO, HTTP_GET, std::bind(&APManager::handleScanInfo, this));
}

void APManager::saveWiFiConfig(const char *ssid, const char *password)
{
    saveToFile(manager::file::WIFITXT, ssid, password, true);
}

// DONE, Get the JSON body here
// DONE, Parse the JSON
// DONE, Try to reconnect to router
void APManager::handleConnectWifi()
{
    static bool connecting = false;

    if (connecting)
    {
        server.send(400, manager::contentType::TEXT_PLAIN, "Trying to establish connection");
        return;
    }

    // NOTE, This is how you get body data ("plain") -> body
    if (!server.hasArg("plain"))
    {
        server.send(400, manager::contentType::TEXT_PLAIN, manager::responses::HTTP_BODYMISSING);
        return;
    }

    // DONE, Parse the JSON received here
    String message = server.arg("plain");
    cJSON *jObj = cJSON_Parse(message.c_str());
    if (jObj == NULL)
    {
        server.send(400, manager::contentType::TEXT_PLAIN, manager::responses::INVALIDJSON);
        return;
    }

    // * Get the objects here and test for condition
    String ssid = getJSONObject(jObj, manager::wifi::SSID);
    String password = getJSONObject(jObj, manager::wifi::PASSWORD);

    if (ssid.equals("") || password.equals(""))
    {
        server.send(400, manager::contentType::TEXT_PLAIN, "SSID/PASSWORD parameters are required");
        return;
    }
    // ? debug
    Serial.printf("%s && %s\n", ssid.c_str(), password.c_str());

    // ! Cleanup
    cJSON_Delete(jObj);

    // send
    // delay is added so that the user gets the response before Wifi mode is changed
    server.send(200, manager::contentType::TEXT_PLAIN, "Data received. Connecting...");
    delay(500);
    connecting = true;

    // Try to connect here
    // DONE, Create wifiConnected function
    startWiFi(ssid.c_str(), password.c_str());
    bool connected = checkWiFiConnect(15, false);

    if (!connected)
    {
        Serial.println("Starting AP Portal again");
        startAP();
        connecting = false;
        return;
    }

    Serial.println("Connected to WIFI Successfully...");
    // Store the data here
    saveWiFiConfig(ssid.c_str(), password.c_str());

    // NOTE, Set this at the very end
    isServerRunning = false;
}

// // * ------ GET FUNCTIONS ---------

// TODO, text/json
void APManager::handleDeviceInfo()
{
    // DONE, We need to send MacAddress
    String DeviceMac = WiFi.macAddress();

    // DONE, Create a JSON string here
    cJSON *object = cJSON_CreateObject();
    cJSON *mac = cJSON_CreateString(DeviceMac.c_str());
    cJSON_AddItemToObject(object, manager::device::MACADDR, mac);
    // TODO, Add other deviceInformation here

    // NOTE, Send info
    char *deviceInfo = cJSON_Print(object);
    server.send(200, manager::contentType::APPLICATION_JSON, deviceInfo);

    // ! Clean up object
    free(deviceInfo);
    cJSON_Delete(object);
}

/**
 * {
 * "networks" : [
 *      {
 *          "ssid": "<ssid>",
 *          "rssi": "<rssi>"
 *          * Add more if needed
 *      },
 *      {
 *      }
 *  ]
 * }
 */
// DONE
// TODO, text/json
void APManager::handleScanInfo()
{

    cJSON *object = cJSON_CreateObject();
    cJSON *networkArray = cJSON_CreateArray();

    int networks = WiFi.scanNetworks();

    String data = "";
    for (int i = 0; i < networks; i++)
    {
        cJSON *networkObject = cJSON_CreateObject();
        cJSON *ssid = cJSON_CreateString(WiFi.SSID(i).c_str());
        cJSON *rssi = cJSON_CreateNumber(WiFi.RSSI(i));
        cJSON_AddItemToObject(networkObject, manager::wifi::SSID, ssid);
        cJSON_AddItemToObject(networkObject, manager::wifi::RSSI, rssi);
        cJSON_AddItemToArray(networkArray, networkObject);

        // ? debug
        Serial.printf("%s : %d\n", ssid->valuestring, rssi->valueint);
    }
    cJSON_AddItemToObject(object, manager::device::NETWORKS, networkArray);

    // Send the object
    server.send(200, manager::contentType::APPLICATION_JSON, cJSON_Print(object));

    // ! Clean the object
    cJSON_Delete(object);
}
