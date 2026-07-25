#include "Button_Handler.h"
#include "ButtonLedMap.h"
#include "Safety_Manager.h"
#include "LED_Manager.h"
#include "Input_Handler.h"
#include "Display_Manager.h"
#include "CNC_Controller.h"

extern PCF8575 pcfA;

bool ButtonHandler::encodersLocked = false;

// ================== STATO PULSANTI ==================
bool ButtonHandler::getLastButtonState(uint8_t index) {
  return controls[index].lastButtonState;
}

void ButtonHandler::setLastButtonState(uint8_t index, bool state) {
  controls[index].lastButtonState = state;
}

// ================== INIZIALIZZAZIONE ==================
void ButtonHandler::begin() {
  Serial.println("🔘 Button Handler initialized");

  Serial.println("🔘 Stato iniziale pulsanti (P4-P15):");
  for (uint8_t i = 4; i <= 15; i++) {
    bool state = (pcfA.read(i) == LOW);
    controls[i - 4].lastButtonState = state;
    Serial.printf("Pulsante %d (P%d): %s\n", i - 4, i, state ? "Premuto" : "Rilasciato");
  }
}

// ================== LOOP DI AGGIORNAMENTO ==================
void ButtonHandler::update() {
  for (uint8_t i = 0; i < NUM_CONTROLS; i++) {
    bool buttonState = (pcfA.read(controls[i].buttonPin) == LOW);

    if (buttonState && !getLastButtonState(i)) {
      handleButtonPress(i);
    }
  }
}

// ================== GESTIONE PRESSIONE PULSANTE ==================
void ButtonHandler::handleButtonPress(uint8_t buttonIndex) {
  if (buttonIndex >= NUM_CONTROLS) return;

  Serial.printf("🔘 Button %d pressed\n", buttonIndex);

  switch (buttonIndex) {
    // Limits
    case 0: machine.zMinusLimit ? SafetyManager::clearZMinusLimit() : SafetyManager::setZMinusLimit(); break;
    case 1: machine.zPlusLimit ? SafetyManager::clearZPlusLimit() : SafetyManager::setZPlusLimit(); break;
    case 2: machine.xMinusLimit ? SafetyManager::clearXMinusLimit() : SafetyManager::setXMinusLimit(); break;
    case 3: machine.xPlusLimit ? SafetyManager::clearXPlusLimit() : SafetyManager::setXPlusLimit(); break;

    // X Functions
    case 4: DisplayManager::toggleRadiusDiameter(); break;
    case 5: setXZero(); break;
    case 6: enterXSafeDistanceMode(); break;
    case 7:
      if (SafetyManager::canExecuteGoToX()) SafetyManager::executeGoToX();
      else Serial.println("⚠️ GoToX non abilitato");
      break;
    // Z Functions
    case 8:
      if (SafetyManager::canExecuteGoToZ()) SafetyManager::executeGoToZ();
      else Serial.println("⚠️ GoToZ non abilitato");
      break;
    case 9: enterZSafeDistanceMode(); break;
    case 10: setZZero(); break;
    case 11: toggleEncodersLock(); break;
  }

  updateButtonLED(buttonIndex);
}

// ================== UPDATE LED ==================
void ButtonHandler::updateButtonLED(uint8_t buttonIndex) {
  bool ledState = false;

  switch (buttonIndex) {
    case 0: ledState = machine.zMinusLimit; break;
    case 1: ledState = machine.zPlusLimit; break;
    case 2: ledState = machine.xMinusLimit; break;
    case 3: ledState = machine.xPlusLimit; break;
    case 4: ledState = machine.radiusDiameterMode; break;
    case 5: /* gestito da flashX */ return;
    case 6: ledState = (machine.xSafeDistance != 0); break;
    case 7: ledState = SafetyManager::canExecuteGoToX(); break;
    case 8: ledState = SafetyManager::canExecuteGoToZ(); break;
    case 9: ledState = (machine.zSafeDistance != 0); break;
    case 10: /* gestito da flashZ */ return;
    case 11: ledState = encodersLocked; break;

    default: return;
  }

  LEDManager::setLEDState(buttonIndex, ledState);
}

// ================== LOCK/UNLOCK ENCODERS ==================
bool ButtonHandler::areEncodersLocked() {
  return encodersLocked;
}

void ButtonHandler::setEncodersLocked(bool state) {
  encodersLocked = state;
}

void ButtonHandler::toggleEncodersLock() {
  bool newState = !ButtonHandler::areEncodersLocked();

  if (newState) {
    InputHandler::suspendEncoderReading();
  } else {
    InputHandler::resumeEncoderReading();
    InputHandler::resetEncoderDelta();   // 🔥 Resetta delta per evitare accumulo
  }

  ButtonHandler::setEncodersLocked(newState);
  Serial.printf("🔒 Encoders lock: %s\n", newState ? "LOCKED" : "UNLOCKED");
}

// ================== SET ZERO ==================
void ButtonHandler::setXZero() {
  LEDManager::triggerXFlash();
  enqueueGCode("G10 P0 L20 X0");
  Serial.println("🎯 X zero set in coordinate system");
  SafetyManager::clearXMinusLimit();
  SafetyManager::clearXPlusLimit();
  updateButtonLED(2); // X- Limit LED
  updateButtonLED(3); // X+ Limit LED
}

void ButtonHandler::setZZero() {
  LEDManager::triggerZFlash();
  enqueueGCode("G10 P0 L20 Z0");
  Serial.println("🎯 Z zero set in coordinate system");
  SafetyManager::clearZMinusLimit();
  SafetyManager::clearZPlusLimit();
  updateButtonLED(0); // Z- Limit LED
  updateButtonLED(1); // Z+ Limit LED
}

// ================== MODALITÀ DISTANZA DI SICUREZZA X ==================
void ButtonHandler::enterXSafeDistanceMode() {
  Serial.println(">>> ENTER X SAFE DISTANCE MODE <<<");

  if (!machine.xSafeDistanceMode) {
    InputHandler::suspendEncoderReading();   // 🔥 Sospendi lettura

    machine.xSafeDistanceMode = true;
    machine.xSafeDistanceEnc = 0;
    machine.xSafeDistance = 0.0f;

    Serial.println("🎯 Entering X Safety Distance Interactive Mode");
    DisplayManager::showSafeDistanceScreen('X', 0.0);

    controls[6].lastButtonState = true;

    while (machine.xSafeDistanceMode) {
      static int lastA = digitalRead(config.encoderXA);
      int currentA = digitalRead(config.encoderXA);

      if (currentA != lastA && currentA == HIGH) {
        if (digitalRead(config.encoderXB) == HIGH) {
          machine.xSafeDistanceEnc++;
        } else {
          machine.xSafeDistanceEnc--;
        }
        machine.xSafeDistance = machine.xSafeDistanceEnc * 0.01f;
        DisplayManager::showSafeDistanceScreen('X', machine.xSafeDistance);
      }
      lastA = currentA;

      bool currentState = (pcfA.read(controls[6].buttonPin) == LOW);
      if (currentState && !controls[6].lastButtonState) {
        delay(50);
        machine.xSafeDistanceMode = false;
      }
      controls[6].lastButtonState = currentState;

      static unsigned long lastBlink = 0;
      static bool blinkState = false;
      if (millis() - lastBlink > 100) {
        lastBlink = millis();
        blinkState = !blinkState;
        LEDManager::setLEDState(6, blinkState);
      }

      delay(1);
    }

    if (machine.xSafeDistanceEnc != 0) {
      SafetyManager::setXSafeDistance(machine.xSafeDistance);
      LEDManager::setLEDState(6, true);
      DisplayManager::showSafeDistanceConfirmation('X', machine.xSafeDistance);
      Serial.printf("✅ X Safety Distance set to: %.2f mm\n", machine.xSafeDistance);
    } else {
      SafetyManager::setXSafeDistance(0);
      LEDManager::setLEDState(6, false);
      DisplayManager::showSafeDistanceCancelled('X');
      Serial.println("❌ X Safety Distance cancelled");
    }

    delay(2000);

    InputHandler::resumeEncoderReading();    // 🔥 Ripristina lettura
    InputHandler::resetEncoderDelta();       // 🔥 Resetta delta
    DisplayManager::update();
  }
  machine.xSafeDistanceMode = false;
  controls[6].lastButtonState = false;
}

// ================== MODALITÀ DISTANZA DI SICUREZZA Z ==================
void ButtonHandler::enterZSafeDistanceMode() {
  Serial.println(">>> ENTER Z SAFE DISTANCE MODE <<<");

  if (!machine.zSafeDistanceMode) {
    InputHandler::suspendEncoderReading();   // 🔥 Sospendi lettura

    machine.zSafeDistanceMode = true;
    machine.zSafeDistanceEnc = 0;
    machine.zSafeDistance = 0.0f;

    Serial.println("🎯 Entering Z Safety Distance Interactive Mode");
    DisplayManager::showSafeDistanceScreen('Z', 0.0);

    controls[9].lastButtonState = true;

    while (machine.zSafeDistanceMode) {
      static int lastA = digitalRead(config.encoderZA);
      int currentA = digitalRead(config.encoderZA);

      if (currentA != lastA && currentA == HIGH) {
        if (digitalRead(config.encoderZB) == HIGH) {
          machine.zSafeDistanceEnc++;
        } else {
          machine.zSafeDistanceEnc--;
        }
        machine.zSafeDistance = machine.zSafeDistanceEnc * 0.01f;
        DisplayManager::showSafeDistanceScreen('Z', machine.zSafeDistance);
      }
      lastA = currentA;

      bool currentState = (pcfA.read(controls[9].buttonPin) == LOW);
      if (currentState && !controls[9].lastButtonState) {
        delay(50);
        machine.zSafeDistanceMode = false;
      }
      controls[9].lastButtonState = currentState;

      static unsigned long lastBlink = 0;
      static bool blinkState = false;
      if (millis() - lastBlink > 100) {
        lastBlink = millis();
        blinkState = !blinkState;
        LEDManager::setLEDState(9, blinkState);
      }

      delay(1);
    }

    if (machine.zSafeDistanceEnc != 0) {
      SafetyManager::setZSafeDistance(machine.zSafeDistance);
      LEDManager::setLEDState(9, true);
      DisplayManager::showSafeDistanceConfirmation('Z', machine.zSafeDistance);
      Serial.printf("✅ Z Safety Distance set to: %.2f mm\n", machine.zSafeDistance);
    } else {
      SafetyManager::setZSafeDistance(0);
      LEDManager::setLEDState(9, false);
      DisplayManager::showSafeDistanceCancelled('Z');
      Serial.println("❌ Z Safety Distance cancelled");
    }

    delay(2000);

    InputHandler::resumeEncoderReading();    // 🔥 Ripristina lettura
    InputHandler::resetEncoderDelta();       // 🔥 Resetta delta
    DisplayManager::update();
  }
  machine.zSafeDistanceMode = false;
  controls[9].lastButtonState = false;
}