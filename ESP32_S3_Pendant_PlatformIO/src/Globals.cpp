// ============================================================
// Globals.cpp - Definizioni di tutte le variabili globali
// ============================================================
#include "Globals.h"
#include "Machine_State.h"

// Configurazione principale
Config config;

// I2C
TwoWire I2Ctwo(0);

// Display OLED
Adafruit_SH1106G xDisplay(128, 64, &I2Ctwo, -1);
Adafruit_SH1106G zDisplay(128, 64, &I2Ctwo, -1);

// Expander PCF8575
PCF8575 pcfA(0x26, &I2Ctwo);
PCF8575 pcfB(0x22, &I2Ctwo);

// Convertitore ADS1115
Adafruit_ADS1115 ads;

// Stato macchina e input
MachineStatus machine;
InputState input;

// Coda comandi
QueueHandle_t gcodeQueue = NULL;

// Flag connessione
bool grblReady = false;

// Flag per sospendere la lettura del PCNT (Lock Encoders / Safe Distance)
volatile bool g_suspendEncoder = false;

// 🔥 Definizione del numero di decimali per i display (valore predefinito 3)
uint8_t displayDecimals = 2;

// ========== ESP-NOW (variabili di stato) ==========
uint8_t bridgeMac[6] = {0};
bool paired = false;
uint16_t sequence = 0;
uint16_t lastAckSeq = 0xFFFF;
unsigned long lastPacketTime = 0;
bool waitingForResponse = false;
String lastResponse = "";
unsigned long packetCounter = 0;
const unsigned long HEARTBEAT_TIMEOUT_MS = 5000;