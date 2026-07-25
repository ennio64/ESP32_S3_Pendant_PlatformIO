#ifndef CNC_CONTROLLER_H
#define CNC_CONTROLLER_H

#include <Arduino.h>
#include "Machine_State.h"

class CNCController {
private:
  static const unsigned long AUTO_REPORT_TIMEOUT = 20;
  static const unsigned long GRBL_CONNECTION_TIMEOUT = 15000;
  static unsigned long lastStatusUpdate;

public:
  static void setup();
  static void update();
  static void initializeGrblSettings();
  static String queryImmediateState(unsigned long timeoutMs = 800);
  static float readParamWithRetry(const char* param, int wait = 200);
  static void checkAutoReportStatus();
  static bool AutoReportStatus;
  static void readMaxFeedrates();
  static void handlePolling();
  static void sendJogCommand(const String& axis, float distance, int feedrate);
  static void sendGCode(const String& command);
  static void sendJogCancel();
  static bool isConnected();
};

#endif