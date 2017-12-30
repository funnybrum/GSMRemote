#include "SIM800.h"

SIM800::SIM800(short rxPin, short txPin, short powerPin, void (*cmdProcessor)(char*)) {
    _powerPin = powerPin;
    _serial = new SoftwareSerial(txPin, rxPin);
    _serial->begin(9600);
    _cmdProcessor = cmdProcessor;
    _lastSerialActivity = millis();
}

void SIM800::togglePowerState() {
    Serial.println(F("Toggling power state of SIM800C"));
    resetResponseBuffer();
    digitalWrite(_powerPin, LOW);
    delay(1100);
    digitalWrite(_powerPin, HIGH);
    _lastSerialActivity = millis();
}

void SIM800::flush(unsigned int delay_ms) {
    delay(delay_ms);

    while (_serial->available()) {
        _lastSerialActivity = millis();
        _serial->read();
    }
}

unsigned int SIM800::sendCommandAndReadResponse(const char* cmd, char* resp, unsigned int timeout, bool flush) {
    if (flush) {
        this->flush(250);
    }

    resetResponseBuffer();
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
        respLen = _responseBufferPos;
        memcpy(resp, _responseBuffer, _responseBufferPos);
    }

    resetResponseBuffer();

    return respLen;
}

bool SIM800::sendCommandAndVerifyResponse(const char* cmd, const char* expectedResponse, unsigned int timeout) {
    char resp[SIM800_RESPONSE_BUFFER_LENGTH];
    memset(resp, 0, SIM800_RESPONSE_BUFFER_LENGTH);
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
        _lastSerialActivity = millis();

        if ('\n' == ch) {
            //Ignore these.
        } else if ('\r' == ch) {
            if (_responseBufferPos > 0) {
                commandReceived = true;
            }
        } else {
            _responseBuffer[_responseBufferPos] = ch;
            _responseBufferPos++;
            if (_responseBufferPos == sizeof(_responseBuffer)) {
                Serial.println(F("SIM800C serial buffer overflow detected."));
                _responseBuffer[sizeof(_responseBuffer)-1] = 0;
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
        _cmdProcessor(_responseBuffer);
        resetResponseBuffer();
    }
}

void SIM800::resetResponseBuffer() {
    memset(_responseBuffer, 0, sizeof(_responseBuffer));
    _responseBufferPos = 0;
}

void SIM800::write(char ch) {
    _serial->write(ch);
}

unsigned long SIM800::getMillisSinceLastSerialActivity() {
    // Millis() overflow is handled.
    return millis() - _lastSerialActivity;
}