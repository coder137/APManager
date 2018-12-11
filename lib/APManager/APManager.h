#ifndef _APManager_H
#define _APManager_H

#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <FS.h>

// ! lib
#include <cJSON.h>

namespace manager {
  namespace file {
      const char WIFITXT[] = "w.txt";
  }
  namespace endpoint {
      const char CONNECTWIFI[] = "/connectWifi";
      const char DEVICEINFO[] = "/deviceInfo";
      const char SCANINFO[] = "/scanInfo";
  }
  namespace wifi {
      const char SSID[] = "ssid";
      const char PASSWORD[] = "password";
      const char RSSI[] = "rssi";
  }
  namespace device {
      const char MACADDR[] = "macaddr";
      const char NETWORKS[] = "networks";
  }
};

class WifiHelper {

protected:
    ESP8266WebServer server;
    bool isServerRunning = false;

private: 
    // Private function
    void handleNotFound();

public:
    // TODO, If needed make this an enum
    static const int AP_ERROR = 0;
    static const int STARTED = 1;

public:
    // Wifi Methods DONE
    void startWiFi(const char * ssid, const char * password);
    bool checkWiFiConnect(int counterMatch, bool noTimeout);

    // AP Methods DONE
    bool startAP();

    // Server Methods DONE
    void startWebserver(int port);
    bool serverRunning() {return isServerRunning; }

    // DONE
    // Saves a key-value pair to a file
    void saveToFile(const char * filename, const char * param1, const char * param2, bool overwrite = false);
    // void saveWiFiConfig(const char * ssid, const char * password);

    // DONE
    ESP8266WebServer& getServerReference();
};

class APManager : public WifiHelper {

// * These are webserver handle functions
private:
    // POST
    void handleConnectWifi();

public:
    // GET DONE
    void handleDeviceInfo();
    void handleScanInfo();

public:
    int autoconnect();
    void configureServer();

    // file methods DONE
    void saveWiFiConfig(const char * ssid, const char * password);
};

#endif
