#ifndef ESP32_GRBLHAL_CONTROLLER_H
#define ESP32_GRBLHAL_CONTROLLER_H

#include "MinimalCNCTelnet.h"

// Wrapper: mantiene il nome originale ma usa MinimalCNCTelnet
class ESP32_GrblHAL_Controller : public MinimalCNCTelnet {
public:
    ESP32_GrblHAL_Controller() : MinimalCNCTelnet() {}
    ESP32_GrblHAL_Controller(const char* host, uint16_t port) 
        : MinimalCNCTelnet(host, port) {}
};

#endif

