#ifndef _SIM800_H
#define _SIM800_H

#include <Arduino.h>
#include <SoftwareSerial.h>

#define SIM800_CMD_BUFFER_LENGTH 48

class SIM800 {
    private:
        SoftwareSerial* _serial;
        uint8_t _powerPin;
        char cmdBuffer[SIM800_CMD_BUFFER_LENGTH];
        int cmdBufferPos = 0;
        void (*_cmdProcessor)(char*);

        bool loop_internal();

        /**
            Flush the SIM800C serial.

            @param delay wait specified milliseconds before flushing.
        */
        void flush(unsigned int delay_ms=0);

        void resetCommandBuffer();

    public:
        SIM800(short rxPin, short txPin, short powerPin, void (*cmdProcessor)(char*));
        void togglePowerState();

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
        bool sendCommandAndVerifyResponse(const char* cmd, const char* expectedResponse, unsigned int timeout=1000);

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
        unsigned int sendCommandAndReadResponse(const char* cmd, char* resp, unsigned int timeout=1000, bool flush=true);

        void sendCommand(const char* cmd, bool flush=true);
        void loop();
        void write(char ch);
};

#endif