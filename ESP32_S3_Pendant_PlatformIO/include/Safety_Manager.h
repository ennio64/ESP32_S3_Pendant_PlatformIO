#ifndef SAFETY_MANAGER_H
#define SAFETY_MANAGER_H

#include <Arduino.h>
#include "Machine_State.h"
#include "Input_Handler.h"
#include "CNC_Controller.h"
#include "LED_Manager.h"

class SafetyManager {
private:
  static const unsigned long JOYSTICK_MAX_DURATION = 300000;  // 5 minutes
  static unsigned long joystickActiveStart;
  static unsigned long lastSafetyCheck;

  static void checkJoystickTimeout();

public:
  static void begin();
  static void update();

  // Limit Management
  static void setXPlusLimit();
  static void setXMinusLimit();
  static void setZPlusLimit();
  static void setZMinusLimit();
  static void clearXPlusLimit();
  static void clearXMinusLimit();
  static void clearZPlusLimit();
  static void clearZMinusLimit();

  // Safety Distance Management
  static bool setXSafeDistance(float distance);
  static bool setZSafeDistance(float distance);

  // GOTO Movements
  static void executeGoToX();
  static void executeGoToZ();

  // GOTO Condition Checks (ORA PUBBLICHE)
  static bool canExecuteGoToX();
  static bool canExecuteGoToZ();

  // Joystick Management
  static void onJoystickActivated();
  static void checkJoystickLimits();
};

#endif