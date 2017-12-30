#ifndef _SIM800_H
#define _SIM800_H

#include <Arduino.h>
#include <SoftwareSerial.h>

#define SIM800_RESPONSE_BUFFER_LENGTH 48

class SIM800 {
    private:
        SoftwareSerial* _serial;
        uint8_t _powerPin;
        char _responseBuffer[SIM800_RESPONSE_BUFFER_LENGTH];
        int _responseBufferPos = 0;
        unsigned long _lastSerialActivity;
        void (*_cmdProcessor)(char*);

        /**
         * Do the actual serial processing and store the data in the internal _responseBuffer.
         */
        bool loop_internal();

        /**
         * Flush the SIM800C serial.
         *
         * @param delay wait specified milliseconds before flushing.
         */
        void flush(unsigned int delay_ms=0);

        /**
         * Reset the response buffer. 
         */
        void resetResponseBuffer();

    public:
        /**
         * Constructor.
         * 
         * @param rxPin the Arduino pin wher the SIM800C RX pin is connected.
         * @param txPin the Arduino pin wher the SIM800C TX pin is connected.
         * @param powerPin the Arduino pin wher the SIM800C PWR pin is connected.
         * @param cmdProcessor Finction to be called when full response is received from SIM800C.
         */
        SIM800(short rxPin, short txPin, short powerPin, void (*cmdProcessor)(char*));

        /**
         * Toggle the SIM800C power state bringing the PWR pin to ground for 1 second.
         */
        void togglePowerState();

        /**
         * 
         * Send the specified command to the SIM800C module and verify that expected response is
         * received.
         *
         * The SIM800C should be in non-echo mode (ATE0). The commands should be with single
         * response line.
         * The provded command should not be terminated with '\n' and '\r'. There should be no
         * ongoing communication with the SIM800.
         *
         * @param cmd the command to be send.
         * @param response expected response.
         * @param timeout response timeout (default = 1000ms).
         * @return true iff the response is the epected one.
         */
        bool sendCommandAndVerifyResponse(const char* cmd, const char* expectedResponse, unsigned int timeout=1000);

        /**
         *  Send command and read the response.
         *
         * The SIM800 should be in non-echo mode (ATE0). The commands should be with single response line.
         * The provded command should not be terminated with '\n' and '\r'. There should be no ongoing
         * communication with the SIM800.
         *
         * @param cmd the command to be send.
         * @param resp buffer where to read the response.
         * @param timeout response timeout (default = 1000ms).
         * @param flush flush the serial buffer of SIM800C (default = true).
         * @return number of characters that were read.
         */
        unsigned int sendCommandAndReadResponse(const char* cmd, char* resp, unsigned int timeout=1000, bool flush=true);

        /**
         * Send a command to SIM800C.
         * 
         * @param cmd the command to be send.
         * @param flush flush the SIM800C serial interface after the command is send and ignore the
         * response.
         */
        void sendCommand(const char* cmd, bool flush=true);

        /**
         * Loop method that should be invoked on regular intervals. Keep the SIM800C functionality
         * running by receiving the deta available in the serial interface.
         */
        void loop();

        /**
         * Write single character to SIM800C serial interface.
         */
        void write(char ch);

        /**
         * Get the elapsed milliseconds since last SIM800C serial activity.
         */
        unsigned long getMillisSinceLastSerialActivity();
};

#endif