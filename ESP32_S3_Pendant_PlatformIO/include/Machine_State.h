#ifndef MACHINE_STATE_H
#define MACHINE_STATE_H

#include <Arduino.h>
#include <string>

enum MachineState {
  STATE_IDLE,
  STATE_RUN,
  STATE_HOLD,
  STATE_ALARM,
  STATE_UNKNOWN
};

struct Position {
  float x = 0;
  float y = 0;
  float z = 0;
  float a = 0;
  
  // Funzione per sottrarre due posizioni (invece dell'operatore)
  Position subtract(const Position& other) const {
    Position result;
    result.x = x - other.x;
    result.y = y - other.y;
    result.z = z - other.z;
    result.a = a - other.a;
    return result;
  }
};

struct MachineStatus {
  // Machine Position (MPos) – dallo stato
  float x = 0.0, y = 0.0, z = 0.0, a = 0.0;
  // Work Position (WPos) – calcolata = MPos - WCO
  float wposX = 0.0, wposY = 0.0, wposZ = 0.0, wposA = 0.0;
  // Work Coordinate Offset (WCO)
  float wcoX = 0.0, wcoY = 0.0, wcoZ = 0.0, wcoA = 0.0;
  
  float rpm = 0.0;
  float feedrate = 0.0;
  MachineState state = STATE_UNKNOWN;

  int feedrateX = 1000;
  int feedrateZ = 1000;
  int minFeedrateX = 50;
  int minFeedrateZ = 50;
  int maxFeedrateX = 2000;
  int maxFeedrateY = 2000;
  int maxFeedrateZ = 2000;
  int maxFeedrateA = 2000;

  bool xMinusLimit = false;
  bool xPlusLimit = false;
  bool zMinusLimit = false;
  bool zPlusLimit = false;
  float xMinusLimitPosition = -300.0;
  float xPlusLimitPosition = 300.0;
  float zMinusLimitPosition = -300.0;
  float zPlusLimitPosition = 300.0;

  float xSafeDistance = 0.0;
  float zSafeDistance = 0.0;
  bool xSafeDistanceMode = false;
  bool zSafeDistanceMode = false;
  float xSafeDistanceEnc = 0;
  float zSafeDistanceEnc = 0;

  bool radiusDiameterMode = false;
  bool cncConnected = false;
  bool autoReportEnabled = false;
};

extern MachineStatus machine;

struct InputState {
  volatile int xSteps = 0;
  volatile int zSteps = 0;
  volatile int xPending = 0;
  volatile int zPending = 0;
  volatile bool xFirstClick = true;
  volatile bool zFirstClick = true;
  volatile int rawState = 15;
  volatile int lastState = 15;
  volatile bool active = false;
  volatile unsigned long lastChange = 0;
  volatile bool interruptFlag = false;
  volatile unsigned long lastDebounce = 0;
};

// ========== FUNZIONI DI PARSING (COMPLETE CON WCO E WPOS) ==========

inline bool startsWith(const String& text, const String& prefix) {
  return text.length() >= prefix.length() && text.substring(0, prefix.length()) == prefix;
}

inline MachineState parseMachineState(const String& status) {
  if (startsWith(status, "<Idle")) return STATE_IDLE;
  if (startsWith(status, "<Run")) return STATE_RUN;
  if (startsWith(status, "<Hold")) return STATE_HOLD;
  if (startsWith(status, "<Alarm")) return STATE_ALARM;
  return STATE_UNKNOWN;
}

inline Position parsePosition(const String& status, const char* tag) {
  Position pos;
  String tagStr = String(tag);
  int start = status.indexOf(tagStr);
  if (start == -1) return pos;
  start += tagStr.length();
  int end = status.indexOf('|', start);
  if (end == -1) end = status.indexOf('>', start);
  if (end == -1) return pos;
  String coords = status.substring(start, end);
  int idx1 = coords.indexOf(',');
  int idx2 = coords.indexOf(',', idx1 + 1);
  int idx3 = coords.indexOf(',', idx2 + 1);
  if (idx1 != -1) pos.x = coords.substring(0, idx1).toFloat();
  if (idx2 != -1) pos.y = coords.substring(idx1 + 1, idx2).toFloat();
  if (idx3 != -1) pos.z = coords.substring(idx2 + 1, idx3).toFloat();
  if (idx3 != -1) pos.a = coords.substring(idx3 + 1).toFloat();
  return pos;
}

inline float parseFeedrate(const String& status) {
  int fsIndex = status.indexOf("FS:");
  if (fsIndex == -1) return 0;
  String fs = status.substring(fsIndex + 3);
  int end = fs.indexOf('|');
  if (end != -1) fs = fs.substring(0, end);
  int comma = fs.indexOf(',');
  if (comma == -1) return 0;
  return fs.substring(0, comma).toFloat();
}

inline float parseSpindleRPM(const String& status) {
  int fsIndex = status.indexOf("FS:");
  if (fsIndex == -1) return 0;
  String fs = status.substring(fsIndex + 3);
  int end = fs.indexOf('|');
  if (end != -1) fs = fs.substring(0, end);
  int comma = fs.indexOf(',');
  if (comma == -1) return 0;
  return fs.substring(comma + 1).toFloat();
}

inline void parseBufferState(const String& status, int& planner, int& serial) {
  int bfIndex = status.indexOf("Bf:");
  if (bfIndex == -1) return;
  String bf = status.substring(bfIndex + 3);
  int end = bf.indexOf('|');
  if (end != -1) bf = bf.substring(0, end);
  int comma = bf.indexOf(',');
  if (comma == -1) return;
  planner = bf.substring(0, comma).toInt();
  serial = bf.substring(comma + 1).toInt();
}

inline void updateMachineFromStatus(MachineStatus& machine, const String& status) {
  if (status.isEmpty() || status[0] != '<') return;
  
  machine.state = parseMachineState(status);
  machine.rpm = parseSpindleRPM(status);
  machine.feedrate = parseFeedrate(status);
  
  Position mpos = parsePosition(status, "MPos:");
  machine.x = mpos.x;
  machine.y = mpos.y;
  machine.z = mpos.z;
  machine.a = mpos.a;
  
  Position wco = parsePosition(status, "WCO:");
  machine.wcoX = wco.x;
  machine.wcoY = wco.y;
  machine.wcoZ = wco.z;
  machine.wcoA = wco.a;
  
  // Calcola Work Position = MPos - WCO (usando la funzione subtract)
  Position wpos = mpos.subtract(wco);
  machine.wposX = wpos.x;
  machine.wposY = wpos.y;
  machine.wposZ = wpos.z;
  machine.wposA = wpos.a;
  
  int planner = 0, serial = 0;
  parseBufferState(status, planner, serial);
  
  machine.cncConnected = true;
}

inline void updateMachineFromStatus(const String& status) {
  updateMachineFromStatus(machine, status);
}

#endif