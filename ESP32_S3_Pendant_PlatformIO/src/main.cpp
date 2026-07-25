// ================================================================
// CNC Pendant - Versione ESP-NOW (PCNT hardware per encoder)
// ================================================================

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "Headers.h"
#include "ESPNow_Manager.h"
#include "CNC_Controller.h"

extern Config config;
extern TwoWire I2Ctwo;
extern Adafruit_SH1106G xDisplay;
extern Adafruit_SH1106G zDisplay;
extern Adafruit_ADS1115 ads;
extern PCF8575 pcfA;
extern PCF8575 pcfB;
extern MachineStatus machine;
extern InputState input;
extern QueueHandle_t gcodeQueue;
extern bool grblReady;
extern ESPNowManager cnc;
extern volatile bool g_suspendEncoder;

TaskHandle_t TaskMainHandle;
TaskHandle_t TaskCNCHandle;

// ========== PROTOTIPI ==========
void IRAM_ATTR handlePCFInterrupt();
void IRAM_ATTR handleJoystickChange();
void taskCNC(void* parameter);
void TaskMain(void* pvParameters);
void initializeOTAMode();
void initializeNormalMode();
bool isJoystickPressed();

// ========== CALLBACK ==========
void onPositionUpdate(const Position& pos) {
    machine.wposX = pos.x;
    machine.wposY = pos.y;
    machine.wposZ = pos.z;
    machine.wposA = pos.a;
}

void onStateChange(MachineState oldState, MachineState newState) {}
void onError(const String& error) { Serial.println("❌ Errore GRBL: " + error); }

void scanWiFiChannel() {}

bool isJoystickPressed() {
    static unsigned long lastCheck = 0;
    static bool lastState = false;
    unsigned long now = millis();
    if (now - lastCheck < 50) return lastState;
    lastCheck = now;

    bool pressed = (digitalRead(config.joyRight) == LOW ||
                    digitalRead(config.joyLeft) == LOW ||
                    digitalRead(config.joyDown) == LOW ||
                    digitalRead(config.joyUp) == LOW);
    if (pressed != lastState) {
        lastState = pressed;
        if (pressed) Serial.println("🕹️ Joystick premuto!");
    }
    return pressed;
}

void initializeOTAMode() {
    Serial.println("📡 Avvio modalità OTA...");
    OTAManager::setup();
    DisplayManager::showOTAScreen();
    while (true) {
        OTAManager::update();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void initializeNormalMode() {
    Serial.println("🚀 Starting in Normal Mode...");

    // PCF8575
    if (!pcfA.begin()) {
        Serial.println("❌ PCF8575A Error");
    } else {
        Serial.println("✅ PCF8575A OK");
        for (uint8_t pin = 0; pin <= 7; pin++) {
            pcfA.write(pin, HIGH);
        }
        for (uint8_t pin = 8; pin <= 15; pin++) {
            pcfA.write(pin, HIGH);
        }
    }

    if (!pcfB.begin()) {
        Serial.println("❌ PCF8575B Error");
    } else {
        Serial.println("✅ PCF8575B OK");
        for (uint8_t pin = 0; pin <= 15; pin++) {
            pcfB.write(pin, HIGH);
        }
    }

    LEDManager::initializeLEDs();
    delay(200);

    ButtonHandler::begin();

    pinMode(config.pcfIntPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(config.pcfIntPin), handlePCFInterrupt, FALLING);

    InputHandler::initADS1115();
    attachInterrupt(digitalPinToInterrupt(config.adsIntPin), InputHandler::ads1115AlertISR, FALLING);

    InputHandler::setupEncoders();

    attachInterrupt(digitalPinToInterrupt(config.joyRight), handleJoystickChange, CHANGE);
    attachInterrupt(digitalPinToInterrupt(config.joyLeft), handleJoystickChange, CHANGE);
    attachInterrupt(digitalPinToInterrupt(config.joyDown), handleJoystickChange, CHANGE);
    attachInterrupt(digitalPinToInterrupt(config.joyUp), handleJoystickChange, CHANGE);

    DisplayManager::begin();

    // OTA prompt per 5 secondi
    DisplayManager::showOTAPrompt();
    Serial.println("⏳ Finestra OTA attiva per 5 secondi. Premere joystick per entrare in OTA.");
    unsigned long otaPromptStart = millis();
    bool otaRequested = false;

    while (millis() - otaPromptStart < 5000) {
        if (isJoystickPressed()) {
            otaRequested = true;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    if (otaRequested) {
        Serial.println("🔄 Joystick premuto! Entro in OTA...");
        xDisplay.clearDisplay();
        zDisplay.clearDisplay();
        DisplayManager::showOTAScreen();
        OTAManager::setup();
        while (true) {
            OTAManager::update();
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    } else {
        Serial.println("⏱️ Nessun joystick premuto, continuo in modalità normale");
        xDisplay.clearDisplay();
        zDisplay.clearDisplay();
        DisplayManager::showWaitController();
        xDisplay.display();
        zDisplay.display();
        delay(100);
    }

    scanWiFiChannel();

    if (gcodeQueue == NULL) {
        gcodeQueue = xQueueCreate(50, sizeof(CNCCommand));
        if (gcodeQueue == NULL) {
            Serial.println("❌ Queue creation failed");
            while (true) delay(1000);
        }
    }

    cnc.onPosition(onPositionUpdate);
    cnc.onStateChange(onStateChange);
    cnc.onError(onError);

    bool paired = cnc.begin();
    machine.cncConnected = paired;

    if (paired) {
        Serial.println("✅ Pairing con il bridge riuscito");
        LEDManager::allLEDsOff();
        CNCController::setup();
        DisplayManager::showMainScreen();
    } else {
        Serial.println("⚠️ Pairing fallito, riproverò in background");
    }

    xTaskCreatePinnedToCore(taskCNC, "TaskCNC", 8192, NULL, 1, &TaskCNCHandle, 0);
    xTaskCreatePinnedToCore(TaskMain, "TaskMain", 16384, NULL, 2, &TaskMainHandle, 1);

    grblReady = true;
    Serial.println("✅ CNC Pendant Ready - Normal Mode");
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n\n🔥 CNC Pendant Starting...");
    Serial.printf("🔹 Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("🔹 CPU cores: %d\n", ESP.getChipCores());

    I2Ctwo.begin(config.sdaPin, config.sclPin, 400000);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    pinMode(config.joyRight, INPUT_PULLUP);
    pinMode(config.joyLeft, INPUT_PULLUP);
    pinMode(config.joyDown, INPUT_PULLUP);
    pinMode(config.joyUp, INPUT_PULLUP);

    initializeNormalMode();
}

void taskCNC(void* parameter) {
    vTaskDelay(pdMS_TO_TICKS(100));
    Serial.println("🧠 TaskCNC avviato sul core 0 (ESP-NOW)");

    CNCCommand cmd;
    unsigned long lastPoll = 0;
    const unsigned long POLL_INTERVAL = 200;

    while (true) {
        cnc.update();

        if (!CNCController::AutoReportStatus && cnc.isReady()) {
            if (millis() - lastPoll >= POLL_INTERVAL) {
                cnc.sendRealtime('?');
                lastPoll = millis();
            }
        }

        if (gcodeQueue != NULL && xQueueReceive(gcodeQueue, &cmd, 0) == pdPASS) {
            switch (cmd.type) {
                case CMD_GCODE:
                    cnc.sendCommand(String(cmd.gcode));
                    break;
                case CMD_JOG:
                    CNCController::sendJogCommand(String(cmd.axis), cmd.distance, cmd.feedrate);
                    break;
                case CMD_JOGCANCEL:
                    CNCController::sendJogCancel();
                    break;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

void TaskMain(void* pvParameters) {
    Serial.println("🧠 TaskMain avviato sul core 1 (UI/Input)");
    vTaskDelay(pdMS_TO_TICKS(50));

    static int lastX = 0, lastZ = 0;
    static unsigned long lastPairingMsg = 0;

    for (;;) {
        unsigned long now = millis();

        // 🔥 Legge i valori attuali degli encoder e aggiorna i riferimenti
        int currentX = InputHandler::getPCNT_X();
        int currentZ = InputHandler::getPCNT_Z();

        if (currentX != lastX || currentZ != lastZ) {
            lastX = currentX;
            lastZ = currentZ;

            // Solo se non bloccati e non sospesi, elabora il movimento
            if (!ButtonHandler::areEncodersLocked() && !g_suspendEncoder) {
                InputHandler::handleEncoderJog();
            }
        }

        // Gestione joystick e pulsanti (sempre attiva)
        InputHandler::handleJoystick();
        InputHandler::handleButtons();

        static unsigned long lastDisplayUpdate = 0;
        static unsigned long lastSafetyUpdate = 0;
        if (now - lastDisplayUpdate >= 50) {
            lastDisplayUpdate = now;
            DisplayManager::update();
        }
        if (now - lastSafetyUpdate >= 200) {
            lastSafetyUpdate = now;
            SafetyManager::update();
            LEDManager::setZeroLEDsOff();
            LEDManager::updateGotoLEDs();
        }

        static unsigned long lastADSUpdate = 0;
        if (now - lastADSUpdate >= 200) {
            lastADSUpdate = now;
            InputHandler::handleADS1115();
        }

        if (!cnc.isReady()) {
            if (now - lastPairingMsg > 5000) {
                lastPairingMsg = now;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void IRAM_ATTR handlePCFInterrupt() {
    input.interruptFlag = true;
}
void IRAM_ATTR handleJoystickChange() {
    InputHandler::readJoystickState();
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}