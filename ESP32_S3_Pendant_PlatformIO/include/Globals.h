#ifndef GLOBALS_H
#define GLOBALS_H

#include "Config.h"
#include "Machine_State.h"
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_SH110X.h>
#include "PCF8575.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// ========== COMANDI CNC ==========
enum CommandType {
  CMD_GCODE,
  CMD_JOG,
  CMD_JOGCANCEL
};

#define MAX_CMD_LEN 64

struct CNCCommand {
  CommandType type;
  char gcode[MAX_CMD_LEN];
  char axis[3];
  float distance;
  int feedrate;
};

extern QueueHandle_t gcodeQueue;

// ========== FUNZIONI INLINE PER LA CODA (NON BLOCCANTI) ==========
inline bool enqueueGCode(const String& g) {
  if (gcodeQueue == NULL) return false;
  CNCCommand cmd;
  cmd.type = CMD_GCODE;
  strncpy(cmd.gcode, g.c_str(), MAX_CMD_LEN);
  cmd.gcode[MAX_CMD_LEN - 1] = '\0';
  return (xQueueSend(gcodeQueue, &cmd, 0) == pdPASS);
}

inline bool enqueueJog(const String& axis, float distance, int feedrate) {
  if (gcodeQueue == NULL) return false;
  CNCCommand cmd;
  cmd.type = CMD_JOG;
  strncpy(cmd.axis, axis.c_str(), sizeof(cmd.axis));
  cmd.axis[sizeof(cmd.axis) - 1] = '\0';
  cmd.distance = distance;
  cmd.feedrate = feedrate;
  return (xQueueSend(gcodeQueue, &cmd, 0) == pdPASS);
}

inline bool enqueueJogCancel() {
  if (gcodeQueue == NULL) return false;
  CNCCommand cmd;
  cmd.type = CMD_JOGCANCEL;
  cmd.gcode[0] = '\0';
  cmd.axis[0] = '\0';
  cmd.distance = 0;
  cmd.feedrate = 0;
  return (xQueueSend(gcodeQueue, &cmd, 0) == pdPASS);
}

// ========== VARIABILI GLOBALI ==========
extern Config config;
extern Adafruit_ADS1115 ads;
extern Adafruit_SH1106G xDisplay;
extern Adafruit_SH1106G zDisplay;
extern PCF8575 pcfA;
extern PCF8575 pcfB;
extern MachineStatus machine;
extern InputState input;
extern TwoWire I2Ctwo;
extern bool grblReady;

// 🔥 Flag per sospendere la lettura del PCNT (Lock Encoders / Safe Distance)
extern volatile bool g_suspendEncoder;

// 🔥 Numero di decimali da visualizzare sui display (default 3, può essere cambiato a 2)
extern uint8_t displayDecimals;

#endif // GLOBALS_H