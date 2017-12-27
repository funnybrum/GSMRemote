#include "SIM800.h"

SIM800::SIM800(short rxPin, short txPin, short powerPin, void (*cmdProcessor)(char*)) {
    _powerPin = powerPin;
    _serial = new SoftwareSerial(txPin, rxPin);
    _serial->begin(9600);
    _cmdProcessor = cmdProcessor;
}

void SIM800::togglePowerState() {
    Serial.println(F("Toggling power state of SIM800"));
    resetCommandBuffer();
    digitalWrite(_powerPin, LOW);
    delay(1100);
    digitalWrite(_powerPin, HIGH);
}

void SIM800::flush(unsigned int delay_ms) {
    delay(delay_ms);

    while (_serial->available()) {
        _serial->read();
    }
}

unsigned int SIM800::sendCommandAndReadResponse(const char* cmd, char* resp, unsigned int timeout, bool flush) {
    if (flush) {
        this->flush(250);
    }

    resetCommandBuffer();
    sendCommand(cmd, false);

    bool received = false;
    while (timeout > 0 && received == false) {
        delay(50);
        timeout -= 50;
        received = loop_internal();
    }

    if (flush) {
        this->flush(250);
    }

    unsigned int respLen = 0;

    if (received) {
        respLen = cmdBufferPos;
        memcpy(resp, cmdBuffer, cmdBufferPos);
    }

    resetCommandBuffer();

    return respLen;
}

bool SIM800::sendCommandAndVerifyResponse(const char* cmd, const char* expectedResponse, unsigned int timeout) {
    char resp[SIM800_CMD_BUFFER_LENGTH];
    memset(resp, 0, SIM800_CMD_BUFFER_LENGTH);
    sendCommandAndReadResponse(cmd, resp, timeout, true);
    if (strcmp(resp, expectedResponse) != 0) {
        Serial.print(">>");
        Serial.println(resp);
    }
    return strcmp(resp, expectedResponse) == 0;
}

void SIM800::sendCommand(const char* cmd, bool flush) {
    _serial->write(cmd);
    _serial->write("\n\r");

    if (flush) {
        this->flush(250);
    }
}

bool SIM800::loop_internal() {
    bool commandReceived = false;

    while (_serial->available() && commandReceived == false) {
        char ch = _serial->read();

        if ('\n' == ch) {
            //Ignore these.
        } else if ('\r' == ch) {
            if (cmdBufferPos > 0) {
                commandReceived = true;
            }
        } else {
            cmdBuffer[cmdBufferPos] = ch;
            cmdBufferPos++;
            if (cmdBufferPos == sizeof(cmdBuffer)) {
                Serial.println(F("SIM800 serial buffer overflow detected."));
                cmdBuffer[sizeof(cmdBuffer)-1] = 0;
                commandReceived = true;
                flush(250);
            }
        }
    }

    return commandReceived;
}

void SIM800::loop() {
    bool cmdReceived = loop_internal();

    if (cmdReceived) {
        _cmdProcessor(cmdBuffer);
        resetCommandBuffer();
    }
}

void SIM800::resetCommandBuffer() {
    memset(cmdBuffer, 0, sizeof(cmdBuffer));
    cmdBufferPos = 0;
}

void SIM800::write(char ch) {
    _serial->write(ch);
}