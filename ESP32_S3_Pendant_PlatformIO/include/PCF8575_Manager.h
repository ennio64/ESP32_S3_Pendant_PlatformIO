#ifndef PCF8575_MANAGER_H
#define PCF8575_MANAGER_H

#include "PCF8575.h"

// Dichiarazioni per la gestione del PCF8575
extern PCF8575 pcfA;
extern PCF8575 pcfB;

class PCF8575Manager {
public:
    static bool begin();
    static void setPin(uint8_t expander, uint8_t pin, bool state);
    static bool readPin(uint8_t expander, uint8_t pin);
};

#endif