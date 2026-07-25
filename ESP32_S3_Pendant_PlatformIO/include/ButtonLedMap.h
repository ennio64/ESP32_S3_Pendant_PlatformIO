#pragma once
#include <Arduino.h> 

struct ButtonLed {
  uint8_t buttonPin;     // Pin fisico del pulsante
  uint8_t ledPin;        // Pin fisico del LED
  bool ledOn;            // Stato attuale del LED
  bool buttonPressed;    // Stato attuale del pulsante
  bool lastButtonState;  // Stato precedente (per debounce)
};
  
// Dichiarazioni esterne
extern ButtonLed controls[];
extern const uint8_t NUM_CONTROLS;
