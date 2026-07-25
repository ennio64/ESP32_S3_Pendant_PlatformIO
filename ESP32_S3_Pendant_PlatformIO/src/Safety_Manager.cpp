#include "Safety_Manager.h"
#include "ButtonLedMap.h"
#include "Input_Handler.h"
#include "CNC_Controller.h"
#include "Globals.h"

unsigned long SafetyManager::joystickActiveStart = 0;
unsigned long SafetyManager::lastSafetyCheck = 0;

void SafetyManager::begin() {
  joystickActiveStart = 0;
  lastSafetyCheck = 0;
  Serial.println("🛡️ Safety Manager initialized");
}

void SafetyManager::update() {
  unsigned long currentTime = millis();

  if (currentTime - lastSafetyCheck >= 100) {
    checkJoystickLimits();
    checkJoystickTimeout();
    LEDManager::updateGotoLEDs();
    lastSafetyCheck = currentTime;
  }
}

// ========== LIMIT MANAGEMENT ==========
void SafetyManager::setXPlusLimit() {
  machine.xPlusLimitPosition = machine.wposX;
  machine.xPlusLimit = true;
  Serial.printf("🔒 X+ Limit set at: %.3f mm\n", machine.xPlusLimitPosition);
}

void SafetyManager::clearXPlusLimit() {
  machine.xPlusLimitPosition = 300.0;
  machine.xPlusLimit = false;
  Serial.println("❌ X+ Limit cleared");
}

void SafetyManager::setXMinusLimit() {
  machine.xMinusLimitPosition = machine.wposX;
  machine.xMinusLimit = true;
  Serial.printf("🔒 X- Limit set at: %.3f mm\n", machine.xMinusLimitPosition);
}

void SafetyManager::clearXMinusLimit() {
  machine.xMinusLimitPosition = -300.0;
  machine.xMinusLimit = false;
  Serial.println("❌ X- Limit cleared");
}

void SafetyManager::setZPlusLimit() {
  machine.zPlusLimitPosition = machine.wposZ;
  machine.zPlusLimit = true;
  Serial.printf("🔒 Z+ Limit set at: %.3f mm\n", machine.zPlusLimitPosition);
}

void SafetyManager::clearZPlusLimit() {
  machine.zPlusLimitPosition = 300.0;
  machine.zPlusLimit = false;
  Serial.println("❌ Z+ Limit cleared");
}

void SafetyManager::setZMinusLimit() {
  machine.zMinusLimitPosition = machine.wposZ;
  machine.zMinusLimit = true;
  Serial.printf("🔒 Z- Limit set at: %.3f mm\n", machine.zMinusLimitPosition);
}

void SafetyManager::clearZMinusLimit() {
  machine.zMinusLimitPosition = -300.0;
  machine.zMinusLimit = false;
  Serial.println("❌ Z- Limit cleared");
}

// ========== JOYSTICK LIMIT CHECK ==========
void SafetyManager::checkJoystickLimits() {
  if (!input.active) return;

  switch (input.rawState) {
    case 13:
      if (machine.xPlusLimit && machine.wposX >= machine.xPlusLimitPosition) {
        InputHandler::stopAllMotion();
        Serial.println("🛑 X+ LIMIT REACHED - Joystick stopped");
      }
      break;
    case 14:
      if (machine.xMinusLimit && machine.wposX <= machine.xMinusLimitPosition) {
        InputHandler::stopAllMotion();
        Serial.println("🛑 X- LIMIT REACHED - Joystick stopped");
      }
      break;
    case 7:
      if (machine.zPlusLimit && machine.wposZ >= machine.zPlusLimitPosition) {
        InputHandler::stopAllMotion();
        Serial.println("🛑 Z+ LIMIT REACHED - Joystick stopped");
      }
      break;
    case 11:
      if (machine.zMinusLimit && machine.wposZ <= machine.zMinusLimitPosition) {
        InputHandler::stopAllMotion();
        Serial.println("🛑 Z- LIMIT REACHED - Joystick stopped");
      }
      break;
  }
}

// ========== SAFETY DISTANCE ==========
bool SafetyManager::setXSafeDistance(float distance) {
  if (abs(distance) > 100.0) {
    Serial.println("❌ X Safety distance too large");
    return false;
  }
  machine.xSafeDistance = distance;
  Serial.printf("✅ X Safety distance: %.2f mm\n", distance);
  return true;
}

bool SafetyManager::setZSafeDistance(float distance) {
  if (abs(distance) > 100.0) {
    Serial.println("❌ Z Safety distance too large");
    return false;
  }
  machine.zSafeDistance = distance;
  Serial.printf("✅ Z Safety distance: %.2f mm\n", distance);
  return true;
}

// ========== GOTO MOVEMENTS ==========
void SafetyManager::executeGoToX() {
  if (!canExecuteGoToX()) return;

  Serial.println("🎯 Executing GOTO X with safety...");

  if (machine.zSafeDistance != 0) {
    float zTarget = machine.wposZ + machine.zSafeDistance;
    String command = "G90 G0 Z" + String(zTarget, 3);
    enqueueGCode(command);
    // ⭐ M400 non è supportato da GrblHAL → rimosso
    Serial.printf("   ↳ Safety Z move: %.3f mm\n", zTarget);
  }

  float xTarget = machine.xPlusLimitPosition;
  String command = "G90 G0 X" + String(xTarget, 3);
  enqueueGCode(command);
  Serial.printf("   ↳ X move to limit: %.3f mm\n", xTarget);

  String returnCommand = "G90 G0 Z" + String(machine.wposZ, 3);
  enqueueGCode(returnCommand);
  Serial.printf("   ↩ Return to Z initial: %.3f mm\n", machine.wposZ);
}

void SafetyManager::executeGoToZ() {
  if (!canExecuteGoToZ()) return;

  Serial.println("🎯 Executing GOTO Z with safety...");

  if (machine.xSafeDistance != 0) {
    float xTarget = machine.wposX + machine.xSafeDistance;
    String command = "G90 G0 X" + String(xTarget, 3);
    enqueueGCode(command);
    // ⭐ M400 rimosso
    Serial.printf("   ↳ Safety X move: %.3f mm\n", xTarget);
  }

  float zTarget = machine.zPlusLimitPosition;
  String command = "G90 G0 Z" + String(zTarget, 3);
  enqueueGCode(command);
  Serial.printf("   ↳ Z move to limit: %.3f mm\n", zTarget);

  String returnCommand = "G90 G0 X" + String(machine.wposX, 3);
  enqueueGCode(returnCommand);
  Serial.printf("   ↩ Return to X initial: %.3f mm\n", machine.wposX);
}

// ========== GOTO CONDITION CHECKS ==========
bool SafetyManager::canExecuteGoToX() {
  if (!machine.cncConnected) return false;
  if (!machine.xPlusLimit) return false;
  if (machine.wposX >= machine.xPlusLimitPosition) return false;
  if (machine.zSafeDistance == 0) return false;
  if (machine.state == STATE_ALARM) return false;
  return true;
}

bool SafetyManager::canExecuteGoToZ() {
  if (!machine.cncConnected) return false;
  if (!machine.zPlusLimit) return false;
  if (machine.wposZ >= machine.zPlusLimitPosition) return false;
  if (machine.xSafeDistance == 0) return false;
  if (machine.state == STATE_ALARM) return false;
  return true;
}

void SafetyManager::onJoystickActivated() {
  joystickActiveStart = millis();
}

// ========== PRIVATE METHODS ==========
void SafetyManager::checkJoystickTimeout() {
  if (input.active && (millis() - joystickActiveStart > JOYSTICK_MAX_DURATION)) {
    // InputHandler::stopAllMotion();
    // Serial.println("🛑 SAFETY: Joystick timeout after 5 minutes");
  }
}