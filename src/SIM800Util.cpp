#include <Arduino.h>
#include "PinMap.h"

void ToggleSIM800PowerState() {
    Serial.println(F("Toggling power state of SIM800"));
    digitalWrite(SIM800_PW_PIN, LOW);
    delay(1100);
    digitalWrite(SIM800_PW_PIN, HIGH);
}