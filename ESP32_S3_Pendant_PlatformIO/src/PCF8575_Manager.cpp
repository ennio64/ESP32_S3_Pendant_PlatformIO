#include "PCF8575_Manager.h"
#include "Config.h"

extern TwoWire I2Ctwo;

extern PCF8575 pcfA;
extern PCF8575 pcfB;

bool PCF8575Manager::begin() {
  bool successA = pcfA.begin(); 
  bool successB = pcfB.begin();

  if (!successA) Serial.println("❌ PCF8575A Error");
  else {
    Serial.println("✅ PCF8575A OK");
    for (uint8_t pin = 0; pin < 4; pin++) {
      pcfA.write(pin, HIGH);  // input con pull-up
    }
  }

  if (!successB) Serial.println("❌ PCF8575B Error");
  else {
    Serial.println("✅ PCF8575B OK");
    for (uint8_t pin = 8; pin <= 15; pin++) {
      pcfB.write(pin, HIGH);  // input con pull-up
    }
  }

  return successA && successB;
}

void PCF8575Manager::setPin(uint8_t expander, uint8_t pin, bool state) {
  if (expander == 0) {
    pcfA.write(pin, state ? LOW : HIGH);
  } else {
    pcfB.write(pin, state ? LOW : HIGH);
  }
}

bool PCF8575Manager::readPin(uint8_t expander, uint8_t pin) {
  if (expander == 0) {
    return (pcfA.read(pin) == LOW);
  } else {
    return (pcfB.read(pin) == LOW);
  }
}
