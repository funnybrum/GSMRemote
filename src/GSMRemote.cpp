#include <Arduino.h>
#include <TaskScheduler.h>
#include "Pins.h"
#include "Secrets.h"
#include "SIM800.h"

void ProcessCommand(char*);

SIM800 sim800 = SIM800(SIM800_RX_PIN, SIM800_TX_PIN, SIM800_PW_PIN, ProcessCommand);

Scheduler runner;
Task* checkNetworkStatusTask = 0;
Task* healthCheckTask = 0;

unsigned long lastOKNetworkStatus;
unsigned long externalDevicePoweredOnAt;
bool externalDevicePoweredOn = false;

// Keep the external deviced turned on for at most X minutes
#define MAX_TURN_ON_DURATION_MIN 3L

// Try to turn off/on the SIM800 if the network status is not OK after X minuites
#define HOLD_ON_BAD_NETWORK_STATUS_MIN 10L


void powerOn(bool powerOn) {
    if (powerOn) {
        digitalWrite(RELAY_PIN, HIGH);
        externalDevicePoweredOnAt = millis();
        Serial.print(millis() / 1000);
        Serial.println(F(" > Turning ON external device"));
    } else {
        digitalWrite(RELAY_PIN, LOW);
        Serial.print(millis() / 1000);
        Serial.println(F(" > Turning OFF external device"));
    }
    externalDevicePoweredOn = powerOn;
}

/**
  RING
  +CLIP: "+35988xxxxxxx",145,"",0,"",0
  RING
  +CLIP: "+35988xxxxxxx",145,"",0,"",0
  NO CARRIER
 */

void CheckNetworkStatus() {
    // Weak up
    sim800.sendCommand("AT", 250);

    // Check network association status, 1 is OK, everyghint else is not OK.
    char resp[SIM800_CMD_BUFFER_LENGTH];
    memset(resp, 0, sizeof(resp));
    
    sim800.sendCommandAndReadResponse("AT+CREG?", resp, 1000);
    int networkStatus = -1;

    // Check if we have suffessful response.
    if (memcmp("+CREG: 0,", resp, 9) == 0) {
        networkStatus = atoi(resp + 9);
    }

    if (networkStatus == 1) {
        lastOKNetworkStatus = millis();
    }

    // Get back to sleep mode.
    bool sleepResult = sim800.sendCommandAndVerifyResponse("AT+CSCLK=2", "OK");
    if (!sleepResult) {
        Serial.println(F("Failed to enter sleep mode!"));
    }
}

void HealthCheck() {
    unsigned long now = millis();

    if (now < lastOKNetworkStatus) {
        // The timer has overflowed (running for more than 50 days). Zero the last response and
        // skip this health check.
        lastOKNetworkStatus = now;
    }

    if (now - lastOKNetworkStatus > HOLD_ON_BAD_NETWORK_STATUS_MIN * 60L * 1000L) {
        // No OK network status for last 10 minutes.
        Serial.println(F("Health check: FAILED, restarting."));
        checkNetworkStatusTask->disable();
        sim800.togglePowerState();
    }

    if (externalDevicePoweredOn) {
        if (now < externalDevicePoweredOnAt) {
            externalDevicePoweredOnAt = now;
        }


        if (now - externalDevicePoweredOnAt > MAX_TURN_ON_DURATION_MIN * 60L * 1000L) {
            powerOn(false);
        }
    }
}

void ProcessCommand(char* commandBuffer) {
    if (strcmp(commandBuffer, "RDY") == 0 ||
        strcmp(commandBuffer, "+CFUN: 1") == 0 ||
        strcmp(commandBuffer, "+CPIN: READY") == 0) {
        // Ignore this one.
        return;
    }

    if (strcmp(commandBuffer, "SMS Ready") == 0 ||
        strcmp(commandBuffer, "Call Ready") == 0) {
        checkNetworkStatusTask->enable();
        return;
    }

    if (memcmp(commandBuffer, "+CLIP: \"", 8) == 0) {
        // Incomming call.
        char callNumber[32];
        int i = 0;
        while (commandBuffer[i+8] != 0 && commandBuffer[i+8] != '"') {
            callNumber[i] = commandBuffer[i+8];
            i++;
        }
        callNumber[i] = 0;

        if (strcmp(callNumber, AUTHORIZED_NUMBER) == 0) {
            Serial.println("Authorized call!");
            powerOn(!externalDevicePoweredOn);
            // Avoid re-triggering the external device for the same call.
            for (int i = 0; i < 5; i++) {
                sim800.sendCommand("ATH");
                delay(1000);
            }
        }

        return;
    }

    if (strcmp(commandBuffer, "NORMAL POWER DOWN") == 0) {
        sim800.togglePowerState();
        checkNetworkStatusTask->disable();
    }

    Serial.println(commandBuffer);
}

void setup() {
    // Prepare the on/off pin.
    pinMode(SIM800_PW_PIN, OUTPUT);
    digitalWrite(SIM800_PW_PIN, HIGH);

    // Prepare the relay pin
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);

    Serial.begin(9600);
    
    while(!Serial)
        delay(100);

    sim800.togglePowerState();
    runner.startNow();

    // TODO -> adjust task scheduling intervals.
    checkNetworkStatusTask = new Task(3L * 10L * 1000L, TASK_FOREVER, &CheckNetworkStatus, &runner, false);
    healthCheckTask = new Task(1L * 60L * 1000L, TASK_FOREVER, &HealthCheck, &runner, true);

    Serial.println(F("Setup completed."));
}

void loop() {
    runner.execute();
    sim800.loop();
    delay(100);

    while (Serial.available()) {
        char ch = Serial.read();
        if (ch == '<') {
            sim800.togglePowerState();
            checkNetworkStatusTask->disable();
        } else if (ch =='>') {
            // Ctrl+z
            sim800.write(0x1A);
        } else if (ch == '~') {
            CheckNetworkStatus();
        } else {
            sim800.write(ch);
        }
    }
}