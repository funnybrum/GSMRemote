#include <Arduino.h>
#include <TaskScheduler.h>
#include "Pins.h"
#include "SIM800.h"

void ProcessCommand(char*);

SIM800 sim800 = SIM800(SIM800_RX_PIN, SIM800_TX_PIN, SIM800_PW_PIN, ProcessCommand);

Scheduler runner;
Task* healthCheckTask;

#define MAX_COMMAND_LENGTH 128
char commandBuffer[MAX_COMMAND_LENGTH];
int commandBufferPosition = 0;
unsigned long lastSIM800read;

/**
  RING
  +CLIP: "+35988xxxxxxx",145,"",0,"",0
  RING
  +CLIP: "+35988xxxxxxx",145,"",0,"",0
  NO CARRIER
 */

void HealthCheck() {
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

    // TODO -> use the network status for something useful.
    Serial.print("Got network status: ");
    Serial.println(networkStatus);

    // Get back to sleep mode.
    bool sleepResult = sim800.sendCommandAndVerifyResponse("AT+CSCLK=2", "OK");
    if (!sleepResult) {
        Serial.println("Failed to enter sleep mode!");
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

void ProcessCommand(char* commandBuffer) {
    if (strcmp(commandBuffer, "RDY") == 0 ||
        strcmp(commandBuffer, "+CFUN: 1") == 0 ||
        strcmp(commandBuffer, "+CPIN: READY") == 0) {
        // Ignore this one.
        return;
    }

    if (strcmp(commandBuffer, "SMS Ready") == 0) {
        healthCheckTask->enable();
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

    healthCheckTask = new Task(1L * 10L * 1000L, TASK_FOREVER, &HealthCheck, &runner, false);

    Serial.println(F("Setup completed."));
}

void loop() {
    runner.execute();
    // Serial.println("sim800.loop()");
    sim800.loop();
    delay(100);

    while (Serial.available()) {
        char ch = Serial.read();
        if (ch == '<') {
            sim800.togglePowerState();
            healthCheckTask->disable();
        } else if (ch =='>') {
            // Ctrl+z
            sim800.write(0x1A);
        } else if (ch == '~') {
            HealthCheck();
        } else {
            sim800.write(ch);
        }
    }
}