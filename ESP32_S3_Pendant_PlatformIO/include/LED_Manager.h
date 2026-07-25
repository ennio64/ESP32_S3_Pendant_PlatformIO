#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <Arduino.h>
#include "PCF8575_Manager.h"

class LEDManager {
public:
  static void initializeLEDs();
  static void setLEDState(uint8_t index, bool state);
  static void testLEDs();
  static void allLEDsOn();
  static void allLEDsOff();
  static void updateBlinkingLEDs();
  static bool getLEDState(uint8_t index);
  static void setZeroLEDsOff();
  static void updateGotoLEDs();
  static void triggerXFlash();
  static void triggerZFlash();

private:
  // Gestione flash LED non bloccante
  struct FlashLED {
    bool active;
    unsigned long startTime;
  };
  static FlashLED flashX;
  static FlashLED flashZ;

  static void blinkLED(uint8_t index, unsigned long interval);
};

#endif