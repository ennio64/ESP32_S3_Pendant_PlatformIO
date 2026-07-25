#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <Arduino.h>
#include "ButtonLedMap.h"

class ButtonHandler {
public:
  static void begin();
  static void update();
  static void handleButtonPress(uint8_t buttonIndex);

  // Stato encoders
  static bool areEncodersLocked();
  static void setEncodersLocked(bool state);
  static void toggleEncodersLock();

  // Funzioni pulsanti
  static void setXZero();
  static void setZZero();
  static void enterXSafeDistanceMode();
  static void enterZSafeDistanceMode();

  // LED
  static void updateButtonLED(uint8_t buttonIndex);

  // Stato pulsanti
  static bool getLastButtonState(uint8_t index);
  static void setLastButtonState(uint8_t index, bool state);

private:
  static bool encodersLocked;
};

#endif
