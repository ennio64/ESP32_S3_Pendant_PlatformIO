#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Adafruit_SH110X.h>

// Dichiarazioni extern per i due display
extern Adafruit_SH1106G xDisplay;
extern Adafruit_SH1106G zDisplay;

class DisplayManager {
private:
  static unsigned long lastUpdate;
  static unsigned long lastOTAUpdate;

  // Offset regolabili per i due display
  static int xOffsetX;
  static int xOffsetY;
  static int zOffsetX;
  static int zOffsetY;

public:
  static void begin();
  static void update();
  static void updateXDisplay();
  static void updateZDisplay();

  // Modalità Radius/Diameter
  static void toggleRadiusDiameter();
  static bool isRadiusDiameterMode();

  // Utility
  static void displayCenter(Adafruit_SH1106G* display, const String& text, int yOffset = 0, bool isX = false);

  // Stato macchina
  static void showMachineState(const String& stateText);

  // Gestione offset
  static void setXOffsets(int dx, int dy);
  static void setZOffsets(int dx, int dy);

  // Schermate per Feed Rate
  static void showFeedScreen();
  static void restoreMainScreen();

  // Schermate per la distanza di sicurezza
  static void showSafeDistanceScreen(char axis, float value);
  static void showSafeDistanceConfirmation(char axis, float value);
  static void showSafeDistanceCancelled(char axis);

  // Schermate di avvio
  static void showOTAPrompt();
  static void showWaitController();

  // Schermate principali / OTA
  static void showMainScreen();
  static void showOTAScreen();

private:
  static void drawRadiusDiameterIndicators();
  static void drawVerticalText(const String& text, int16_t x, int16_t y);
  static unsigned long lastFeedDisplayTime;
  static bool showingFeed;
  static bool stateMessageShown;
  static MachineState lastState;
};

#endif
