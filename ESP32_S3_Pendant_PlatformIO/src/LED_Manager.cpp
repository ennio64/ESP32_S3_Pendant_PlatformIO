#include "LED_Manager.h"
#include "ButtonLedMap.h"
#include "PCF8575_Manager.h"
#include "Safety_Manager.h"

extern PCF8575 pcfA;
extern PCF8575 pcfB;

// Stato flash LED
LEDManager::FlashLED LEDManager::flashX = { false, 0 };
LEDManager::FlashLED LEDManager::flashZ = { false, 0 };

void LEDManager::initializeLEDs() {
    Serial.println("💡 Initializing LEDs...");
    
    // Spegni tutti i LED
    for (uint8_t i = 0; i < NUM_CONTROLS; i++) {
        setLEDState(i, false);
    }
    
    // Test sequenza
    testLEDs();
}

void LEDManager::setLEDState(uint8_t index, bool state) {
    if (index >= NUM_CONTROLS) return; // protezione
    
    uint8_t pin = controls[index].ledPin;

    // 🔎 Decidi se il pin è su pcfA o pcfB
    if (pin <= 7) {
        pcfA.write(pin, state ? LOW : HIGH); // attivo-basso
    } else {
        pcfB.write(pin, state ? LOW : HIGH); // attivo-basso
    }

    controls[index].ledOn = state; // aggiorna stato interno
}

bool LEDManager::getLEDState(uint8_t index) {
    if (index >= NUM_CONTROLS) return false;
    uint8_t pin = controls[index].ledPin;

    if (pin <= 7) {
        return (pcfA.read(pin) == LOW);
    } else {
        return (pcfB.read(pin) == LOW);
    }
}

void LEDManager::testLEDs() {
    Serial.println("💡 Testing LEDs...");
    
    // Sequenza uno per uno
    for (uint8_t i = 0; i < NUM_CONTROLS; i++) {
        setLEDState(i, true);
        delay(100);
        setLEDState(i, false);
    }
    
    delay(500);

    // Tutti accesi
    allLEDsOn();
    //delay(1000);

    // Tutti spenti
    //allLEDsOff();
    
    Serial.println("✅ LED Test Complete");
}

void LEDManager::allLEDsOn() {
    for (uint8_t i = 0; i < NUM_CONTROLS; i++) {
        setLEDState(i, true);
    }
}

void LEDManager::allLEDsOff() {
    for (uint8_t i = 0; i < NUM_CONTROLS; i++) {
        setLEDState(i, false);
    }
}

void LEDManager::triggerXFlash() {
    setLEDState(5, true);
    flashX.active = true;
    flashX.startTime = millis();
}

void LEDManager::triggerZFlash() {
    setLEDState(10, true);
    flashZ.active = true;
    flashZ.startTime = millis();
}

void LEDManager::setZeroLEDsOff() {
  if (flashX.active && millis() - flashX.startTime >= 2000) {
    LEDManager::setLEDState(5, false);
    flashX.active = false;
  }

  if (flashZ.active && millis() - flashZ.startTime >= 2000) {
    LEDManager::setLEDState(10, false);
    flashZ.active = false;
  }
}

void LEDManager::updateGotoLEDs() {
    static unsigned long lastBlink = 0;
    static bool blinkState = false;

    if (millis() - lastBlink >= 500) {
        blinkState = !blinkState;
        lastBlink = millis();

        // 🎯 GOTO X LED (pulsante 7 → controls[7].ledPin)
        bool gotoXConditions = SafetyManager::canExecuteGoToX();
        setLEDState(7, gotoXConditions ? blinkState : false);

        // 🎯 GOTO Z LED (pulsante 8 → controls[8].ledPin)
        bool gotoZConditions = SafetyManager::canExecuteGoToZ();
        setLEDState(8, gotoZConditions ? blinkState : false);
    }
}

void LEDManager::updateBlinkingLEDs() {
    static unsigned long lastBlink = 0;
    static bool blinkState = false;
    
    if (millis() - lastBlink >= 500) {
        blinkState = !blinkState;
        lastBlink = millis();

        // Lampeggio GoTo X (pulsante 7)
        if (SafetyManager::canExecuteGoToX()) {
            setLEDState(7, blinkState);
        }

        // Lampeggio GoTo Z (pulsante 8)
        if (SafetyManager::canExecuteGoToZ()) {
            setLEDState(8, blinkState);
        }

        // Lampeggio Safe Distance X (pulsante 6) se in modalità interattiva
        if (machine.xSafeDistanceMode) {
            setLEDState(6, blinkState);
        }

        // Lampeggio Safe Distance Z (pulsante 9) se in modalità interattiva
        if (machine.zSafeDistanceMode) {
            setLEDState(9, blinkState);
        }
    }
}