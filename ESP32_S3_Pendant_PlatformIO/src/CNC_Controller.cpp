#include "Headers.h"
#include "ESPNow_Manager.h"

extern ESPNowManager cnc;

unsigned long CNCController::lastStatusUpdate = 0;
bool CNCController::AutoReportStatus = false;

void CNCController::setup() {
  Serial.println("🎯 Initializing GrblHAL Controller via ESP-NOW...");
  
  unsigned long start = millis();
  while (!cnc.isReady() && (millis() - start < 5000)) {
    cnc.update();
    delay(10);
  }
  
  if (cnc.isReady()) {
    initializeGrblSettings();
  } else {
    Serial.println("⚠️ Bridge non associato, riproverò in background");
  }
}

void CNCController::update() {
  cnc.update();
  if (!cnc.isReady()) return;
  if (!AutoReportStatus) {
    handlePolling();
  }
}

void CNCController::initializeGrblSettings() {
  grblReady = false;
  
  cnc.sendCommand("G21");
  delay(100);
  
  String status = queryImmediateState(1000);
  if (status.length() > 0) {
    updateMachineFromStatus(status);
    Serial.println("📡 Stato iniziale: " + status);
  }
  readMaxFeedrates();
  checkAutoReportStatus();
  grblReady = true;
  Serial.println("✅ GrblHAL Initialized via ESP-NOW");
}

String CNCController::queryImmediateState(unsigned long timeoutMs) {
  if (!cnc.isReady()) return "";
  return cnc.queryReply("?", timeoutMs);
}

float CNCController::readParamWithRetry(const char* param, int wait) {
  float value = -1;
  int retries = 0;
  while (value <= 0 && retries < 5) {
    String response = cnc.queryReply(param, 500);
    if (response.length() > 0) {
      int eq = response.indexOf('=');
      if (eq > 0) {
        String num = response.substring(eq + 1);
        value = num.toFloat();
        if (value > 0) break;
      }
    }
    retries++;
    delay(wait);
  }
  return value;
}

void CNCController::readMaxFeedrates() {
  Serial.println("📊 Lettura feedrate massimi...");
  machine.maxFeedrateX = (int)readParamWithRetry("$110", 200);
  machine.maxFeedrateY = (int)readParamWithRetry("$111", 200);
  machine.maxFeedrateZ = (int)readParamWithRetry("$112", 200);
  machine.maxFeedrateA = (int)readParamWithRetry("$113", 200);
  Serial.printf("⚙️ Max Feedrates -> X:%d Y:%d Z:%d A:%d\n",
                machine.maxFeedrateX, machine.maxFeedrateY,
                machine.maxFeedrateZ, machine.maxFeedrateA);
}

void CNCController::checkAutoReportStatus() {
  float interval = readParamWithRetry("$481", 200);
  if (interval >= 0) {
    AutoReportStatus = (interval > 0);
    Serial.printf("📡 Auto-Report: %s (%.0fms)\n",
                  AutoReportStatus ? "ENABLED" : "DISABLED", interval);
  } else {
    AutoReportStatus = false;
    Serial.println("📡 Auto-Report: UNABLE TO DETECT");
  }
}

void CNCController::handlePolling() {
  if (millis() - lastStatusUpdate >= config.statusUpdateInterval) {
    cnc.sendRealtime('?');
    lastStatusUpdate = millis();
  }
}

void CNCController::sendJogCommand(const String& axis, float distance, int feedrate) {
  if (!cnc.isReady()) {
    Serial.println("⚠️ cnc.isReady() = false");
    return;
  }
  if (machine.state == STATE_ALARM) {
    Serial.println("⚠️ Stato ALARM, jog bloccato");
    return;
  }
  
  if (fabs(distance) < 0.001) {
    distance = (distance >= 0) ? 0.01f : -0.01f;
  }
  
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "$J=G91 %s%.3f F%d", 
           axis.c_str(), distance, feedrate);
  String command = String(buffer);
  
  Serial.println("📤 Jog command: " + command);
  cnc.sendCommand(command);
}

void CNCController::sendGCode(const String& command) {
  if (!cnc.isReady()) return;
  cnc.sendCommand(command);
}

void CNCController::sendJogCancel() {
  if (!cnc.isReady()) return;
  cnc.sendRealtime(0x85);
}

bool CNCController::isConnected() {
  return cnc.isReady() && machine.cncConnected;
}