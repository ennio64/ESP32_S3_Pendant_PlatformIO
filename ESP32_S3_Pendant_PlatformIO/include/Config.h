#ifndef CONFIG_H
#define CONFIG_H

#include <WiFi.h>

// ========== MAIN CONFIGURATION STRUCTURE ==========
struct Config {
  // GrblHAL Connection
  const char* grblHost = "192.168.1.123";
  const uint16_t grblPort = 23;

  // WiFi Configuration (NORMAL MODE)
  const char* wifiSSID = "TISCALI-5311";
  const char* wifiPassword = "TFLF7NK3GJ";

  // Display Configuration
  const uint8_t xScreenAddress = 0x3D;
  const uint8_t zScreenAddress = 0x3C;
  const uint8_t sdaPin = 11;
  const uint8_t sclPin = 10;

  // Encoder Pins
  const uint8_t encoderXA = 13;
  const uint8_t encoderXB = 12;
  const uint8_t encoderZA = 44;
  const uint8_t encoderZB = 43;

  // Joystick Pins
  const uint8_t joyRight = 3;
  const uint8_t joyLeft = 4;
  const uint8_t joyDown = 5;
  const uint8_t joyUp = 6;

  // ADS1115 Interrupt pin
  const uint8_t adsIntPin = 7;

  // PCF8575 Configuration
  const uint8_t pcfIntPin = 14;
  const uint8_t pcfAAddr = 0x26;
  const uint8_t pcfBAddr = 0x22;

  // Timing Configuration
  const unsigned long statusUpdateInterval = 100;
  const unsigned long displayUpdateInterval = 100;
  const unsigned long potReadInterval = 100;
  const unsigned long joystickDebounce = 50;
  const unsigned long encoderOptimalDt = 80;
  const unsigned long loopDelay = 2;

  // 🔥 ESP-NOW Channel (verrà rilevato automaticamente)
  uint8_t espNowChannel = 6;   // default, poi sovrascritto dalla scansione

  // OTA Configuration - Access Point Mode
  struct OTAConfig {
    const char* ssid = "MyESPfamily";
    const char* password = "12345678";
    const IPAddress localIP = { 192, 168, 5, 1 };
    const IPAddress gateway = { 192, 168, 5, 1 };
    const IPAddress subnet = { 255, 255, 255, 0 };
    const uint16_t serverPort = 80;
    const bool enabled = true;
    const unsigned long timeout = 30 * 60 * 1000;
  } ota;
};

// ========== EXTERNAL DECLARATIONS ==========
extern Config config;

#endif