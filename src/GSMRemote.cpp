#include <Arduino.h>
#include <TaskScheduler.h>
#include <SoftwareSerial.h>
#include "PinMap.h"
#include "SIM800Util.h"


#define MAX_COMMAND_LENGTH 128

Scheduler runner;

void HealthCheck();
void CheckSIM800Commands();

Task healthCheckTask(30000, TASK_FOREVER, &HealthCheck, &runner, true);
Task checkSIM800CommandsTask(1000, TASK_FOREVER, &CheckSIM800Commands, &runner, true);


SoftwareSerial SIM800(SIM800_TX_PIN, SIM800_RX_PIN);
char commandBuffer[MAX_COMMAND_LENGTH];
int commandBufferPosition = 0;
int failureCount = 0;
unsigned long lastSIM800read = 0;

void HealthCheck() {
    // Primitive health check - send "AT" to SIM800. This should triger "OK" response on the serial
    // interface.

    // Send only \r to terminate the line as specified in the SIM800 command manual
    // "AT Command syntax" section.
    SIM800.write("AT\r");

    unsigned long now = millis();

    if (now < lastSIM800read) {
        // The timer has overflowed (running for more than 50 days). Zero the last response and
        // skip this health check.
        lastSIM800read = millis(); 
    }

    if (now - lastSIM800read > 45000) {
        // No response for last 45 seconds from the SIM800 module. Most likely the module is off.
        Serial.println(F("Health check: FAILED, restarting."));
        ToggleSIM800PowerState();
        // asm volatile ("  jmp 0");
    }
}

void processCommand() {
    if (strcmp(commandBuffer, "OK") == 0) {
        // Ignore this one.
        return;
    }
    if (strcmp(commandBuffer, "AT") == 0) {
        // Ignore this one.
        return;
    }
    if (strcmp(commandBuffer, "RDY") == 0) {
        // Ignore this one. Indicates serial interface is ready in non auto-baunding mode.
        return;
    }
    //NORMAL POWER DOWN
    //AT
    //OK
    //RDY
    //+CFUN: 1
    //+CPIN: READY
    //Call Ready
    //SMS Ready
    Serial.print(F("Got command: "));
    Serial.println(commandBuffer);
    memset(commandBuffer, 0, MAX_COMMAND_LENGTH);
    commandBufferPosition = 0;
}

void CheckSIM800Commands() {
    while (SIM800.available()) {
        lastSIM800read = millis();
        char ch = SIM800.read();

        if ('\n' == ch) {
            //Ignore these.
        }
        if ('\r' == ch) {
            if (commandBufferPosition > 0) {
                processCommand();
            }
        } else {
            commandBuffer[commandBufferPosition] = ch;
            commandBufferPosition++;
            if (commandBufferPosition == MAX_COMMAND_LENGTH) {
                // Something went wrong. A command longer than MAX_COMMAND_LENGTH seems to be
                // incoming. Try to process it and reset the buffer.
                Serial.println(F("Detected buffer overflow..."));
                processCommand();
                delay(100);
                SIM800.flush();
            }
        }
    }
}

void setup() {
    Serial.begin(9600);
    
    while(!Serial)
        delay(100);

    SIM800.begin(9600);

    // Prepare the on/off pin.
    pinMode(SIM800_PW_PIN, OUTPUT);
    digitalWrite(SIM800_PW_PIN, HIGH);

    // Prepare the relay pin
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);

    ToggleSIM800PowerState();

    lastSIM800read = millis();

    runner.startNow();

    Serial.println(F("Setup completed."));
}

void loop() {
    runner.execute();

    while (Serial.available()) {
        char ch = Serial.read();
        if (ch == '<') {
            ToggleSIM800PowerState();
        } else if (ch =='>') {
            // Ctrl+z
            SIM800.write(0x1A);
        } else {
            SIM800.write(ch);
        }
    }
}