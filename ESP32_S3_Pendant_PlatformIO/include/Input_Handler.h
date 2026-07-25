#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include "Globals.h"
#include "driver/pcnt.h"

class InputHandler {
private:
  static const int8_t ENCODER_STATES[16];
  static constexpr float JOG_DISTANCE_X = 0.01f;
  static constexpr float JOG_DISTANCE_Z = 0.01f;

  static int16_t lastPCNT_X;
  static int16_t lastPCNT_Z;

  static volatile bool newADS1115DataAvailable;

  // 🔥 Variabili per gestire i delta degli encoder (risolve accumulo durante lock/safe distance)
  static int16_t lastEncoderX;
  static int16_t lastEncoderZ;
  static int pendingEncoderX;
  static int pendingEncoderZ;

public:
  static void setupEncoders();   // Inizializza PCNT

  static void handleEncoderJog();
  static void handleJoystick();
  static void handleButtons();
  static void handleADS1115();
  static void stopAllMotion();

  static void IRAM_ATTR readJoystickState();
  static void initADS1115();
  static void IRAM_ATTR ads1115AlertISR();

  static int16_t getPCNT_X();
  static int16_t getPCNT_Z();

  static void processPendingDebug();

  // Gestione sospensione encoder (lock / safe distance)
  static void suspendEncoderReading();
  static void resumeEncoderReading();
  static void resetEncoderCounters();

  // 🔥 Resetta i delta interni per evitare accumulo di movimenti
  static void resetEncoderDelta();

private:
  static void initializeEncoderX();
  static void initializeEncoderZ();
  static void executeJogCommandOptimized(char axis, int stepDifference);
  static void configureADS1115Comparator();
  static void stopCurrentJog();
  static void startContinuousJog();
  static char joystickAxis;
  static void updateFeedrates();
  static int readPotFeedrate(uint8_t channel);
  static void sendFeedOverride(int percent);
  static float applyLimits(char axis, float requestedDistance);

  static bool encoderDebugOnly;
  static int pendingX_debug;
  static int pendingZ_debug;
};

#endif