#include "Headers.h"
#include "ESPNow_Manager.h"  // per cnc.isReady()

extern ESPNowManager cnc;
extern uint8_t displayDecimals;   // definita in Globals.cpp, default 3

unsigned long DisplayManager::lastUpdate = 0;
unsigned long DisplayManager::lastOTAUpdate = 0;

bool DisplayManager::stateMessageShown = false;
MachineState DisplayManager::lastState = STATE_IDLE;

unsigned long DisplayManager::lastFeedDisplayTime = 0;
bool DisplayManager::showingFeed = false;

int DisplayManager::xOffsetX = 0;
int DisplayManager::xOffsetY = 6;
int DisplayManager::zOffsetX = 2;
int DisplayManager::zOffsetY = 0;

void DisplayManager::begin() {
  Serial.println("📺 Display Manager initializing...");

  if (!xDisplay.begin(0x3D, true)) {
    Serial.println("❌ Errore inizializzazione display X (0x3D)");
  } else {
    Serial.println("✅ Display X (0x3D) OK");
    xDisplay.clearDisplay();
    xDisplay.display();
  }

  if (!zDisplay.begin(0x3C, true)) {
    Serial.println("❌ Errore inizializzazione display Z (0x3C)");
  } else {
    Serial.println("✅ Display Z (0x3C) OK");
    zDisplay.clearDisplay();
    zDisplay.display();
  }

  delay(100);
  showMainScreen();
  Serial.println("📺 Display Manager initialized");
}

void DisplayManager::update() {
  if (millis() - lastUpdate < config.displayUpdateInterval) return;

  MachineState currentState = machine.state;

  // ===== STATO ALARM =====
  if (currentState == STATE_ALARM) {
    if (!stateMessageShown || lastState != STATE_ALARM) {
      showMachineState("ALARM");
      stateMessageShown = true;
      lastState = STATE_ALARM;
    }
    lastUpdate = millis();
    return;
  }
  // ===== STATO HOLD =====
  else if (currentState == STATE_HOLD) {
    if (!stateMessageShown || lastState != STATE_HOLD) {
      showMachineState("HOLD");
      stateMessageShown = true;
      lastState = STATE_HOLD;
    }
    lastUpdate = millis();
    return;
  }

  // ===== ATTESA CONNESSIONE (se CNC non è pronto) =====
  if (!machine.cncConnected || !cnc.isReady()) {
    if (!stateMessageShown || lastState != STATE_UNKNOWN) {
      showWaitController();
      stateMessageShown = true;
      lastState = STATE_UNKNOWN;
    }
    lastUpdate = millis();
    return;
  }

  // ===== RESET FLAG STATO =====
  if (stateMessageShown) {
    showMainScreen();
    stateMessageShown = false;
  }
  lastState = currentState;

  // ===== NORMALE REFRESH =====
  if (showingFeed) {
    if (millis() - lastFeedDisplayTime > 2000) {   // 🔥 2 secondi
      restoreMainScreen();
    }
  } else {
    updateXDisplay();
    updateZDisplay();
  }

  lastUpdate = millis();
}

void DisplayManager::toggleRadiusDiameter() {
  machine.radiusDiameterMode = !machine.radiusDiameterMode;
  Serial.printf("📏 Display mode: %s\n", machine.radiusDiameterMode ? "DIAMETER" : "RADIUS");
}

bool DisplayManager::isRadiusDiameterMode() {
  return machine.radiusDiameterMode;
}

void DisplayManager::setXOffsets(int dx, int dy) {
  xOffsetX = dx;
  xOffsetY = dy;
}

void DisplayManager::setZOffsets(int dx, int dy) {
  zOffsetX = dx;
  zOffsetY = dy;
}

// ========== FEED RATE ==========
void DisplayManager::showFeedScreen() {
  showingFeed = true;
  lastFeedDisplayTime = millis();   // 🔥 Resetta il timer

  int clampedFeedX = machine.feedrateX;
  if (clampedFeedX < machine.minFeedrateX) clampedFeedX = machine.minFeedrateX;
  if (clampedFeedX > machine.maxFeedrateX) clampedFeedX = machine.maxFeedrateX;

  int clampedFeedZ = machine.feedrateZ;
  if (clampedFeedZ < machine.minFeedrateZ) clampedFeedZ = machine.minFeedrateZ;
  if (clampedFeedZ > machine.maxFeedrateZ) clampedFeedZ = machine.maxFeedrateZ;

  // X Display
  xDisplay.clearDisplay();
  xDisplay.setTextSize(1);
  xDisplay.setTextColor(SH110X_WHITE);

  String feedX = "FEED X";
  displayCenter(&xDisplay, feedX, -20, true);

  String valueX = String(clampedFeedX) + "/" + String(machine.maxFeedrateX);
  displayCenter(&xDisplay, valueX, 0, true);

  xDisplay.display();

  // Z Display
  zDisplay.clearDisplay();
  zDisplay.setTextSize(1);
  zDisplay.setTextColor(SH110X_WHITE);

  String feedZ = "FEED Z";
  displayCenter(&zDisplay, feedZ, -20, false);

  String valueZ = String(clampedFeedZ) + "/" + String(machine.maxFeedrateZ);
  displayCenter(&zDisplay, valueZ, 0, false);

  zDisplay.display();
}

void DisplayManager::restoreMainScreen() {
  showingFeed = false;
  showMainScreen();
}

// ========== UTILITY ==========
void DisplayManager::displayCenter(Adafruit_SH1106G* display, const String& text, int yOffset, bool isX) {
  int16_t x1, y1;
  uint16_t w, h;
  display->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

  int16_t x = (128 - w) / 2;
  int16_t y = (64 - h) / 2 + yOffset;

  if (isX) {
    x += xOffsetX;
    y += xOffsetY;
  } else {
    x += zOffsetX;
    y += zOffsetY;
  }

  display->setCursor(x, y);
  display->print(text);
}

// ========== PRIVATE METHODS ==========
void DisplayManager::updateXDisplay() {
  xDisplay.clearDisplay();
  xDisplay.setTextSize(2);
  xDisplay.setTextColor(SH110X_WHITE);

  float displayValue = machine.radiusDiameterMode ? machine.wposX * 2 : machine.wposX;
  char buffer[16];
  dtostrf(displayValue, 5, displayDecimals, buffer);   // 🔥 usa displayDecimals
  displayCenter(&xDisplay, String(buffer), 0, true);

  drawRadiusDiameterIndicators();
  xDisplay.display();
}

void DisplayManager::updateZDisplay() {
  zDisplay.clearDisplay();
  zDisplay.setTextSize(2);
  zDisplay.setTextColor(SH110X_WHITE);

  char buffer[16];
  dtostrf(machine.wposZ, 5, displayDecimals, buffer);   // 🔥 usa displayDecimals
  displayCenter(&zDisplay, String(buffer), 0, false);

  zDisplay.display();
}

void DisplayManager::drawRadiusDiameterIndicators() {
  xDisplay.setTextSize(1);

  if (machine.radiusDiameterMode) {
    drawVerticalText("D", 1, 23);
    drawVerticalText("I", 1, 32);
    drawVerticalText("A", 1, 41);

    drawVerticalText("D", 118, 23);
    drawVerticalText("I", 118, 32);
    drawVerticalText("A", 118, 41);
  } else {
    drawVerticalText("R", 1, 23);
    drawVerticalText("A", 1, 32);
    drawVerticalText("D", 1, 41);

    drawVerticalText("R", 118, 23);
    drawVerticalText("A", 118, 32);
    drawVerticalText("D", 118, 41);
  }
}

void DisplayManager::drawVerticalText(const String& text, int16_t x, int16_t y) {
  xDisplay.setCursor(x, y);
  xDisplay.print(text);
}

// ================== SAFETY DISTANCE ==================
void DisplayManager::showSafeDistanceScreen(char axis, float value) {
  Adafruit_SH1106G* display = (axis == 'X') ? &xDisplay : &zDisplay;

  display->clearDisplay();
  display->setTextSize(1);
  display->setTextColor(SH110X_WHITE);

  String title = String(axis) + " SAFE DISTANCE";
  displayCenter(display, title, -20, (axis == 'X'));

  display->setTextSize(2);
  String valueStr = String(value, 2) + " mm";
  displayCenter(display, valueStr, 0, (axis == 'X'));

  display->setTextSize(1);
  String label;

  if (value < 0) {
    label = (axis == 'X') ? "SD INTERNAL" : "SD LEFT";
  } else if (value > 0) {
    label = (axis == 'X') ? "SD EXTERNAL" : "SD RIGHT";
  } else {
    label = "SD NOT SET";
  }

  displayCenter(display, label, 20, (axis == 'X'));
  display->display();
}

void DisplayManager::showSafeDistanceConfirmation(char axis, float value) {
  Adafruit_SH1106G* display = (axis == 'X') ? &xDisplay : &zDisplay;

  display->clearDisplay();
  display->setTextSize(2);
  display->setTextColor(SH110X_WHITE);

  displayCenter(display, "SD IS SET", -10, (axis == 'X'));

  display->setTextSize(1);
  String valueStr = String(axis) + ": " + String(value, 2) + " mm";
  displayCenter(display, valueStr, 15, (axis == 'X'));

  display->display();
}

void DisplayManager::showSafeDistanceCancelled(char axis) {
  Adafruit_SH1106G* display = (axis == 'X') ? &xDisplay : &zDisplay;

  display->clearDisplay();
  display->setTextSize(2);
  display->setTextColor(SH110X_WHITE);

  displayCenter(display, "CANCELLED", 0, (axis == 'X'));

  display->display();
}

// ================== STATE MACHINE ==================
void DisplayManager::showMachineState(const String& stateText) {
  xDisplay.clearDisplay();
  xDisplay.setTextSize(2);
  xDisplay.setTextColor(SH110X_WHITE);
  displayCenter(&xDisplay, stateText, 0, true);
  xDisplay.display();

  zDisplay.clearDisplay();
  zDisplay.setTextSize(2);
  zDisplay.setTextColor(SH110X_WHITE);
  displayCenter(&zDisplay, stateText, 0, false);
  zDisplay.display();
}

// ================== MAIN / OTA ==================

void DisplayManager::showOTAPrompt() {
  xDisplay.clearDisplay();
  xDisplay.setTextSize(1);
  xDisplay.setTextColor(SH110X_WHITE);
  displayCenter(&xDisplay, "Move Joystick", -10, true);
  displayCenter(&xDisplay, "to enter OTA MODE", 10, true);
  xDisplay.display();

  zDisplay.clearDisplay();
  zDisplay.setTextSize(1);
  zDisplay.setTextColor(SH110X_WHITE);
  displayCenter(&zDisplay, "OTA ENTRY WINDOW", -10, false);
  displayCenter(&zDisplay, "Use joystick now", 10, false);
  zDisplay.display();
}

void DisplayManager::showWaitController() {
  xDisplay.clearDisplay();
  xDisplay.setTextSize(1);
  xDisplay.setTextColor(SH110X_WHITE);
  displayCenter(&xDisplay, "Wait", -15, true);
  displayCenter(&xDisplay, "to connect", 0, true);
  displayCenter(&xDisplay, "controller", 15, true);
  xDisplay.display();

  zDisplay.clearDisplay();
  zDisplay.setTextSize(1);
  zDisplay.setTextColor(SH110X_WHITE);
  displayCenter(&zDisplay, "Wait", -15, false);
  displayCenter(&zDisplay, "to connect", 0, false);
  displayCenter(&zDisplay, "controller", 15, false);
  zDisplay.display();
}

void DisplayManager::showMainScreen() {
  updateXDisplay();
  updateZDisplay();
}

void DisplayManager::showOTAScreen() {
  xDisplay.clearDisplay();
  xDisplay.setTextSize(1);
  xDisplay.setTextColor(SH110X_WHITE);

  xDisplay.setCursor(35 + xOffsetX, 0 + xOffsetY);
  xDisplay.println("");

  xDisplay.setCursor(25 + xOffsetX, 15 + xOffsetY);
  xDisplay.println("Move Joystick");

  xDisplay.setCursor(30 + xOffsetX, 25 + xOffsetY);
  xDisplay.println("to connect");

  xDisplay.setCursor(30 + xOffsetX, 35 + xOffsetY);
  xDisplay.println("controller");

  xDisplay.setCursor(0 + xOffsetX, 45 + xOffsetY);
  xDisplay.println("");

  xDisplay.setCursor(0 + xOffsetX, 55 + xOffsetY);
  // xDisplay.print("Clients: ");
  // xDisplay.print(WiFi.softAPgetStationNum());

  xDisplay.display();

  zDisplay.clearDisplay();
  zDisplay.setTextSize(1);
  zDisplay.setTextColor(SH110X_WHITE);

  zDisplay.setCursor(32 + zOffsetX, 0 + zOffsetY);
  zDisplay.println("OTA UPDATE");

  zDisplay.setCursor(6 + zOffsetX, 15 + zOffsetY);
  zDisplay.println("SSID: MyESPfamily");

  zDisplay.setCursor(6 + zOffsetX, 25 + zOffsetY);
  zDisplay.println("IP: 192.168.5.1");

  zDisplay.setCursor(6 + zOffsetX, 35 + zOffsetY);
  zDisplay.println("URL: /update");

  zDisplay.setCursor(6 + zOffsetX, 45 + zOffsetY);
  zDisplay.println("Upload firmware");

  zDisplay.setCursor(6 + zOffsetX, 55 + zOffsetY);
  zDisplay.print("Uptime: ");
  zDisplay.print(OTAManager::getOTAUptime() / 1000);
  zDisplay.print("s");

  zDisplay.display();
}