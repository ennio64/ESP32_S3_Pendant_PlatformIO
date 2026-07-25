#include "Headers.h"
#include <cstdint>
#include "Globals.h"
#include "driver/pcnt.h"
#include "Button_Handler.h"   // per areEncodersLocked()

// ================== VARIABILI STATICHE ==================
const int8_t InputHandler::ENCODER_STATES[16] = {
  0, -1, 1, 0,
  1, 0, 0, -1,
  -1, 0, 0, 1,
  0, 1, -1, 0
};

int16_t InputHandler::lastPCNT_X = 0;
int16_t InputHandler::lastPCNT_Z = 0;

volatile bool InputHandler::newADS1115DataAvailable = false;

bool InputHandler::encoderDebugOnly = false;   // FALSE = invio comandi reali
int InputHandler::pendingX_debug = 0;
int InputHandler::pendingZ_debug = 0;

char InputHandler::joystickAxis = 'X';

// 🔥 Definizioni delle variabili per il delta degli encoder
int16_t InputHandler::lastEncoderX = 0;
int16_t InputHandler::lastEncoderZ = 0;
int InputHandler::pendingEncoderX = 0;
int InputHandler::pendingEncoderZ = 0;

static const int FIXED_FEEDRATE = 100;   // Solo per encoder

// ================== INIZIALIZZAZIONE PCNT ==================
void InputHandler::initializeEncoderX() {
    pcnt_config_t cfg = {};
    cfg.unit = PCNT_UNIT_0;
    cfg.channel = PCNT_CHANNEL_0;
    cfg.pulse_gpio_num = config.encoderXA;
    cfg.ctrl_gpio_num = config.encoderXB;
    cfg.pos_mode = PCNT_COUNT_INC;
    cfg.neg_mode = PCNT_COUNT_DEC;
    cfg.lctrl_mode = PCNT_MODE_KEEP;
    cfg.hctrl_mode = PCNT_MODE_REVERSE;
    cfg.counter_h_lim = 32767;
    cfg.counter_l_lim = -32767;

    pcnt_unit_config(&cfg);
    pcnt_counter_pause(PCNT_UNIT_0);
    pcnt_counter_clear(PCNT_UNIT_0);
    pcnt_counter_resume(PCNT_UNIT_0);
    lastPCNT_X = 0;
}

void InputHandler::initializeEncoderZ() {
    pcnt_config_t cfg = {};
    cfg.unit = PCNT_UNIT_1;
    cfg.channel = PCNT_CHANNEL_0;
    cfg.pulse_gpio_num = config.encoderZA;
    cfg.ctrl_gpio_num = config.encoderZB;
    cfg.pos_mode = PCNT_COUNT_INC;
    cfg.neg_mode = PCNT_COUNT_DEC;
    cfg.lctrl_mode = PCNT_MODE_KEEP;
    cfg.hctrl_mode = PCNT_MODE_REVERSE;
    cfg.counter_h_lim = 32767;
    cfg.counter_l_lim = -32767;

    pcnt_unit_config(&cfg);
    pcnt_counter_pause(PCNT_UNIT_1);
    pcnt_counter_clear(PCNT_UNIT_1);
    pcnt_counter_resume(PCNT_UNIT_1);
    lastPCNT_Z = 0;
}

int16_t InputHandler::getPCNT_X() {
    int16_t raw;
    pcnt_get_counter_value(PCNT_UNIT_0, &raw);
    return -(raw / 2);   // divisione per 2 + inversione senso
}

int16_t InputHandler::getPCNT_Z() {
    int16_t raw;
    pcnt_get_counter_value(PCNT_UNIT_1, &raw);
    return -(raw / 2);   // divisione per 2 + inversione senso
}

void InputHandler::setupEncoders() {
    initializeEncoderX();
    initializeEncoderZ();
    // Inizializza anche i delta
    lastEncoderX = getPCNT_X();
    lastEncoderZ = getPCNT_Z();
    pendingEncoderX = 0;
    pendingEncoderZ = 0;
    Serial.println("✅ Encoders initialized with PCNT (1 step = 0.01 mm)");
}

// ================== SOSPENSIONE LETTURA ENCODER ==================
void InputHandler::suspendEncoderReading() {
    g_suspendEncoder = true;
    Serial.println("⏸️ Lettura encoder sospesa");
}

void InputHandler::resumeEncoderReading() {
    // Resetta i contatori PCNT
    pcnt_counter_clear(PCNT_UNIT_0);
    pcnt_counter_clear(PCNT_UNIT_1);
    lastPCNT_X = 0;
    lastPCNT_Z = 0;
    g_suspendEncoder = false;
    Serial.println("▶️ Lettura encoder ripristinata (contatori resettati)");
}

void InputHandler::resetEncoderCounters() {
    pcnt_counter_clear(PCNT_UNIT_0);
    pcnt_counter_clear(PCNT_UNIT_1);
    lastPCNT_X = 0;
    lastPCNT_Z = 0;
    Serial.println("🔄 Contatori encoder resettati");
}

// 🔥 Resetta i delta interni per evitare accumulo
void InputHandler::resetEncoderDelta() {
    lastEncoderX = getPCNT_X();
    lastEncoderZ = getPCNT_Z();
    pendingEncoderX = 0;
    pendingEncoderZ = 0;
    Serial.println("🔄 Delta encoder resettati");
}

// ================== HANDLE ENCODER JOG ==================
void InputHandler::handleEncoderJog() {
    if (ButtonHandler::areEncodersLocked() || g_suspendEncoder) {
        return;
    }

    int16_t currentX = getPCNT_X();
    int16_t currentZ = getPCNT_Z();

    if (currentX != lastEncoderX) {
        int delta = currentX - lastEncoderX;
        if (delta != 0) {
            pendingEncoderX += delta;
            lastEncoderX = currentX;
        }
    }

    if (currentZ != lastEncoderZ) {
        int delta = currentZ - lastEncoderZ;
        if (delta != 0) {
            pendingEncoderZ += delta;
            lastEncoderZ = currentZ;
        }
    }

    if (encoderDebugOnly) {
        pendingX_debug += pendingEncoderX;
        pendingZ_debug += pendingEncoderZ;
        pendingEncoderX = 0;
        pendingEncoderZ = 0;
        return;
    }

    if (pendingEncoderX != 0) {
        executeJogCommandOptimized('X', pendingEncoderX);
        pendingEncoderX = 0;
    }
    if (pendingEncoderZ != 0) {
        executeJogCommandOptimized('Z', pendingEncoderZ);
        pendingEncoderZ = 0;
    }
}

void InputHandler::processPendingDebug() {
    static int lastPrintedX = 0;
    static int lastPrintedZ = 0;

    if (pendingX_debug == 0 && pendingZ_debug == 0) return;

    int x = pendingX_debug;
    int z = pendingZ_debug;

    if (x != lastPrintedX || z != lastPrintedZ) {
        Serial.printf("📊 [ENC DEBUG] X: steps=%d (delta=%d) Z: steps=%d (delta=%d)\n",
                      getPCNT_X(), x, getPCNT_Z(), z);
        lastPrintedX = x;
        lastPrintedZ = z;
    }

    pendingX_debug = 0;
    pendingZ_debug = 0;
}

void InputHandler::executeJogCommandOptimized(char axis, int stepDifference) {
    bool isX = (axis == 'X');
    int feedrate = FIXED_FEEDRATE;

    float rawDistance = stepDifference * (isX ? JOG_DISTANCE_X : JOG_DISTANCE_Z);
    if (fabs(rawDistance) < 0.001) {
        rawDistance = (rawDistance >= 0) ? 0.01f : -0.01f;
    }

    float limitedDistance = applyLimits(axis, rawDistance);
    if (fabs(limitedDistance) < 0.001) {
        Serial.printf("⛔ Jog %c bloccato: limite raggiunto\n", axis);
        return;
    }

    if (encoderDebugOnly) {
        return;
    }

    enqueueJog(String(axis), limitedDistance, feedrate);
    vTaskDelay(pdMS_TO_TICKS(5));

    Serial.printf("🎯 Jog %c (ENC): %+.3fmm F%d | Steps: %d\n",
                  axis, limitedDistance, feedrate, stepDifference);
}

// ================== APPLY LIMITS ==================
float InputHandler::applyLimits(char axis, float requestedDistance) {
    float currentPos = (axis == 'X') ? machine.wposX : machine.wposZ;
    float limitedDistance = requestedDistance;

    if (axis == 'X') {
        if (requestedDistance > 0 && machine.xPlusLimit) {
            float maxAllowed = machine.xPlusLimitPosition - currentPos;
            if (maxAllowed < 0) maxAllowed = 0;
            if (requestedDistance > maxAllowed) {
                limitedDistance = maxAllowed;
                Serial.printf("🔒 X+ limit: ridotto da %.3f a %.3f\n", requestedDistance, limitedDistance);
            }
        }
        if (requestedDistance < 0 && machine.xMinusLimit) {
            float maxAllowed = machine.xMinusLimitPosition - currentPos;
            if (maxAllowed > 0) maxAllowed = 0;
            if (requestedDistance < maxAllowed) {
                limitedDistance = maxAllowed;
                Serial.printf("🔒 X- limit: ridotto da %.3f a %.3f\n", requestedDistance, limitedDistance);
            }
        }
    } else if (axis == 'Z') {
        if (requestedDistance > 0 && machine.zPlusLimit) {
            float maxAllowed = machine.zPlusLimitPosition - currentPos;
            if (maxAllowed < 0) maxAllowed = 0;
            if (requestedDistance > maxAllowed) {
                limitedDistance = maxAllowed;
                Serial.printf("🔒 Z+ limit: ridotto da %.3f a %.3f\n", requestedDistance, limitedDistance);
            }
        }
        if (requestedDistance < 0 && machine.zMinusLimit) {
            float maxAllowed = machine.zMinusLimitPosition - currentPos;
            if (maxAllowed > 0) maxAllowed = 0;
            if (requestedDistance < maxAllowed) {
                limitedDistance = maxAllowed;
                Serial.printf("🔒 Z- limit: ridotto da %.3f a %.3f\n", requestedDistance, limitedDistance);
            }
        }
    }

    if (fabs(limitedDistance) < 0.001) {
        return 0.0f;
    }
    return limitedDistance;
}

// ================== JOYSTICK HANDLING ==================
void IRAM_ATTR InputHandler::readJoystickState() {
    bool right = !digitalRead(config.joyRight);
    bool left = !digitalRead(config.joyLeft);
    bool down = !digitalRead(config.joyDown);
    bool up = !digitalRead(config.joyUp);
    input.rawState = (right << 3) | (left << 2) | (down << 1) | up;
}

void InputHandler::handleJoystick() {
    if (input.rawState != input.lastState) {
        if (millis() - input.lastChange > config.joystickDebounce) {
            if (input.rawState == 0) {
                stopCurrentJog();
            } else {
                startContinuousJog();
                switch (input.rawState) {
                    case 1: joystickAxis = 'X'; break;
                    case 2: joystickAxis = 'X'; break;
                    case 4: joystickAxis = 'Z'; break;
                    case 8: joystickAxis = 'Z'; break;
                }
            }
            input.lastState = input.rawState;
            input.lastChange = millis();
        }
    }
}

void InputHandler::stopCurrentJog() {
    if (!input.active) return;
    enqueueJogCancel();
    input.active = false;
    Serial.println("🛑 Jog Cancel inviato");
}

void InputHandler::startContinuousJog() {
    String axis = "";
    float requestedDistance = 0;
    int feedrate = 0;

    switch (input.rawState) {
        case 1:  axis = "X"; requestedDistance = -1000.0f; 
                 feedrate = machine.feedrateX;
                 if (feedrate < machine.minFeedrateX) feedrate = machine.minFeedrateX;
                 if (feedrate > machine.maxFeedrateX) feedrate = machine.maxFeedrateX;
                 break;
        case 2:  axis = "X"; requestedDistance = 1000.0f;
                 feedrate = machine.feedrateX;
                 if (feedrate < machine.minFeedrateX) feedrate = machine.minFeedrateX;
                 if (feedrate > machine.maxFeedrateX) feedrate = machine.maxFeedrateX;
                 break;
        case 4:  axis = "Z"; requestedDistance = -1000.0f;
                 feedrate = machine.feedrateZ;
                 if (feedrate < machine.minFeedrateZ) feedrate = machine.minFeedrateZ;
                 if (feedrate > machine.maxFeedrateZ) feedrate = machine.maxFeedrateZ;
                 break;
        case 8:  axis = "Z"; requestedDistance = 1000.0f;
                 feedrate = machine.feedrateZ;
                 if (feedrate < machine.minFeedrateZ) feedrate = machine.minFeedrateZ;
                 if (feedrate > machine.maxFeedrateZ) feedrate = machine.maxFeedrateZ;
                 break;
        default: return;
    }

    float limitedDistance = applyLimits(axis[0], requestedDistance);
    if (fabs(limitedDistance) < 0.001) {
        Serial.printf("⛔ Jog %s bloccato: limite raggiunto\n", axis.c_str());
        return;
    }

    if (enqueueJog(axis, limitedDistance, feedrate)) {
        input.active = true;
        Serial.printf("🎯 Jog Start: %s%+.3f F%d\n", axis.c_str(), limitedDistance, feedrate);
    }
}

// ================== BUTTON HANDLING ==================
void InputHandler::handleButtons() {
    if (input.interruptFlag && (millis() - input.lastDebounce > 50)) {
        ButtonHandler::update();
        input.interruptFlag = false;
        input.lastDebounce = millis();
    }
}

// ================== ADS1115 ==================
unsigned long lastADSUpdateTime = 0;
const unsigned long ADS_UPDATE_INTERVAL = 200;
const int RAW_VARIATION_THRESHOLD = 150;

int16_t lastPotZ_raw = 0;
int16_t lastPotX_raw = 0;
bool adsReady = false;

static int maxADC_X = 0;
static int maxADC_Z = 0;
static int minADC_X = 99999;
static int minADC_Z = 99999;
static bool calibrated = false;
static unsigned long calStart = 0;

void IRAM_ATTR InputHandler::ads1115AlertISR() {
    if (adsReady) {
        newADS1115DataAvailable = true;
    }
}

void InputHandler::initADS1115() {
    if (ads.begin(0x48, &I2Ctwo)) {
        adsReady = true;
        configureADS1115Comparator();
        Serial.println("✅ ADS1115 inizializzato con ALERT");
    } else {
        adsReady = false;
        Serial.println("❌ Errore inizializzazione ADS1115!");
    }
}

void InputHandler::handleADS1115() {
    if (!adsReady) return;

    if (newADS1115DataAvailable) {
        newADS1115DataAvailable = false;
        unsigned long now = millis();
        if (now - lastADSUpdateTime >= ADS_UPDATE_INTERVAL) {
            int16_t potZ_raw = ads.readADC_SingleEnded(0);
            int16_t potX_raw = ads.readADC_SingleEnded(1);
            if (abs(potZ_raw - lastPotZ_raw) > RAW_VARIATION_THRESHOLD || abs(potX_raw - lastPotX_raw) > RAW_VARIATION_THRESHOLD) {
                
                // Calibrazione automatica
                if (potX_raw > maxADC_X) maxADC_X = potX_raw;
                if (potZ_raw > maxADC_Z) maxADC_Z = potZ_raw;
                if (potX_raw < minADC_X) minADC_X = potX_raw;
                if (potZ_raw < minADC_Z) minADC_Z = potZ_raw;

                if (!calibrated) {
                    if (calStart == 0) calStart = millis();
                    if (millis() - calStart > 3000) {
                        calibrated = true;
                        maxADC_X += 10;
                        maxADC_Z += 10;
                        if (maxADC_X < 1) maxADC_X = 1;
                        if (maxADC_Z < 1) maxADC_Z = 1;
                        Serial.printf("📏 Calibrazione ADS1115 completata: X range %d-%d, Z range %d-%d\n", 
                                      minADC_X, maxADC_X, minADC_Z, maxADC_Z);
                    }
                }

                int rangeX = calibrated ? (maxADC_X - minADC_X) : 3000;
                int rangeZ = calibrated ? (maxADC_Z - minADC_Z) : 3000;
                if (rangeX < 1) rangeX = 1;
                if (rangeZ < 1) rangeZ = 1;

                if (machine.feedrateZ >= 0 && machine.feedrateX >= 0) {
                    int rawX_clamped = constrain(potX_raw, minADC_X, maxADC_X);
                    int rawZ_clamped = constrain(potZ_raw, minADC_Z, maxADC_Z);
                    
                    machine.feedrateZ = map(rawZ_clamped - minADC_Z, 0, rangeZ, machine.minFeedrateZ, machine.maxFeedrateZ);
                    machine.feedrateX = map(rawX_clamped - minADC_X, 0, rangeX, machine.minFeedrateX, machine.maxFeedrateX);
                    
                    machine.feedrateZ = constrain(machine.feedrateZ, machine.minFeedrateZ, machine.maxFeedrateZ);
                    machine.feedrateX = constrain(machine.feedrateX, machine.minFeedrateX, machine.maxFeedrateX);
                    
                    Serial.printf("📊 ADS1115 -> Z:%d/%d X:%d/%d (ADC: X=%d Z=%d)\n",
                                  machine.feedrateZ, machine.maxFeedrateZ,
                                  machine.feedrateX, machine.maxFeedrateX,
                                  potX_raw, potZ_raw);
                    DisplayManager::showFeedScreen();
                }
                lastPotZ_raw = potZ_raw;
                lastPotX_raw = potX_raw;
            }
            lastADSUpdateTime = now;
        }
        configureADS1115Comparator();
    }
}

void InputHandler::sendFeedOverride(int percent) {
    if (!input.active) return;
    if (percent == 100) return;
    percent = constrain(percent, 10, 200);
    String command = "M220 S" + String(percent);
    enqueueGCode(command);
    Serial.printf("⚡ Override inviato: %s\n", command.c_str());
}

void InputHandler::configureADS1115Comparator() {
    I2Ctwo.beginTransmission(0x48);
    I2Ctwo.write(0x02); I2Ctwo.write(0x7F); I2Ctwo.write(0xFF);
    I2Ctwo.endTransmission();

    I2Ctwo.beginTransmission(0x48);
    I2Ctwo.write(0x03); I2Ctwo.write(0x80); I2Ctwo.write(0x00);
    I2Ctwo.endTransmission();

    uint16_t configReg = 0;
    configReg |= (1 << 15);
    configReg |= (0b100 << 12);
    configReg |= (0b001 << 9);
    configReg |= (0 << 8);
    configReg |= (0b111 << 5);
    configReg |= (0 << 4);
    configReg |= (0 << 3);
    configReg |= (0 << 2);
    configReg |= (0b01 << 0);

    I2Ctwo.beginTransmission(0x48);
    I2Ctwo.write(0x01);
    I2Ctwo.write(configReg >> 8);
    I2Ctwo.write(configReg & 0xFF);
    I2Ctwo.endTransmission();
}

void InputHandler::stopAllMotion() {
    enqueueJogCancel();
    input.active = false;
}