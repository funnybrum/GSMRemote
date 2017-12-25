#include <Arduino.h>
#include "Common.h"

void ToggleSIM800PowerState() {
    Serial.println(F("Toggling power state of SIM800"));
    digitalWrite(SIM800_PW_PIN, LOW);
    delay(1100);
    digitalWrite(SIM800_PW_PIN, HIGH);
}

/**
    Send command and read the response.

    The SIM800 should be in non-echo mode (ATE0). The commands should be with single response line.
    The provded command should not be terminated with '\n' and '\r'. There should be no ongoing
    communication with the SIM800.

    @param cmd the command to be send.
    @param resp buffer where to read the response.
    @param timeout response timeout (default = 1000ms).
    @param flush flush the serial buffer of SIM800C (default = true).
    @return number of characters that were read.
*/
unsigned int SendCommandAndReadResponse(char* cmd, char* resp, unsigned int timeout=1000, bool flush=true) {
    if (flush) {
        delay(250);
        while (SIM800.available()) {
            SIM800.read();
        }
    }

    SIM800.write(cmd);
    SIM800.write("\n\r");

    unsigned int pos = 0;
    bool received = false;

    while (timeout > 0 && !received) {
        delay(50);
        timeout -= 50;
        while (SIM800.available()) {
            char ch = SIM800.read();
            if (ch == '\r') {
                received = true;
            } else if (ch == '\n') {
                // Do nothing.
            } else {
                resp[pos] = ch;
                pos++;
            }
        }
    }

    if (pos > 2 && resp[pos-2] == 'O' && resp[pos-1] == 'K') {
        // Command is executed. We have trailing OK at the response end. Trim it.
        resp[pos-1] = 0;
        resp[pos-2] = 0;
        pos -= 2;
    }

    if (flush) {
        delay(250);
        while (SIM800.available()) {
            SIM800.read();
        }
    }

    return pos;
}

/**
    Send the specified command to the SIM800 module and verify that expected response is received.

    The SIM800 should be in non-echo mode (ATE0). The commands should be with single response line.
    The provded command should not be terminated with '\n' and '\r'. There should be no ongoing
    communication with the SIM800.

    @param cmd the command to be send.
    @param response expected response.
    @param timeout response timeout (default = 1000ms).
    @return true iff the response is the epected one.
*/
bool SendCommandAndVerifyResponse(char* cmd, char* response, unsigned int timeout=1000) {
    char resp[128];
    memset(resp, 0, sizeof(resp));
    unsigned int len = SendCommandAndReadResponse(cmd, resp, timeout);

    if (len != strlen(response)) {
        return false;
    }

    return strcmp(resp, response) == 0;
}