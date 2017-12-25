#include <Arduino.h>
#include <TaskScheduler.h>
#include <SoftwareSerial.h>
#include "Common.h"
#include "SIM800Util.h"


#define MAX_COMMAND_LENGTH 128

Scheduler runner;
Task* healthCheckTask;
Task* checkSIM800CommandsTask;

SoftwareSerial SIM800 = SoftwareSerial(SIM800_TX_PIN, SIM800_RX_PIN);
char commandBuffer[MAX_COMMAND_LENGTH];
int commandBufferPosition = 0;
unsigned long lastSIM800read;

/**
  RING
  +CLIP: "+359886501955",145,"",0,"",0
  RING
  +CLIP: "+359886501955",145,"",0,"",0
  NO CARRIER
 */

void HealthCheck() {
    // Primitive health check - send "AT" to SIM800. This should triger "OK" response on the serial
    // interface.

    // Send only \r to terminate the line as specified in the SIM800 command manual
    // "AT Command syntax" section.

    // Weak up
    SIM800.write("AT\n\r");
    delay(250);
    SIM800.flush();

    // Check network association status, 1 is OK, everyghint else is not OK
    char resp[128];
    memset(resp, 0, sizeof(resp));
    unsigned int len = SendCommandAndReadResponse("AT+CREG?", resp, 1000);

    if (len == 10) {
        if (memcmp("+CREG: 0,", resp, 9) == 0) {
            int networkStatus = atoi(resp + 9);
            Serial.print("Network status: ");
            Serial.println(networkStatus);
        } else {
            Serial.print("Healt check failed, got wrong response(len=10): ");
            Serial.println(resp);
        }
    } else {
        Serial.print("Healt check failed, got response with len ");
        Serial.println(len);
        Serial.print("Response: ");
        Serial.println(resp);
    }

    bool sleepResult = SendCommandAndVerifyResponse("AT+CSCLK=2", "OK");
    if (sleepResult) {
        Serial.println("Entering sleep mode: success");
    } else {
        Serial.println("Entering sleep mode: error");
    }

    unsigned long now = millis();

    if (now < lastSIM800read) {
        // The timer has overflowed (running for more than 50 days). Zero the last response and
        // skip this health check.
        lastSIM800read = millis(); 
    }

    // if (now - lastSIM800read > 45000) {
    //     // No response for last 45 seconds from the SIM800 module. Most likely the module is off.
    //     Serial.println(F("Health check: FAILED, restarting."));
    //     ToggleSIM800PowerState();
    //     // asm volatile ("  jmp 0");
    // }
}

void processCommand() {
    if (strcmp(commandBuffer, "RDY") == 0 ||
        strcmp(commandBuffer, "+CFUN: 1") == 0 ||
        strcmp(commandBuffer, "+CPIN: READY") == 0) {
        // Ignore this one.
        memset(commandBuffer, 0, MAX_COMMAND_LENGTH);
        commandBufferPosition = 0;
        return;
    }
    
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
        } else if ('\r' == ch) {
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

    // healthCheckTask = new Task(1L * 60L * 1000L, TASK_FOREVER, &HealthCheck, &runner, true);
    checkSIM800CommandsTask = new Task(100L, TASK_FOREVER, &CheckSIM800Commands, &runner, true);

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
        } else if (ch == '~') {
            HealthCheck();
        } else {
            SIM800.write(ch);
        }
    }
}