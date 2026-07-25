#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <WebServer.h>
#include <HTTPUpdateServer.h>

class OTAManager {
private:
  static WebServer server;
  static HTTPUpdateServer httpUpdater;
  static bool otaEnabled;
  static unsigned long otaStartTime;

public:
  static void setup();
  static void update();
  static bool isOTAEnabled();
  static void disableOTA();
  static String getConnectionInfo();
  static unsigned long getOTAUptime();
};

#endif