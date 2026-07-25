#include "ButtonLedMap.h"

// Definizione array pulsanti/LED
ButtonLed controls[] = {
  { 4, 0, false, false, false },    // Pulsante 0: P4 -> LED P0 → Toggle Z- Limit (SafetyManager::set/clearZMinusLimit)
  { 5, 1, false, false, false },    // Pulsante 1: P5 -> LED P1 → Toggle Z+ Limit (SafetyManager::set/clearZPlusLimit)
  { 6, 2, false, false, false },    // Pulsante 2: P6 -> LED P2 → Toggle X- Limit (SafetyManager::set/clearXMinusLimit)
  { 7, 3, false, false, false },    // Pulsante 3: P7 -> LED P3 → Toggle X+ Limit (SafetyManager::set/clearXPlusLimit)
  { 8, 8, false, false, false },    // Pulsante 4: P8 -> LED P8 → Toggle Radius/Diameter (DisplayManager::toggleRadiusDiameter)
  { 9, 9, false, false, false },    // Pulsante 5: P9 -> LED P9 → Set X Zero (ButtonHandler::setXZero)
  { 10, 10, false, false, false },  // Pulsante 6: P10 -> LED P10 → Enter X Safe Distance Mode (ButtonHandler::enterXSafeDistanceMode)
  { 11, 11, false, false, false },  // Pulsante 7: P11 -> LED P11 → Execute GoTo X (SafetyManager::executeGoToX)
  { 12, 12, false, false, false },  // Pulsante 8: P12 -> LED P12 → Execute GoTo Z (SafetyManager::executeGoToZ)
  { 13, 13, false, false, false },  // Pulsante 9: P13 -> LED P13 → Enter Z Safe Distance Mode (ButtonHandler::enterZSafeDistanceMode)
  { 14, 14, false, false, false },  // Pulsante 10: P14 -> LED P14 → Set Z Zero (ButtonHandler::setZZero)
  { 15, 15, false, false, false }   // Pulsante 11: P15 -> LED P15 → Toggle Encoders Lock (InputHandler::toggleEncodersLock)
};

// Numero totale di controlli
const uint8_t NUM_CONTROLS = sizeof(controls) / sizeof(controls[0]);


